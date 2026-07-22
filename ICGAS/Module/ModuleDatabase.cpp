#include "pch.hpp"
#include "ModuleDatabase.hpp"
#include "../Core/Util/UtilJson.hpp"

QMutex			ModuleDatabase::mutex;
ModuleDatabase* ModuleDatabase::instance = nullptr;
QThread*		ModuleDatabase::thread = nullptr;

namespace {
void reset_coa(S_COA& coa)
{
	for (int index = 0; index < static_cast<int>(E_STAGE::COUNT); ++index) {
		coa.coa_of_stage[index].stage = static_cast<E_STAGE>(index);
		coa.coa_of_stage[index].own_fmt_act.clear();
	}
}
}

ModuleDatabase* ModuleDatabase::GetInstance()
{
	if (instance == nullptr) {
		QMutexLocker locker(&mutex);
		if (instance == nullptr) {
			thread = new QThread();
			instance = new ModuleDatabase();
			instance->moveToThread(thread);
			QObject::connect(thread, &QThread::started,
				instance, &ModuleDatabase::start);
			thread->start();
		}
	}
	return instance;
}

void ModuleDatabase::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleDatabase::ModuleDatabase(QObject* parent)
	: QObject(parent)
{
	reset_coa(coa_own);
}

ModuleDatabase::~ModuleDatabase()
{
}

void ModuleDatabase::start()
{
	load_config();
}

void ModuleDatabase::load_config()
{
	const QString config_path = "Config/ConfigModule.json";

	QJsonObject root_obj;
	if (!UtilJson::read_json_object_file(config_path, root_obj)) return;
	const QJsonObject time_align_obj = root_obj.value(QString("DATA_BASE")).toObject();

	plt_timeout_ms = std::max(1, time_align_obj.value(QString("plt_timeout_ms")).toInt(plt_timeout_ms));
	algo_step_ms = std::max(1, time_align_obj.value(QString("algo_step_ms")).toInt(algo_step_ms));
}

bool ModuleDatabase::save_config(int _timeout_ms, int _algo_step_ms)
{
	plt_timeout_ms = std::max(1, _timeout_ms);
	algo_step_ms = std::max(1, _algo_step_ms);

	const QString config_path = "Config/ConfigModule.json";
	QJsonObject root_obj;
	UtilJson::read_json_object_file(config_path, root_obj);

	QJsonObject time_align_obj = root_obj.value(QString("DATA_BASE")).toObject();
	time_align_obj["plt_timeout_ms"] = plt_timeout_ms;
	time_align_obj["algo_step_ms"] = algo_step_ms;
	root_obj["DATA_BASE"] = time_align_obj;

	if (!UtilJson::write_json_object_file(config_path, root_obj)) return false;
	if (timer_algo != nullptr) {
		timer_algo->setInterval(algo_step_ms);
	}
	emit send_config();
	return true;
}

