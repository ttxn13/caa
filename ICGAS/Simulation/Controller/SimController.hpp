#pragma once
#include "pch.hpp"
#include "../../Core/Sim/DefPlatform.hpp"
#include "../../Core/Sim/DefSensor.hpp"
#include "../../Core/Sim/DefWeapon.hpp"
#include "../Platform/SimPlatform.hpp"
#include "../Weapon/SimWeapon.hpp"

// 智能指针，跨线程传递数据快照
using SNAPSHOT_PLAT_DEFINE_MAP		= QSharedPointer<const std::map<QString, S_PLAT_DEFINE>>;
using SNAPSHOT_PLAT_CONFIG_MAP		= QSharedPointer<const std::map<QString, std::vector<S_PLAT_CONFIG>>>;
using SNAPSHOT_WEAPON_DEFINE_MAP	= QSharedPointer<const std::map<QString, S_WEAPON_DEFINE>>;
using SNAPSHOT_SENSOR_DEFINE_MAP	= QSharedPointer<const std::map<QString, S_SENSOR_DEFINE>>;

using SNAPSHOT_PLAT_DEFINE_PAIR		= QSharedPointer<const std::pair<QString, S_PLAT_DEFINE>>;
using SNAPSHOT_PLAT_CONFIG_PAIR		= QSharedPointer<const std::pair<QString, std::vector<S_PLAT_CONFIG>>>;
using SNAPSHOT_WEAPON_DEFINE_PAIR	= QSharedPointer<const std::pair<QString, S_WEAPON_DEFINE>>;
using SNAPSHOT_SENSOR_DEFINE_PAIR	= QSharedPointer<const std::pair<QString, S_SENSOR_DEFINE>>;

using SNAPSHOT_SIM_PLATFORM			= QSharedPointer<const std::map<QString, S_SNAPSHOT_PLAT_STATE>>;
using SNAPSHOT_SIM_WEAPON			= QSharedPointer<const std::map<QString, S_SNAPSHOT_WEAPON_STATE>>;

// 智能指针添加导QT元元素
Q_DECLARE_METATYPE(SNAPSHOT_PLAT_DEFINE_MAP);
Q_DECLARE_METATYPE(SNAPSHOT_PLAT_CONFIG_MAP);
Q_DECLARE_METATYPE(SNAPSHOT_WEAPON_DEFINE_MAP);
Q_DECLARE_METATYPE(SNAPSHOT_SENSOR_DEFINE_MAP);
Q_DECLARE_METATYPE(SNAPSHOT_PLAT_DEFINE_PAIR);
Q_DECLARE_METATYPE(SNAPSHOT_PLAT_CONFIG_PAIR);
Q_DECLARE_METATYPE(SNAPSHOT_WEAPON_DEFINE_PAIR);
Q_DECLARE_METATYPE(SNAPSHOT_SENSOR_DEFINE_PAIR);
Q_DECLARE_METATYPE(SNAPSHOT_SIM_PLATFORM);
Q_DECLARE_METATYPE(SNAPSHOT_SIM_WEAPON);

