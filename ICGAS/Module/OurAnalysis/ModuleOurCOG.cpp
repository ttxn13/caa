#include "pch.hpp"
#include "ModuleOurCOG.hpp"

#include <array>
#include <limits>

QMutex ModuleOurCOG::mutex;
ModuleOurCOG* ModuleOurCOG::instance = nullptr;

namespace {
constexpr int kAbilityNum = 5;
constexpr double kDoubleLimit = 1.0e-5;

double clamp01(const double value)
{
	if (!std::isfinite(value)) return 0.0;
	return std::clamp(value, 0.0, 1.0);
}

E_LEVEL cog_level_by_sort(const int sort, const int count, const E_PLAT_TYPE type)
{
	if (count <= 0 || sort <= 0) return E_LEVEL::UKN;
	if (type == E_PLAT_TYPE::MIS) return E_LEVEL::HIGH;

	const int high_end = std::max(1, (count + 2) / 3);
	const int low_begin = std::max(high_end + 1, (2 * count + 2) / 3 + 1);
	if (sort <= high_end) return E_LEVEL::HIGH;
	if (sort >= low_begin) return E_LEVEL::LOW;
	return E_LEVEL::MID;
}

int type_sort_index(const E_PLAT_TYPE type)
{
	switch (type) {
	case E_PLAT_TYPE::CAR: return 1;
	case E_PLAT_TYPE::MIS: return 2;
	case E_PLAT_TYPE::BOM: return 3;
	case E_PLAT_TYPE::WAN: return 4;
	case E_PLAT_TYPE::ELC: return 5;
	case E_PLAT_TYPE::FLT: return 6;
	case E_PLAT_TYPE::DST:
	case E_PLAT_TYPE::FRI:
		return 8;
	default:
		return 100;
	}
}

E_PLAT_TYPE representative_type(
	const S_FORMAT& format,
	const std::map<QString, S_PLAT>& list_tgt_plat_align)
{
	std::map<E_PLAT_TYPE, int> type_count;
	for (const QString& plat_id : format.plat_id_list) {
		const auto plat_iter = list_tgt_plat_align.find(plat_id);
		if (plat_iter == list_tgt_plat_align.end() || !plat_iter->second.valid) continue;
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

bool has_ocog_ability(const E_PLAT_TYPE type, const int ability)
{
	switch (type) {
	case E_PLAT_TYPE::MIS:
	case E_PLAT_TYPE::BOM:
		return ability == 0 || ability == 3;
	case E_PLAT_TYPE::FLT:
		return ability == 0;
	case E_PLAT_TYPE::WAN:
		return ability == 1 || ability == 4;
	case E_PLAT_TYPE::ELC:
		return ability == 2;
	case E_PLAT_TYPE::CAR:
		return ability == 3 || ability == 4;
	case E_PLAT_TYPE::DST:
	case E_PLAT_TYPE::FRI:
		return ability == 3;
	default:
		return false;
	}
}

double ocog_ability_weight(const int ability)
{
	static constexpr std::array<double, kAbilityNum> kWeights = {
		5.0,  // 制空能力
		3.0,  // 预警探测能力
		2.0,  // 电子作战能力
		10.0, // 对舰打击能力
		4.0   // 指挥通信能力
	};
	if (ability < 0 || ability >= kAbilityNum) return 0.0;
	return kWeights[ability];
}
}

ModuleOurCOG* ModuleOurCOG::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleOurCOG();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleOurCOG::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleOurCOG::ModuleOurCOG(QObject* parent)
	: QObject(parent)
{
}

ModuleOurCOG::~ModuleOurCOG()
{
}

void ModuleOurCOG::func_our_cog(
	std::map<QString, S_FORMAT>& list_tgt_fmt_align,
	const std::map<QString, S_PLAT>& list_tgt_plat_align)
{
	for (auto& item : list_tgt_fmt_align) {
		item.second.algo_cog = S_ALGO_COG{};
	}

	std::vector<QString> group_ids;
	std::vector<E_PLAT_TYPE> group_types;
	for (const auto& item : list_tgt_fmt_align) {
		const S_FORMAT& format = item.second;
		if (!format.valid) continue;
		group_ids.push_back(item.first);
		group_types.push_back(representative_type(format, list_tgt_plat_align));
	}
	const int group_num = static_cast<int>(group_ids.size());
	if (group_num <= 0) return;

	std::vector<int> threat_order(group_num, 0);
	std::vector<int> threat_index(group_num, -1);
	std::vector<int> unsorted_index;
	unsorted_index.reserve(group_num);
	for (int index = 0; index < group_num; ++index) {
		const auto fmt_iter = list_tgt_fmt_align.find(group_ids[index]);
		if (fmt_iter == list_tgt_fmt_align.end()) continue;

		const int sort = fmt_iter->second.algo_threat.threat_sort;
		if (sort > 0 && sort <= group_num && threat_index[sort - 1] < 0) {
			threat_order[index] = sort;
			threat_index[sort - 1] = index;
		}
		else {
			unsorted_index.push_back(index);
		}
	}
	std::sort(unsorted_index.begin(), unsorted_index.end(), [&](const int lhs, const int rhs) {
		const auto lhs_iter = list_tgt_fmt_align.find(group_ids[lhs]);
		const auto rhs_iter = list_tgt_fmt_align.find(group_ids[rhs]);
		const double lhs_threat = lhs_iter == list_tgt_fmt_align.end() ? 0.0 : lhs_iter->second.algo_threat.threat_value;
		const double rhs_threat = rhs_iter == list_tgt_fmt_align.end() ? 0.0 : rhs_iter->second.algo_threat.threat_value;
		if (std::abs(lhs_threat - rhs_threat) > 1.0e-9) return lhs_threat > rhs_threat;
		return group_ids[lhs] < group_ids[rhs];
	});
	for (int slot = 0, fill_index = 0; slot < group_num && fill_index < static_cast<int>(unsorted_index.size()); ++slot) {
		if (threat_index[slot] >= 0) continue;
		const int index = unsorted_index[fill_index++];
		threat_order[index] = slot + 1;
		threat_index[slot] = index;
	}

	std::array<std::vector<double>, kAbilityNum> group_weight;
	for (auto& item : group_weight) {
		item.assign(group_num, 0.0);
	}

	for (int ability = 0; ability < kAbilityNum; ++ability) {
		E_PLAT_TYPE last_type = E_PLAT_TYPE::UKN;
		double last_weight = -1.0;

		for (int order = 0; order < group_num; ++order) {
			const int group_index = threat_index[order];
			if (group_index < 0 || group_index >= group_num) continue;
			const E_PLAT_TYPE type = group_types[group_index];
			if (!has_ocog_ability(type, ability)) continue;

			double weight = 1.0;
			if (last_weight >= 0.0) {
				weight = (type == last_type ? 0.9 : 0.7) * last_weight;
			}
			group_weight[ability][group_index] = weight;
			last_type = type;
			last_weight = weight;
		}

		double sum = 0.0;
		for (const double weight : group_weight[ability]) {
			sum += weight;
		}
		if (std::abs(sum) < kDoubleLimit) continue;
		for (double& weight : group_weight[ability]) {
			weight /= sum;
		}
	}

	double ability_weight_sum = 0.0;
	for (int ability = 0; ability < kAbilityNum; ++ability) {
		ability_weight_sum += ocog_ability_weight(ability);
	}
	if (std::abs(ability_weight_sum) < kDoubleLimit) return;

	std::vector<double> cog_value(group_num, 0.0);
	for (int selected_group = 0; selected_group < group_num; ++selected_group) {
		double selected_cog = 0.0;

		for (int ability = 0; ability < kAbilityNum; ++ability) {
			double ability_prob = 0.0;
			for (int group_index = 0; group_index < group_num; ++group_index) {
				const double group_prior = group_index == selected_group ? 0.98 : 0.02;
				ability_prob += group_weight[ability][group_index] * group_prior;
			}
			selected_cog += ability_prob * ocog_ability_weight(ability) / ability_weight_sum;
		}
		cog_value[selected_group] = clamp01(selected_cog);
	}

	std::vector<int> cog_index(group_num);
	for (int index = 0; index < group_num; ++index) {
		cog_index[index] = index;
	}
	std::sort(cog_index.begin(), cog_index.end(), [&](const int lhs, const int rhs) {
		if (std::abs(cog_value[lhs] - cog_value[rhs]) > 1.0e-9) return cog_value[lhs] > cog_value[rhs];
		if (threat_order[lhs] != threat_order[rhs]) return threat_order[lhs] < threat_order[rhs];
		return group_ids[lhs] < group_ids[rhs];
	});

	for (int rank = 0; rank < group_num; ++rank) {
		const int group_index = cog_index[rank];
		S_ALGO_COG cog{};
		cog.cog_value = cog_value[group_index];
		cog.cog_sort = rank + 1;
		cog.cog_level = cog_level_by_sort(cog.cog_sort, group_num, group_types[group_index]);

		auto fmt_iter = list_tgt_fmt_align.find(group_ids[group_index]);
		if (fmt_iter != list_tgt_fmt_align.end()) {
			fmt_iter->second.algo_cog = cog;
		}
	}
}
