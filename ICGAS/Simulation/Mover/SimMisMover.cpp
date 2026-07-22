#include "pch.hpp"
#include "SimMisMover.hpp"
#include "../../Core/Util/UtilCoor.hpp"

SimMisMover::SimMisMover(const S_MIS_MOVER_CAP& mover_cap):
	cap(mover_cap)
{
}

E_MOVER SimMisMover::mover_type() const
{
	return E_MOVER::MIS_MOVER;
}

double SimMisMover::step(S_WEAPON_STATE& weapon_state, const S_PLAT_STATE& tgt_state, double step_s)
{
	if (step_s <= 0.0) return 0.0;

	const S_POS_ECEF before_pos_ecef = weapon_state.pos_ecef;
	const double target_speed_ms = cap.max_speed_ms > 0.0 ? cap.max_speed_ms : weapon_state.vel_loc.speed_ms;
	const double linear_acc_ms2 = cap.max_linear_load_g * GRAVITY_MS2;

	if (linear_acc_ms2 > 0.0) {
		if (target_speed_ms > weapon_state.vel_loc.speed_ms) {
			weapon_state.vel_loc.speed_ms = std::min(target_speed_ms,
				weapon_state.vel_loc.speed_ms + linear_acc_ms2 * step_s);
		}
		else {
			weapon_state.vel_loc.speed_ms = std::max(target_speed_ms,
				weapon_state.vel_loc.speed_ms - linear_acc_ms2 * step_s);
		}
	}
	else {
		weapon_state.vel_loc.speed_ms = target_speed_ms;
	}
	if (cap.min_speed_ms > 0.0) weapon_state.vel_loc.speed_ms = std::max(weapon_state.vel_loc.speed_ms, cap.min_speed_ms);
	if (cap.max_speed_ms > 0.0) weapon_state.vel_loc.speed_ms = std::min(weapon_state.vel_loc.speed_ms, cap.max_speed_ms);

	S_POS_LLA target_pos_lla = tgt_state.pos_lla;
	if (cap.max_alt_m > cap.min_alt_m) {
		target_pos_lla.alt_m = std::clamp(target_pos_lla.alt_m, cap.min_alt_m, cap.max_alt_m);
	}

	const double climb_rate_ms = cap.max_climb_vel_ms * GRAVITY_MS2;
	const double drop_rate_ms = cap.max_drop_vel_ms * GRAVITY_MS2;
	if (target_pos_lla.alt_m > weapon_state.pos_lla.alt_m) {
		weapon_state.pos_lla.alt_m = climb_rate_ms > 0.0 ?
			std::min(target_pos_lla.alt_m, weapon_state.pos_lla.alt_m + climb_rate_ms * step_s) :
			target_pos_lla.alt_m;
		weapon_state.vel_loc.path_ang_rad = weapon_state.vel_loc.speed_ms <= 0.0 ? 0.0 :
			std::atan(climb_rate_ms / weapon_state.vel_loc.speed_ms);
	}
	else {
		weapon_state.pos_lla.alt_m = drop_rate_ms > 0.0 ?
			std::max(target_pos_lla.alt_m, weapon_state.pos_lla.alt_m - drop_rate_ms * step_s) :
			target_pos_lla.alt_m;
		weapon_state.vel_loc.path_ang_rad = weapon_state.vel_loc.speed_ms <= 0.0 ? 0.0 :
			std::atan(-drop_rate_ms / weapon_state.vel_loc.speed_ms);
	}

	weapon_state.pos_ecef = UtilCoor::pos_lla2ecef(weapon_state.pos_lla);
	const S_POS_ECEF target_pos_ecef = UtilCoor::pos_lla2ecef(target_pos_lla);
	const S_POS_ECEF target_pos_del(target_pos_ecef.x_m - weapon_state.pos_ecef.x_m,
		target_pos_ecef.y_m - weapon_state.pos_ecef.y_m,
		target_pos_ecef.z_m - weapon_state.pos_ecef.z_m);
	const S_POS_ENU target_pos_enu = UtilCoor::pos_ecef2enu(target_pos_del, weapon_state.pos_lla);

	const double target_track_ang_rad = UtilCoor::norm_rad_0_2pi(std::atan2(target_pos_enu.e_m, target_pos_enu.n_m));
	const double del_track_ang_rad = UtilCoor::norm_rad_pi_pi(target_track_ang_rad - weapon_state.vel_loc.track_ang_rad);

	if (weapon_state.vel_loc.speed_ms <= 0.0 || cap.max_radial_load_g <= 0.0) {
		S_WEAPON_STATE del = step_del_linear(&weapon_state);
		add_weapon_state(&weapon_state, &del, step_s);
	}
	else {
		const double omega_rads = cap.max_radial_load_g * GRAVITY_MS2 / weapon_state.vel_loc.speed_ms;
		if (std::fabs(del_track_ang_rad) < ARRIVE_AIR_ANG_DEG DEG2RAD) {
			S_WEAPON_STATE del = step_del_linear(&weapon_state);
			add_weapon_state(&weapon_state, &del, step_s);
		}
		else if (omega_rads * step_s < std::fabs(del_track_ang_rad)) {
			const double del_angle_rad = del_track_ang_rad > 0.0 ? omega_rads * step_s : -omega_rads * step_s;
			S_WEAPON_STATE del = step_del_radial(&weapon_state, del_angle_rad, cap.max_radial_load_g);
			add_weapon_state(&weapon_state, &del, step_s);
		}
		else {
			const double step_radial_s = std::fabs(del_track_ang_rad) / omega_rads;
			const double step_linear_s = step_s - step_radial_s;
			S_WEAPON_STATE radial_del = step_del_radial(&weapon_state, del_track_ang_rad, cap.max_radial_load_g);
			add_weapon_state(&weapon_state, &radial_del, step_radial_s);
			S_WEAPON_STATE linear_del = step_del_linear(&weapon_state);
			add_weapon_state(&weapon_state, &linear_del, step_linear_s);
		}
	}

	const double dx_m = weapon_state.pos_ecef.x_m - before_pos_ecef.x_m;
	const double dy_m = weapon_state.pos_ecef.y_m - before_pos_ecef.y_m;
	const double dz_m = weapon_state.pos_ecef.z_m - before_pos_ecef.z_m;
	return std::sqrt(dx_m * dx_m + dy_m * dy_m + dz_m * dz_m);
}

