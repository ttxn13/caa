#pragma once
#include "pch.hpp"
#include "../Core/Comm/CoreDefine.hpp"
#include "../Core/Comm/CoreGraph.hpp"
#include "../Core/Comm/CoreSystem.hpp"
#include "../Core/Model/DefEngage.hpp"
#include "../Core/Sim/DefPlatform.hpp"
#include "../Core/Sim/DefWeapon.hpp"
#include "../Core/Util/UtilCoor.hpp"

#include "DataManage/ModuleBvrSolver.hpp"
#include "DataManage/ModuleGraph.hpp"
#include "DataManage/ModuleMission.hpp"
#include "OurAnalysis/ModuleEmyGroup.hpp"
#include "OurAnalysis/ModuleEmyIntent.hpp"
#include "OurAnalysis/ModuleEmyThreat.hpp"
#include "OurAnalysis/ModuleOurCOG.hpp"
#include "OurAnalysis/ModuleOurLOO.hpp"
#include "OurAnalysis/ModuleOurCOA.hpp"
#include "EmyAnalysis/ModuleOurGroup.hpp"
#include "EmyAnalysis/ModuleOurIntent.hpp"
#include "EmyAnalysis/ModuleOurThreat.hpp"
#include "EmyAnalysis/ModuleEmyCOG.hpp"
#include "EmyAnalysis/ModuleEmyLOO.hpp"
#include "EmyAnalysis/ModuleEmyCOA.hpp"

using SNAPSHOT_SIM_PLATFORM = QSharedPointer<const std::map<QString, S_SNAPSHOT_PLAT_STATE>>;
using SNAPSHOT_SIM_WEAPON	= QSharedPointer<const std::map<QString, S_SNAPSHOT_WEAPON_STATE>>;

using SNAPSHOT_ALGO_PLAT	= QSharedPointer<const std::map<QString, S_PLAT>>;
using SNAPSHOT_ALGO_WEAPON	= QSharedPointer<const std::map<QString, S_WEAPON>>;
using SNAPSHOT_ALGO_FMT		= QSharedPointer<const std::map<QString, S_FORMAT>>;
using SNAPSHOT_ALGO_COA		= QSharedPointer<const S_COA>;

Q_DECLARE_METATYPE(SNAPSHOT_ALGO_PLAT)	;
Q_DECLARE_METATYPE(SNAPSHOT_ALGO_WEAPON);
Q_DECLARE_METATYPE(SNAPSHOT_ALGO_FMT)	;
Q_DECLARE_METATYPE(SNAPSHOT_ALGO_COA)	;

class ModuleDatabase : public QObject
{
	Q_OBJECT

public:
	static ModuleDatabase* GetInstance();
	static void DstInstance();
	ModuleDatabase(QObject* parent = nullptr);
	~ModuleDatabase() override;

public slots:
	void start()											;	// 启动子线程
	void load_config()										;	// 加载配置项
	bool save_config(int _timeout_ms, int _algo_step_ms)	;	// 更新并保存配置项

	void on_afs_recv_entity_state	(const S_PLAT& plat)					;	// from AFSim	实体数据更新
	void on_sim_platform_update		(SNAPSHOT_SIM_PLATFORM list_platform)	;	// from SimCtrl 平台数据更新
	void on_sim_weapon_update		(SNAPSHOT_SIM_WEAPON list_weapon)		;	// from SimCtrl 武器数据更新

	void on_sim_start()		;	// 仿真开始
	void on_sim_stop()		;	// 仿真停止
	void clear_run_data()	;	// 清理运行时态势缓存
	void on_timer_update()	;	// 定时参数时间对齐，只由on_sim_start()调用
	void on_timer_algo()	;	// 定时调用算法模块，只由on_sim_start()调用

signals:
	void send_config()	;	// 发送配置项
	void send_algo_plat		(SNAPSHOT_ALGO_PLAT		list_algo_plat)		;	// to Ui 平台对齐信息列表
	void send_algo_weapon	(SNAPSHOT_ALGO_WEAPON	list_algo_weapon)	;	// to Ui 武器对齐信息列表
	void send_algo_fmt		(SNAPSHOT_ALGO_FMT		list_algo_fmt)		;	// to Ui 编队对齐信息列表
	void send_algo_coa		(SNAPSHOT_ALGO_COA		algo_coa)			;	// to Ui COA信息

private:	// 私有函数
	void align_platform(const long long current_time_ms);
	void align_weapon(const long long current_time_ms);

public:		// 公有变量
	// 算法时间直接记录
	std::map<QString, S_PLAT>				list_plat_raw	;	// from Sim 平台原始信息列表
	std::map<QString, S_WEAPON>				list_weapon_raw	;	// from Sim 武器原始信息列表

	std::map<QString, S_PLAT>		list_own_plat_align		;	// to Algo  红方平台对齐信息列表
	std::map<QString, S_PLAT>		list_tgt_plat_align		;	// to Algo  蓝方平台对齐信息列表
	std::map<QString, S_WEAPON>		list_own_weapon_align	;	// to Algo  红方武器对齐信息列表
	std::map<QString, S_WEAPON>		list_tgt_weapon_align	;	// to Algo  蓝方武器对齐信息列表
	std::map<QString, S_FORMAT>		list_own_fmt_align		;	// to Algo  红方编队对齐信息列表
	std::map<QString, S_FORMAT>		list_tgt_fmt_align		;	// to Algo  蓝方编队对齐信息列表
	S_COA							coa_own					;	// to Algo  红方COA信息

	int			plt_timeout_ms	= 10000	;	// 平台超时清理阈值
	int			algo_step_ms	= 1000	;	// 时序算法步长			(可修改，影响算法调用频率)

	long long sim_start_time_ms = 0		;	// 仿真开始时间 ms	，用于对齐align
	long long sim_running_num	= 0		;	// 仿真运行计数		，用于对齐align
	long long algo_running_num	= 0		;	// 算法运行计数
	bool is_running				= false	;	// 运行状态

private:	// 私有变量
	static QMutex			mutex		;
	static ModuleDatabase*	instance	;
	static QThread*			thread		;
	QTimer* timer_update	= nullptr	;	// 以align_step_ms	定时更新list_plat_align
	QTimer* timer_algo		= nullptr	;	// 以algo_step_ms	定时调用算法模块

	ModuleEmyGroup	module_emy_group	;	// 敌方编队分析模块
	ModuleEmyIntent	module_emy_intent	;	// 敌方意图分析模块
	ModuleEmyThreat	module_emy_threat	;	// 敌方威胁分析模块
	ModuleOurCOG	module_our_cog		;	// 我方COG分析模块
	ModuleOurLOO	module_our_loo		;	// 我方LOO分析模块

};
