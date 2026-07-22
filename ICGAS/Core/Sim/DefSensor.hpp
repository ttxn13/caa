#pragma once
#include "pch.hpp"
#include "CoreSimEnum.hpp"

// 传感器探测包线
struct S_SENSOR_CAP_ENV {
	// 探测距离单位特征值：RCS：1 m2, 光学面积：1 m2, 红外强度：1 W/sr
	double min_range_m	= 0.0;	// 最小探测距离, m
	double max_range_m	= 0.0;	// 最大探测距离, m
	double az_fov_deg	= 0.0;	// 方位视场角, deg
	double min_el_deg	= 0.0;	// 最小俯仰角, deg
	double max_el_deg	= 0.0;	// 最大俯仰角, deg
	double min_alt_m	= 0.0;	// 最小探测高度, m
	double max_alt_m	= 0.0;	// 最大探测高度, m
};

// 传感器探测概率参数
struct S_SENSOR_CAP_PROB {
	double detection_prob	= 0.0;	// 基础探测概率, 0-1
	double false_alarm_prob	= 0.0;	// 基础虚警概率, 0-1
	double sensitive_slope	= 0.0;	// 灵敏度曲线斜率
};

// 传感器测量精度
struct S_SENSOR_CAP_ACC {
	bool measure_range	= true	;	// 是否测量距离
	bool measure_az		= true	;	// 是否测量方位角
	bool measure_el		= true	;	// 是否测量俯仰角
	bool measure_speed	= false	;	// 是否测量速度

	double range_sigma_m		= 0.0;	// 距离测量标准差, m
	double azimuth_sigma_deg	= 0.0;	// 方位角测量标准差, deg
	double elevation_sigma_deg	= 0.0;	// 俯仰角测量标准差, deg
	double speed_sigma_ms		= 0.0;	// 速度测量标准差, m/s
};

// 传感器时序参数
struct S_SENSOR_CAP_TIME {
	double update_period_s	= 0.0;	// 扫描周期, s
	double delay_ms			= 0.0;	// 观测延迟, ms
};

// 传感器单任务模式能力
struct S_SENSOR_TASK_MODE {
	int max_task_num	= 0;	// 最大目标容量

	S_SENSOR_CAP_ENV	cap_env		;	// 探测包线
	S_SENSOR_CAP_PROB	cap_prob	;	// 探测概率
	S_SENSOR_CAP_ACC	cap_acc		;	// 测量精度
	S_SENSOR_CAP_TIME	cap_time	;	// 时序参数
};

// 传感器静态定义
struct S_SENSOR_DEFINE {
	QString			type_name						;	// 传感器型号名称
	E_SENSOR_TYPE	type		= E_SENSOR_TYPE::UKN;	// 传感器类型
	double			reliability = 1.0				;	// 传感器源可靠度, 0-1

	std::vector<E_DOMAIN>						act_domain_list	;	// 可探测的作战域
	std::map<E_SENSOR_TASK, S_SENSOR_TASK_MODE> task_mode_list	;	// 分任务模式参数
};