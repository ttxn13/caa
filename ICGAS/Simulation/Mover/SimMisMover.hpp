#pragma once
#include "SimMover.hpp"
#include "../../Core/Sim/DefWeapon.hpp"

class SimMisMover : public SimMover
{
public:
	SimMisMover() = default;
	explicit SimMisMover(const S_MIS_MOVER_CAP& mover_cap);
	E_MOVER mover_type() const override;
	double step(S_WEAPON_STATE& weapon_state, const S_PLAT_STATE& tgt_state, double step_s);

private:
	void add_weapon_state(S_WEAPON_STATE* weapon_state, const S_WEAPON_STATE* del_weapon_state, double step_s) const;
	S_WEAPON_STATE step_del_linear(const S_WEAPON_STATE* weapon_state) const;
	S_WEAPON_STATE step_del_radial(const S_WEAPON_STATE* weapon_state, double del_angle_rad, double max_radial_load_g) const;

	S_MIS_MOVER_CAP cap;
};
