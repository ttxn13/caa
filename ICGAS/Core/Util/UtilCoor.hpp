#pragma once
#include "pch.hpp"
#include "../Comm/CoreDefine.hpp"
#include "../Model/DefCoor.hpp"

namespace UtilCoor {
	S_POS_ECEF	pos_lla2ecef	(const S_POS_LLA& pos_lla)								;	// 坐标 LLA  转换 ECEF
	S_POS_LLA	pos_ecef2lla	(const S_POS_ECEF& pos_ecef)							;	// 坐标 ECEF 转换 LLA
	S_POS_ENU	pos_ecef2enu	(const S_POS_ECEF& ecef_del, const S_POS_LLA& lla_own)	;	// 坐标 ECEF 转换 ENU
	S_VEL_ENU	vel_ecef2enu	(const S_VEL_ECEF& ecef_del, const S_POS_LLA& lla_own)	;	// 速度 ECEF 转换 ENU
	S_VEL_ECEF	vel_enu2ecef	(const S_VEL_ENU& vel_enu, const S_POS_LLA& lla_own)	;	// 速度 ENU  转换 ECEF
	S_VEL_ECEF	vel_loc2ecef	(const S_VEL_LOC& vel_loc, const S_POS_LLA& lla_own)	;	// 速度 LOC  转换 ECEF
	S_VEL_LOC	vel_ecef2loc	(const S_POS_LLA& pos_lla, const S_VEL_ECEF& vel_ecef)	;	// 速度 ECEF 转换 LOC

	double cal_dis_ecef		(const S_POS_ECEF& ecef_own, const S_POS_ECEF& ecef_tgt)	;	// 直线距离计算 欧氏距离
	double cal_dis_surf		(const S_POS_LLA& lla_own, const S_POS_LLA& lla_tgt)		;	// 曲面距离计算 haversine公式
	double norm_rad_0_2pi	(const double value)										;	// 角度归一化	0~2pi
	double norm_rad_pi_pi	(const double value)										;	// 角度归一化	-pi~pi

	S_POS_ECEF equal_pos_ecef(const S_POS_ECEF& raw_ecef, const S_POS_ECEF& hst_ecef, const double raw_ratio);	// 计算两点加权平均位置
	S_VEL_ECEF equal_vel_ecef(const S_VEL_ECEF& raw_ecef, const S_VEL_ECEF& hst_ecef, const double raw_ratio);	// 计算两速度加权平均
	S_POS_LLA  equal_pos_lla (const S_POS_LLA&  raw_lla,  const S_POS_LLA&  hst_lla,  const double raw_ratio);	// 计算两点加权平均位置
	S_VEL_LOC  equal_vel_loc (const S_VEL_LOC&  raw_loc,  const S_VEL_LOC&  hst_loc,  const double raw_ratio);	// 计算两速度加权平均
}