void SimMisMover::add_weapon_state(S_WEAPON_STATE* weapon_state, const S_WEAPON_STATE* del_weapon_state, double step_s) const
{
	weapon_state->pos_ecef.x_m += del_weapon_state->pos_ecef.x_m * step_s;
	weapon_state->pos_ecef.y_m += del_weapon_state->pos_ecef.y_m * step_s;
	weapon_state->pos_ecef.z_m += del_weapon_state->pos_ecef.z_m * step_s;
	weapon_state->pos_lla = UtilCoor::pos_ecef2lla(weapon_state->pos_ecef);

	weapon_state->vel_loc.speed_ms += del_weapon_state->vel_loc.speed_ms * step_s;
	weapon_state->vel_loc.track_ang_rad += del_weapon_state->vel_loc.track_ang_rad * step_s;
	weapon_state->vel_loc.path_ang_rad += del_weapon_state->vel_loc.path_ang_rad * step_s;
	weapon_state->vel_loc.track_ang_rad = UtilCoor::norm_rad_0_2pi(weapon_state->vel_loc.track_ang_rad);
	weapon_state->vel_loc.path_ang_rad = UtilCoor::norm_rad_pi_pi(weapon_state->vel_loc.path_ang_rad);
}

S_WEAPON_STATE SimMisMover::step_del_linear(const S_WEAPON_STATE* weapon_state) const
{
	S_WEAPON_STATE del_weapon_state;
	const S_VEL_ENU vel_enu(weapon_state->vel_loc.speed_ms * std::sin(weapon_state->vel_loc.track_ang_rad),
		weapon_state->vel_loc.speed_ms * std::cos(weapon_state->vel_loc.track_ang_rad), 0.0);
	const S_VEL_ECEF vel_ecef = UtilCoor::vel_enu2ecef(vel_enu, weapon_state->pos_lla);
	del_weapon_state.pos_ecef = S_POS_ECEF(vel_ecef.vx_ms, vel_ecef.vy_ms, vel_ecef.vz_ms);
	return del_weapon_state;
}

S_WEAPON_STATE SimMisMover::step_del_radial(const S_WEAPON_STATE* weapon_state, double del_angle_rad, double max_radial_load_g) const
{
	S_WEAPON_STATE del_weapon_state;

	if (weapon_state->vel_loc.speed_ms <= 0.0 || max_radial_load_g <= 0.0) return del_weapon_state;

	const double omega_rads = max_radial_load_g * GRAVITY_MS2 / weapon_state->vel_loc.speed_ms;
	const double step_s = std::abs(del_angle_rad) / omega_rads;
	if (step_s <= 0.0) return del_weapon_state;

	const double turn_dir = del_angle_rad > 0.0 ? 1.0 : -1.0;
	const double abs_angle_rad = std::abs(del_angle_rad);
	const double turn_radius_m = weapon_state->vel_loc.speed_ms * weapon_state->vel_loc.speed_ms /
		(max_radial_load_g * GRAVITY_MS2);
	const double forward_m = turn_radius_m * std::sin(abs_angle_rad);
	const double side_m = turn_dir * turn_radius_m * (1.0 - std::cos(abs_angle_rad));
	const double sin_track = std::sin(weapon_state->vel_loc.track_ang_rad);
	const double cos_track = std::cos(weapon_state->vel_loc.track_ang_rad);
	const S_VEL_ENU vel_enu((forward_m * sin_track + side_m * cos_track) / step_s,
		(forward_m * cos_track - side_m * sin_track) / step_s, 0.0);
	const S_VEL_ECEF vel_ecef = UtilCoor::vel_enu2ecef(vel_enu, weapon_state->pos_lla);

	del_weapon_state.pos_ecef = S_POS_ECEF(vel_ecef.vx_ms, vel_ecef.vy_ms, vel_ecef.vz_ms);
	del_weapon_state.vel_loc.track_ang_rad = del_angle_rad / step_s;
	return del_weapon_state;
}
