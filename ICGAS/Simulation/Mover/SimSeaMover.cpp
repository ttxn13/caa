#include "pch.hpp"
#include "SimSeaMover.hpp"
#include "../Platform/SimPlatform.hpp"
#include "../../Core/Util/UtilCoor.hpp"

E_MOVER SimSeaMover::mover_type() const
{
	return E_MOVER::SEA_MOVER;
}

void SimSeaMover::step(SimPlatform* platform, S_ROUTE_POINT* route_point, double step_s)
{
	// 创建变量，存储平台状态和能力
	S_PLAT_STATE* plat_state		= &platform->plat_state;
	const S_SEA_MOVER_CAP* cap	= std::get_if<S_SEA_MOVER_CAP>(&platform->cap);

	// 无航路点，保持速度和航迹直航
	if (route_point == nullptr) {
		[&] { S_PLAT_STATE del = step_del_linear(plat_state); add_plat_state(plat_state, &del, step_s); }();
		return;
	}

	// 航路点高度和速度限制
	S_ROUTE_POINT target_point = *route_point;
	target_point.pos_lla.alt_m	= 0.0;
	target_point.speed_ms		= std::clamp(target_point.speed_ms, cap->min_speed_ms, cap->max_speed_ms);

	// 计算目标速度
	target_point.speed_ms > plat_state->vel_loc.speed_ms ?
		plat_state->vel_loc.speed_ms = std::min(target_point.speed_ms, plat_state->vel_loc.speed_ms + cap->max_linear_acc_ms2 * step_s) :
		plat_state->vel_loc.speed_ms = std::max(target_point.speed_ms, plat_state->vel_loc.speed_ms - cap->max_linear_acc_ms2 * step_s);

	// 计算相对ECEF和ENU坐标
	plat_state->pos_ecef = UtilCoor::pos_lla2ecef(plat_state->pos_lla);
	const S_POS_ECEF target_pos_ecef = UtilCoor::pos_lla2ecef(target_point.pos_lla);
	const S_POS_ECEF target_pos_del(target_pos_ecef.x_m - plat_state->pos_ecef.x_m,
		target_pos_ecef.y_m - plat_state->pos_ecef.y_m,
		target_pos_ecef.z_m - plat_state->pos_ecef.z_m);
	const S_POS_ENU target_pos_enu = UtilCoor::pos_ecef2enu(target_pos_del, plat_state->pos_lla);

	// 计算航点方位角，航点在目标运动方向右侧为正，左侧为负，-pi~pi
	const double target_track_ang_rad = UtilCoor::norm_rad_0_2pi(std::atan2(target_pos_enu.e_m, target_pos_enu.n_m));
	double del_track_ang_rad = UtilCoor::norm_rad_pi_pi(target_track_ang_rad - plat_state->vel_loc.track_ang_rad);
	const double omega_rads = cap->max_radial_acc_ms2 / plat_state->vel_loc.speed_ms;

	// 如果航点在正前方，直航
	if (fabs(del_track_ang_rad) < ARRIVE_AIR_ANG_DEG DEG2RAD) {
		[&] { S_PLAT_STATE del = step_del_linear(plat_state); add_plat_state(plat_state, &del, step_s); }();
		return;
	}

	// 完整转向段迭代
	if (omega_rads * step_s < fabs(del_track_ang_rad)) {
		(del_track_ang_rad > 0) ?
			[&] { S_PLAT_STATE del = step_del_radial(plat_state, omega_rads * step_s, cap->max_radial_acc_ms2); add_plat_state(plat_state, &del, step_s); }() :
			[&] { S_PLAT_STATE del = step_del_radial(plat_state, -omega_rads * step_s, cap->max_radial_acc_ms2); add_plat_state(plat_state, &del, step_s); }();
		return;
	}

	// 转向段+直线段迭代
	const double step_radial_s = fabs(del_track_ang_rad) / omega_rads;
	const double step_linear_s = step_s - step_radial_s;
	[&] { S_PLAT_STATE del = step_del_radial(plat_state, del_track_ang_rad, cap->max_radial_acc_ms2); add_plat_state(plat_state, &del, step_radial_s); }();
	[&] { S_PLAT_STATE del = step_del_linear(plat_state); add_plat_state(plat_state, &del, step_linear_s); }();
}

