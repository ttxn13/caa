#include "pch.hpp"
#include "SimController.hpp"
#include "UtilSimConfig.hpp"
#include "../../Core/Util/UtilCoor.hpp"

QMutex			SimController::mutex;
SimController*	SimController::instance = nullptr;
QThread*		SimController::thread	= nullptr;
QTimer*			SimController::timer	= nullptr;

SimController* SimController::GetInstance()
{
	if (instance == nullptr) {
		QMutexLocker locker(&mutex);
		if (instance == nullptr) {
			thread		= new QThread();
			instance	= new SimController();
			instance	->moveToThread(thread);
			QObject::connect(
				thread,		&QThread::started,
				instance,	&SimController::thread_start);
			thread->start();
		}
	}
	return instance;
}

void SimController::DstInstance()
{
	delete instance;
	instance = NULL;
}

SimController::SimController(QObject* parent) :
	QObject(parent)
{
	// 在GetInstance()前置，唯一主线程函数

}

SimController::~SimController()
{
	// 被DstInstance()调用，释放所有内存
	thread->quit();
	thread->wait();
	delete thread;
	thread = nullptr;
}

void SimController::thread_start()
{
	load_sim_ctrl();
	load_plat_define();
	load_weapon_define();
	load_sensor_define();
	load_plat_config();
}

void SimController::load_sim_ctrl()
{
	const bool is_success = UtilSimConfig::load_sim_ctrl(sim_iter_ms, time_acc_ratio);
	emit signal_load_sim_ctrl(is_success, sim_iter_ms, time_acc_ratio);
}

void SimController::load_plat_define()
{
	list_plat_define.clear();
	const bool is_success = UtilSimConfig::load_plat_define(list_plat_define);	// 清缓存+读配置
	emit signal_load_plat_define(is_success, SNAPSHOT_PLAT_DEFINE_MAP
	(new std::map<QString, S_PLAT_DEFINE>(list_plat_define)));					// 发快照 -> UI
}

void SimController::load_plat_config()
{
	list_plat_config.clear();
	const bool is_success = UtilSimConfig::load_plat_config(list_plat_config);	// 清缓存+读配置
	emit signal_load_plat_config(is_success, SNAPSHOT_PLAT_CONFIG_MAP
	(new std::map<QString, std::vector<S_PLAT_CONFIG>>(list_plat_config)));		// 发快照 -> UI
}

void SimController::load_weapon_define()
{
	list_weapon_define.clear();
	const bool is_success = UtilSimConfig::load_weapon_define(list_weapon_define);
	emit signal_load_weapon_define(is_success, SNAPSHOT_WEAPON_DEFINE_MAP
	(new std::map<QString, S_WEAPON_DEFINE>(list_weapon_define)));
}

void SimController::load_sensor_define()
{
	list_sensor_define.clear();
	const bool is_success = UtilSimConfig::load_sensor_define(list_sensor_define);
	emit signal_load_sensor_define(is_success, SNAPSHOT_SENSOR_DEFINE_MAP
	(new std::map<QString, S_SENSOR_DEFINE>(list_sensor_define)));
}

void SimController::save_sim_ctrl(int _sim_iter_ms, double _time_acc_ratio)
{
	bool is_success = false;
	if (_sim_iter_ms > 0 && _time_acc_ratio > 0.0) {
		sim_iter_ms = _sim_iter_ms;
		time_acc_ratio = _time_acc_ratio;
		is_success = UtilSimConfig::save_sim_ctrl(sim_iter_ms, time_acc_ratio);
	}
	emit signal_save_sim_ctrl(is_success);
}

void SimController::save_plat_define(SNAPSHOT_PLAT_DEFINE_PAIR _plat_define)
{
	const std::pair<QString, S_PLAT_DEFINE>& plat_define = *_plat_define;
	list_plat_define[plat_define.first] = plat_define.second				;	// 写入成员变量
	const bool is_success = UtilSimConfig::save_plat_define(_plat_define)	;	// 写入json配置
	emit signal_save_plat_define(is_success);
}

void SimController::save_plat_config(SNAPSHOT_PLAT_CONFIG_PAIR _plat_config)
{
	const std::pair<QString, std::vector<S_PLAT_CONFIG>>& plat_config = *_plat_config;
	list_plat_config[plat_config.first] = plat_config.second				;	// 写入成员变量
	const bool is_success = UtilSimConfig::save_plat_config(_plat_config)	;	// 写入json配置
	emit signal_save_plat_config(is_success);
}