void ModuleDatabase::on_afs_recv_entity_state(const S_PLAT& plat)
{
	if (!is_running) return;

	// AFSim平台元信息赋值，运动数据统一放在list_motion中
	S_PLAT recv_plat{};
	recv_plat.type_name		= plat.type_name;
	recv_plat.plt_id		= plat.plt_id.trimmed();
	recv_plat.fmt_id		= plat.fmt_id.trimmed();
	if (recv_plat.fmt_id.isEmpty()) recv_plat.fmt_id = recv_plat.plt_id;
	recv_plat.cmd_id		= plat.cmd_id.trimmed();
	if (recv_plat.cmd_id.isEmpty()) recv_plat.cmd_id = QStringLiteral("SELF");
	recv_plat.side			= plat.side;
	recv_plat.type			= plat.type;
	recv_plat.valid			= plat.valid;
	recv_plat.algo_threat	= S_ALGO_THREAT{};
	recv_plat.weapon_load	= plat.weapon_load;
	recv_plat.sensor_load	= plat.sensor_load;

	// 删除无效平台，同时清理原始缓存和对齐缓存
	if (recv_plat.plt_id.isEmpty()) return;
	if (!recv_plat.valid) {
		list_plat_raw		.erase(recv_plat.plt_id);
		list_own_plat_align	.erase(recv_plat.plt_id);
		list_tgt_plat_align	.erase(recv_plat.plt_id);
		return;
	}
	if (plat.list_motion.empty()) return;

	// AFSim输入为ECEF坐标和速度，这里补齐LLA坐标和本地速度
	S_MOTION_FRAME motion = plat.list_motion.back();
	motion.pos_lla		= UtilCoor::pos_ecef2lla(motion.pos_ecef);
	motion.vel_loc		= UtilCoor::vel_ecef2loc(motion.pos_lla, motion.vel_ecef);
	if (motion.update_time_ms <= 0) return;

	// 更新平台元信息，不清空已有原始运动帧
	S_PLAT& raw_plat = list_plat_raw[recv_plat.plt_id];
	raw_plat.type_name		= recv_plat.type_name;
	raw_plat.plt_id			= recv_plat.plt_id;
	raw_plat.fmt_id			= recv_plat.fmt_id;
	raw_plat.cmd_id			= recv_plat.cmd_id;
	raw_plat.side			= recv_plat.side;
	raw_plat.type			= recv_plat.type;
	raw_plat.valid			= recv_plat.valid;
	raw_plat.algo_threat	= recv_plat.algo_threat;
	raw_plat.weapon_load	= recv_plat.weapon_load;
	raw_plat.sensor_load	= recv_plat.sensor_load;

	// 原始运动帧只保留当前对齐时间前后最近两帧
	const long long align_time_ms = sim_start_time_ms + sim_running_num * static_cast<long long>(ALIGN_STEP_MS);
	std::vector<S_MOTION_FRAME> candidates = raw_plat.list_motion;
	candidates.push_back(motion);

	S_MOTION_FRAME hst_motion{};
	S_MOTION_FRAME raw_motion{};
	S_MOTION_FRAME latest_motion{};
	bool has_hst_motion = false;
	bool has_raw_motion = false;
	bool has_latest_motion = false;
	for (const S_MOTION_FRAME& item : candidates) {
		if (item.update_time_ms <= 0) continue;
		if (item.update_time_ms <= align_time_ms &&
			(!has_hst_motion || item.update_time_ms > hst_motion.update_time_ms)) {
			hst_motion = item;
			has_hst_motion = true;
		}
		if (item.update_time_ms >= align_time_ms &&
			(!has_raw_motion || item.update_time_ms < raw_motion.update_time_ms)) {
			raw_motion = item;
			has_raw_motion = true;
		}
		if (!has_latest_motion || item.update_time_ms > latest_motion.update_time_ms) {
			latest_motion = item;
			has_latest_motion = true;
		}
	}
	if (!has_raw_motion && has_latest_motion) {
		raw_motion = latest_motion;
		has_raw_motion = true;
	}

	// 重新写入原始运动帧，确保list_plat_raw最多只保存两帧
	raw_plat.list_motion.clear();
	if (has_hst_motion) raw_plat.list_motion.push_back(hst_motion);
	if (has_raw_motion &&
		(raw_plat.list_motion.empty() || raw_plat.list_motion.back().update_time_ms != raw_motion.update_time_ms)) {
		raw_plat.list_motion.push_back(raw_motion);
	}
}