void SimSeaMover::add_plat_state(S_PLAT_STATE* plat_state, const S_PLAT_STATE* del_plat_state, double step_s)
{
	plat_state->pos_ecef.x_m	+= del_plat_state->pos_ecef.x_m * step_s;
	plat_state->pos_ecef.y_m	+= del_plat_state->pos_ecef.y_m * step_s;
	plat_state->pos_ecef.z_m	+= del_plat_state->pos_ecef.z_m * step_s;
	plat_state->pos_lla = UtilCoor::pos_ecef2lla(plat_state->pos_ecef);

	plat_state->pos_lla.alt_m			= 0.0;
	plat_state->pos_ecef				= UtilCoor::pos_lla2ecef(plat_state->pos_lla);
	plat_state->vel_loc.speed_ms		+= del_plat_state->vel_loc.speed_ms * step_s;
	plat_state->vel_loc.track_ang_rad	+= del_plat_state->vel_loc.track_ang_rad * step_s;
	plat_state->vel_loc.path_ang_rad	 = 0.0;
	plat_state->vel_loc.track_ang_rad	 = UtilCoor::norm_rad_0_2pi(plat_state->vel_loc.track_ang_rad);
}

S_PLAT_STATE SimSeaMover::step_del_linear(const S_PLAT_STATE* plat_state) const
{
	S_PLAT_STATE del_plat_state;
	const S_VEL_ENU vel_enu(plat_state->vel_loc.speed_ms * std::sin(plat_state->vel_loc.track_ang_rad),
							plat_state->vel_loc.speed_ms * std::cos(plat_state->vel_loc.track_ang_rad), 0.0);
	const S_VEL_ECEF vel_ecef = UtilCoor::vel_enu2ecef(vel_enu, plat_state->pos_lla);
	del_plat_state.pos_ecef = S_POS_ECEF(vel_ecef.vx_ms, vel_ecef.vy_ms, vel_ecef.vz_ms);
	return del_plat_state;
}

S_PLAT_STATE SimSeaMover::step_del_radial(const S_PLAT_STATE* plat_state, double del_angle_rad, double max_radial_acc_ms2) const
{
	S_PLAT_STATE del_plat_state;

	const double omega_rads = max_radial_acc_ms2 / plat_state->vel_loc.speed_ms;
	const double step_s = std::abs(del_angle_rad) / omega_rads;

	const double turn_dir = del_angle_rad > 0.0 ? 1.0 : -1.0;
	const double abs_angle_rad = std::abs(del_angle_rad);
	const double turn_radius_m = plat_state->vel_loc.speed_ms * plat_state->vel_loc.speed_ms / max_radial_acc_ms2;
	const double forward_m = turn_radius_m * std::sin(abs_angle_rad);
	const double side_m = turn_dir * turn_radius_m * (1.0 - std::cos(abs_angle_rad));
	const double sin_track = std::sin(plat_state->vel_loc.track_ang_rad);
	const double cos_track = std::cos(plat_state->vel_loc.track_ang_rad);
	const S_VEL_ENU vel_enu((forward_m * sin_track + side_m * cos_track) / step_s,
							(forward_m * cos_track - side_m * sin_track) / step_s, 0.0);
	const S_VEL_ECEF vel_ecef = UtilCoor::vel_enu2ecef(vel_enu, plat_state->pos_lla);

	del_plat_state.pos_ecef = S_POS_ECEF(vel_ecef.vx_ms, vel_ecef.vy_ms, vel_ecef.vz_ms);
	del_plat_state.vel_loc.track_ang_rad = del_angle_rad / step_s;
	return del_plat_state;
}
