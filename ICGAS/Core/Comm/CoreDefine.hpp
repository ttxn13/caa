#pragma once
#include "pch.hpp"

// ---------- 输出控制定义 ----------
#define PRINT_QTHREAD 1

// ---------- 通用数学常量 ----------
#define CORE_PI			std::numbers::pi_v<double>	// 圆周率
#define RAD2DEG			* 180.0 / CORE_PI			// 弧度转角度
#define DEG2RAD			* CORE_PI / 180.0			// 角度转弧度

// ---------- WGS-84地球参数 ----------
#define WGS84_A_M		6378137.0					// WGS-84 长半轴, m
#define WGS84_F			(1.0 / 298.257223563)		// WGS-84 扁率
#define WGS84_E2		(WGS84_F * (2.0 - WGS84_F))	// WGS-84 第一偏心率平方
#define EARTH_RADIUS_M	6371000.0					// 平均地球半径, m
#define EARTH_LOS_K		(4.0 / 3.0)					// 大气折射等效地球半径系数

// ---------- UI显示控制定义 ----------
#define FMT_OUT_RANGE_M 2000	// 编队半径扩展距离 m

// ---------- 通信相关定义 ----------
#define PDU_HEADER_LEN	12

// ---------- 算法定义 ----------
#define ALIGN_STEP_MS	1000	// 时间对齐步长			(不可修改，影响时序算法)
#define ALIGN_MAX_SIZE	100		// 单平台对齐历史长度	(不可修改，影响时序算法)