#pragma once
#include "pch.hpp"
#include "../../Core/Model/DefEngage.hpp"
#include "../../Core/Sim/DefPlatform.hpp"
#include "../Mover/SimMover.hpp"


// 通用仿真平台：静态定义给出能力边界，实例配置给出初始态势和挂载标识
class SimPlatform
{
public:	// 公有函数
	SimPlatform(const S_PLAT_DEFINE& define, const S_PLAT_CONFIG& config);
	void step(S_ROUTE_POINT* route_point = nullptr, double step_s = 1.0);

	// 获取平台快照
	S_SNAPSHOT_PLAT_STATE to_snapshot() const {
		S_SNAPSHOT_PLAT_STATE snapshot;
		snapshot.type_name	= this->type_name	;
		snapshot.plt_id		= this->plt_id		;
		snapshot.cmd_id		= this->cmd_id		;
		snapshot.fmt_id		= this->fmt_id		;

		snapshot.side	= this->side	;
		snapshot.domain	= this->domain	;
		snapshot.type	= this->type	;
		snapshot.mover	= this->mover	;

		snapshot.plat_life		= this->plat_life	;
		snapshot.plat_state		= this->plat_state	;
		snapshot.weapon_load	= this->weapon_load	;
		snapshot.sensor_load	= this->sensor_load	;

		return snapshot;
	}

public:	// 公有变量
	QString	type_name	;	// 平台类型
	QString plt_id		;	// 当前平台ID（唯一标识）
	QString cmd_id		;	// 上级平台ID（指挥关系）
	QString fmt_id		;	// 所属编队ID（需要唯一）

	E_SIDE		side	= E_SIDE		::UKN;	// 阵营
	E_DOMAIN	domain	= E_DOMAIN		::UKN;	// 作战域
	E_PLAT_TYPE	type	= E_PLAT_TYPE	::UKN;	// 平台类型
	E_MOVER		mover	= E_MOVER		::UKN;	// Mover模型

	S_MOVER_CAP	cap	;	// 能力边界
	S_PLAT_SIGN	sign;	// 探测特征

	S_PLAT_STATE	init_state	;	// 初始态势，配置文件读取和写入只有pos_lla和vel_loc
	S_PLAT_ROUTE	init_route	;	// 预设航线	(初始态势作为第一个点，用户不可修改)

	S_PLAT_LIFE		plat_life	;	// 存活状态
	S_PLAT_STATE	plat_state	;	// 当前平台态势

	std::unique_ptr<SimMover>	plat_mover	;	// mover模型
	std::map<QString, int>		weapon_load	;	// 武器挂载
	std::map<QString, int>		sensor_load	;	// 传感器挂载
};
