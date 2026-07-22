#include "pch.hpp"
#include "SimWeapon.hpp"
#include "../Platform/SimPlatform.hpp"
#include "../../Core/Util/UtilCoor.hpp"

SimWeapon::SimWeapon(const SimPlatform& own_platform, const SimPlatform& tgt_platform,
	const  S_WEAPON_DEFINE& define, const QString ID)
{
	this->weapon_id		= ID;
	this->type_name		= define.type_name;
	this->own_plat_id	= own_platform.plt_id;
	this->tgt_plat_id	= tgt_platform.plt_id;

	this->side			= own_platform.side	;
	this->domain		= define.domain		;
	this->type			= define.type		;
	this->mover			= define.mover		;
	this->mover_cap		= define.mover_cap	;
	this->launch_cap	= define.launch_cap	;
	this->kill_cap		= define.kill_cap	;
	this->sign			= define.sign		;

	init_state.pos_lla			= own_platform.plat_state.pos_lla		;
	init_state.pos_ecef			= own_platform.plat_state.pos_ecef		;
	init_state.vel_loc			= own_platform.plat_state.vel_loc		;
	init_state.update_time_ms	= own_platform.plat_state.update_time_ms;
	init_state.move_range_m = 0.0;
	init_state.move_time_s	= 0.0;

	weapon_state		= init_state;
	weapon_life.state	= E_WEAPON_LIFE::FLYING;
	weapon_life.active	= true;
	if (const auto* mis_cap = std::get_if<S_MIS_MOVER_CAP>(&mover_cap)) {
		weapon_mover = std::make_unique<SimMisMover>(*mis_cap);
	}
}

void SimWeapon::step(const S_PLAT_STATE& tgt_plat_state,double step_s)
{
	if (!weapon_life.active || step_s <= 0.0 || weapon_mover == nullptr) return;

	// 更新时间戳
	weapon_state.update_time_ms += static_cast<long long>(std::llround(step_s * 1000.0));
	const double before_time_s = std::max(0.0, weapon_state.move_time_s);
	weapon_state.move_time_s = before_time_s + step_s;

	const double hit_range_m = kill_cap.kill_range_m > 0.0 ? kill_cap.kill_range_m : ARRIVE_AIR_DIS_M;
	if (UtilCoor::cal_dis_ecef(weapon_state.pos_ecef, tgt_plat_state.pos_ecef) > hit_range_m) {
		const double move_dis_m = weapon_mover->step(this->weapon_state, tgt_plat_state, step_s);
		weapon_state.move_range_m += move_dis_m;

		if (UtilCoor::cal_dis_ecef(weapon_state.pos_ecef, tgt_plat_state.pos_ecef) <= hit_range_m) {
			weapon_life.state = E_WEAPON_LIFE::HIT;
			return;
		}

		// 距离寿命和时间寿命判定
		if (const auto* mis_cap = std::get_if<S_MIS_MOVER_CAP>(&mover_cap)) {
			const double max_range_m = mis_cap->life_range_m > 0.0 ? mis_cap->life_range_m : launch_cap.max_range_m;
			if (max_range_m > 0.0 && weapon_state.move_range_m >= max_range_m) {
				weapon_life.state = E_WEAPON_LIFE::OVER_RANGE;
				return;
			}
			if (mis_cap->life_time_s > 0.0 && weapon_state.move_time_s >= mis_cap->life_time_s) {
				weapon_life.state = E_WEAPON_LIFE::OVER_TIME;
				return;
			}
		}
	}
	else {
		weapon_life.state = E_WEAPON_LIFE::HIT;
	}
}