void ModuleDatabase::on_sim_platform_update(SNAPSHOT_SIM_PLATFORM list_platform)
{
	if (!is_running || list_platform.isNull()) return;

	for (const auto& item : *list_platform) {
		const S_SNAPSHOT_PLAT_STATE& sim_plat = item.second;

		// 仿真平台元信息赋值，运动数据统一放在list_motion中
		S_PLAT plat{};
		plat.type_name		= sim_plat.type_name;
		plat.plt_id			= sim_plat.plt_id.trimmed();
		plat.fmt_id			= sim_plat.fmt_id.trimmed();
		if (plat.fmt_id.isEmpty()) plat.fmt_id = plat.plt_id;
		plat.cmd_id			= sim_plat.cmd_id.trimmed();
		if (plat.cmd_id.isEmpty()) plat.cmd_id = QStringLiteral("SELF");
		plat.side			= sim_plat.side;
		plat.type			= sim_plat.type;
		plat.algo_threat	= S_ALGO_THREAT{};
		plat.weapon_load	= sim_plat.weapon_load;
		plat.sensor_load	= sim_plat.sensor_load;
		plat.valid			= sim_plat.plat_life.active &&
			sim_plat.plat_life.state != E_PLAT_LIFE::DESTROY;

		// 删除无效平台，同时清理原始缓存和对齐缓存
		if (plat.plt_id.isEmpty() || !plat.valid) {
			list_plat_raw		.erase(plat.plt_id);
			list_own_plat_align	.erase(plat.plt_id);
			list_tgt_plat_align	.erase(plat.plt_id);
			continue;
		}

		// 将仿真平台状态转换为统一运动帧
		S_MOTION_FRAME motion{};
		motion.pos_ecef		= sim_plat.plat_state.pos_ecef;
		motion.vel_ecef		= UtilCoor::vel_loc2ecef(sim_plat.plat_state.vel_loc, sim_plat.plat_state.pos_lla);
		motion.pos_lla		= sim_plat.plat_state.pos_lla;
		motion.vel_loc		= sim_plat.plat_state.vel_loc;
		motion.update_time_ms	= sim_plat.plat_state.update_time_ms;
		if (motion.update_time_ms <= 0) continue;

		// 更新平台元信息，不清空已有原始运动帧
		S_PLAT& raw_plat = list_plat_raw[plat.plt_id];
		raw_plat.type_name		= plat.type_name;
		raw_plat.plt_id			= plat.plt_id;
		raw_plat.fmt_id			= plat.fmt_id;
		raw_plat.cmd_id			= plat.cmd_id;
		raw_plat.side			= plat.side;
		raw_plat.type			= plat.type;
		raw_plat.valid			= plat.valid;
		raw_plat.algo_threat	= plat.algo_threat;
		raw_plat.weapon_load	= plat.weapon_load;
		raw_plat.sensor_load	= plat.sensor_load;

		// 原始运动帧只保留当前对齐时间前后最近两帧
		const long long align_time_ms = sim_start_time_ms + sim_running_num * static_cast<long long>(ALIGN_STEP_MS);
		std::vector<S_MOTION_FRAME> candidates = raw_plat.list_motion;
		candidates.push_back(motion);

		S_MOTION_FRAME hst_motion{};
		S_MOTION_FRAME raw_motion{};
		S_MOTION_FRAME latest_motion{};
		bool has_hst_motion = false;
		bool has_raw_motion = false;
		bool has_latest_motion = false;
		for (const S_MOTION_FRAME& item : candidates) {
			if (item.update_time_ms <= 0) continue;
			if (item.update_time_ms <= align_time_ms &&
				(!has_hst_motion || item.update_time_ms > hst_motion.update_time_ms)) {
				hst_motion = item;
				has_hst_motion = true;
			}
			if (item.update_time_ms >= align_time_ms &&
				(!has_raw_motion || item.update_time_ms < raw_motion.update_time_ms)) {
				raw_motion = item;
				has_raw_motion = true;
			}
			if (!has_latest_motion || item.update_time_ms > latest_motion.update_time_ms) {
				latest_motion = item;
				has_latest_motion = true;
			}
		}
		if (!has_raw_motion && has_latest_motion) {
			raw_motion = latest_motion;
			has_raw_motion = true;
		}

		// 重新写入原始运动帧，确保list_plat_raw最多只保存两帧
		raw_plat.list_motion.clear();
		if (has_hst_motion) raw_plat.list_motion.push_back(hst_motion);
		if (has_raw_motion &&
			(raw_plat.list_motion.empty() || raw_plat.list_motion.back().update_time_ms != raw_motion.update_time_ms)) {
			raw_plat.list_motion.push_back(raw_motion);
		}
	}
}