void SimController::save_weapon_define(SNAPSHOT_WEAPON_DEFINE_PAIR _weapon_define)
{
	const std::pair<QString, S_WEAPON_DEFINE>& weapon_define = *_weapon_define;
	list_weapon_define[weapon_define.first] = weapon_define.second;
	const bool is_success = UtilSimConfig::save_weapon_define(_weapon_define);
	emit signal_save_weapon_define(is_success);
}

void SimController::save_sensor_define(SNAPSHOT_SENSOR_DEFINE_PAIR _sensor_define)
{
	const std::pair<QString, S_SENSOR_DEFINE>& sensor_define = *_sensor_define;
	list_sensor_define[sensor_define.first] = sensor_define.second;
	const bool is_success = UtilSimConfig::save_sensor_define(_sensor_define);
	emit signal_save_sensor_define(is_success);
}

void SimController::rmov_plat_define(SNAPSHOT_PLAT_DEFINE_PAIR _plat_define)
{
	const std::pair<QString, S_PLAT_DEFINE>& plat_define = *_plat_define;
	list_plat_define.erase(plat_define.first)								;	// 删除成员变量
	const bool is_success = UtilSimConfig::rmov_plat_define(_plat_define)	;	// 删除json配置
	emit signal_rmov_plat_define(is_success);
}

void SimController::rmov_plat_config(SNAPSHOT_PLAT_CONFIG_PAIR _plat_config)
{
	const std::pair<QString, std::vector<S_PLAT_CONFIG>>& plat_config = *_plat_config;
	list_plat_config.erase(plat_config.first)								;	// 删除成员变量
	const bool is_success = UtilSimConfig::rmov_plat_config(_plat_config)	;	// 删除json配置
	emit signal_rmov_plat_config(is_success);
}

void SimController::rmov_weapon_define(SNAPSHOT_WEAPON_DEFINE_PAIR _weapon_define)
{
	const std::pair<QString, S_WEAPON_DEFINE>& weapon_define = *_weapon_define;
	list_weapon_define.erase(weapon_define.first);
	const bool is_success = UtilSimConfig::rmov_weapon_define(_weapon_define);
	emit signal_rmov_weapon_define(is_success);
}

void SimController::rmov_sensor_define(SNAPSHOT_SENSOR_DEFINE_PAIR _sensor_define)
{
	const std::pair<QString, S_SENSOR_DEFINE>& sensor_define = *_sensor_define;
	list_sensor_define.erase(sensor_define.first);
	const bool is_success = UtilSimConfig::rmov_sensor_define(_sensor_define);
	emit signal_rmov_sensor_define(is_success);
}

bool SimController::confirm_scenario(QString scenario_name)
{
	// 清缓存
	list_platform.clear();
	list_weapon.clear();

	const QString scenario_id = scenario_name.trimmed();
	const auto scenario_iter = list_plat_config.find(scenario_id);
	if (scenario_iter == list_plat_config.end() || scenario_iter->second.empty()) {
		return false;
	}

	const long long init_time_ms = QDateTime::currentMSecsSinceEpoch();
	// 创建平台实例
	for (S_PLAT_CONFIG plat_config : scenario_iter->second) {
		const auto define_iter = list_plat_define.find(plat_config.type_name);
		if (define_iter == list_plat_define.end()) continue;

		// 平台初始化
		plat_config.init_state.update_time_ms = init_time_ms;
		plat_config.init_state.pos_ecef = UtilCoor::pos_lla2ecef(plat_config.init_state.pos_lla);
		list_platform.insert_or_assign(plat_config.plt_id,
			SimPlatform(define_iter->second, plat_config));

		// 武器初始化

		// 传感器初始化

	}
	return !list_platform.empty();
}

bool SimController::sim_start(QString scenario_name)
{
	if (timer != nullptr) {
		timer->stop();
		delete timer;
		timer = nullptr;
	}
	is_running = false;
	load_sim_ctrl();
	if (!this->confirm_scenario(scenario_name)) {
		return false;
	}

	is_running = true;
	// 开始仿真主循环
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &SimController::sim_process);
	timer->start(std::max(1, static_cast<int>(this->sim_iter_ms / this->time_acc_ratio)));
	return true;
}

void SimController::sim_stop()
{
	is_running = false;
	// 停止仿真主循环
	if (timer == nullptr) return;
	timer->stop();
	delete timer;
	timer = nullptr;
}