// 仿真控制器：管理平台配置缓存、运行时平台实例和平台mover推进
class SimController : public QObject
{
	Q_OBJECT

public:
	static SimController* GetInstance();
	static void DstInstance();
	SimController(QObject* parent = nullptr);
	~SimController() override;

public slots:
	void thread_start()	;	// 线程开始函数
	// 读取配置文件
	void load_sim_ctrl		()	;	// 读取仿真配置
	void load_plat_define	()	;	// 读取配置文件
	void load_weapon_define	()	;	// 读取武器定义
	void load_sensor_define	()	;	// 读取传感器定义
	void load_plat_config	()	;	// 读取配置文件
	// 更新并写入
	void save_sim_ctrl		(int _sim_iter_ms, double _time_acc_ratio)	;	// 更新并写入仿真控制配置
	void save_plat_define	(SNAPSHOT_PLAT_DEFINE_PAIR _plat_define)	;	// 更新并写入平台定义配置
	void save_weapon_define	(SNAPSHOT_WEAPON_DEFINE_PAIR _weapon_define);	// 更新并写入武器定义配置
	void save_sensor_define	(SNAPSHOT_SENSOR_DEFINE_PAIR _sensor_define);	// 更新并写入传感器定义配置
	void save_plat_config	(SNAPSHOT_PLAT_CONFIG_PAIR _plat_config)	;	// 更新并写入初始态势配置
	// 清理并删除
	void rmov_plat_define	(SNAPSHOT_PLAT_DEFINE_PAIR _plat_define)	;	// 清理并删除平台定义配置
	void rmov_weapon_define	(SNAPSHOT_WEAPON_DEFINE_PAIR _weapon_define);	// 清理并删除武器定义配置
	void rmov_sensor_define	(SNAPSHOT_SENSOR_DEFINE_PAIR _sensor_define);	// 清理并删除传感器定义配置
	void rmov_plat_config	(SNAPSHOT_PLAT_CONFIG_PAIR _plat_config)	;	// 清理并删除初始态势配置
	// 场景初始化
	bool confirm_scenario	(QString scenario_name)	;	// 场景  实例化
	bool sim_start			(QString scenario_name)	;	// 仿真开始函数
	void sim_stop()									;	// 仿真停止函数
	void sim_process()								;	// 仿真推进函数

signals:
	void signal_load_sim_ctrl		(bool _is_success, int _sim_iter_ms, double _time_acc_ratio)		;	// 发送仿真控制配置
	void signal_load_plat_define	(bool _is_success, SNAPSHOT_PLAT_DEFINE_MAP _list_plat_define)		;	// 发送平台定义配置
	void signal_load_weapon_define	(bool _is_success, SNAPSHOT_WEAPON_DEFINE_MAP _list_weapon_define)	;	// 发送武器定义配置
	void signal_load_sensor_define	(bool _is_success, SNAPSHOT_SENSOR_DEFINE_MAP _list_sensor_define)	;	// 发送传感器定义配置
	void signal_load_plat_config	(bool _is_success, SNAPSHOT_PLAT_CONFIG_MAP _list_plat_config)		;	// 发送初始态势配置

	void signal_save_sim_ctrl		(bool _is_success)	;	// 仿真控制保存成功
	void signal_save_plat_define	(bool _is_success)	;	// 平台定义保存成功
	void signal_save_weapon_define	(bool _is_success)	;	// 武器定义保存成功
	void signal_save_sensor_define	(bool _is_success)	;	// 传感器定义保存成功
	void signal_save_plat_config	(bool _is_success)	;	// 初始态势保存成功

	void signal_rmov_plat_define	(bool _is_success)	;	// 平台定义删除成功
	void signal_rmov_weapon_define	(bool _is_success)	;	// 武器定义删除成功
	void signal_rmov_sensor_define	(bool _is_success)	;	// 传感器定义删除成功
	void signal_rmov_plat_config	(bool _is_success)	;	// 初始态势删除成功

	void signal_platform_update	(SNAPSHOT_SIM_PLATFORM _list_platform)	;	// 发送平台态势更新
	void signal_weapon_update	(SNAPSHOT_SIM_WEAPON _list_weapon)		;	// 发送武器态势更新

public:		// 公有变量
	std::map<QString, S_PLAT_DEFINE>				list_plat_define	;	// 平台静态定义缓存 first = type_name
	std::map<QString, S_WEAPON_DEFINE>				list_weapon_define	;	// 武器静态定义缓存 first = type_name
	std::map<QString, S_SENSOR_DEFINE>				list_sensor_define	;	// 传感器静态定义缓存 first = type_name
	std::map<QString, std::vector<S_PLAT_CONFIG>>	list_plat_config	;	// 初始态势配置缓存 first = json name

	std::map<QString, SimPlatform>					list_platform		;	// 平台实体列表 first = plt_id
	std::map<QString, SimWeapon>					list_weapon			;	// 武器实体列表

	int		sim_iter_ms		= 1000	;	// 仿真步长, ms
	double  time_acc_ratio	= 1.0	;	// 倍速
	bool	is_running		= false	;	// 仿真运行状态

private:	// 私有变量
	static QMutex			mutex;
	static SimController*	instance;
	static QThread*			thread;
	static QTimer*			timer;
};
