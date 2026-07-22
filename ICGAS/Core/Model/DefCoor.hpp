#pragma once
#include "pch.hpp"

#pragma pack(push, 1) // 1字节对齐

// 地心坐标结构体（ECEF）
struct S_POS_ECEF {
	double x_m;	// X, m
	double y_m;	// Y, m
	double z_m;	// Z, m

	S_POS_ECEF(double _x_m, double _y_m, double _z_m) :
		x_m(_x_m), y_m(_y_m), z_m(_z_m) {}
	S_POS_ECEF() : S_POS_ECEF(0.0, 0.0, 0.0) {}
};

// 地心速度结构体（ECEF）
struct S_VEL_ECEF {
	double vx_ms;	// VX, m/s
	double vy_ms;	// VY, m/s
	double vz_ms;	// VZ, m/s

	S_VEL_ECEF(double _vx_ms, double _vy_ms, double _vz_ms) :
		vx_ms(_vx_ms), vy_ms(_vy_ms), vz_ms(_vz_ms) {}
	S_VEL_ECEF() : S_VEL_ECEF(0.0, 0.0, 0.0) {}
};

// LLA坐标结构体（WGS-84）
struct S_POS_LLA {
	double lon_rad;	// 经度, rad
	double lat_rad;	// 纬度, rad
	double alt_m;	// 高度, m

	S_POS_LLA(double _lon_rad, double _lat_rad, double _alt_m) :
		lon_rad(_lon_rad), lat_rad(_lat_rad), alt_m(_alt_m) {}
	S_POS_LLA() : S_POS_LLA(0.0, 0.0, 0.0) {}
};

// 航迹速度结构体（ENU下速度分解）
struct S_VEL_LOC {
	double speed_ms		;	// 速度大小, m/s
	double track_ang_rad;	// 航迹角	 , rad，正北0°，顺时为正
	double path_ang_rad	;	// 航迹俯仰角, rad，水平0°，抬头为正

	S_VEL_LOC(double _speed_ms, double _track_ang_rad, double _path_ang_rad) :
		speed_ms(_speed_ms), track_ang_rad(_track_ang_rad), path_ang_rad(_path_ang_rad) {}
	S_VEL_LOC() : S_VEL_LOC(0.0, 0.0, 0.0) {}
};

// ENU坐标结构体（依赖基准点）
struct S_POS_ENU {
	double e_m;	// 东向, m
	double n_m;	// 北向, m
	double u_m;	// 天向, m

	S_POS_ENU(double _e_m, double _n_m, double _u_m) :
		e_m(_e_m), n_m(_n_m), u_m(_u_m) {}
	S_POS_ENU() : S_POS_ENU(0.0, 0.0, 0.0) {}
};

// ENU速度结构体（依赖基准点）
struct S_VEL_ENU {
	double ve_ms;	// 东向, m/s
	double vn_ms;	// 北向, m/s
	double vu_ms;	// 天向, m/s

	S_VEL_ENU(double _ve_ms, double _vn_ms, double _vu_ms) :
		ve_ms(_ve_ms), vn_ms(_vn_ms), vu_ms(_vu_ms) {}
	S_VEL_ENU() : S_VEL_ENU(0.0, 0.0, 0.0) {}
};

// RAE坐标结构体（相对方位）
struct S_POS_RAE {
	double rng_m;	// 直线距离, m
	double az_rad;	// 相对方位角, rad
	double el_rad;	// 相对俯仰角, rad

	S_POS_RAE(double _rng_m, double _az_rad, double _el_rad) :
		rng_m(_rng_m), az_rad(_az_rad), el_rad(_el_rad) {}
	S_POS_RAE() : S_POS_RAE(0.0, 0.0, 0.0) {}
};

#pragma pack(pop) // 恢复默认对齐方式
