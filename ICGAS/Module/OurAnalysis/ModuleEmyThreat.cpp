#include "pch.hpp"
#include "ModuleEmyThreat.hpp"
#include "../../Core/Util/UtilCoor.hpp"

QMutex ModuleEmyThreat::mutex;
ModuleEmyThreat* ModuleEmyThreat::instance = nullptr;

namespace {
constexpr double kDoubleLimit = 1.0e-5;
constexpr double kNormDistanceM = 5.0e6;
constexpr double kNormTimeS = 100.0;

struct RelativeMotion {
	double x_m = 0.0;
	double y_m = 0.0;
	double z_m = 0.0;
	double vx_ms = 0.0;
	double vy_ms = 0.0;
	double vz_ms = 0.0;
};

const S_MOTION_FRAME* latest_motion(const S_PLAT& plat)
{
	return plat.list_motion.empty() ? nullptr : &plat.list_motion.back();
}

const S_MOTION_FRAME* latest_motion(const S_FORMAT& format)
{
	return format.list_motion.empty() ? nullptr : &format.list_motion.back();
}

double clamp01(const double value)
{
	if (!std::isfinite(value)) return 0.0;
	return std::clamp(value, 0.0, 1.0);
}

E_LEVEL threat_level(const double threat_value)
{
	if (threat_value >= 0.75) return E_LEVEL::HIGH;
	if (threat_value >= 0.45) return E_LEVEL::MID;
	return E_LEVEL::LOW;
}

S_ALGO_THREAT make_threat(const double value, const int sort = 0)
{
	S_ALGO_THREAT threat{};
	threat.threat_value = clamp01(value);
	threat.threat_level = threat_level(threat.threat_value);
	threat.threat_sort = sort;
	return threat;
}

double average_angle_rad(const double sin_sum, const double cos_sum, const double fallback_rad, const bool norm_0_2pi)
{
	if (std::abs(sin_sum) < 1.0e-12 && std::abs(cos_sum) < 1.0e-12) {
		return norm_0_2pi ? UtilCoor::norm_rad_0_2pi(fallback_rad) : UtilCoor::norm_rad_pi_pi(fallback_rad);
	}

	const double angle_rad = std::atan2(sin_sum, cos_sum);
	return norm_0_2pi ? UtilCoor::norm_rad_0_2pi(angle_rad) : UtilCoor::norm_rad_pi_pi(angle_rad);
}

S_MOTION_FRAME average_motion_frame(const std::vector<const S_MOTION_FRAME*>& motions)
{
	S_MOTION_FRAME motion{};
	if (motions.empty()) return motion;

	double pos_ecef_x_sum = 0.0;
	double pos_ecef_y_sum = 0.0;
	double pos_ecef_z_sum = 0.0;
	double vel_ecef_x_sum = 0.0;
	double vel_ecef_y_sum = 0.0;
	double vel_ecef_z_sum = 0.0;
	double lon_sin_sum = 0.0;
	double lon_cos_sum = 0.0;
	double lat_sum = 0.0;
	double alt_sum = 0.0;
	double speed_sum = 0.0;
	double track_sin_sum = 0.0;
	double track_cos_sum = 0.0;
	double path_sin_sum = 0.0;
	double path_cos_sum = 0.0;
	long double update_time_sum = 0.0;

	for (const S_MOTION_FRAME* item : motions) {
		if (item == nullptr) continue;
		pos_ecef_x_sum += item->pos_ecef.x_m;
		pos_ecef_y_sum += item->pos_ecef.y_m;
		pos_ecef_z_sum += item->pos_ecef.z_m;
		vel_ecef_x_sum += item->vel_ecef.vx_ms;
		vel_ecef_y_sum += item->vel_ecef.vy_ms;
		vel_ecef_z_sum += item->vel_ecef.vz_ms;
		lon_sin_sum += std::sin(item->pos_lla.lon_rad);
		lon_cos_sum += std::cos(item->pos_lla.lon_rad);
		lat_sum += item->pos_lla.lat_rad;
		alt_sum += item->pos_lla.alt_m;
		speed_sum += item->vel_loc.speed_ms;
		track_sin_sum += std::sin(item->vel_loc.track_ang_rad);
		track_cos_sum += std::cos(item->vel_loc.track_ang_rad);
		path_sin_sum += std::sin(item->vel_loc.path_ang_rad);
		path_cos_sum += std::cos(item->vel_loc.path_ang_rad);
		update_time_sum += item->update_time_ms;
	}

	const double inv_size = 1.0 / static_cast<double>(motions.size());
	const S_MOTION_FRAME* first = motions.front();
	motion.pos_ecef.x_m = pos_ecef_x_sum * inv_size;
	motion.pos_ecef.y_m = pos_ecef_y_sum * inv_size;
	motion.pos_ecef.z_m = pos_ecef_z_sum * inv_size;
	motion.vel_ecef.vx_ms = vel_ecef_x_sum * inv_size;
	motion.vel_ecef.vy_ms = vel_ecef_y_sum * inv_size;
	motion.vel_ecef.vz_ms = vel_ecef_z_sum * inv_size;
	motion.pos_lla.lon_rad = average_angle_rad(lon_sin_sum, lon_cos_sum, first->pos_lla.lon_rad, false);
	motion.pos_lla.lat_rad = lat_sum * inv_size;
	motion.pos_lla.alt_m = alt_sum * inv_size;
	motion.vel_loc.speed_ms = speed_sum * inv_size;
	motion.vel_loc.track_ang_rad = average_angle_rad(track_sin_sum, track_cos_sum, first->vel_loc.track_ang_rad, true);
	motion.vel_loc.path_ang_rad = average_angle_rad(path_sin_sum, path_cos_sum, first->vel_loc.path_ang_rad, false);
	motion.update_time_ms = static_cast<long long>(update_time_sum * inv_size + 0.5);
	return motion;
}

bool make_reference_motion(
	const std::map<QString, S_FORMAT>& reference_formats,
	const std::map<QString, S_PLAT>& reference_plats,
	S_MOTION_FRAME& reference_motion)
{
	for (const auto& item : reference_plats) {
		const S_PLAT& plat = item.second;
		if (!plat.valid || plat.type != E_PLAT_TYPE::CAR) continue;
		if (const S_MOTION_FRAME* motion = latest_motion(plat)) {
			reference_motion = *motion;
			return true;
		}
	}

	std::vector<const S_MOTION_FRAME*> motions;
	for (const auto& item : reference_formats) {
		const S_FORMAT& format = item.second;
		if (!format.valid) continue;
		if (const S_MOTION_FRAME* motion = latest_motion(format)) {
			motions.push_back(motion);
		}
	}
	if (motions.empty()) {
		for (const auto& item : reference_plats) {
			const S_PLAT& plat = item.second;
			if (!plat.valid) continue;
			if (const S_MOTION_FRAME* motion = latest_motion(plat)) {
				motions.push_back(motion);
			}
		}
	}
	if (motions.empty()) return false;

	reference_motion = average_motion_frame(motions);
	return reference_motion.update_time_ms > 0;
}

RelativeMotion relative_motion(const S_MOTION_FRAME& target_motion, const S_MOTION_FRAME& reference_motion)
{
	const S_POS_ECEF pos_delta(
		target_motion.pos_ecef.x_m - reference_motion.pos_ecef.x_m,
		target_motion.pos_ecef.y_m - reference_motion.pos_ecef.y_m,
		target_motion.pos_ecef.z_m - reference_motion.pos_ecef.z_m);
	const S_VEL_ECEF vel_delta(
		target_motion.vel_ecef.vx_ms - reference_motion.vel_ecef.vx_ms,
		target_motion.vel_ecef.vy_ms - reference_motion.vel_ecef.vy_ms,
		target_motion.vel_ecef.vz_ms - reference_motion.vel_ecef.vz_ms);

	const S_POS_ENU pos_enu = UtilCoor::pos_ecef2enu(pos_delta, reference_motion.pos_lla);
	const S_VEL_ENU vel_enu = UtilCoor::vel_ecef2enu(vel_delta, reference_motion.pos_lla);

	RelativeMotion result;
	result.x_m = pos_enu.e_m;
	result.y_m = pos_enu.n_m;
	result.z_m = pos_enu.u_m;
	result.vx_ms = vel_enu.ve_ms;
	result.vy_ms = vel_enu.vn_ms;
	result.vz_ms = vel_enu.vu_ms;
	return result;
}

double base_threat_value(const RelativeMotion& motion)
{
	const double height_sign = (motion.z_m > 0.0) - (motion.z_m < 0.0);
	double norm_h = height_sign * kNormDistanceM * kNormDistanceM /
		(motion.z_m * motion.z_m + kNormDistanceM * kNormDistanceM);
	norm_h = (norm_h + 1.0) / 2.0;

	const double a = motion.vx_ms * motion.vx_ms + motion.vy_ms * motion.vy_ms + motion.vz_ms * motion.vz_ms;
	const double b = motion.x_m * motion.vx_ms + motion.y_m * motion.vy_ms + motion.z_m * motion.vz_ms;
	const double c = motion.x_m * motion.x_m + motion.y_m * motion.y_m + motion.z_m * motion.z_m;

	double dcpa = 0.0;
	double tcpa = 0.0;
	if (std::abs(a) < kDoubleLimit || b > 0.0) {
		dcpa = std::sqrt(std::max(0.0, c));
	}
	else {
		dcpa = std::sqrt(std::max(0.0, 2.0 * c - 2.0 * b * b / a));
		tcpa = -b / a;
	}

	const double norm_dcpa = std::exp(-dcpa / kNormDistanceM / std::exp(1.0));
	const double norm_tcpa = kNormTimeS * kNormTimeS / (tcpa * tcpa + kNormTimeS * kNormTimeS);

	double angle_gain = 0.5;
	const double vel_horizontal = std::hypot(motion.vx_ms, motion.vy_ms);
	const double pos_horizontal = std::hypot(motion.x_m, motion.y_m);
	if (vel_horizontal >= kDoubleLimit && pos_horizontal >= kDoubleLimit) {
		const double vec_emy_x = motion.vx_ms / vel_horizontal;
		const double vec_emy_y = motion.vy_ms / vel_horizontal;
		const double vec_dis_x = motion.x_m / pos_horizontal;
		const double vec_dis_y = motion.y_m / pos_horizontal;
		const double dot = std::clamp(vec_emy_x * vec_dis_x + vec_emy_y * vec_dis_y, -1.0, 1.0);
		angle_gain = (dot + 2.0) / 4.0;
	}

	return clamp01((norm_h + norm_dcpa + norm_tcpa + angle_gain) / 4.0);
}

int max_member_for_type(const E_PLAT_TYPE type)
{
	switch (type) {
	case E_PLAT_TYPE::MIS: return 12;
	case E_PLAT_TYPE::BOM: return 3;
	case E_PLAT_TYPE::FLT: return 4;
	case E_PLAT_TYPE::WAN:
	case E_PLAT_TYPE::ELC:
	case E_PLAT_TYPE::CAR:
		return 1;
	case E_PLAT_TYPE::DST:
	case E_PLAT_TYPE::FRI:
		return 2;
	default:
		return 1;
	}
}

int threat_type_index(const E_PLAT_TYPE type)
{
	switch (type) {
	case E_PLAT_TYPE::MIS: return 2;
	case E_PLAT_TYPE::BOM: return 3;
	case E_PLAT_TYPE::WAN: return 4;
	case E_PLAT_TYPE::ELC: return 5;
	case E_PLAT_TYPE::FLT: return 6;
	case E_PLAT_TYPE::CAR: return 1;
	case E_PLAT_TYPE::DST: return 8;
	case E_PLAT_TYPE::FRI: return 8;
	default: return 1;
	}
}

E_PLAT_TYPE representative_type(
	const S_FORMAT& format,
	const std::map<QString, S_PLAT>& platforms,
	int& valid_member_count)
{
	std::map<E_PLAT_TYPE, int> type_counts;
	valid_member_count = 0;
	for (const QString& plat_id : format.plat_id_list) {
		const auto plat_iter = platforms.find(plat_id);
		if (plat_iter == platforms.end() || !plat_iter->second.valid || latest_motion(plat_iter->second) == nullptr) {
			continue;
		}
		type_counts[plat_iter->second.type]++;
		valid_member_count++;
	}

	E_PLAT_TYPE result = E_PLAT_TYPE::UKN;
	int best_count = -1;
	int best_type_index = std::numeric_limits<int>::max();
	for (const auto& item : type_counts) {
		const int index = threat_type_index(item.first);
		if (item.second > best_count || (item.second == best_count && index < best_type_index)) {
			result = item.first;
			best_count = item.second;
			best_type_index = index;
		}
	}
	return result;
}

double format_threat_value(
	const S_FORMAT& format,
	const std::map<QString, S_PLAT>& platforms,
	const S_MOTION_FRAME& reference_motion)
{
	const S_MOTION_FRAME* motion = latest_motion(format);
	if (motion == nullptr) return 0.0;

	int valid_member_count = 0;
	const E_PLAT_TYPE type = representative_type(format, platforms, valid_member_count);
	const int member_count = std::max(1, valid_member_count);
	const int max_member = std::max(1, max_member_for_type(type));
	const double num_gain = std::pow(1.1, static_cast<double>(member_count) / static_cast<double>(max_member) - 1.0);
	const double type_gain = 5.0 / (static_cast<double>(threat_type_index(type)) + 3.0);
	const double base_value = base_threat_value(relative_motion(*motion, reference_motion));

	return clamp01((base_value * 4.0 + num_gain + type_gain) / 6.0);
}

template<typename Item>
void assign_global_sort(std::vector<Item>& items)
{
	std::sort(items.begin(), items.end(), [](const Item& lhs, const Item& rhs) {
		const double lhs_value = lhs.threat->threat_value;
		const double rhs_value = rhs.threat->threat_value;
		if (std::abs(lhs_value - rhs_value) > 1.0e-9) return lhs_value > rhs_value;
		return lhs.id < rhs.id;
	});

	for (int index = 0; index < static_cast<int>(items.size()); ++index) {
		items[index].threat->threat_sort = index + 1;
	}
}

struct SortItem {
	QString id;
	S_ALGO_THREAT* threat = nullptr;
};

void assign_platform_sort_in_format(
	const S_FORMAT& format,
	std::map<QString, S_PLAT>& platforms)
{
	std::vector<SortItem> items;
	for (const QString& plat_id : format.plat_id_list) {
		auto plat_iter = platforms.find(plat_id);
		if (plat_iter == platforms.end() || !plat_iter->second.valid || latest_motion(plat_iter->second) == nullptr) {
			continue;
		}
		items.push_back({plat_iter->first, &plat_iter->second.algo_threat});
	}
	assign_global_sort(items);
}

void solve_side_threat(
	std::map<QString, S_FORMAT>& target_formats,
	std::map<QString, S_PLAT>& target_plats,
	const std::map<QString, S_FORMAT>& reference_formats,
	const std::map<QString, S_PLAT>& reference_plats)
{
	for (auto& item : target_formats) item.second.algo_threat = S_ALGO_THREAT{};
	for (auto& item : target_plats) item.second.algo_threat = S_ALGO_THREAT{};

	S_MOTION_FRAME reference_motion{};
	if (!make_reference_motion(reference_formats, reference_plats, reference_motion)) {
		return;
	}

	for (auto& item : target_plats) {
		S_PLAT& plat = item.second;
		if (!plat.valid) continue;
		const S_MOTION_FRAME* motion = latest_motion(plat);
		if (motion == nullptr) continue;
		plat.algo_threat = make_threat(base_threat_value(relative_motion(*motion, reference_motion)));
	}

	std::vector<SortItem> format_sort_items;
	for (auto& item : target_formats) {
		S_FORMAT& format = item.second;
		if (!format.valid || latest_motion(format) == nullptr) continue;
		format.algo_threat = make_threat(format_threat_value(format, target_plats, reference_motion));
		format_sort_items.push_back({item.first, &format.algo_threat});
	}
	assign_global_sort(format_sort_items);

	std::set<QString> sorted_platform_ids;
	for (const auto& item : target_formats) {
		assign_platform_sort_in_format(item.second, target_plats);
		for (const QString& plat_id : item.second.plat_id_list) {
			sorted_platform_ids.insert(plat_id);
		}
	}

	std::map<QString, std::vector<SortItem>> fallback_groups;
	for (auto& item : target_plats) {
		S_PLAT& plat = item.second;
		if (!plat.valid || latest_motion(plat) == nullptr || sorted_platform_ids.contains(item.first)) continue;
		const QString fmt_id = plat.fmt_id.trimmed().isEmpty() ? item.first : plat.fmt_id.trimmed();
		fallback_groups[fmt_id].push_back({item.first, &plat.algo_threat});
	}
	for (auto& item : fallback_groups) {
		assign_global_sort(item.second);
	}
}
}

ModuleEmyThreat* ModuleEmyThreat::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleEmyThreat();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleEmyThreat::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleEmyThreat::ModuleEmyThreat(QObject* parent)
	: QObject(parent)
{
}

ModuleEmyThreat::~ModuleEmyThreat()
{
}

void ModuleEmyThreat::func_emy_threat(
	std::map<QString, S_FORMAT>& list_own_fmt_align,
	std::map<QString, S_FORMAT>& list_tgt_fmt_align,
	std::map<QString, S_PLAT>& list_own_plat_align,
	std::map<QString, S_PLAT>& list_tgt_plat_align)
{
	for (auto& item : list_own_fmt_align) item.second.algo_threat = S_ALGO_THREAT{};
	for (auto& item : list_own_plat_align) item.second.algo_threat = S_ALGO_THREAT{};
	solve_side_threat(list_tgt_fmt_align, list_tgt_plat_align, list_own_fmt_align, list_own_plat_align);
}
