#pragma once
#include "pch.hpp"
#include "../Comm/CoreGraph.hpp"
#include "../Sim/CoreSimEnum.hpp"
#include "DefCoor.hpp"


#pragma pack(push, 1) // 1字节对齐

// 作战阶段
enum class E_STAGE {
	WAN,	// 0 - 预警区
	FLT,	// 1 - 战斗机交战区
	COP,	// 2 - 协同交战区
	VSL,	// 3 - 舰艇自防区
	COUNT,	// 4 - 阶段数量 
};

// 等级枚举
enum class E_LEVEL {
	UKN,	// 0 - 无
	LOW,	// 1 - 低
	MID,	// 2 - 中
	HIGH,	// 3 - 高
};

// 行动枚举
enum class E_ACT {
	UKN,	// 0 - 不明
	DETECT,	// 1 - 探测	
	TRACK,	// 2 - 跟踪
	ATTACK,	// 3 - 攻击
	GUIDE,	// 4 - 制导
};

// 上级使命结构体
struct S_MISSION {
	QString mission_id;         // 上级使命编号， MIS-ATTACK-001
	QString source_text;        // 原始上级使命文本




};



// 运动帧结构体,用于算法及数据库
struct S_MOTION_FRAME {
	S_POS_ECEF pos_ecef		;	// ECEF坐标
	S_VEL_ECEF vel_ecef		;	// ECEF速度
	S_POS_LLA pos_lla		;	// LLA 坐标
	S_VEL_LOC vel_loc		;	// 航迹速度
	long long update_time_ms;	// 更新时间 ms
};

// 威胁评估算法处理结果
struct S_ALGO_THREAT {
	double	threat_value;	// 威胁值	0-1
	E_LEVEL	threat_level;	// 威胁等级
	int		threat_sort	;	// 威胁排序	1,2,3...（1最高）
};

// COG评估算法处理结果
struct S_ALGO_COG {
	double	cog_value	;	// COG评分
	E_LEVEL cog_level	;	// COG等级
	int		cog_sort	;	// COG排序，1最高
};

// 武器态势结构体,用于算法及数据库
struct S_WEAPON {
	QString		type_name	;	// 武器型号名称
	QString		weapon_id	;	// 武器ID
	QString		own_plat_id	;	// 发射平台ID
	QString		tgt_plat_id	;	// 目标平台ID

	E_SIDE			side	;	// 阵营
	E_WEAPON_TYPE	type	;	// 类型
	bool			valid	;	// 是否有效

	S_ALGO_THREAT				algo_threat;	// 威胁评估
	std::vector<S_MOTION_FRAME> list_motion;	// 运动帧列表
};

// 平台态势结构体,用于算法及数据库
struct S_PLAT {
	QString		type_name	;	// 平台型号名称
	QString		plt_id		;	// 平台ID
	QString		fmt_id		;	// 编队ID
	QString		cmd_id		;	// 上级平台ID（指挥关系）

	E_SIDE		side		;	// 阵营
	E_PLAT_TYPE type		;	// 类型
	bool		valid		;	// 是否有效

	S_ALGO_THREAT				algo_threat;	// 威胁评估
	std::vector<S_MOTION_FRAME> list_motion;	// 运动帧列表
	std::map<QString, int>		weapon_load;	// 武器挂载		first = type_name
	std::map<QString, int>		sensor_load;	// 传感器挂载	first = type_name

};

// 编队态势结构体,用于算法及数据库
struct S_FORMAT {
	E_SIDE					side		;	// 阵营
	QString					fmt_id		;	// 编队ID
	std::vector<QString>	plat_id_list;	// 成员平台ID列表
	bool					valid		;	// 是否有效
	
	S_ALGO_THREAT				algo_threat	;	// 威胁评估
	S_ALGO_COG					algo_cog	;	// COG 评估
	double						fmt_range_m	;	// 编队半径 m
	std::vector<S_MOTION_FRAME> list_motion	;	// 运动帧列表
};

// 行动结构体,用于算法及数据库
struct S_ACT {
	QString tgt_fmt_id			;	// 目标编队ID
	std::vector<E_ACT> act_list	;	// 行动列表，按执行顺序排列
};

// 行动方案结构体,用于算法及数据库
struct S_COA {
	struct S_COA_STAGE {
		E_STAGE stage;		// 阶段
		std::map<QString, std::vector<S_ACT>> own_fmt_act;	// 自身编队行动方案	first = fmt_id
	}coa_of_stage[static_cast<int>(E_STAGE::COUNT)];
};

// 平台相对参数结构体
struct S_PLAT_REL {
	QString own_plat_id;	// 自身平台ID
	QString tgt_plat_id;	// 目标平台ID

	S_POS_ENU pos_enu_own;	// ENU坐标 以own为原点
	S_VEL_ENU vel_enu_own;	// ENU速度 以own为原点
	S_POS_RAE pos_los_own;	// RAE坐标 以own为原点

	double dis_surf_m			= 0.0	;	// 地表曲面距离
	bool over_horizon			= false	;	// 超视距

	long long update_time_ms	= 0.0	;	// 更新时间 ms
	bool valid					= false	;	// 是否有效
};

#pragma pack(pop) // 恢复默认对齐方式
