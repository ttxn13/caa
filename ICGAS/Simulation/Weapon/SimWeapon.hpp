#pragma once
#include "pch.hpp"
#include "../../Core/Sim/DefWeapon.hpp"
#include "../Platform/SimPlatform.hpp"
#include "../Mover/SimMisMover.hpp"

// 通用仿真武器：静态定义给出武器/弹体能力，实例配置给出所属平台、目标和初始态势
class SimWeapon
{
public:
	SimWeapon(const SimPlatform& own_platform, const SimPlatform& tgt_platform,
		const S_WEAPON_DEFINE& define, const QString ID);
	void step(const S_PLAT_STATE& tgt_plat_state, double step_s = 1.0);

	// 获取武器快照
	S_SNAPSHOT_WEAPON_STATE to_snapshot() const {
		S_SNAPSHOT_WEAPON_STATE snapshot;
		snapshot.type_name		= this->type_name;
		snapshot.weapon_id		= this->weapon_id;
		snapshot.own_plat_id	= this->own_plat_id;
		snapshot.tgt_plat_id	= this->tgt_plat_id;

		snapshot.side	= this->side	;
		snapshot.domain = this->domain	;
		snapshot.type	= this->type	;
		snapshot.mover	= this->mover	;

		snapshot.weapon_life = this->weapon_life;
		snapshot.weapon_state = this->weapon_state;

		return snapshot;
	}

public:
	QString type_name	;	// 武器类型名称
	QString weapon_id	;	// 武器ID
	QString own_plat_id	;	// 发射平台
	QString tgt_plat_id	;	// 目标平台

	E_SIDE			side	= E_SIDE		::UKN;	// 阵营
	E_DOMAIN		domain	= E_DOMAIN		::UKN;	// 作战域
	E_WEAPON_TYPE	type	= E_WEAPON_TYPE	::UKN;	// 武器类型
	E_MOVER			mover	= E_MOVER		::UKN;	// 武器mover模型

	S_MOVER_CAP		mover_cap	;	// 运动能力
	S_LAUNCH_CAP	launch_cap	;	// 发射能力
	S_KILL_CAP		kill_cap	;	// 杀伤能力
	S_WEAPON_SIGN	sign		;	// 探测特征

	S_WEAPON_STATE	init_state	;	// 初始态势，由发射平台决定

	S_WEAPON_LIFE	weapon_life	;	// 存活状态
	S_WEAPON_STATE	weapon_state;	// 当前武器态势

	std::unique_ptr<SimMisMover> weapon_mover;	// mover模型
};