void ModuleDatabase::on_sim_weapon_update(SNAPSHOT_SIM_WEAPON list_weapon)
{
	if (!is_running || list_weapon.isNull()) return;

	for (const auto& item : *list_weapon) {
		const S_SNAPSHOT_WEAPON_STATE& sim_weapon = item.second;

		// 仿真武器元信息赋值，运动数据统一放在list_motion中
		S_WEAPON weapon{};
		weapon.type_name	= sim_weapon.type_name;
		weapon.weapon_id	= sim_weapon.weapon_id.trimmed();
		weapon.own_plat_id	= sim_weapon.own_plat_id.trimmed();
		weapon.tgt_plat_id	= sim_weapon.tgt_plat_id.trimmed();
		weapon.side			= sim_weapon.side;
		weapon.type			= sim_weapon.type;
		weapon.algo_threat	= S_ALGO_THREAT{};
		weapon.valid		= sim_weapon.weapon_life.active &&
			sim_weapon.weapon_life.state == E_WEAPON_LIFE::FLYING;

		// 删除无效武器，同时清理原始缓存和对齐缓存
		if (weapon.weapon_id.isEmpty() || !weapon.valid) {
			list_weapon_raw			.erase(weapon.weapon_id);
			list_own_weapon_align	.erase(weapon.weapon_id);
			list_tgt_weapon_align	.erase(weapon.weapon_id);
			continue;
		}

		// 将仿真武器状态转换为统一运动帧
		S_MOTION_FRAME motion{};
		motion.pos_ecef		= sim_weapon.weapon_state.pos_ecef;
		motion.vel_ecef		= UtilCoor::vel_loc2ecef(sim_weapon.weapon_state.vel_loc, sim_weapon.weapon_state.pos_lla);
		motion.pos_lla		= sim_weapon.weapon_state.pos_lla;
		motion.vel_loc		= sim_weapon.weapon_state.vel_loc;
		motion.update_time_ms	= sim_weapon.weapon_state.update_time_ms;
		if (motion.update_time_ms <= 0) continue;

		// 更新武器元信息，不清空已有原始运动帧
		S_WEAPON& raw_weapon = list_weapon_raw[weapon.weapon_id];
		raw_weapon.type_name	= weapon.type_name;
		raw_weapon.weapon_id	= weapon.weapon_id;
		raw_weapon.own_plat_id	= weapon.own_plat_id;
		raw_weapon.tgt_plat_id	= weapon.tgt_plat_id;
		raw_weapon.side			= weapon.side;
		raw_weapon.type			= weapon.type;
		raw_weapon.valid		= weapon.valid;
		raw_weapon.algo_threat	= weapon.algo_threat;

		// 原始运动帧只保留当前对齐时间前后最近两帧
		const long long align_time_ms = sim_start_time_ms + sim_running_num * static_cast<long long>(ALIGN_STEP_MS);
		std::vector<S_MOTION_FRAME> candidates = raw_weapon.list_motion;
		candidates.push_back(motion);

		S_MOTION_FRAME hst_motion{};
		S_MOTION_FRAME raw_motion{};
		S_MOTION_FRAME latest_motion{};
		bool has_hst_motion = false;
		bool has_raw_motion = false;
		bool has_latest_motion = false;
		for (const S_MOTION_FRAME& item : candidates) {
			if (item.update_time_ms <= 0) continue;
			if (item.update_time_ms <= align_time_ms &&
				(!has_hst_motion || item.update_time_ms > hst_motion.update_time_ms)) {
				hst_motion = item;
				has_hst_motion = true;
			}
			if (item.update_time_ms >= align_time_ms &&
				(!has_raw_motion || item.update_time_ms < raw_motion.update_time_ms)) {
				raw_motion = item;
				has_raw_motion = true;
			}
			if (!has_latest_motion || item.update_time_ms > latest_motion.update_time_ms) {
				latest_motion = item;
				has_latest_motion = true;
			}
		}
		if (!has_raw_motion && has_latest_motion) {
			raw_motion = latest_motion;
			has_raw_motion = true;
		}

		// 重新写入原始运动帧，确保list_weapon_raw最多只保存两帧
		raw_weapon.list_motion.clear();
		if (has_hst_motion) raw_weapon.list_motion.push_back(hst_motion);
		if (has_raw_motion &&
			(raw_weapon.list_motion.empty() || raw_weapon.list_motion.back().update_time_ms != raw_motion.update_time_ms)) {
			raw_weapon.list_motion.push_back(raw_motion);
		}
	}
}

void ModuleDatabase::on_sim_start()
{
	// 启动前重置运行计数
	is_running			= true;
	sim_start_time_ms	= QDateTime::currentMSecsSinceEpoch();
	sim_running_num		= 0;
	algo_running_num	= 0;

	// 已存在定时器时直接复用，避免重复连接timeout信号
	if (timer_update != nullptr) {
		if (!timer_update->isActive()) {
			timer_update->start();
		}
		return;
	}

	// 定时执行原始数据到对齐数据的插值
	timer_update = new QTimer(this);
	timer_update->setInterval(ALIGN_STEP_MS);
	connect(timer_update, &QTimer::timeout, this, &ModuleDatabase::on_timer_update);
	timer_update->start();

	// 定时调用算法并发送快照
	timer_algo = new QTimer(this);
	timer_algo->setInterval(algo_step_ms);
	connect(timer_algo, &QTimer::timeout, this, &ModuleDatabase::on_timer_algo);
	timer_algo->start();
}

