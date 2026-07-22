#pragma once
#include "pch.hpp"
#include "CoreSimEnum.hpp"

// 飞机Mover能力边界
struct S_AIR_MOVER_CAP {
	double min_speed_ms			= 0.0;	// 最小飞行速度, m/s
	double max_speed_ms			= 0.0;	// 最大飞行速度, m/s
	double min_alt_m			= 0.0;	// 最低飞行高度, m
	double max_alt_m			= 0.0;	// 最高飞行高度, m
	double max_linear_load_g	= 0.0;	// 最大线性过载, g
	double max_radial_load_g	= 0.0;	// 最大转向过载, g
	double max_climb_vel_ms		= 0.0;	// 最大爬升速度, m/s
	double max_drop_vel_ms		= 0.0;	// 最大俯冲速度, m/s
};

// 舰船Mover能力边界
struct S_SEA_MOVER_CAP {
	double min_speed_ms			= 0.0;	// 最小航速, m/s
	double max_speed_ms			= 0.0;	// 最大航速, m/s
	double max_linear_acc_ms2	= 0.0;	// 最大线性加速度, m/s2
	double max_radial_acc_ms2	= 0.0;	// 最大转向加速度, m/s2
};

// 导弹Mover能力边界
struct S_MIS_MOVER_CAP {
	double min_speed_ms			= 0.0;	// 最小飞行速度, m/s
	double max_speed_ms			= 0.0;	// 最大飞行速度, m/s
	double min_alt_m			= 0.0;	// 最低飞行高度, m
	double max_alt_m			= 0.0;	// 最高飞行高度, m
	double max_linear_load_g	= 0.0;	// 最大线性过载, g
	double max_radial_load_g	= 0.0;	// 最大转向过载, g
	double max_climb_vel_ms		= 0.0;	// 最大爬升速度, m/s
	double max_drop_vel_ms		= 0.0;	// 最大俯冲速度, m/s
	double life_range_m			= 0.0;	// 距离寿命, m
	double life_time_s			= 0.0;	// 时间寿命, s
};

// Mover能力边界
using S_MOVER_CAP = std::variant<	S_AIR_MOVER_CAP,
									S_SEA_MOVER_CAP,
									S_MIS_MOVER_CAP>;