#pragma once
#include "SimMover.hpp"

class SimSeaMover : public SimMover
{
public:
	E_MOVER mover_type() const override;
	void step(SimPlatform* platform, S_ROUTE_POINT* route_point, double step_s);

private:
	void add_plat_state(S_PLAT_STATE* plat_state, const S_PLAT_STATE* del_plat_state, double step_s);
	S_PLAT_STATE step_del_linear(const S_PLAT_STATE* plat_state) const;
	S_PLAT_STATE step_del_radial(const S_PLAT_STATE* plat_state, double del_angle_rad, double max_radial_acc_ms2) const;
};