void ModuleDatabase::on_sim_stop()
{
	// 停止并释放对齐定时器
	if (timer_update != nullptr) {
		timer_update->stop();
		delete timer_update;
		timer_update = nullptr;
	}
	// 停止并释放算法定时器
	if (timer_algo != nullptr) {
		timer_algo->stop();
		delete timer_algo;
		timer_algo = nullptr;
	}

	// 清理缓存前重置运行状态
	sim_start_time_ms	= 0;
	sim_running_num		= 0;
	algo_running_num	= 0;
	is_running			= false;

	clear_run_data();
}

void ModuleDatabase::clear_run_data()
{
	// 同时清理原始缓存和对齐缓存
	list_plat_raw			.clear();	list_weapon_raw			.clear();
	list_own_plat_align		.clear();	list_tgt_plat_align		.clear();
	list_own_weapon_align	.clear();	list_tgt_weapon_align	.clear();
	list_own_fmt_align		.clear();	list_tgt_fmt_align		.clear();
	reset_coa(coa_own);

	// 发送空快照，避免UI和算法继续使用过期目标
	emit send_algo_plat(SNAPSHOT_ALGO_PLAT(new std::map<QString, S_PLAT>()));
	emit send_algo_weapon(SNAPSHOT_ALGO_WEAPON(new std::map<QString, S_WEAPON>()));
	emit send_algo_fmt(SNAPSHOT_ALGO_FMT(new std::map<QString, S_FORMAT>()));
	emit send_algo_coa(SNAPSHOT_ALGO_COA(new S_COA(coa_own)));
}

void ModuleDatabase::on_timer_update()
{
	if (!is_running) return;

	// 根据仿真开始时间和对齐计数计算当前对齐时间
	const long long current_time_ms = sim_start_time_ms + sim_running_num * ALIGN_STEP_MS;

	// 平台和武器使用同一时间轴进行对齐
	align_platform(current_time_ms);
	align_weapon(current_time_ms);

	// 仿真对齐计数递增
	sim_running_num++;
}

void ModuleDatabase::on_timer_algo()
{
	if (!is_running) return;

	/* ---------- 算法调用 ---------- */
	qDebug() << "RUN NUM = " << algo_running_num;
	qDebug() << "ALGO START TIME: " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

	// 敌方分群算法
	module_emy_group.func_emy_group(list_own_fmt_align, list_tgt_fmt_align,
		list_own_plat_align, list_tgt_plat_align, list_own_weapon_align, list_tgt_weapon_align);

	// 敌方威胁算法
	module_emy_threat.func_emy_threat(list_own_fmt_align, list_tgt_fmt_align,
		list_own_plat_align, list_tgt_plat_align);

	// Our COG算法
	module_our_cog.func_our_cog(list_tgt_fmt_align, list_tgt_plat_align);

	// Our LOO算法（暂为群目标分配）
	module_our_loo.func_our_loo(coa_own, list_own_fmt_align, list_tgt_fmt_align,
		list_own_plat_align, list_tgt_plat_align);

	this->algo_running_num++;
	qDebug() << "ALGO FINISH TIME: " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
	qDebug() << "----------------------------------------";
	/* ---------- 算法调用 ---------- */

	// 构造平台快照
	auto list_algo_plat = new std::map<QString, S_PLAT>();
	for (const auto& item : list_own_plat_align) {
		const S_PLAT& plat = item.second;
		if (!plat.valid || plat.list_motion.empty()) continue;
		(*list_algo_plat)[item.first] = plat;
	}
	for (const auto& item : list_tgt_plat_align) {
		const S_PLAT& plat = item.second;
		if (!plat.valid || plat.list_motion.empty()) continue;
		(*list_algo_plat)[item.first] = plat;
	}

	// 构造武器快照
	auto list_algo_weapon = new std::map<QString, S_WEAPON>();
	for (const auto& item : list_own_weapon_align) {
		const S_WEAPON& weapon = item.second;
		if (!weapon.valid || weapon.list_motion.empty()) continue;
		(*list_algo_weapon)[item.first] = weapon;
	}
	for (const auto& item : list_tgt_weapon_align) {
		const S_WEAPON& weapon = item.second;
		if (!weapon.valid || weapon.list_motion.empty()) continue;
		(*list_algo_weapon)[item.first] = weapon;
	}

	// 构造编队快照
	auto list_algo_fmt = new std::map<QString, S_FORMAT>();
	for (const auto& item : list_own_fmt_align) {
		const S_FORMAT& format = item.second;
		if (!format.valid || format.list_motion.empty()) continue;
		(*list_algo_fmt)[QStringLiteral("RED::%1").arg(item.first)] = format;
	}
	for (const auto& item : list_tgt_fmt_align) {
		const S_FORMAT& format = item.second;
		if (!format.valid || format.list_motion.empty()) continue;
		(*list_algo_fmt)[QStringLiteral("BLUE::%1").arg(item.first)] = format;
	}

	// 对齐对象拷贝完成后发送快照
	emit send_algo_plat(SNAPSHOT_ALGO_PLAT(list_algo_plat));
	emit send_algo_weapon(SNAPSHOT_ALGO_WEAPON(list_algo_weapon));
	emit send_algo_fmt(SNAPSHOT_ALGO_FMT(list_algo_fmt));
	emit send_algo_coa(SNAPSHOT_ALGO_COA(new S_COA(coa_own)));
}

