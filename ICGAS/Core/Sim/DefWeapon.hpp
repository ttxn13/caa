#pragma once
#include "pch.hpp"
#include "CoreSimEnum.hpp"
#include "DefSimMover.hpp"
#include "../Model/DefCoor.hpp"

// 武器探测特征
struct S_WEAPON_SIGN {
	double radar_rcs_m2	= 0.0;	// 雷达截面积	, m2
	double opt_area_m2	= 0.0;	// 光学可见面积	, m2
	double infrared_wsr = 0.0;	// 红外强度		，W/sr
};

// 武器发射参数
struct S_LAUNCH_CAP {
	double max_range_m		= 0.0;		// 最大射程, m
	double min_range_m		= 0.0;		// 最小射程, m
	double firing_delay_s	= 0.0;		// 发射延迟, s
};

// 武器杀伤参数
struct S_KILL_CAP {
	double kill_range_m	= 0.0;	// 杀伤半径, m
	double kill_prob	= 0.0;	// 杀伤概率, 0-1
};

// 平台存活状态
struct S_WEAPON_LIFE {
	E_WEAPON_LIFE state = E_WEAPON_LIFE::FLYING;	// 存活状态
	bool active = true		;						// 是否参与仿真
};

// 武器空间态势
struct S_WEAPON_STATE {
	S_POS_ECEF	pos_ecef		;	// ECEF坐标
	S_POS_LLA	pos_lla			;	// LLA 坐标
	S_VEL_LOC	vel_loc			;	// LLA 速度
	long long	update_time_ms	;	// 更新时间 ms
	double		move_range_m	;	// 已运动距离 m
	double		move_time_s		;	// 已运动时间 s
};

// 武器静态定义
struct S_WEAPON_DEFINE {
	QString type_name;				// 武器类型名称（唯一标识）
	QString icon_path;				// 图标资源路径

	E_DOMAIN		domain	= E_DOMAIN		::UKN;	// 作战域
	E_WEAPON_TYPE	type	= E_WEAPON_TYPE	::UKN;	// 武器类型
	E_MOVER			mover	= E_MOVER		::UKN;	// 武器mover模型

	S_MOVER_CAP		mover_cap	;	// 运动能力
	S_LAUNCH_CAP	launch_cap	;	// 发射能力
	S_KILL_CAP		kill_cap	;	// 杀伤能力
	S_WEAPON_SIGN	sign		;	// 探测特征
};

struct S_SNAPSHOT_WEAPON_STATE {
	QString	type_name	;	// 武器类型
	QString weapon_id	;	// 武器ID
	QString own_plat_id	;	// 发射平台
	QString tgt_plat_id	;	// 目标平台

	E_SIDE			side	= E_SIDE		::UKN;	// 阵营
	E_DOMAIN		domain	= E_DOMAIN		::UKN;	// 作战域
	E_WEAPON_TYPE	type	= E_WEAPON_TYPE	::UKN;	// 武器类型
	E_MOVER			mover	= E_MOVER		::UKN;	// 武器mover模型

	S_WEAPON_LIFE	weapon_life	;	// 存活状态
	S_WEAPON_STATE	weapon_state;	// 当前武器态势
};