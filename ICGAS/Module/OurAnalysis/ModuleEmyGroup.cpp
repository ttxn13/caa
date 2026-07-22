#include "pch.hpp"
#include "ModuleEmyGroup.hpp"
#include "../../Core/Comm/CoreDefine.hpp"
#include "../../Core/Util/UtilCoor.hpp"

QMutex ModuleEmyGroup::mutex;
ModuleEmyGroup* ModuleEmyGroup::instance = nullptr;

ModuleEmyGroup* ModuleEmyGroup::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleEmyGroup();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleEmyGroup::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleEmyGroup::ModuleEmyGroup(QObject* parent)
	: QObject(parent)
{
}

ModuleEmyGroup::~ModuleEmyGroup()
{
}

void ModuleEmyGroup::func_emy_group(
			std::map<QString, S_FORMAT>	& list_own_fmt_align	,		std::map<QString, S_FORMAT>	& list_tgt_fmt_align	,
	const std::map<QString, S_PLAT>		& list_own_plat_align	, const std::map<QString, S_PLAT>	& list_tgt_plat_align	,
	const std::map<QString, S_WEAPON>	& list_own_weapon_align	, const std::map<QString, S_WEAPON>	& list_tgt_weapon_align	)
{

	this->inherit_fmt(list_own_fmt_align, list_own_plat_align, E_SIDE::RED);
	this->inherit_fmt(list_tgt_fmt_align, list_tgt_plat_align, E_SIDE::BLUE);
	//this->solve_tgt_fmt		(list_tgt_fmt_align, list_tgt_plat_align, list_tgt_weapon_align);
}

void ModuleEmyGroup::inherit_fmt(
	std::map<QString, S_FORMAT>& list_fmt_align,
	const std::map<QString, S_PLAT>& list_plat_align,
	const E_SIDE side)
{
	std::map<QString, std::vector<QString>> list_fmt_plat_id;
	for (const auto& plat_item : list_plat_align) {
		QString fmt_id = plat_item.second.fmt_id.trimmed();
		if (fmt_id.isEmpty()) fmt_id = plat_item.first;
		list_fmt_plat_id[fmt_id].push_back(plat_item.first);
	}

	for (auto fmt_iter = list_fmt_align.begin(); fmt_iter != list_fmt_align.end();) {
		if (list_fmt_plat_id.find(fmt_iter->first) == list_fmt_plat_id.end()) {
			fmt_iter = list_fmt_align.erase(fmt_iter);
		}
		else {
			++fmt_iter;
		}
	}

	for (const auto& fmt_plat_item : list_fmt_plat_id) {
		S_FORMAT& format = list_fmt_align[fmt_plat_item.first];
		format.side = side;
		format.fmt_id = fmt_plat_item.first;
		format.valid = true;
		if (!same_plat_id_list(format.plat_id_list, fmt_plat_item.second)) {
			format.plat_id_list = fmt_plat_item.second;
		}
	}

	for (auto& fmt_item : list_fmt_align) {
		S_FORMAT& format = fmt_item.second;
		std::vector<const S_MOTION_FRAME*> list_member_motion;
		bool has_all_member_motion = true;

		for (const QString& plat_id : format.plat_id_list) {
			const auto plat_iter = list_plat_align.find(plat_id);
			if (plat_iter == list_plat_align.end() || plat_iter->second.list_motion.empty()) {
				has_all_member_motion = false;
				break;
			}
			list_member_motion.push_back(&plat_iter->second.list_motion.back());
		}

		if (!has_all_member_motion || list_member_motion.empty()) continue;
		const S_MOTION_FRAME center_motion = average_motion_frame(list_member_motion);
		format.fmt_range_m = format_range_m(center_motion, list_member_motion);
		push_format_motion(format, center_motion);
	}
}

void ModuleEmyGroup::solve_tgt_fmt(
	std::map<QString, S_FORMAT>& list_tgt_fmt_align,
	const std::map<QString, S_PLAT>& list_tgt_plat_align,
	const std::map<QString, S_WEAPON>& list_tgt_weapon_align,
	const E_SIDE side)
{
	(void)list_tgt_weapon_align;
	this->inherit_fmt(list_tgt_fmt_align, list_tgt_plat_align, side);
}

bool ModuleEmyGroup::same_plat_id_list(const std::vector<QString>& lhs, const std::vector<QString>& rhs) const
{
	if (lhs.size() != rhs.size()) return false;
	for (int i = 0; i < static_cast<int>(lhs.size()); ++i) {
		if (lhs[i] != rhs[i]) return false;
	}
	return true;
}