void ModuleDatabase::align_platform(const long long current_time_ms)
{
	for (auto& raw_iter : list_plat_raw) {
		S_PLAT& raw_plat = raw_iter.second;
		if (!raw_plat.valid || raw_plat.plt_id.isEmpty()) continue;

		// 查找当前对齐时间前后最近的平台原始帧
		const S_MOTION_FRAME* hst_motion = nullptr;
		const S_MOTION_FRAME* raw_motion = nullptr;
		for (const S_MOTION_FRAME& motion : raw_plat.list_motion) {
			if (motion.update_time_ms <= 0) continue;
			if (motion.update_time_ms <= current_time_ms &&
				(hst_motion == nullptr || motion.update_time_ms > hst_motion->update_time_ms)) {
				hst_motion = &motion;
			}
			if (motion.update_time_ms >= current_time_ms &&
				(raw_motion == nullptr || motion.update_time_ms < raw_motion->update_time_ms)) {
				raw_motion = &motion;
			}
		}

		// 在成员函数内直接完成平台运动插值
		S_MOTION_FRAME aligned_motion{};
		bool has_aligned_motion = false;
		if (hst_motion != nullptr && hst_motion->update_time_ms == current_time_ms) {
			aligned_motion = *hst_motion;
			has_aligned_motion = true;
		}
		else if (raw_motion != nullptr && raw_motion->update_time_ms == current_time_ms) {
			aligned_motion = *raw_motion;
			has_aligned_motion = true;
		}
		else if (hst_motion != nullptr && raw_motion != nullptr) {
			const long long align_delta_ms = raw_motion->update_time_ms - hst_motion->update_time_ms;
			if (align_delta_ms > 0) {
				const double raw_ratio = static_cast<double>(current_time_ms - hst_motion->update_time_ms) /
					static_cast<double>(align_delta_ms);
				if (raw_ratio >= 0.0 && raw_ratio <= 1.0) {
					aligned_motion.pos_ecef			= UtilCoor::equal_pos_ecef(raw_motion->pos_ecef, hst_motion->pos_ecef, raw_ratio);
					aligned_motion.vel_ecef			= UtilCoor::equal_vel_ecef(raw_motion->vel_ecef, hst_motion->vel_ecef, raw_ratio);
					aligned_motion.pos_lla			= UtilCoor::equal_pos_lla(raw_motion->pos_lla, hst_motion->pos_lla, raw_ratio);
					aligned_motion.vel_loc			= UtilCoor::equal_vel_loc(raw_motion->vel_loc, hst_motion->vel_loc, raw_ratio);
					aligned_motion.update_time_ms	= current_time_ms;
					has_aligned_motion = true;
				}
			}
		}
		if (!has_aligned_motion) continue;

		// 按阵营选择平台对齐缓存，并清理另一侧残留数据
		S_PLAT* aligned_plat = nullptr;
		switch (raw_plat.side) {
		case E_SIDE::RED:
			list_tgt_plat_align.erase(raw_plat.plt_id);
			aligned_plat = &list_own_plat_align[raw_plat.plt_id];
			break;
		case E_SIDE::BLUE:
			list_own_plat_align.erase(raw_plat.plt_id);
			aligned_plat = &list_tgt_plat_align[raw_plat.plt_id];
			break;
		default:
			list_own_plat_align.erase(raw_plat.plt_id);
			list_tgt_plat_align.erase(raw_plat.plt_id);
			continue;
		}

		// 更新平台元信息，同时保留已对齐的运动历史
		aligned_plat->type_name		= raw_plat.type_name;
		aligned_plat->plt_id		= raw_plat.plt_id;
		aligned_plat->fmt_id		= raw_plat.fmt_id;
		aligned_plat->cmd_id		= raw_plat.cmd_id;
		aligned_plat->side			= raw_plat.side;
		aligned_plat->type			= raw_plat.type;
		aligned_plat->valid			= raw_plat.valid;
		aligned_plat->algo_threat	= raw_plat.algo_threat;
		aligned_plat->weapon_load	= raw_plat.weapon_load;
		aligned_plat->sensor_load	= raw_plat.sensor_load;

		// 按时间顺序插入或替换平台对齐运动帧
		std::vector<S_MOTION_FRAME>& aligned_motions = aligned_plat->list_motion;
		int insert_index = 0;
		while (insert_index < static_cast<int>(aligned_motions.size()) &&
			aligned_motions[insert_index].update_time_ms < aligned_motion.update_time_ms) {
			insert_index++;
		}
		if (insert_index < static_cast<int>(aligned_motions.size()) &&
			aligned_motions[insert_index].update_time_ms == aligned_motion.update_time_ms) {
			aligned_motions[insert_index] = aligned_motion;
		}
		else {
			aligned_motions.insert(aligned_motions.begin() + insert_index, aligned_motion);
		}

		// 平台对齐历史只保留最新align_max_size帧
		const int overflow_size = static_cast<int>(aligned_motions.size()) - ALIGN_MAX_SIZE;
		if (overflow_size > 0) {
			aligned_motions.erase(aligned_motions.begin(), aligned_motions.begin() + overflow_size);
		}
	}

	// 清理超时平台，同时移除原始缓存和对齐缓存
	for (auto raw_iter = list_plat_raw.begin(); raw_iter != list_plat_raw.end();) {
		const QString plt_id = raw_iter->first;
		const S_MOTION_FRAME* latest_motion = nullptr;
		for (const S_MOTION_FRAME& motion : raw_iter->second.list_motion) {
			if (motion.update_time_ms <= 0) continue;
			if (latest_motion == nullptr || motion.update_time_ms > latest_motion->update_time_ms) {
				latest_motion = &motion;
			}
		}
		if (!raw_iter->second.valid || latest_motion == nullptr ||
			current_time_ms - latest_motion->update_time_ms > plt_timeout_ms) {
			list_own_plat_align.erase(plt_id);
			list_tgt_plat_align.erase(plt_id);
			raw_iter = list_plat_raw.erase(raw_iter);
		}
		else {
			++raw_iter;
		}
	}
}

