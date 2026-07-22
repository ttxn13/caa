#include "pch.hpp"
#include "SimPlatform.hpp"
#include "../../Core/Util/UtilCoor.hpp"
#include "../Mover/SimAirMover.hpp"
#include "../Mover/SimSeaMover.hpp"

namespace {
void step_platform_mover(SimMover* mover, SimPlatform* platform, S_ROUTE_POINT* route_point, double step_s)
{
	if (mover == nullptr) return;

	switch (mover->mover_type()) {
	case E_MOVER::AIR_MOVER:
		static_cast<SimAirMover*>(mover)->step(platform, route_point, step_s);
		break;
	case E_MOVER::SEA_MOVER:
		static_cast<SimSeaMover*>(mover)->step(platform, route_point, step_s);
		break;
	default:
		break;
	}
}
}

SimPlatform::SimPlatform(const S_PLAT_DEFINE& define, const S_PLAT_CONFIG& config)
{
	this->type_name	= define.type_name	;
	this->plt_id	= config.plt_id		;
	this->cmd_id	= config.cmd_id		;
	this->fmt_id	= config.fmt_id		;

	this->side		= config.side	;
	this->domain	= define.domain	;
	this->type		= define.type	;
	this->mover		= define.mover	;

	this->cap		= define.cap	;
	this->sign		= define.sign	;

	this->init_state = config.init_state;
	this->init_route = config.init_route;
	this->plat_state = this->init_state;

	this->plat_mover	= make_sim_mover(define.mover);
	this->weapon_load	= config.weapon_load;
	this->sensor_load	= config.sensor_load;
}

void SimPlatform::step(S_ROUTE_POINT* route_point, double step_s)
{
	// 更新时间戳
	plat_state.update_time_ms += static_cast<long long>(std::llround(step_s * 1000.0));

	// 优先使用外部输入的航路点推进
	if (route_point != nullptr) {
		UtilCoor::cal_dis_surf(plat_state.pos_lla, route_point->pos_lla) > ARRIVE_AIR_DIS_M ?
			step_platform_mover(plat_mover.get(), this, route_point, step_s) :
			step_platform_mover(plat_mover.get(), this, nullptr, step_s);
		return;
	}

	// 预设航点完成判定
	const int point_count = static_cast<int>(init_route.point_list.size());
	int checked_count = 0;
	while (checked_count < point_count) {
		if (init_route.hist_index >= point_count) {
			if (init_route.is_circulate) {	// 循环航路，重置航路
				init_route.hist_index = 0;
			}
			else {	// 非循环航路，传空值
				step_platform_mover(plat_mover.get(), this, nullptr, step_s);
				return;
			}
		}

		if (UtilCoor::cal_dis_surf(plat_state.pos_lla,
			init_route.point_list.at(init_route.hist_index).pos_lla) > ARRIVE_AIR_DIS_M) {
			break;
		}
		init_route.hist_index++;
		checked_count++;
	}

	if (point_count == 0 || checked_count >= point_count) {
		step_platform_mover(plat_mover.get(), this, nullptr, step_s);
		return;
	}

	// 正常航路推进
	step_platform_mover(plat_mover.get(), this, &init_route.point_list.at(init_route.hist_index), step_s);

	// 航点到达判定
	if (UtilCoor::cal_dis_surf(plat_state.pos_lla,
		init_route.point_list.at(init_route.hist_index).pos_lla) <= ARRIVE_AIR_DIS_M) {
		init_route.hist_index++;
	}
}
