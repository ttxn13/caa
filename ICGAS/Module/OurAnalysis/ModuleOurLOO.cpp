#include "pch.hpp"
#include "ModuleOurLOO.hpp"
#include "../../Core/Util/UtilCoor.hpp"

#include <limits>

QMutex ModuleOurLOO::mutex;
ModuleOurLOO* ModuleOurLOO::instance = nullptr;

namespace {
constexpr double kDoubleLimit = 1.0e-5;
constexpr double kNormDistanceM = 5.0e6;
constexpr double kNormTimeS = 100.0;

const S_MOTION_FRAME* latest_motion(const S_FORMAT& format)
{
	return format.list_motion.empty() ? nullptr : &format.list_motion.back();
}

int type_sort_index(const E_PLAT_TYPE type)
{
	switch (type) {
	case E_PLAT_TYPE::CAR: return 1;
	case E_PLAT_TYPE::DST: return 2;
	case E_PLAT_TYPE::FRI: return 3;
	case E_PLAT_TYPE::WAN: return 4;
	case E_PLAT_TYPE::ELC: return 5;
	case E_PLAT_TYPE::FLT: return 6;
	case E_PLAT_TYPE::BOM: return 7;
	case E_PLAT_TYPE::MIS: return 8;
	default:
		return 100;
	}
}

E_PLAT_TYPE representative_type(
	const S_FORMAT& format,
	const std::map<QString, S_PLAT>& list_plat_align)
{
	std::map<E_PLAT_TYPE, int> type_count;
	for (const QString& plat_id : format.plat_id_list) {
		const auto plat_iter = list_plat_align.find(plat_id);
		if (plat_iter == list_plat_align.end() || !plat_iter->second.valid) continue;
		type_count[plat_iter->second.type]++;
	}

	E_PLAT_TYPE result = E_PLAT_TYPE::UKN;
	int best_count = -1;
	int best_type_index = std::numeric_limits<int>::max();
	for (const auto& item : type_count) {
		const int item_type_index = type_sort_index(item.first);
		if (item.second > best_count || (item.second == best_count && item_type_index < best_type_index)) {
			result = item.first;
			best_count = item.second;
			best_type_index = item_type_index;
		}
	}
	return result;
}

bool greedy_attack_eligible(const E_PLAT_TYPE type)
{
	return type != E_PLAT_TYPE::WAN &&
		type != E_PLAT_TYPE::ELC &&
		type != E_PLAT_TYPE::CAR &&
		type != E_PLAT_TYPE::DST;
}

bool target_cover_eligible(const E_PLAT_TYPE type)
{
	return type != E_PLAT_TYPE::WAN &&
		type != E_PLAT_TYPE::ELC &&
		type != E_PLAT_TYPE::CAR;
}

double clamp01(const double value)
{
	if (!std::isfinite(value)) return 0.0;
	return std::clamp(value, 0.0, 1.0);
}

double allocation_priority(
	const E_STAGE stage,
	const S_FORMAT& own_format,
	const S_FORMAT& tgt_format)
{
	const S_MOTION_FRAME* own_motion = latest_motion(own_format);
	const S_MOTION_FRAME* tgt_motion = latest_motion(tgt_format);
	if (own_motion == nullptr || tgt_motion == nullptr) return 0.0;

	const S_POS_ECEF pos_delta(
		tgt_motion->pos_ecef.x_m - own_motion->pos_ecef.x_m,
		tgt_motion->pos_ecef.y_m - own_motion->pos_ecef.y_m,
		tgt_motion->pos_ecef.z_m - own_motion->pos_ecef.z_m);
	const S_POS_ENU pos_enu = UtilCoor::pos_ecef2enu(pos_delta, own_motion->pos_lla);

	const double del_h = pos_enu.u_m;
	double norm_h = ((del_h > 0.0) - (del_h < 0.0)) * del_h * del_h /
		(del_h * del_h + kNormDistanceM * kNormDistanceM);
	norm_h = (norm_h + 1.0) / 2.0;

	const double dx = pos_enu.e_m;
	const double dy = pos_enu.n_m;
	const double c = dx * dx + dy * dy;
	const double dcpa = std::sqrt(std::max(0.0, c));
	const double tcpa = 0.0;

	const double norm_dcpa = std::exp(-dcpa / kNormDistanceM / std::exp(1.0));
	const double norm_tcpa = kNormTimeS * kNormTimeS / (tcpa * tcpa + kNormTimeS * kNormTimeS);
	const double cog_value = clamp01(tgt_format.algo_cog.cog_value);

	switch (stage) {
	case E_STAGE::WAN:
		return cog_value * (norm_h / 2.0 + norm_dcpa / 6.0 + norm_tcpa / 6.0);
	case E_STAGE::FLT:
		return cog_value * (norm_h / 6.0 + norm_dcpa / 6.0 + norm_tcpa / 6.0);
	case E_STAGE::COP:
		return cog_value * (norm_h / 6.0 + norm_dcpa / 6.0 + norm_tcpa / 2.0);
	case E_STAGE::VSL:
		return cog_value * (norm_h / 6.0 + norm_dcpa / 2.0 + norm_tcpa / 6.0);
	default:
		return 0.0;
	}
}

std::vector<E_ACT> allocation_actions()
{
	return { E_ACT::ATTACK };
}
}

