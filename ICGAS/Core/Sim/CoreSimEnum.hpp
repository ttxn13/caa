#pragma once
#include "pch.hpp"

#define GRAVITY_MS2			9.80665	// 重力加速度，m/s2
#define ARRIVE_AIR_DIS_M	3000	// 空中平台航路点到达判定距离，m
#define ARRIVE_AIR_ANG_DEG	1		// 空中平台航路点角度对齐判定，deg

// 阵营枚举
enum class E_SIDE {
	UKN,	// 0 - 不明
	RED,	// 1 - 红方
	BLUE,	// 2 - 蓝方
	WHITE,	// 3 - 白方
};

// 作战域
enum class E_DOMAIN {
	UKN,	// 0 - 不明
	AIR,	// 1 - 空中平台
	SEA,	// 2 - 水面平台
	MIS,	// 3 - 导弹平台
};

// Mover模型类型
enum class E_MOVER {
	UKN,		// 0 - 不明
	AIR_MOVER,	// 1 - 空中平台
	SEA_MOVER,	// 2 - 水面平台
	MIS_MOVER,	// 3 - 导弹平台
};

// 平台类型
enum class E_PLAT_TYPE {
	UKN,	// 0 - 不明类型
	CAR,	// 1 - 航空母舰
	DST,	// 2 - 驱逐舰
	FRI,	// 3 - 护卫舰
	WAN,	// 4 - 预警机
	ELC,	// 5 - 电子战飞机
	FLT,	// 6 - 战斗机
	BOM,	// 7 - 轰炸机
	MIS,	// 8 - 巡航导弹
};

// 武器类型
enum class E_WEAPON_TYPE {
	UKN		,
	AAM		,	// 空空弹 通用
	SAM		,	// 舰空弹 通用
	ASM		,	// 空舰船弹 通用
};

// 传感器类型
enum class E_SENSOR_TYPE {
	UKN		,
	RADAR	,	// 电磁雷达
	OPTICAL	,	// 光电雷达
	INFRARED,	// 红外雷达
};

// 传感器任务类型
enum class E_SENSOR_TASK {
	UKN		,
	DETECT	,	// 探测
	TRACK	,	// 跟踪
	GUIDE	,	// 制导
};

// 平台存活状态
enum class E_PLAT_LIFE {
	UKN,		// 0 - 不明
	ALIVE,		// 1 - 存活
	DAMAGED,	// 2 - 受损
	DISABLED,	// 3 - 失能
	DESTROY,	// 4 - 摧毁
};

// 武器存活状态
enum class E_WEAPON_LIFE {
	UKN,		// 0 - 不明
	FLYING,		// 1 - 飞行中
	HIT,		// 2 - 命中
	OVER_RANGE,	// 3 - 销毁-超过最大飞行距离
	OVER_TIME,	// 4 - 销毁-超过最大飞行时间
};
