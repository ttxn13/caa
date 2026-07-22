#pragma once
#include "pch.hpp"
#include "CoreSimEnum.hpp"
#include "DefSimMover.hpp"
#include "../Model/DefCoor.hpp"

// 平台探测特征
struct S_PLAT_SIGN {
	double radar_rcs_m2	= 0.0;	// 雷达截面积	, m2
	double opt_area_m2	= 0.0;	// 光学可见面积	, m2
	double infrared_wsr = 0.0;	// 红外强度		，W/sr
};

// 平台存活状态
struct S_PLAT_LIFE {
	E_PLAT_LIFE state	= E_PLAT_LIFE::ALIVE;	// 存活状态
	bool active			= true;					// 是否参与仿真
	double health_rate	= 1.0;					// 健康度 0-1
};

// 航路点定义：初始态势配置中保存平台预设航线
struct S_ROUTE_POINT {
	QString		point_id		;	// 航路点ID
	S_POS_LLA	pos_lla			;	// 航路点位置
	double		speed_ms = 0.0	;	// 目标速度, m/s
};

// 平台预设航线
struct S_PLAT_ROUTE {
	QString route_id						;	// 航线ID
	int		hist_index = 0					;	// 历史航点索引
	bool	is_circulate = false			;	// 是否循环航线
	std::vector<S_ROUTE_POINT> point_list	;	// 航路点列表
};

// 平台空间态势
struct S_PLAT_STATE {
	S_POS_ECEF	pos_ecef		;	// ECEF坐标
	S_POS_LLA	pos_lla			;	// LLA 坐标
	S_VEL_LOC	vel_loc			;	// LLA 速度
	long long  update_time_ms	;	// 更新时间 ms
};

// 平台静态定义
struct S_PLAT_DEFINE {
	QString	type_name	;	// 平台类型名称
	QString	icon_path	;	// 图标资源路径

	E_DOMAIN	domain	= E_DOMAIN		::UKN;	// 作战域
	E_PLAT_TYPE	type	= E_PLAT_TYPE	::UKN;	// 平台类型
	E_MOVER		mover	= E_MOVER		::UKN;	// Mover模型

	S_MOVER_CAP	cap		;	// 能力边界
	S_PLAT_SIGN	sign	;	// 探测特征
};

// 平台配置定义
struct S_PLAT_CONFIG {
	QString plt_id		;	// 平台ID	(需要唯一)
	QString fmt_id		;	// 编队ID	(需要唯一)
	QString type_name	;	// 平台类型	(和类型一致)
	QString cmd_id		;	// 指挥关系	(上级平台ID)
	E_SIDE	side{}		;	// 阵营

	S_PLAT_STATE	init_state	;	// 初始态势，配置文件读取和写入只有pos_lla和vel_loc
	S_PLAT_ROUTE	init_route	;	// 预设航线	(初始态势作为第一个点，用户不可修改)

	std::map<QString, int> weapon_load;	// 武器挂载
	std::map<QString, int> sensor_load;	// 传感器挂载标识
};

struct S_SNAPSHOT_PLAT_STATE {
	QString	type_name	;	// 平台类型
	QString plt_id		;	// 平台ID
	QString fmt_id		;	// 编队ID
	QString cmd_id		;	// 上级平台ID（指挥关系）

	E_SIDE		side	= E_SIDE		::UKN;	// 阵营
	E_DOMAIN	domain	= E_DOMAIN		::UKN;	// 作战域
	E_PLAT_TYPE	type	= E_PLAT_TYPE	::UKN;	// 平台类型
	E_MOVER		mover	= E_MOVER		::UKN;	// Mover模型

	S_PLAT_LIFE		plat_life	;	// 存活状态
	S_PLAT_STATE	plat_state	;	// 当前平台态势

	std::map<QString, int> weapon_load;	// 武器挂载		first = type_name
	std::map<QString, int> sensor_load;	// 传感器挂载	first = type_name
};