double ModuleEmyGroup::average_angle_rad(const double sin_sum, const double cos_sum, const double fallback_rad, const bool norm_0_2pi) const
{
	if (std::abs(sin_sum) < 1.0e-12 && std::abs(cos_sum) < 1.0e-12) {
		return norm_0_2pi ? UtilCoor::norm_rad_0_2pi(fallback_rad) : UtilCoor::norm_rad_pi_pi(fallback_rad);
	}

	const double angle_rad = std::atan2(sin_sum, cos_sum);
	return norm_0_2pi ? UtilCoor::norm_rad_0_2pi(angle_rad) : UtilCoor::norm_rad_pi_pi(angle_rad);
}

S_MOTION_FRAME ModuleEmyGroup::average_motion_frame(const std::vector<const S_MOTION_FRAME*>& list_member_motion) const
{
	S_MOTION_FRAME motion{};
	if (list_member_motion.empty()) return motion;

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

	for (const S_MOTION_FRAME* member_motion : list_member_motion) {
		pos_ecef_x_sum += member_motion->pos_ecef.x_m;
		pos_ecef_y_sum += member_motion->pos_ecef.y_m;
		pos_ecef_z_sum += member_motion->pos_ecef.z_m;
		vel_ecef_x_sum += member_motion->vel_ecef.vx_ms;
		vel_ecef_y_sum += member_motion->vel_ecef.vy_ms;
		vel_ecef_z_sum += member_motion->vel_ecef.vz_ms;

		lon_sin_sum += std::sin(member_motion->pos_lla.lon_rad);
		lon_cos_sum += std::cos(member_motion->pos_lla.lon_rad);
		lat_sum += member_motion->pos_lla.lat_rad;
		alt_sum += member_motion->pos_lla.alt_m;

		speed_sum += member_motion->vel_loc.speed_ms;
		track_sin_sum += std::sin(member_motion->vel_loc.track_ang_rad);
		track_cos_sum += std::cos(member_motion->vel_loc.track_ang_rad);
		path_sin_sum += std::sin(member_motion->vel_loc.path_ang_rad);
		path_cos_sum += std::cos(member_motion->vel_loc.path_ang_rad);
		update_time_sum += member_motion->update_time_ms;
	}

	const double inv_size = 1.0 / static_cast<double>(list_member_motion.size());
	const S_MOTION_FRAME* first_motion = list_member_motion.front();

	motion.pos_ecef.x_m = pos_ecef_x_sum * inv_size;
	motion.pos_ecef.y_m = pos_ecef_y_sum * inv_size;
	motion.pos_ecef.z_m = pos_ecef_z_sum * inv_size;
	motion.vel_ecef.vx_ms = vel_ecef_x_sum * inv_size;
	motion.vel_ecef.vy_ms = vel_ecef_y_sum * inv_size;
	motion.vel_ecef.vz_ms = vel_ecef_z_sum * inv_size;

	motion.pos_lla.lon_rad = average_angle_rad(lon_sin_sum, lon_cos_sum, first_motion->pos_lla.lon_rad, false);
	motion.pos_lla.lat_rad = lat_sum * inv_size;
	motion.pos_lla.alt_m = alt_sum * inv_size;

	motion.vel_loc.speed_ms = speed_sum * inv_size;
	motion.vel_loc.track_ang_rad = average_angle_rad(track_sin_sum, track_cos_sum, first_motion->vel_loc.track_ang_rad, true);
	motion.vel_loc.path_ang_rad = average_angle_rad(path_sin_sum, path_cos_sum, first_motion->vel_loc.path_ang_rad, false);
	motion.update_time_ms = static_cast<long long>(update_time_sum * inv_size + 0.5);

	return motion;
}

double ModuleEmyGroup::format_range_m(
	const S_MOTION_FRAME& center_motion,
	const std::vector<const S_MOTION_FRAME*>& list_member_motion) const
{
	double max_range_m = 0.0;
	for (const S_MOTION_FRAME* member_motion : list_member_motion) {
		if (member_motion == nullptr) continue;

		const double dx_m = member_motion->pos_ecef.x_m - center_motion.pos_ecef.x_m;
		const double dy_m = member_motion->pos_ecef.y_m - center_motion.pos_ecef.y_m;
		const double dz_m = member_motion->pos_ecef.z_m - center_motion.pos_ecef.z_m;
		const double range_m = std::sqrt(dx_m * dx_m + dy_m * dy_m + dz_m * dz_m);
		if (std::isfinite(range_m)) {
			max_range_m = std::max(max_range_m, range_m);
		}
	}
	return max_range_m;
}

void ModuleEmyGroup::push_format_motion(S_FORMAT& format, const S_MOTION_FRAME& motion) const
{
	if (motion.update_time_ms <= 0) return;

	format.list_motion.push_back(motion);
	const int overflow_size = static_cast<int>(format.list_motion.size()) - ALIGN_MAX_SIZE;
	if (overflow_size > 0) {
		format.list_motion.erase(format.list_motion.begin(), format.list_motion.begin() + overflow_size);
	}
}