void SimController::sim_process()
{
	// 仿真推进时间
	const double step_s = static_cast<double>(sim_iter_ms) / 1000.0;

	/* ---------- 清理无效平台武器 ---------- */
	for (auto platform_iter = list_platform.begin(); platform_iter != list_platform.end();) {
		if (platform_iter->second.plat_life.active == false) {
			platform_iter = list_platform.erase(platform_iter);
		}	else	++platform_iter;
	}
	for (auto weapon_iter = list_weapon.begin(); weapon_iter != list_weapon.end();) {
		if (weapon_iter->second.weapon_life.active == false) {
			weapon_iter = list_weapon.erase(weapon_iter);
		}	else	++weapon_iter;
	}

	/* ---------- 推进平台模型 ---------- */
	for (auto& platform : list_platform) platform.second.step(nullptr, step_s);

	/* ---------- 推进武器模型 ---------- */
	for (auto weapon_iter = list_weapon.begin(); weapon_iter != list_weapon.end();) {
		SimWeapon& weapon = weapon_iter->second;
		// 目标平台无效或已被销毁，武器过时
		auto tgt_plat_iter = list_platform.find(weapon.tgt_plat_id);
		if (tgt_plat_iter == list_platform.end() || tgt_plat_iter->second.plat_life.active == false) {
			weapon.weapon_life.state = E_WEAPON_LIFE::OVER_TIME;
			weapon.weapon_life.active = false;
			++weapon_iter;
			continue;
		}
		// 推进武器模型
		weapon.step(tgt_plat_iter->second.plat_state, step_s);
		if (weapon.weapon_life.state == E_WEAPON_LIFE::HIT) {
			weapon.weapon_life.active = false;	// 命中后不再参与仿真，等待清理
			SimPlatform& tgt_plat = tgt_plat_iter->second;
			const double damage_prob = rand() % (10001) / static_cast<double>(10000);
			if (damage_prob < weapon.kill_cap.kill_prob) {
				tgt_plat.plat_life.state	= E_PLAT_LIFE::DESTROY;
				tgt_plat.plat_life.active	= false;
			}
			else {
				const double denom = std::max(1.0e-6, 1.0 - damage_prob);
				const double damage_rate = (damage_prob - weapon.kill_cap.kill_prob) / denom;
				tgt_plat.plat_life.health_rate -= damage_rate;
				if (tgt_plat.plat_life.health_rate <= 0.0) {
					tgt_plat.plat_life.state = E_PLAT_LIFE::DESTROY;
					tgt_plat.plat_life.active = false;
				}
				else {
					tgt_plat.plat_life.state = E_PLAT_LIFE::DAMAGED;
				}
			}
		}
		++weapon_iter;
	}

	/* ---------- 推进传感器模型 ---------- */


	/* ---------- 武器发射判定 ---------- */
	QString own_id		= "10905";
	QString tgt_id		= "20704";
	QString weapon_type = "PL-12";
	auto own_plat_iter = list_platform.find(own_id);
	auto tgt_plat_iter = list_platform.find(tgt_id);
	auto weapon_define_iter = list_weapon_define.find(weapon_type);

	if (own_plat_iter != list_platform.end() &&
		tgt_plat_iter != list_platform.end() &&
		weapon_define_iter != list_weapon_define.end() &&
		own_plat_iter->second.plat_life.active &&
		tgt_plat_iter->second.plat_life.active)
	{
		SimPlatform& own_plat = own_plat_iter->second;
		SimPlatform& tgt_plat = tgt_plat_iter->second;
		S_WEAPON_DEFINE& weapon_define = weapon_define_iter->second;

		int load_index = own_plat.weapon_load[weapon_type];
		if (load_index > 0) {
			double rela_dis = UtilCoor::cal_dis_ecef(own_plat.plat_state.pos_ecef, tgt_plat.plat_state.pos_ecef);
			if (rela_dis < weapon_define.launch_cap.max_range_m &&
				rela_dis > weapon_define.launch_cap.min_range_m) {
				QString weapon_id = own_plat.plt_id + "_" + weapon_type + "_" + QString::number(load_index);
				list_weapon.insert_or_assign(weapon_id,
					SimWeapon(own_plat, tgt_plat, weapon_define, weapon_id));
				own_plat.weapon_load[weapon_type]--;
			}
		}
	}

	/* ---------- 发送信号，传出快照 ---------- */
	auto snapshot_platform = new std::map<QString, S_SNAPSHOT_PLAT_STATE>;
	for (const auto& item : list_platform)	snapshot_platform->emplace(item.first, item.second.to_snapshot());
	emit signal_platform_update(SNAPSHOT_SIM_PLATFORM(snapshot_platform));

	auto snapshot_weapon = new std::map<QString, S_SNAPSHOT_WEAPON_STATE>;
	for (const auto& item : list_weapon)	snapshot_weapon->emplace(item.first, item.second.to_snapshot());
	emit signal_weapon_update(SNAPSHOT_SIM_WEAPON(snapshot_weapon));
}