ModuleOurLOO* ModuleOurLOO::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleOurLOO();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleOurLOO::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleOurLOO::ModuleOurLOO(QObject* parent)
	: QObject(parent)
{
}

ModuleOurLOO::~ModuleOurLOO()
{
}

void ModuleOurLOO::func_our_loo(
	S_COA& coa_own,
	const std::map<QString, S_FORMAT>& list_own_fmt_align,
	const std::map<QString, S_FORMAT>& list_tgt_fmt_align,
	const std::map<QString, S_PLAT>& list_own_plat_align,
	const std::map<QString, S_PLAT>& list_tgt_plat_align)
{
	(void)list_tgt_plat_align;

	for (int index = 0; index < static_cast<int>(E_STAGE::COUNT); ++index) {
		coa_own.coa_of_stage[index].stage = static_cast<E_STAGE>(index);
		coa_own.coa_of_stage[index].own_fmt_act.clear();
	}

	std::vector<QString> own_ids;
	std::vector<E_PLAT_TYPE> own_types;
	for (const auto& item : list_own_fmt_align) {
		const S_FORMAT& format = item.second;
		if (!format.valid) continue;
		own_ids.push_back(item.first);
		own_types.push_back(representative_type(format, list_own_plat_align));
	}

	std::vector<QString> tgt_ids;
	for (const auto& item : list_tgt_fmt_align) {
		const S_FORMAT& format = item.second;
		if (!format.valid) continue;
		tgt_ids.push_back(item.first);
	}

	const int own_num = static_cast<int>(own_ids.size());
	const int tgt_num = static_cast<int>(tgt_ids.size());
	if (own_num <= 0 || tgt_num <= 0) return;

	std::vector<int> cog_order(tgt_num, -1);
	std::vector<int> unsorted_tgt_index;
	unsorted_tgt_index.reserve(tgt_num);
	for (int index = 0; index < tgt_num; ++index) {
		const auto fmt_iter = list_tgt_fmt_align.find(tgt_ids[index]);
		if (fmt_iter == list_tgt_fmt_align.end()) continue;

		const int sort = fmt_iter->second.algo_cog.cog_sort;
		if (sort > 0 && sort <= tgt_num && cog_order[sort - 1] < 0) {
			cog_order[sort - 1] = index;
		}
		else {
			unsorted_tgt_index.push_back(index);
		}
	}
	std::sort(unsorted_tgt_index.begin(), unsorted_tgt_index.end(), [&](const int lhs, const int rhs) {
		const auto lhs_iter = list_tgt_fmt_align.find(tgt_ids[lhs]);
		const auto rhs_iter = list_tgt_fmt_align.find(tgt_ids[rhs]);
		const S_FORMAT& lhs_format = lhs_iter->second;
		const S_FORMAT& rhs_format = rhs_iter->second;
		if (std::abs(lhs_format.algo_cog.cog_value - rhs_format.algo_cog.cog_value) > 1.0e-9) {
			return lhs_format.algo_cog.cog_value > rhs_format.algo_cog.cog_value;
		}
		if (lhs_format.algo_threat.threat_sort != rhs_format.algo_threat.threat_sort) {
			return lhs_format.algo_threat.threat_sort < rhs_format.algo_threat.threat_sort;
		}
		return tgt_ids[lhs] < tgt_ids[rhs];
	});
	for (int slot = 0, fill_index = 0; slot < tgt_num && fill_index < static_cast<int>(unsorted_tgt_index.size()); ++slot) {
		if (cog_order[slot] >= 0) continue;
		cog_order[slot] = unsorted_tgt_index[fill_index++];
	}

	for (int stage_index = 0; stage_index < static_cast<int>(E_STAGE::COUNT); ++stage_index) {
		const E_STAGE stage = static_cast<E_STAGE>(stage_index);

		std::vector<std::vector<double>> priority(
			own_num, std::vector<double>(tgt_num, 0.0));
		for (int own_index = 0; own_index < own_num; ++own_index) {
			const auto own_iter = list_own_fmt_align.find(own_ids[own_index]);
			if (own_iter == list_own_fmt_align.end()) continue;
			for (int tgt_index = 0; tgt_index < tgt_num; ++tgt_index) {
				const auto tgt_iter = list_tgt_fmt_align.find(tgt_ids[tgt_index]);
				if (tgt_iter == list_tgt_fmt_align.end()) continue;
				priority[own_index][tgt_index] = allocation_priority(stage, own_iter->second, tgt_iter->second);
			}
		}

		std::vector<std::vector<int>> allocation(
			own_num, std::vector<int>(tgt_num, 0));
		std::vector<int> deleted_own(own_num, 0);
		std::vector<int> deleted_tgt(tgt_num, 0);

		for (int loop = 0; loop < own_num; ++loop) {
			bool all_targets_covered = true;
			for (int order_index = 0; order_index < tgt_num; ++order_index) {
				const int tgt_index = cog_order[order_index];
				if (tgt_index < 0 || tgt_index >= tgt_num) continue;

				bool covered = false;
				for (int own_index = 0; own_index < own_num; ++own_index) {
					if (!target_cover_eligible(own_types[own_index])) continue;
					if (allocation[own_index][tgt_index] != 0) {
						covered = true;
						break;
					}
				}
				if (!covered) {
					all_targets_covered = false;
					break;
				}
			}

			double max_gain_score = 0.0;
			int best_own = -1;
			int best_tgt = -1;
			for (int own_index = 0; own_index < own_num; ++own_index) {
				if (deleted_own[own_index] != 0) continue;
				if (!greedy_attack_eligible(own_types[own_index])) continue;

				for (int order_index = 0; order_index < tgt_num; ++order_index) {
					const int tgt_index = cog_order[order_index];
					if (tgt_index < 0 || tgt_index >= tgt_num) continue;
					if (!all_targets_covered && deleted_tgt[tgt_index] != 0) continue;

					if (priority[own_index][tgt_index] > max_gain_score + kDoubleLimit) {
						max_gain_score = priority[own_index][tgt_index];
						best_own = own_index;
						best_tgt = tgt_index;
					}
				}
			}

			if (best_own < 0 || best_tgt < 0) break;
			allocation[best_own][best_tgt] = 1;
			deleted_own[best_own] = 1;
			deleted_tgt[best_tgt] = 1;
		}

		if (stage == E_STAGE::COP) {
			std::vector<int> dst_indices;
			std::vector<int> flt_target_indices;
			for (int own_index = 0; own_index < own_num; ++own_index) {
				if (own_types[own_index] == E_PLAT_TYPE::DST) {
					dst_indices.push_back(own_index);
					std::fill(allocation[own_index].begin(), allocation[own_index].end(), 0);
				}
				else if (own_types[own_index] == E_PLAT_TYPE::FLT) {
					for (int tgt_index = 0; tgt_index < tgt_num; ++tgt_index) {
						if (allocation[own_index][tgt_index] != 0) {
							flt_target_indices.push_back(tgt_index);
						}
					}
				}
			}

			if (!dst_indices.empty() && !flt_target_indices.empty()) {
				int dst_slot = 0;
				for (const int tgt_index : flt_target_indices) {
					allocation[dst_indices[dst_slot]][tgt_index] = 1;
					dst_slot = (dst_slot + 1) % static_cast<int>(dst_indices.size());
				}
			}
		}
		else if (stage == E_STAGE::VSL) {
			std::vector<int> dst_indices;
			for (int own_index = 0; own_index < own_num; ++own_index) {
				if (own_types[own_index] == E_PLAT_TYPE::DST) {
					dst_indices.push_back(own_index);
					std::fill(allocation[own_index].begin(), allocation[own_index].end(), 0);
				}
			}

			if (!dst_indices.empty()) {
				int dst_slot = 0;
				for (int order_index = 0; order_index < tgt_num; ++order_index) {
					const int tgt_index = cog_order[order_index];
					if (tgt_index < 0 || tgt_index >= tgt_num) continue;
					allocation[dst_indices[dst_slot]][tgt_index] = 1;
					dst_slot = (dst_slot + 1) % static_cast<int>(dst_indices.size());
				}
			}
		}

		for (int own_index = 0; own_index < own_num; ++own_index) {
			if ((stage == E_STAGE::WAN &&
					(own_types[own_index] == E_PLAT_TYPE::FLT || own_types[own_index] == E_PLAT_TYPE::DST)) ||
				(stage == E_STAGE::FLT && own_types[own_index] == E_PLAT_TYPE::DST) ||
				(stage == E_STAGE::VSL && own_types[own_index] == E_PLAT_TYPE::FLT)) {
				std::fill(allocation[own_index].begin(), allocation[own_index].end(), 0);
			}
		}

		for (int own_index = 0; own_index < own_num; ++own_index) {
			std::vector<S_ACT> actions;
			for (int tgt_index = 0; tgt_index < tgt_num; ++tgt_index) {
				if (allocation[own_index][tgt_index] == 0) continue;

				S_ACT action{};
				action.tgt_fmt_id = tgt_ids[tgt_index];
				action.act_list = allocation_actions();
				actions.push_back(action);
			}
			if (!actions.empty()) {
				coa_own.coa_of_stage[stage_index].own_fmt_act[own_ids[own_index]] = actions;
			}
		}
	}
}