void ModuleDatabase::align_weapon(const long long current_time_ms)
{
	for (auto& raw_iter : list_weapon_raw) {
		S_WEAPON& raw_weapon = raw_iter.second;
		if (!raw_weapon.valid || raw_weapon.weapon_id.isEmpty()) continue;

		// 查找当前对齐时间前后最近的武器原始帧
		const S_MOTION_FRAME* hst_motion = nullptr;
		const S_MOTION_FRAME* raw_motion = nullptr;
		for (const S_MOTION_FRAME& motion : raw_weapon.list_motion) {
			if (motion.update_time_ms <= 0) continue;
			if (motion.update_time_ms <= current_time_ms &&
				(hst_motion == nullptr || motion.update_time_ms > hst_motion->update_time_ms)) {
				hst_motion = &motion;
			}
			if (motion.update_time_ms >= current_time_ms &&
				(raw_motion == nullptr || motion.update_time_ms < raw_motion->update_time_ms)) {
				raw_motion = &motion;
			}
		}

		// 在成员函数内直接完成武器运动插值
		S_MOTION_FRAME aligned_motion{};
		bool has_aligned_motion = false;
		if (hst_motion != nullptr && hst_motion->update_time_ms == current_time_ms) {
			aligned_motion = *hst_motion;
			has_aligned_motion = true;
		}
		else if (raw_motion != nullptr && raw_motion->update_time_ms == current_time_ms) {
			aligned_motion = *raw_motion;
			has_aligned_motion = true;
		}
		else if (hst_motion != nullptr && raw_motion != nullptr) {
			const long long align_delta_ms = raw_motion->update_time_ms - hst_motion->update_time_ms;
			if (align_delta_ms > 0) {
				const double raw_ratio = static_cast<double>(current_time_ms - hst_motion->update_time_ms) /
					static_cast<double>(align_delta_ms);
				if (raw_ratio >= 0.0 && raw_ratio <= 1.0) {
					aligned_motion.pos_ecef			= UtilCoor::equal_pos_ecef(raw_motion->pos_ecef, hst_motion->pos_ecef, raw_ratio);
					aligned_motion.vel_ecef			= UtilCoor::equal_vel_ecef(raw_motion->vel_ecef, hst_motion->vel_ecef, raw_ratio);
					aligned_motion.pos_lla			= UtilCoor::equal_pos_lla(raw_motion->pos_lla, hst_motion->pos_lla, raw_ratio);
					aligned_motion.vel_loc			= UtilCoor::equal_vel_loc(raw_motion->vel_loc, hst_motion->vel_loc, raw_ratio);
					aligned_motion.update_time_ms	= current_time_ms;
					has_aligned_motion = true;
				}
			}
		}
		if (!has_aligned_motion) continue;

		// 按阵营选择武器对齐缓存，并清理另一侧残留数据
		S_WEAPON* aligned_weapon = nullptr;
		switch (raw_weapon.side) {
		case E_SIDE::RED:
			list_tgt_weapon_align.erase(raw_weapon.weapon_id);
			aligned_weapon = &list_own_weapon_align[raw_weapon.weapon_id];
			break;
		case E_SIDE::BLUE:
			list_own_weapon_align.erase(raw_weapon.weapon_id);
			aligned_weapon = &list_tgt_weapon_align[raw_weapon.weapon_id];
			break;
		default:
			list_own_weapon_align.erase(raw_weapon.weapon_id);
			list_tgt_weapon_align.erase(raw_weapon.weapon_id);
			continue;
		}

		// 更新武器元信息，同时保留已对齐的运动历史
		aligned_weapon->type_name	= raw_weapon.type_name;
		aligned_weapon->weapon_id	= raw_weapon.weapon_id;
		aligned_weapon->own_plat_id	= raw_weapon.own_plat_id;
		aligned_weapon->tgt_plat_id	= raw_weapon.tgt_plat_id;
		aligned_weapon->side		= raw_weapon.side;
		aligned_weapon->type		= raw_weapon.type;
		aligned_weapon->valid		= raw_weapon.valid;
		aligned_weapon->algo_threat	= raw_weapon.algo_threat;

		// 按时间顺序插入或替换武器对齐运动帧
		std::vector<S_MOTION_FRAME>& aligned_motions = aligned_weapon->list_motion;
		int insert_index = 0;
		while (insert_index < static_cast<int>(aligned_motions.size()) &&
			aligned_motions[insert_index].update_time_ms < aligned_motion.update_time_ms) {
			insert_index++;
		}
		if (insert_index < static_cast<int>(aligned_motions.size()) &&
			aligned_motions[insert_index].update_time_ms == aligned_motion.update_time_ms) {
			aligned_motions[insert_index] = aligned_motion;
		}
		else {
			aligned_motions.insert(aligned_motions.begin() + insert_index, aligned_motion);
		}

		// 武器对齐历史只保留最新align_max_size帧
		const int overflow_size = static_cast<int>(aligned_motions.size()) - ALIGN_MAX_SIZE;
		if (overflow_size > 0) {
			aligned_motions.erase(aligned_motions.begin(), aligned_motions.begin() + overflow_size);
		}
	}

	// 清理超时武器，同时移除原始缓存和对齐缓存
	for (auto weapon_iter = list_weapon_raw.begin(); weapon_iter != list_weapon_raw.end();) {
		const QString weapon_id = weapon_iter->first;
		const S_MOTION_FRAME* latest_motion = nullptr;
		for (const S_MOTION_FRAME& motion : weapon_iter->second.list_motion) {
			if (motion.update_time_ms <= 0) continue;
			if (latest_motion == nullptr || motion.update_time_ms > latest_motion->update_time_ms) {
				latest_motion = &motion;
			}
		}
		if (!weapon_iter->second.valid || latest_motion == nullptr ||
			current_time_ms - latest_motion->update_time_ms > plt_timeout_ms) {
			list_own_weapon_align.erase(weapon_id);
			list_tgt_weapon_align.erase(weapon_id);
			weapon_iter = list_weapon_raw.erase(weapon_iter);
		}
		else {
			++weapon_iter;
		}
	}
}
