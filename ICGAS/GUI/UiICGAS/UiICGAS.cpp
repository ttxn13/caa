#include "pch.hpp"
#include "UiICGAS.hpp"
#include "UiAFSimTest.hpp"
#include "UiGraph.hpp"
#include "UiInitMap.hpp"
#include "UiOsgInitMap.hpp"
#include "UiScenario.hpp"
#include "UiSitMonitor.hpp"
#include "UiWorkflow.hpp"
#include "../../Module/DataManage/ModuleBvrSolver.hpp"
#include "../../Simulation/Controller/SimController.hpp"

#include <cmath>
#include <QDir>
#include <QFileInfo>
#include <QtQml/qqml.h>

QMutex UiICGAS::mutex;
UiICGAS* UiICGAS::instance = NULL;

namespace {
QString mesh_dir_path()
{
	const QDir app_dir(QCoreApplication::applicationDirPath());
	const QStringList candidates = {
		app_dir.filePath(QStringLiteral("App/mesh")),
		app_dir.filePath(QStringLiteral("../App/mesh")),
		app_dir.filePath(QStringLiteral("mesh")),
		QDir::current().filePath(QStringLiteral("ICGAS/App/mesh")),
		QDir::current().filePath(QStringLiteral("App/mesh"))
	};
	for (const QString& candidate : candidates) {
		if (QDir(candidate).exists()) return QDir(candidate).absolutePath();
	}
	return QDir::current().filePath(QStringLiteral("ICGAS/App/mesh"));
}

QString current_sim_scenario_id(const std::map<QString, std::vector<S_PLAT_CONFIG>>& config_map,
	const QString& selected_scenario)
{
	const QString scenario_id = selected_scenario.trimmed();
	if (config_map.empty()) return scenario_id;
	if (!scenario_id.isEmpty() && config_map.find(scenario_id) != config_map.end()) {
		return scenario_id;
	}
	return config_map.begin()->first;
}

const std::vector<S_PLAT_CONFIG>* exact_sim_config_list(
	const std::map<QString, std::vector<S_PLAT_CONFIG>>& config_map,
	const QString& scenario_id)
{
	const QString id = scenario_id.trimmed();
	if (id.isEmpty()) return nullptr;
	const auto iter = config_map.find(id);
	return iter == config_map.end() ? nullptr : &iter->second;
}

QString first_loaded_sim_scenario_id(const QStringList& scenario_names,
	const std::map<QString, std::vector<S_PLAT_CONFIG>>& config_map)
{
	for (const QString& name : scenario_names) {
		if (exact_sim_config_list(config_map, name) != nullptr) {
			return name;
		}
	}
	return config_map.empty() ? QString() : config_map.begin()->first;
}

std::vector<S_PLAT_CONFIG>* current_sim_config_list(
	std::map<QString, std::vector<S_PLAT_CONFIG>>& config_map,
	const QString& selected_scenario)
{
	const QString scenario_id = current_sim_scenario_id(config_map, selected_scenario);
	if (scenario_id.isEmpty()) return nullptr;
	return &config_map[scenario_id];
}

const std::vector<S_PLAT_CONFIG>* current_sim_config_list(
	const std::map<QString, std::vector<S_PLAT_CONFIG>>& config_map,
	const QString& selected_scenario)
{
	const QString scenario_id = current_sim_scenario_id(config_map, selected_scenario);
	if (scenario_id.isEmpty()) return nullptr;
	const auto iter = config_map.find(scenario_id);
	return iter == config_map.end() ? nullptr : &iter->second;
}

int sim_config_count(const std::map<QString, std::vector<S_PLAT_CONFIG>>& config_map)
{
	int count = 0;
	for (const auto& item : config_map) {
		count += static_cast<int>(item.second.size());
	}
	return count;
}

int sim_define_count(
	const std::map<QString, S_PLAT_DEFINE>& define_map)
{
	return static_cast<int>(define_map.size());
}

int sim_weapon_define_count(
	const std::map<QString, S_WEAPON_DEFINE>& define_map)
{
	return static_cast<int>(define_map.size());
}

int sim_sensor_define_count(
	const std::map<QString, S_SENSOR_DEFINE>& define_map)
{
	return static_cast<int>(define_map.size());
}

QString sensor_type_text(E_SENSOR_TYPE type)
{
	const QString type_text = UtilEnum::trans_sensor_type_en(type);
	return type_text == QStringLiteral("UKN") ? QStringLiteral("不明") : type_text;
}

S_SENSOR_TASK_MODE default_sensor_task_mode(E_SENSOR_TASK task, E_SENSOR_TYPE type)
{
	S_SENSOR_TASK_MODE mode;
	mode.max_task_num = task == E_SENSOR_TASK::DETECT ? 64 : 16;
	mode.cap_env.min_range_m = 500.0;
	mode.cap_env.max_range_m = type == E_SENSOR_TYPE::RADAR ? 180000.0 : 50000.0;
	mode.cap_env.az_fov_deg = type == E_SENSOR_TYPE::RADAR ? 120.0 : 60.0;
	mode.cap_env.min_el_deg = -5.0;
	mode.cap_env.max_el_deg = 70.0;
	mode.cap_env.min_alt_m = 0.0;
	mode.cap_env.max_alt_m = 30000.0;
	mode.cap_prob.detection_prob = 0.85;
	mode.cap_prob.false_alarm_prob = 0.02;
	mode.cap_prob.sensitive_slope = 1.0;
	mode.cap_acc.measure_range = true;
	mode.cap_acc.measure_az = true;
	mode.cap_acc.measure_el = true;
	mode.cap_acc.measure_speed = type == E_SENSOR_TYPE::RADAR;
	mode.cap_acc.range_sigma_m = type == E_SENSOR_TYPE::RADAR ? 50.0 : 20.0;
	mode.cap_acc.azimuth_sigma_deg = type == E_SENSOR_TYPE::RADAR ? 0.2 : 0.05;
	mode.cap_acc.elevation_sigma_deg = type == E_SENSOR_TYPE::RADAR ? 0.3 : 0.05;
	mode.cap_acc.speed_sigma_ms = type == E_SENSOR_TYPE::RADAR ? 5.0 : 0.0;
	mode.cap_time.update_period_s = task == E_SENSOR_TASK::DETECT ? 4.0 : 1.0;
	mode.cap_time.delay_ms = 100.0;
	return mode;
}

S_MOVER_CAP default_mover_cap(E_MOVER mover)
{
	switch (mover) {
	case E_MOVER::SEA_MOVER: return S_SEA_MOVER_CAP{};
	case E_MOVER::MIS_MOVER: return S_MIS_MOVER_CAP{};
	case E_MOVER::AIR_MOVER:
	default: return S_AIR_MOVER_CAP{};
	}
}

QString route_point_id(const S_PLAT_ROUTE& route, int index)
{
	QString route_id = route.route_id.trimmed().isEmpty()
		? QStringLiteral("route")
		: route.route_id.trimmed();
	if (route_id.endsWith(QStringLiteral("_route"), Qt::CaseInsensitive)) {
		route_id.chop(6);
	}
	return QStringLiteral("%1_wp_%2").arg(route_id).arg(index + 1, 3, 10, QChar('0'));
}

void ensure_route_defaults(S_PLAT_CONFIG& config)
{
	if (config.init_route.route_id.trimmed().isEmpty()) {
		config.init_route.route_id = config.plt_id.trimmed() + QStringLiteral("_route");
	}
	if (!config.init_route.point_list.empty()) return;

	S_ROUTE_POINT first_point;
	first_point.point_id = route_point_id(config.init_route, 0);
	first_point.pos_lla = config.init_state.pos_lla;
	first_point.speed_ms = config.init_state.vel_loc.speed_ms;
	config.init_route.point_list.push_back(first_point);
}

void set_initial_state_route_point(S_PLAT_CONFIG& config)
{
	ensure_route_defaults(config);

	S_ROUTE_POINT first_point;
	first_point.point_id = route_point_id(config.init_route, 0);
	first_point.pos_lla = config.init_state.pos_lla;
	first_point.speed_ms = config.init_state.vel_loc.speed_ms;
	config.init_route.point_list[0] = first_point;
}

void renumber_route_points(S_PLAT_ROUTE& route)
{
	for (int index = 0; index < static_cast<int>(route.point_list.size()); ++index) {
		route.point_list[index].point_id = route_point_id(route, index);
	}
}

QString platform_config_name_for_type(const QString& type_id, const QString& platform_id)
{
	Q_UNUSED(platform_id);
	const QString type = type_id.trimmed();
	return type;
}

QString sanitized_initial_scenario_id(const QString& value)
{
	QString id = value.trimmed();
	id.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
	id.replace(QRegularExpression(QStringLiteral(R"(\s+)")), QStringLiteral("_"));
	return id;
}

E_SIDE side_from_detail_value(const QString& value)
{
	const QString side = value.trimmed().toUpper();
	if (side == QStringLiteral("RED") || side == QStringLiteral("红方")) return E_SIDE::RED;
	if (side == QStringLiteral("BLUE") || side == QStringLiteral("蓝方")) return E_SIDE::BLUE;
	if (side == QStringLiteral("WHITE") || side == QStringLiteral("白方")) return E_SIDE::WHITE;
	return E_SIDE::UKN;
}

QString save_status_text(bool success, const QString& success_text, const QString& fail_text)
{
	return success ? success_text : fail_text;
}

QString side_map_key(E_SIDE side)
{
	switch (side) {
	case E_SIDE::RED: return QStringLiteral("red");
	case E_SIDE::BLUE: return QStringLiteral("blue");
	case E_SIDE::WHITE: return QStringLiteral("white");
	default: return QStringLiteral("unknown");
	}
}

QString weapon_type_text(E_WEAPON_TYPE type)
{
	const QString type_text = UtilEnum::trans_weapon_type_en(type);
	return type_text == QStringLiteral("UKN") ? QStringLiteral("不明") : type_text;
}

QString compact_number_text(double value, int precision = 4)
{
	QString text = QString::number(value, 'f', precision);
	text.remove(QRegularExpression(QStringLiteral(R"(\.?0+$)")));
	return text == QStringLiteral("-0") ? QStringLiteral("0") : text;
}

const S_MOTION_FRAME* latest_motion(const S_PLAT& plat)
{
	return plat.list_motion.empty() ? nullptr : &plat.list_motion.back();
}

const S_MOTION_FRAME* latest_motion(const S_WEAPON& weapon)
{
	return weapon.list_motion.empty() ? nullptr : &weapon.list_motion.back();
}

const S_MOTION_FRAME* latest_motion(const S_FORMAT& format)
{
	return format.list_motion.empty() ? nullptr : &format.list_motion.back();
}

QString format_monitor_id(const S_FORMAT& format)
{
	const QString id = format.fmt_id.trimmed();
	return QStringLiteral("FMT::%1::%2")
		.arg(static_cast<int>(format.side))
		.arg(id.isEmpty() ? QStringLiteral("UNKNOWN") : id);
}

QString format_member_text(const S_FORMAT& format)
{
	QStringList list_member;
	for (const QString& plat_id : format.plat_id_list) {
		const QString id = plat_id.trimmed();
		if (!id.isEmpty()) list_member.push_back(id);
	}
	return list_member.join(QStringLiteral(", "));
}

double threat_value_number(const S_ALGO_THREAT& threat)
{
	return std::isfinite(threat.threat_value) ? threat.threat_value : 0.0;
}

QString threat_value_text(const S_ALGO_THREAT& threat)
{
	return QString::number(threat_value_number(threat), 'f', 3);
}

int threat_level_number(const S_ALGO_THREAT& threat)
{
	return static_cast<int>(threat.threat_level);
}

QString threat_level_text(const S_ALGO_THREAT& threat)
{
	const int level = threat_level_number(threat);
	return level > 0 ? QString::number(level) : QStringLiteral("--");
}

QString threat_sort_text(const S_ALGO_THREAT& threat)
{
	return threat.threat_sort > 0 ? QString::number(threat.threat_sort) : QStringLiteral("--");
}

void append_threat_fields(QVariantMap& row, const S_ALGO_THREAT& threat)
{
	row[QStringLiteral("threatValue")] = threat_value_number(threat);
	row[QStringLiteral("threatValueText")] = threat_value_text(threat);
	row[QStringLiteral("threatLevel")] = threat_level_number(threat);
	row[QStringLiteral("threatLevelText")] = threat_level_text(threat);
	row[QStringLiteral("threatSort")] = threat.threat_sort;
	row[QStringLiteral("threatSortText")] = threat_sort_text(threat);
}

bool has_cog_result(const S_ALGO_COG& cog)
{
	return cog.cog_sort > 0 || cog.cog_level != E_LEVEL::UKN || cog.cog_value > 0.0;
}

double cog_value_number(const S_ALGO_COG& cog)
{
	return std::isfinite(cog.cog_value) ? cog.cog_value : 0.0;
}

QString cog_value_text(const S_ALGO_COG& cog)
{
	return has_cog_result(cog) ? QString::number(cog_value_number(cog), 'f', 3) : QStringLiteral("--");
}

int cog_level_number(const S_ALGO_COG& cog)
{
	return static_cast<int>(cog.cog_level);
}

QString cog_level_text(const S_ALGO_COG& cog)
{
	const int level = cog_level_number(cog);
	return has_cog_result(cog) && level > 0 ? QString::number(level) : QStringLiteral("--");
}

QString cog_sort_text(const S_ALGO_COG& cog)
{
	return cog.cog_sort > 0 ? QString::number(cog.cog_sort) : QStringLiteral("--");
}

void append_cog_fields(QVariantMap& row, const S_ALGO_COG& cog)
{
	row[QStringLiteral("cogValue")] = cog_value_number(cog);
	row[QStringLiteral("cogValueText")] = cog_value_text(cog);
	row[QStringLiteral("cogLevel")] = cog_level_number(cog);
	row[QStringLiteral("cogLevelText")] = cog_level_text(cog);
	row[QStringLiteral("cogSort")] = cog.cog_sort;
	row[QStringLiteral("cogSortText")] = cog_sort_text(cog);
}
}

UiICGAS* UiICGAS::GetInstance()
{
	if (instance == NULL) {
		mutex.lock();
		if (instance == NULL) {
			instance = new UiICGAS();
		}
		mutex.unlock();
	}
	return instance;
}

UiICGAS::UiICGAS(QObject* parent)
	: QObject(parent)
{
	this->sim_controller = SimController::GetInstance();
	this->module_db		= ModuleDatabase::GetInstance();
	this->afs_client	= AfsimClient::GetInstance();
	this->afs_server	= AfsimServer::GetInstance();
	this->tac_server	= TacviewServer::GetInstance();
	this->afsim_test	= new UiAFSimTest(this);
	this->ui_scenario	= new UiScenario(this, this);
	this->ui_graph		= new UiGraph(this, this);
	this->ui_sit_monitor = new UiSitMonitor(this, this);
	this->ui_workflow	= new UiWorkflow(this, this);

	qRegisterMetaType<SNAPSHOT_PLAT_DEFINE_MAP>("SNAPSHOT_PLAT_DEFINE_MAP");
	qRegisterMetaType<SNAPSHOT_PLAT_CONFIG_MAP>("SNAPSHOT_PLAT_CONFIG_MAP");
	qRegisterMetaType<SNAPSHOT_WEAPON_DEFINE_MAP>("SNAPSHOT_WEAPON_DEFINE_MAP");
	qRegisterMetaType<SNAPSHOT_SENSOR_DEFINE_MAP>("SNAPSHOT_SENSOR_DEFINE_MAP");
	qRegisterMetaType<SNAPSHOT_PLAT_DEFINE_PAIR>("SNAPSHOT_PLAT_DEFINE_PAIR");
	qRegisterMetaType<SNAPSHOT_PLAT_CONFIG_PAIR>("SNAPSHOT_PLAT_CONFIG_PAIR");
	qRegisterMetaType<SNAPSHOT_WEAPON_DEFINE_PAIR>("SNAPSHOT_WEAPON_DEFINE_PAIR");
	qRegisterMetaType<SNAPSHOT_SENSOR_DEFINE_PAIR>("SNAPSHOT_SENSOR_DEFINE_PAIR");
	qRegisterMetaType<SNAPSHOT_SIM_PLATFORM>("SNAPSHOT_SIM_PLATFORM");
	qRegisterMetaType<SNAPSHOT_SIM_WEAPON>("SNAPSHOT_SIM_WEAPON");
	qRegisterMetaType<SNAPSHOT_ALGO_PLAT>("SNAPSHOT_ALGO_PLAT");
	qRegisterMetaType<SNAPSHOT_ALGO_WEAPON>("SNAPSHOT_ALGO_WEAPON");
	qRegisterMetaType<SNAPSHOT_ALGO_FMT>("SNAPSHOT_ALGO_FMT");
	qRegisterMetaType<SNAPSHOT_ALGO_COA>("SNAPSHOT_ALGO_COA");
	init_connect();
	init_ui_state();
	connect(qApp, &QCoreApplication::aboutToQuit, this, &UiICGAS::shutdown);

	const QIcon appIcon(QStringLiteral(":/UiIcon/ICGAS_256.png"));
	this->setWindowIcon(appIcon);
	this->afsim_test->setWindowIcon(appIcon);

	timer_sync = new QTimer(this);
	timer_sync->setInterval(std::max(1, ui_sync_iter_ms.toInt()));
	connect(timer_sync, &QTimer::timeout, this, &UiICGAS::on_timer_sync);
	timer_sync->start();
}

UiICGAS::~UiICGAS()
{
	shutdown();
}

QVariantList UiICGAS::redSituationRows() const
{
	QVariantList rows;
	for (const QVariant& value : platform_rows) {
		const QVariantMap row = value.toMap();
		if (row.value(QStringLiteral("sideKey")).toString() == QStringLiteral("red")) {
			rows.push_back(row);
		}
	}
	return rows;
}

QVariantList UiICGAS::blueSituationRows() const
{
	QVariantList rows;
	for (const QVariant& value : platform_rows) {
		const QVariantMap row = value.toMap();
		if (row.value(QStringLiteral("sideKey")).toString() == QStringLiteral("blue")) {
			rows.push_back(row);
		}
	}
	return rows;
}

void UiICGAS::shutdown()
{
	if (shutdown_done) return;
	shutdown_done = true;

	if (timer_sync != nullptr) {
		timer_sync->stop();
	}

	stop_workflow_comm(false);

	if (module_db != nullptr) {
		set_module_database_running(false, false);
	}
}

void UiICGAS::show()
{
	if (engine.rootObjects().isEmpty()) {
		static bool qml_types_registered = false;
		if (!qml_types_registered) {
			qmlRegisterType<UiInitMap>("ICGAS.Ui", 1, 0, "UiInitMap");
			qmlRegisterType<UiOsgInitMap>("ICGAS.Ui", 1, 0, "UiOsgInitMap");
			qml_types_registered = true;
		}
		engine.rootContext()->setContextProperty("uiICGAS", this);
		engine.rootContext()->setContextProperty("uiAFSimTest", afsim_test);
		engine.rootContext()->setContextProperty("uiScenario", ui_scenario);
		engine.rootContext()->setContextProperty("uiGraph", ui_graph);
		engine.rootContext()->setContextProperty("uiSitMonitor", ui_sit_monitor);
		engine.rootContext()->setContextProperty("uiWorkflow", ui_workflow);
		const QUrl qml_url(QStringLiteral("qrc:/UiICGAS/UiICGAS.qml"));
		engine.load(qml_url);
		if (engine.rootObjects().isEmpty()) {
			qDebug() << "Error: 无法加载UiICGAS QML" << qml_url;
			return;
		}

		if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
			window->setIcon(window_icon);
			window->show();
			schedule_deferred_startup();
			QTimer::singleShot(0, window, [this, window]() {
				window->setProperty("uiStartupComplete", true);
				center_on_primary_screen(window);
				window->raise();
			});
		}
		else if (QObject* root = root_object()) {
			root->setProperty("visible", true);
		}
		return;
	}

	if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
		window->show();
		schedule_deferred_startup();
		QTimer::singleShot(0, window, [this, window]() {
			window->setProperty("uiStartupComplete", true);
			center_on_primary_screen(window);
			window->raise();
		});
	}
	else if (QObject* root = root_object()) {
		root->setProperty("visible", true);
	}
}

void UiICGAS::setWindowIcon(const QIcon& icon)
{
	window_icon = icon;
	if (afsim_test != nullptr) {
		afsim_test->setWindowIcon(window_icon);
	}
	if (QQuickWindow* window = qobject_cast<QQuickWindow*>(root_object())) {
		window->setIcon(window_icon);
	}
}

bool UiICGAS::workflowRunnable() const
{
	if (workflow_module_states.empty()) return false;
	for (const auto& item : workflow_module_states) {
		if (workflow_module_blocks(item.second)) {
			return false;
		}
	}
	return true;
}

QStringList UiICGAS::unsaved_workflow_changes() const
{
	QStringList items;
	if (comm_config_dirty) items << QStringLiteral("通信服务器配置");
	if (afs_client_config_dirty) items << QStringLiteral("AFSIM客户端配置");
	if (database_config_dirty) items << QStringLiteral("数据库配置");
	if (mission_config_dirty) items << QStringLiteral("上级指令");
	if (initial_sim_ctrl_dirty) items << QStringLiteral("仿真控制参数");
	if (initial_platform_type_dirty) items << QStringLiteral("平台类型配置");
	if (initial_weapon_type_dirty) items << QStringLiteral("武器类型配置");
	if (initial_sensor_type_dirty) items << QStringLiteral("传感器类型配置");
	if (initial_scenario_dirty) items << QStringLiteral("初始态势场景配置");
	if (initial_platform_detail_dirty) items << QStringLiteral("初始态势平台详情");
	if (conceptPendingDetailChanges()) items << QStringLiteral("概念图谱详情修改");
	if (conceptDirty()) items << QStringLiteral("概念图谱配置");
	items.removeDuplicates();
	return items;
}

bool UiICGAS::hasUnsavedWorkflowChanges() const
{
	return !unsaved_workflow_changes().isEmpty();
}

QString UiICGAS::unsavedWorkflowMessage() const
{
	const QStringList items = unsaved_workflow_changes();
	if (items.isEmpty()) return QString();

	QStringList rows;
	for (const QString& item : items) {
		rows << QStringLiteral("· %1").arg(item);
	}
	return QStringLiteral("以下内容尚未保存，请先保存后再启动流程：\n%1").arg(rows.join(QStringLiteral("\n")));
}

void UiICGAS::set_comm_config_dirty(bool dirty)
{
	if (comm_config_dirty == dirty) return;
	comm_config_dirty = dirty;
	sync_workflow_module_states();
}

void UiICGAS::set_initial_sim_ctrl_dirty(bool dirty)
{
	if (initial_sim_ctrl_dirty == dirty) return;
	initial_sim_ctrl_dirty = dirty;
	sync_workflow_module_states();
}

void UiICGAS::set_initial_platform_type_dirty(bool dirty)
{
	if (initial_platform_type_dirty == dirty) return;
	initial_platform_type_dirty = dirty;
	sync_workflow_module_states();
}

void UiICGAS::set_initial_weapon_type_dirty(bool dirty)
{
	if (initial_weapon_type_dirty == dirty) return;
	initial_weapon_type_dirty = dirty;
	sync_workflow_module_states();
}

void UiICGAS::set_initial_sensor_type_dirty(bool dirty)
{
	if (initial_sensor_type_dirty == dirty) return;
	initial_sensor_type_dirty = dirty;
	sync_workflow_module_states();
}

void UiICGAS::set_initial_scenario_dirty(bool dirty)
{
	if (initial_scenario_dirty == dirty) return;
	initial_scenario_dirty = dirty;
	sync_workflow_module_states();
}

void UiICGAS::set_initial_platform_detail_dirty(bool dirty)
{
	if (initial_platform_detail_dirty == dirty) return;
	initial_platform_detail_dirty = dirty;
	sync_workflow_module_states();
}

void UiICGAS::mark_initial_platform_type_dirty(const QString& type_id)
{
	const QString id = type_id.trimmed();
	if (!id.isEmpty()) dirty_initial_platform_type_names.insert(id);
	set_initial_platform_type_dirty(true);
}

void UiICGAS::clear_initial_platform_type_dirty(const QString& type_id)
{
	dirty_initial_platform_type_names.erase(type_id.trimmed());
	for (const QString& deleted_name : pending_initial_platform_type_deleted_contexts) {
		dirty_initial_platform_type_names.erase(deleted_name.trimmed());
	}
	pending_initial_platform_type_deleted_contexts.clear();
	set_initial_platform_type_dirty(!dirty_initial_platform_type_names.empty() ||
		!deleted_initial_platform_type_names.empty());
}

void UiICGAS::mark_initial_weapon_type_dirty(const QString& type_id)
{
	const QString id = type_id.trimmed();
	if (!id.isEmpty()) dirty_initial_weapon_type_names.insert(id);
	set_initial_weapon_type_dirty(true);
}

void UiICGAS::clear_initial_weapon_type_dirty(const QString& type_id)
{
	dirty_initial_weapon_type_names.erase(type_id.trimmed());
	for (const QString& deleted_name : pending_initial_weapon_type_deleted_contexts) {
		dirty_initial_weapon_type_names.erase(deleted_name.trimmed());
	}
	pending_initial_weapon_type_deleted_contexts.clear();
	set_initial_weapon_type_dirty(!dirty_initial_weapon_type_names.empty() ||
		!deleted_initial_weapon_type_names.empty());
}

void UiICGAS::mark_initial_sensor_type_dirty(const QString& type_id)
{
	const QString id = type_id.trimmed();
	if (!id.isEmpty()) dirty_initial_sensor_type_names.insert(id);
	set_initial_sensor_type_dirty(true);
}

void UiICGAS::clear_initial_sensor_type_dirty(const QString& type_id)
{
	dirty_initial_sensor_type_names.erase(type_id.trimmed());
	for (const QString& deleted_name : pending_initial_sensor_type_deleted_contexts) {
		dirty_initial_sensor_type_names.erase(deleted_name.trimmed());
	}
	pending_initial_sensor_type_deleted_contexts.clear();
	set_initial_sensor_type_dirty(!dirty_initial_sensor_type_names.empty() ||
		!deleted_initial_sensor_type_names.empty());
}

void UiICGAS::mark_initial_scenario_dirty(const QString& scenario_id)
{
	const QString id = scenario_id.trimmed();
	if (!id.isEmpty()) dirty_initial_scenario_names.insert(id);
	set_initial_scenario_dirty(true);
}

void UiICGAS::clear_initial_scenario_dirty(const QString& scenario_id)
{
	dirty_initial_scenario_names.erase(scenario_id.trimmed());
	set_initial_scenario_dirty(!dirty_initial_scenario_names.empty());
}

void UiICGAS::mark_initial_platform_detail_dirty(const QString& scenario_id)
{
	const QString id = scenario_id.trimmed();
	if (!id.isEmpty()) dirty_initial_platform_detail_scenario_names.insert(id);
	set_initial_platform_detail_dirty(true);
}

void UiICGAS::clear_initial_platform_detail_dirty(const QString& scenario_id)
{
	dirty_initial_platform_detail_scenario_names.erase(scenario_id.trimmed());
	set_initial_platform_detail_dirty(!dirty_initial_platform_detail_scenario_names.empty());
}

QString UiICGAS::defaultMissionId() const
{
	return QStringLiteral("MIS_MOVER-WORKFLOW-001");
}

QStringList UiICGAS::initialMoverTypeOptions() const
{
	return { QStringLiteral("AIR_MOVER"), QStringLiteral("SEA_MOVER"), QStringLiteral("MIS_MOVER") };
}

QStringList UiICGAS::initialDomainOptions() const
{
	return { QStringLiteral("AIR"), QStringLiteral("SEA"), QStringLiteral("MIS") };
}

QStringList UiICGAS::initialPlatformClassOptions() const
{
	return {
		QStringLiteral("CAR"), QStringLiteral("DST"), QStringLiteral("FRI"), QStringLiteral("WAN"),
		QStringLiteral("ELC"), QStringLiteral("FLT"), QStringLiteral("BOM"), QStringLiteral("MIS")
	};
}

QStringList UiICGAS::initialWeaponClassOptions() const
{
	QStringList options;
	for (const E_WEAPON_TYPE type : {
		E_WEAPON_TYPE::AAM, E_WEAPON_TYPE::SAM, E_WEAPON_TYPE::ASM }) {
		options << UtilEnum::trans_weapon_type_en(type);
	}
	return options;
}

QStringList UiICGAS::initialSensorClassOptions() const
{
	QStringList options;
	for (const E_SENSOR_TYPE type : {
		E_SENSOR_TYPE::RADAR, E_SENSOR_TYPE::OPTICAL, E_SENSOR_TYPE::INFRARED }) {
		options << UtilEnum::trans_sensor_type_en(type);
	}
	return options;
}

QString UiICGAS::missionId() const
{
	if (module_mission == nullptr) return defaultMissionId();
	return module_mission->mission_snapshot().mission_id;
}

QString UiICGAS::missionSourceText() const
{
	return module_mission == nullptr ? QString() : module_mission->mission_snapshot().source_text;
}

QStringList UiICGAS::replayLogNames() const
{
	QStringList name_list;

	QDir root_dir(replay_log_root_path());
	if (!root_dir.exists()) return name_list;

	const QFileInfoList replay_dir_list = root_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
	for (const QFileInfo& replay_info : replay_dir_list) {
		name_list << replay_info.fileName();
	}

	return name_list;
}

void UiICGAS::setSelectedScenario(const QString& scenario)
{
	const QString value = scenario.trimmed();
	if (selected_scenario == value) return;

	selected_scenario = value;
	platform_state_cache.clear();
	primary_plat_id.clear();
	secondary_plat_id.clear();
	primary_rows.clear();
	secondary_rows.clear();
	relative_rows.clear();
	initial_selected_scenario_platform_id.clear();
	rebuild_initial_situation_rows();
	refreshSituation();
	emit selectedScenarioChanged();
	emit detailChanged();
}

void UiICGAS::setAfsClientAddr(const QString& value)
{
	if (afs_client_addr == value) return;
	afs_client_addr = value.trimmed();
	set_comm_config_dirty(true);
	emit commConfigChanged();
}

void UiICGAS::setAfsClientPort(const QString& value)
{
	if (afs_client_port == value) return;
	afs_client_port = value.trimmed();
	set_comm_config_dirty(true);
	emit commConfigChanged();
}

void UiICGAS::setAfsServerAddr(const QString& value)
{
	if (afs_server_addr == value) return;
	afs_server_addr = value.trimmed();
	set_comm_config_dirty(true);
	emit commConfigChanged();
}

void UiICGAS::setAfsServerPort(const QString& value)
{
	if (afs_server_port == value) return;
	afs_server_port = value.trimmed();
	set_comm_config_dirty(true);
	emit commConfigChanged();
}

void UiICGAS::setTacServerAddr(const QString& value)
{
	if (tac_server_addr == value) return;
	tac_server_addr = value.trimmed();
	set_comm_config_dirty(true);
	emit commConfigChanged();
}

void UiICGAS::setTacServerPort(const QString& value)
{
	if (tac_server_port == value) return;
	tac_server_port = value.trimmed();
	set_comm_config_dirty(true);
	emit commConfigChanged();
}

void UiICGAS::setTacServerUser(const QString& value)
{
	if (tac_server_user == value) return;
	tac_server_user = value.trimmed();
	set_comm_config_dirty(true);
	emit commConfigChanged();
}

void UiICGAS::setTacSendIterMs(const QString& value)
{
	if (tac_send_iter_ms == value) return;
	tac_send_iter_ms = value.trimmed();
	set_comm_config_dirty(true);
	emit commConfigChanged();
}

void UiICGAS::setMissionId(const QString& value)
{
	if (module_mission == nullptr) return;

	S_MISSION mission = module_mission->mission_snapshot();
	const QString next_id = value.trimmed();
	if (mission.mission_id == next_id) return;

	mission.mission_id = next_id;
	module_mission->update_mission(mission);
	setMissionConfigDraftDirty(true);
	emit missionChanged();
	sync_workflow_module_states();
}

void UiICGAS::setMissionSourceText(const QString& value)
{
	if (module_mission == nullptr) return;
	if (missionSourceText() == value) return;
	module_mission->update_mission_source_text(value);
	setMissionConfigDraftDirty(true);
	emit missionChanged();
	sync_workflow_module_states();
}

void UiICGAS::setSelectedConceptScenario(const QString& scenario)
{
	const QString value = scenario.trimmed();
	if (selected_concept_scenario == value) return;

	if (concept_dirty) {
		dirty_concept_scenarios.insert(selected_concept_scenario);
		concept_dirty = false;
	}
	selected_concept_scenario = value;
	concept_detail_mode = "empty";
	concept_detail_type = E_NODE_TYPE::UKN;
	concept_detail_node_id.clear();
	concept_detail_dirty = false;
	clear_concept_undo_state();
	clear_concept_focus_state();
    concept_overview_apply_status = QStringLiteral("无修改");
    concept_node_apply_status = QStringLiteral("无修改");
	concept_dirty = dirty_concept_scenarios.count(selected_concept_scenario) > 0;
	concept_config_confirmed = false;
	concept_status = conceptDirty() ? QStringLiteral("存在未确认修改") : QStringLiteral("读取成功");
	init_concept_state();
	sync_workflow_module_states();
}

void UiICGAS::toggleTacServer()
{
	if (tac_server == nullptr) return;

	if (!tac_server_running) {
		start_tac_server_comm();
	}
	else {
		stop_tac_server_comm();
	}

	emit commStateChanged();
	emit commConfigChanged();
}

void UiICGAS::toggleAfsServer()
{
	if (afs_server == nullptr) return;
	if (!workflow_afsim_mode()) return;

	if (!afs_server_running) {
		start_afs_server_comm();
	}
	else {
		stop_afs_server_comm();
	}

	emit commStateChanged();
	emit commConfigChanged();
}

void UiICGAS::runWorkflow()
{
	sync_workflow_module_states();
	if (hasUnsavedWorkflowChanges()) {
		QMessageBox::information(nullptr, QStringLiteral("存在未保存内容"), unsavedWorkflowMessage());
		if (append_log(QString("[%1] [WARN] [Workflow] Unsaved config blocks start")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
			emit logRowsChanged();
		}
		emit workflowModuleStateChanged();
		return;
	}
	if (!workflowRunnable()) {
		QString module_name = first_not_ready_workflow_module_name();
		if (module_name.trimmed().isEmpty()) {
			module_name = QStringLiteral("流程");
		}
		QMessageBox::information(nullptr, QStringLiteral("模块未就绪"),
			QStringLiteral("%1模块未就绪，请等待").arg(module_name));
		if (append_log(QString("[%1] [WARN] [Workflow] Module not ready")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
			emit logRowsChanged();
		}
		emit workflowModuleStateChanged();
		return;
	}

	bool started = true;
	if (workflow_afsim_mode()) {
		started = start_afs_client_comm() && started;
	}
	else if (workflow_icgas_mode()) {
		started = start_internal_sim_comm() && started;
	}
	started = start_tac_server_comm() && started;

	if (started) {
		set_workflow_run_state("running");
		if (append_log(QString("[%1] [STATE] [Workflow] Running")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
			emit logRowsChanged();
		}
	}
	else {
		afs_client_has_error = true;
		afs_client_error_text = QStringLiteral("流程启动不完整");
		stop_workflow_comm(false);
		set_workflow_run_state("stopped");
		afs_client_has_error = true;
		afs_client_error_text = QStringLiteral("流程启动不完整");
		if (append_log(QString("[%1] [WARN] [Workflow] Start incomplete")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
			emit logRowsChanged();
		}
	}

	emit commStateChanged();
	emit commConfigChanged();
	sync_workflow_module_states();
}

void UiICGAS::pauseWorkflow()
{
	stop_workflow_comm(false);
	set_workflow_run_state("paused");
	if (append_log(QString("[%1] [STATE] [Workflow] Paused, database memory retained")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
		emit logRowsChanged();
	}

	emit commStateChanged();
	emit commConfigChanged();
	sync_workflow_module_states();
}

void UiICGAS::stopWorkflow()
{
	stop_workflow_comm(true);
	set_workflow_run_state("stopped");
	if (append_log(QString("[%1] [STATE] [Workflow] Stopped, database data cleared")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
		emit logRowsChanged();
	}
	refreshSituation();

	emit commStateChanged();
	emit commConfigChanged();
	sync_workflow_module_states();
}

bool UiICGAS::saveWorkflowConfig()
{
	bool ok = false;
	InfoExpert* expert = InfoExpert::GetInstance();
	if (expert != nullptr) {
		const QString next_afs_client_addr = afs_client_addr;
		const QString next_afs_client_port = afs_client_port;
		const QString next_afs_server_addr = afs_server_addr;
		const QString next_afs_server_port = afs_server_port;
		const QString next_tac_server_addr = tac_server_addr;
		const QString next_tac_server_port = tac_server_port;
		const QString next_tac_server_user = tac_server_user;
		const QString next_tac_send_iter_ms = tac_send_iter_ms;
		ok = expert->save_comm_config(
			next_afs_client_addr, next_afs_client_port,
			next_afs_server_addr, next_afs_server_port,
			next_tac_server_addr, next_tac_server_port,
			next_tac_server_user, next_tac_send_iter_ms);
	}
	if (append_log(QString("[%1] [%2] [Workflow] Communication config %3")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
		.arg(ok ? "STATE" : "ERROR")
		.arg(ok ? "saved" : "save failed"))) {
		emit logRowsChanged();
	}
	if (ok) {
		set_comm_config_dirty(false);
	}
	emit commConfigChanged();
	return ok;
}

bool UiICGAS::saveAfsClientConfig(const QString& addr, const QString& port)
{
	setAfsClientAddr(addr);
	setAfsClientPort(port);
	const bool ok = saveWorkflowConfig();
	if (ok) {
		setAfsClientConfigDraftDirty(false);
	}
	return ok;
}

bool UiICGAS::saveDatabaseConfig(const QString& plt_timeout_ms, const QString& algo_step_ms)
{
	if (module_db == nullptr) return false;

	bool timeout_ok = false;
	bool algo_step_ok = false;
	const int next_timeout_ms = plt_timeout_ms.trimmed().toInt(&timeout_ok);
	const int next_algo_step_ms = algo_step_ms.trimmed().toInt(&algo_step_ok);
	if (!timeout_ok || !algo_step_ok ||
		next_timeout_ms <= 0 || next_algo_step_ms <= 0) {
		if (append_log(QString("[%1] [ERROR] [Database] Config save failed: invalid parameter")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
			emit logRowsChanged();
		}
		return false;
	}

	bool ok = false;
	ModuleDatabase* database = module_db;
	QMetaObject::invokeMethod(database, [database, next_timeout_ms, next_algo_step_ms, &ok]() {
		if (database == nullptr) return;
		ok = database->save_config(next_timeout_ms, next_algo_step_ms);
	}, Qt::BlockingQueuedConnection);

	if (ok) {
		database_plat_timeout_ms = QString::number(next_timeout_ms);
		database_algo_step_ms = QString::number(next_algo_step_ms);
		setDatabaseConfigDraftDirty(false);
	}

	if (append_log(QString("[%1] [%2] [Database] Config %3")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
		.arg(ok ? "STATE" : "ERROR")
		.arg(ok ? "saved" : "save failed"))) {
		emit logRowsChanged();
	}
	emit databaseConfigChanged();
	sync_workflow_module_states();
	return ok;
}

void UiICGAS::setDatabaseConfigDraftDirty(bool dirty)
{
	if (database_config_dirty == dirty) return;
	database_config_dirty = dirty;
	emit databaseConfigDirtyChanged();
	sync_workflow_module_states();
}

void UiICGAS::resetAfsClientConfigDraft()
{
	setAfsClientConfigDraftDirty(false);
	emit commConfigChanged();
}

void UiICGAS::setAfsClientConfigDraftDirty(bool dirty)
{
	if (afs_client_config_dirty == dirty) return;
	afs_client_config_dirty = dirty;
	emit afsClientConfigDirtyChanged();
	sync_workflow_module_states();
}

bool UiICGAS::saveMissionConfig(const QString& mission_id, const QString& source_text)
{
	if (module_mission == nullptr) return false;

	module_mission->update_mission_source_text(source_text);
	S_MISSION mission = module_mission->mission_snapshot();
	const QString next_id = mission_id.trimmed();
	if (!next_id.isEmpty() && mission.mission_id != next_id) {
		mission.mission_id = next_id;
		module_mission->update_mission(mission);
	}

	setMissionConfigDraftDirty(false);
	if (append_log(QString("[%1] [STATE] [Mission] Config saved")
		.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
		emit logRowsChanged();
	}
	emit missionChanged();
	sync_workflow_module_states();
	return true;
}

void UiICGAS::setMissionConfigDraftDirty(bool dirty)
{
	if (mission_config_dirty == dirty) return;
	mission_config_dirty = dirty;
	emit missionConfigDirtyChanged();
	sync_workflow_module_states();
}

QString UiICGAS::normalized_workflow_input_mode(const QString& input_mode) const
{
	const QString mode = input_mode.trimmed().toLower();
	if (mode == QStringLiteral("afsim") || mode == QStringLiteral("realtime") ||
		mode == QStringLiteral("real_time") || mode == QStringLiteral("实时模式")) {
		return mode == QStringLiteral("afsim") ? QStringLiteral("afsim") : QStringLiteral("icgas");
	}
	if (mode == QStringLiteral("replay") || mode == QStringLiteral("回放") ||
		mode == QStringLiteral("回放模式")) {
		return QStringLiteral("replay");
	}
	return QStringLiteral("icgas");
}

bool UiICGAS::workflow_afsim_mode() const
{
	return workflow_input_mode == QStringLiteral("afsim");
}

bool UiICGAS::workflow_icgas_mode() const
{
	return workflow_input_mode == QStringLiteral("icgas");
}

bool UiICGAS::workflow_replay_mode() const
{
	return workflow_input_mode == QStringLiteral("replay");
}

void UiICGAS::setWorkflowInputMode(const QString& input_mode)
{
	const QString next_mode = normalized_workflow_input_mode(input_mode);
	if (workflow_input_mode == next_mode) {
		sync_workflow_module_states();
		return;
	}

	if (workflow_afsim_mode() && next_mode != QStringLiteral("afsim")) {
		stop_afs_client_comm(false);
	}
	if (workflow_icgas_mode() && next_mode != QStringLiteral("icgas")) {
		stop_internal_sim_comm(false);
	}
	if (next_mode != QStringLiteral("afsim")) {
		stop_afs_server_comm();
	}

	workflow_input_mode = next_mode;
	workflow_realtime_mode = next_mode != QStringLiteral("replay");
	emit workflowInputModeChanged();
	if (workflow_icgas_mode() &&
		exact_sim_config_list(sim_plat_config_cache, selected_scenario) == nullptr) {
		const QString scenario_id = first_loaded_sim_scenario_id(sim_scenario_name_list, sim_plat_config_cache);
		if (!scenario_id.isEmpty()) {
			setSelectedScenario(scenario_id);
		}
	}
	emit commStateChanged();
	sync_workflow_module_states();
}

void UiICGAS::setWorkflowRealtimeMode(bool realtime_mode)
{
	setWorkflowInputMode(realtime_mode ? QStringLiteral("icgas") : QStringLiteral("replay"));
}

QString UiICGAS::workflowModuleStateKind(const QString& module_id) const
{
	const auto iter = workflow_module_states.find(module_id);
	if (iter == workflow_module_states.end()) return QStringLiteral("unknown");
	return workflow_module_state_key(iter->second.effective_state);
}

void UiICGAS::selectPrimaryPlat(const QString& plat_id)
{
	const QString new_id = plat_id.trimmed();
	if (primary_plat_id == new_id) return;
	primary_plat_id = new_id;
	refresh_detail();
	emit detailChanged();
}

void UiICGAS::selectSecondaryPlat(const QString& plat_id)
{
	const QString new_id = plat_id.trimmed();
	if (secondary_plat_id == new_id) return;
	secondary_plat_id = new_id;
	refresh_detail();
	emit detailChanged();
}

void UiICGAS::selectInitialScenarioPlatform(const QString& platform_id)
{
	const QString id = platform_id.trimmed();
	if (initial_selected_scenario_platform_id == id) return;
	initial_selected_scenario_platform_id = id;
	rebuild_initial_selected_platform();
	emit_initial_changed(false, false, true, false);
}

QString UiICGAS::addInitialPlatformType()
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	rebuild_initial_platform_icon_options();

	QString name;
	for (int index = 1; index < 1000; ++index) {
		const QString candidate = QStringLiteral("PLATFORM_TYPE_%1").arg(index, 3, 10, QChar('0'));
		if (find_initial_platform_define(candidate) == nullptr) {
			name = candidate;
			break;
		}
	}
	if (name.isEmpty()) return QString();

	S_PLAT_DEFINE define;
	define.type_name = name;
	define.icon_path = initial_platform_icon_options.contains(QStringLiteral("FixedWing.F-18E.obj"))
		? QStringLiteral("FixedWing.F-18E.obj") :
		(initial_platform_icon_options.contains(QStringLiteral("FixedWing.F-15.obj"))
			? QStringLiteral("FixedWing.F-15.obj") :
			(initial_platform_icon_options.isEmpty() ? QString() : initial_platform_icon_options.first()));
	define.domain = E_DOMAIN::AIR;
	define.type = E_PLAT_TYPE::FLT;
	define.mover = E_MOVER::AIR_MOVER;
	define.cap = default_mover_cap(define.mover);
	sim_plat_define_cache[define.type_name] = define;

	mark_initial_platform_type_dirty(name);
	initial_situation_status = QStringLiteral("已新增平台类型 %1").arg(name);
	rebuild_initial_situation_rows(true, true, true, true);
	return name;
}

bool UiICGAS::deleteInitialPlatformType(const QString& type_id)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	const QString id = type_id.trimmed();
	if (id.isEmpty()) return false;

	const bool in_use = std::any_of(sim_plat_config_cache.begin(), sim_plat_config_cache.end(),
		[&](const auto& scenario) {
			return std::any_of(scenario.second.begin(), scenario.second.end(), [&](const S_PLAT_CONFIG& config) {
				const S_PLAT_DEFINE* define = find_sim_platform_define(config);
				return config.type_name.compare(id, Qt::CaseInsensitive) == 0 ||
					(define != nullptr && define->type_name.compare(id, Qt::CaseInsensitive) == 0);
			});
		});
	if (in_use) {
		initial_situation_status = QStringLiteral("删除失败：平台类型 %1 仍被初始平台使用").arg(id);
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const auto define_iter = sim_plat_define_cache.find(id);
	if (define_iter != sim_plat_define_cache.end()) {
		if (!deleted_initial_platform_type_names.contains(id)) {
			deleted_initial_platform_type_names << id;
		}
		sim_plat_define_cache.erase(define_iter);
		mark_initial_platform_type_dirty(id);
		initial_situation_status = QStringLiteral("已删除平台类型 %1，保存后生效").arg(id);
		rebuild_initial_situation_rows(true, true, true, true);
		return true;
	}
	return false;
}

bool UiICGAS::saveInitialPlatformType(const QString& type_id)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	if (sim_controller == nullptr) {
		initial_platform_type_save_status = QStringLiteral("保存失败：SimController 未就绪");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const QString id = type_id.trimmed();
	const S_PLAT_DEFINE* define = find_initial_platform_define(id);
	if (id.isEmpty() || define == nullptr) {
		initial_platform_type_save_status = QStringLiteral("保存失败：平台类型不存在");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const S_PLAT_DEFINE save_define = *define;
	const QStringList renamed_or_deleted_names = deleted_initial_platform_type_names;
	pending_initial_platform_type_deleted_contexts = renamed_or_deleted_names;
	deleted_initial_platform_type_names.clear();
	initial_platform_type_save_status = QStringLiteral("正在保存...");
	pending_initial_platform_type_save_context = id;
	emit_initial_changed(false, false, false, true);

	SimController* controller = sim_controller;
	bool ok = false;
	QMetaObject::invokeMethod(controller, [controller, id, save_define, renamed_or_deleted_names, &ok]() {
		if (controller == nullptr) return;
		ok = true;
		for (const QString& old_name : renamed_or_deleted_names) {
			const QString name = old_name.trimmed();
			if (name.isEmpty() || name == id) continue;
			controller->rmov_plat_define(SNAPSHOT_PLAT_DEFINE_PAIR(
				new std::pair<QString, S_PLAT_DEFINE>(name, S_PLAT_DEFINE())));
		}
		controller->save_plat_define(SNAPSHOT_PLAT_DEFINE_PAIR(
			new std::pair<QString, S_PLAT_DEFINE>(id, save_define)));
	}, Qt::BlockingQueuedConnection);
	if (!ok) {
		for (const QString& old_name : renamed_or_deleted_names) {
			if (!old_name.trimmed().isEmpty() && old_name.trimmed() != id) {
				deleted_initial_platform_type_names << old_name;
			}
		}
		initial_platform_type_save_status = QStringLiteral("保存失败：平台类型 %1 写入失败").arg(id);
		pending_initial_platform_type_deleted_contexts.clear();
		pending_initial_platform_type_save_context.clear();
		emit_initial_changed(false, false, false, true);
		return false;
	}
	emit_initial_changed(false, false, false, true);
	return true;
}

QString UiICGAS::addInitialWeaponType()
{
	if (!sim_config_loaded) refresh_sim_config_cache();

	QString name;
	for (int index = 1; index < 1000; ++index) {
		const QString candidate = QStringLiteral("WEAPON_TYPE_%1").arg(index, 3, 10, QChar('0'));
		if (find_initial_weapon_define(candidate) == nullptr) {
			name = candidate;
			break;
		}
	}
	if (name.isEmpty()) return QString();

	S_WEAPON_DEFINE define;
	define.type_name = name;
	define.domain = E_DOMAIN::MIS;
	define.type = E_WEAPON_TYPE::AAM;
	define.mover = E_MOVER::MIS_MOVER;
	define.mover_cap = S_MIS_MOVER_CAP{};
	if (auto* cap = std::get_if<S_MIS_MOVER_CAP>(&define.mover_cap)) {
		cap->min_speed_ms = 200.0;
		cap->max_speed_ms = 850.0;
		cap->min_alt_m = 0.0;
		cap->max_alt_m = 25000.0;
		cap->max_linear_load_g = 10.0;
		cap->max_radial_load_g = 25.0;
		cap->max_climb_vel_ms = 80.0;
		cap->max_drop_vel_ms = 120.0;
		cap->life_range_m = 120000.0;
		cap->life_time_s = 120.0;
	}
	define.launch_cap.max_range_m = 120000.0;
	define.launch_cap.firing_delay_s = 0.2;
	define.kill_cap.kill_range_m = 90.0;
	define.kill_cap.kill_prob = 0.66;
	sim_weapon_define_cache[define.type_name] = define;

	mark_initial_weapon_type_dirty(name);
	initial_situation_status = QStringLiteral("已新增武器类型 %1").arg(name);
	rebuild_initial_situation_rows(true, false, false, true);
	return name;
}

bool UiICGAS::deleteInitialWeaponType(const QString& type_id)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	const QString id = type_id.trimmed();
	if (id.isEmpty()) return false;

	const bool in_use = std::any_of(sim_plat_config_cache.begin(), sim_plat_config_cache.end(),
		[&](const auto& scenario) {
			return std::any_of(scenario.second.begin(), scenario.second.end(), [&](const S_PLAT_CONFIG& config) {
				for (const auto& load : config.weapon_load) {
					if (load.first.compare(id, Qt::CaseInsensitive) == 0) return true;
				}
				return false;
			});
		});
	if (in_use) {
		initial_situation_status = QStringLiteral("删除失败：武器类型 %1 仍被平台挂载").arg(id);
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const auto define_iter = sim_weapon_define_cache.find(id);
	if (define_iter != sim_weapon_define_cache.end()) {
		if (!deleted_initial_weapon_type_names.contains(id)) {
			deleted_initial_weapon_type_names << id;
		}
		sim_weapon_define_cache.erase(define_iter);
		mark_initial_weapon_type_dirty(id);
		initial_situation_status = QStringLiteral("已删除武器类型 %1，保存后生效").arg(id);
		rebuild_initial_situation_rows(true, true, true, true);
		return true;
	}
	return false;
}

bool UiICGAS::saveInitialWeaponType(const QString& type_id)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	if (sim_controller == nullptr) {
		initial_weapon_type_save_status = QStringLiteral("保存失败：SimController 未就绪");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const QString id = type_id.trimmed();
	const S_WEAPON_DEFINE* define = find_initial_weapon_define(id);
	if (id.isEmpty() || define == nullptr) {
		initial_weapon_type_save_status = QStringLiteral("保存失败：武器类型不存在");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const S_WEAPON_DEFINE save_define = *define;
	const QStringList renamed_or_deleted_names = deleted_initial_weapon_type_names;
	pending_initial_weapon_type_deleted_contexts = renamed_or_deleted_names;
	deleted_initial_weapon_type_names.clear();
	initial_weapon_type_save_status = QStringLiteral("正在保存...");
	pending_initial_weapon_type_save_context = id;
	emit_initial_changed(false, false, false, true);

	SimController* controller = sim_controller;
	bool ok = false;
	QMetaObject::invokeMethod(controller, [controller, id, save_define, renamed_or_deleted_names, &ok]() {
		if (controller == nullptr) return;
		ok = true;
		for (const QString& old_name : renamed_or_deleted_names) {
			const QString name = old_name.trimmed();
			if (name.isEmpty() || name == id) continue;
			controller->rmov_weapon_define(SNAPSHOT_WEAPON_DEFINE_PAIR(
				new std::pair<QString, S_WEAPON_DEFINE>(name, S_WEAPON_DEFINE())));
		}
		controller->save_weapon_define(SNAPSHOT_WEAPON_DEFINE_PAIR(
			new std::pair<QString, S_WEAPON_DEFINE>(id, save_define)));
	}, Qt::BlockingQueuedConnection);
	if (!ok) {
		for (const QString& old_name : renamed_or_deleted_names) {
			if (!old_name.trimmed().isEmpty() && old_name.trimmed() != id) {
				deleted_initial_weapon_type_names << old_name;
			}
		}
		initial_weapon_type_save_status = QStringLiteral("保存失败：武器类型 %1 写入失败").arg(id);
		pending_initial_weapon_type_deleted_contexts.clear();
		pending_initial_weapon_type_save_context.clear();
		emit_initial_changed(false, false, false, true);
		return false;
	}
	emit_initial_changed(false, false, false, true);
	return true;
}

QString UiICGAS::addInitialSensorType()
{
	if (!sim_config_loaded) refresh_sim_config_cache();

	QString name;
	for (int index = 1; index < 1000; ++index) {
		const QString candidate = QStringLiteral("SENSOR_TYPE_%1").arg(index, 3, 10, QChar('0'));
		if (find_initial_sensor_define(candidate) == nullptr) {
			name = candidate;
			break;
		}
	}
	if (name.isEmpty()) return QString();

	S_SENSOR_DEFINE define;
	define.type_name = name;
	define.type = E_SENSOR_TYPE::RADAR;
	define.reliability = 0.9;
	define.act_domain_list = { E_DOMAIN::AIR, E_DOMAIN::SEA, E_DOMAIN::MIS };
	define.task_mode_list[E_SENSOR_TASK::DETECT] =
		default_sensor_task_mode(E_SENSOR_TASK::DETECT, define.type);
	define.task_mode_list[E_SENSOR_TASK::TRACK] =
		default_sensor_task_mode(E_SENSOR_TASK::TRACK, define.type);
	sim_sensor_define_cache[define.type_name] = define;

	mark_initial_sensor_type_dirty(name);
	initial_situation_status = QStringLiteral("已新增传感器类型 %1").arg(name);
	rebuild_initial_situation_rows(true, false, false, true);
	return name;
}

bool UiICGAS::deleteInitialSensorType(const QString& type_id)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	const QString id = type_id.trimmed();
	if (id.isEmpty()) return false;

	const bool in_use = std::any_of(sim_plat_config_cache.begin(), sim_plat_config_cache.end(),
		[&](const auto& scenario) {
			return std::any_of(scenario.second.begin(), scenario.second.end(), [&](const S_PLAT_CONFIG& config) {
				for (const auto& load : config.sensor_load) {
					if (load.first.compare(id, Qt::CaseInsensitive) == 0) return true;
				}
				return false;
			});
		});
	if (in_use) {
		initial_situation_status = QStringLiteral("删除失败：传感器类型 %1 仍被平台挂载").arg(id);
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const auto define_iter = sim_sensor_define_cache.find(id);
	if (define_iter != sim_sensor_define_cache.end()) {
		if (!deleted_initial_sensor_type_names.contains(id)) {
			deleted_initial_sensor_type_names << id;
		}
		sim_sensor_define_cache.erase(define_iter);
		mark_initial_sensor_type_dirty(id);
		initial_situation_status = QStringLiteral("已删除传感器类型 %1，保存后生效").arg(id);
		rebuild_initial_situation_rows(true, true, true, true);
		return true;
	}
	return false;
}

bool UiICGAS::saveInitialSensorType(const QString& type_id)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	if (sim_controller == nullptr) {
		initial_sensor_type_save_status = QStringLiteral("保存失败：SimController 未就绪");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const QString id = type_id.trimmed();
	const S_SENSOR_DEFINE* define = find_initial_sensor_define(id);
	if (id.isEmpty() || define == nullptr) {
		initial_sensor_type_save_status = QStringLiteral("保存失败：传感器类型不存在");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const S_SENSOR_DEFINE save_define = *define;
	const QStringList renamed_or_deleted_names = deleted_initial_sensor_type_names;
	pending_initial_sensor_type_deleted_contexts = renamed_or_deleted_names;
	deleted_initial_sensor_type_names.clear();
	initial_sensor_type_save_status = QStringLiteral("正在保存...");
	pending_initial_sensor_type_save_context = id;
	emit_initial_changed(false, false, false, true);

	SimController* controller = sim_controller;
	bool ok = false;
	QMetaObject::invokeMethod(controller, [controller, id, save_define, renamed_or_deleted_names, &ok]() {
		if (controller == nullptr) return;
		ok = true;
		for (const QString& old_name : renamed_or_deleted_names) {
			const QString name = old_name.trimmed();
			if (name.isEmpty() || name == id) continue;
			controller->rmov_sensor_define(SNAPSHOT_SENSOR_DEFINE_PAIR(
				new std::pair<QString, S_SENSOR_DEFINE>(name, S_SENSOR_DEFINE())));
		}
		controller->save_sensor_define(SNAPSHOT_SENSOR_DEFINE_PAIR(
			new std::pair<QString, S_SENSOR_DEFINE>(id, save_define)));
	}, Qt::BlockingQueuedConnection);
	if (!ok) {
		for (const QString& old_name : renamed_or_deleted_names) {
			if (!old_name.trimmed().isEmpty() && old_name.trimmed() != id) {
				deleted_initial_sensor_type_names << old_name;
			}
		}
		initial_sensor_type_save_status = QStringLiteral("保存失败：传感器类型 %1 写入失败").arg(id);
		pending_initial_sensor_type_deleted_contexts.clear();
		pending_initial_sensor_type_save_context.clear();
		emit_initial_changed(false, false, false, true);
		return false;
	}
	emit_initial_changed(false, false, false, true);
	return true;
}

QString UiICGAS::addInitialScenario()
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	const QString scenario_id = make_initial_scenario_id();
	return createInitialScenario(scenario_id, false);
}

QString UiICGAS::createInitialScenario(const QString& scenario_id, bool copy_current)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	const QString id = sanitized_initial_scenario_id(scenario_id);
	if (id.isEmpty()) {
		initial_scenario_save_status = QStringLiteral("新增失败：场景名称不能为空");
		emit_initial_changed(false, false, false, true);
		return QString();
	}
	if (sim_plat_config_cache.find(id) != sim_plat_config_cache.end()) {
		initial_scenario_save_status = QStringLiteral("新增失败：场景 %1 已存在").arg(id);
		emit_initial_changed(false, false, false, true);
		return QString();
	}

	std::vector<S_PLAT_CONFIG> next_config;
	if (copy_current) {
		const std::vector<S_PLAT_CONFIG>* current_config =
			current_sim_config_list(sim_plat_config_cache, selected_scenario);
		if (current_config != nullptr) next_config = *current_config;
	}

	sim_plat_config_cache[id] = std::move(next_config);
	selected_scenario = id;
	initial_selected_scenario_platform_id.clear();
	mark_initial_scenario_dirty(id);
	initial_scenario_save_status = copy_current ?
		QStringLiteral("已复制当前场景，保存后生效") :
		QStringLiteral("已新增空场景，保存后生效");
	rebuild_sim_scenario_names();
	rebuild_initial_situation_rows(false, true, true, true);
	emit selectedScenarioChanged();
	return id;
}

bool UiICGAS::deleteInitialScenario(const QString& scenario_id)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	if (sim_controller == nullptr) {
		initial_scenario_save_status = QStringLiteral("删除失败：SimController 未就绪");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const QString id = scenario_id.trimmed().isEmpty() ? selected_scenario.trimmed() : scenario_id.trimmed();
	if (id.isEmpty() || sim_plat_config_cache.find(id) == sim_plat_config_cache.end()) {
		initial_scenario_save_status = QStringLiteral("删除失败：场景不存在");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	SimController* controller = sim_controller;
	bool submitted = false;
	pending_initial_config_delete_context = QStringLiteral("scenario");
	pending_initial_config_delete_scenario_id = id;
	initial_scenario_save_status = QStringLiteral("正在删除...");
	emit_initial_changed(false, false, false, true);
	QMetaObject::invokeMethod(controller, [controller, id, &submitted]() {
		if (controller == nullptr) return;
		submitted = true;
		controller->rmov_plat_config(SNAPSHOT_PLAT_CONFIG_PAIR(
			new std::pair<QString, std::vector<S_PLAT_CONFIG>>(id, std::vector<S_PLAT_CONFIG>())));
	}, Qt::BlockingQueuedConnection);
	if (!submitted) {
		pending_initial_config_delete_context.clear();
		pending_initial_config_delete_scenario_id.clear();
		initial_scenario_save_status = QStringLiteral("删除失败：请求未发出");
		emit_initial_changed(false, false, false, true);
	}
	return submitted;
}

bool UiICGAS::resetInitialScenarioConfig()
{
	if (selected_scenario.trimmed().isEmpty()) {
		initial_scenario_save_status = QStringLiteral("重置失败：未选择场景");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const QString keep_scenario = selected_scenario.trimmed();
	refresh_sim_config_cache(true);
	if (sim_plat_config_cache.find(keep_scenario) == sim_plat_config_cache.end()) {
		initial_scenario_save_status = QStringLiteral("重置失败：场景不存在");
		rebuild_initial_situation_rows(false, true, true, true);
		emit_initial_changed(false, false, false, true);
		return false;
	}

	selected_scenario = keep_scenario;
	initial_selected_scenario_platform_id.clear();
	dirty_initial_scenario_names.clear();
	dirty_initial_platform_detail_scenario_names.clear();
	set_initial_scenario_dirty(false);
	set_initial_platform_detail_dirty(false);
	initial_scenario_save_status = QStringLiteral("已重置为磁盘配置");
	rebuild_sim_scenario_names();
	rebuild_initial_situation_rows(false, true, true, true);
	emit selectedScenarioChanged();
	return true;
}

bool UiICGAS::addInitialScenarioPlatform(const QString& type_id, double lon_deg, double lat_deg)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	if (!sim_config_loaded) return false;
	const S_PLAT_DEFINE* define = find_initial_platform_define(type_id);
	if (define == nullptr) {
		initial_situation_status = QStringLiteral("新增平台失败：平台类型不存在");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const QString platform_id = make_initial_platform_id(type_id.trimmed());
	S_ROUTE_POINT point;
	point.point_id = platform_id + QStringLiteral("_route_wp_001");
	point.pos_lla = S_POS_LLA(lon_deg DEG2RAD, lat_deg DEG2RAD, 0.0);

	S_PLAT_CONFIG config;
	config.plt_id = platform_id;
	config.type_name = platform_config_name_for_type(type_id, platform_id);
	config.cmd_id = QStringLiteral("SELF");
	config.side = E_SIDE::UKN;
	config.init_state.pos_lla = point.pos_lla;
	config.init_state.vel_loc = S_VEL_LOC(point.speed_ms, 0.0, 0.0);
	config.init_route.route_id = platform_id + QStringLiteral("_route");
	config.init_route.point_list.push_back(point);
	renumber_route_points(config.init_route);

	std::vector<S_PLAT_CONFIG>* config_list = current_sim_config_list(sim_plat_config_cache, selected_scenario);
	if (config_list == nullptr) {
		initial_situation_status = QStringLiteral("新增平台失败：未找到初始态势场景");
		emit_initial_changed(false, false, false, true);
		return false;
	}
	config_list->push_back(config);
	initial_selected_scenario_platform_id = platform_id;
	mark_initial_scenario_dirty(selected_scenario);
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已创建平台 %1").arg(platform_id);
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::mountInitialEquipment(const QString& platform_id, const QString& equipment_kind, const QString& type_id)
{
	if (!sim_config_loaded) refresh_sim_config_cache();
	if (!sim_config_loaded) return false;
	const QString kind = equipment_kind.trimmed();
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) {
		initial_situation_status = QStringLiteral("挂载失败：平台不存在");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	if (kind == QStringLiteral("sensor")) {
		const S_SENSOR_DEFINE* define = find_initial_sensor_define(type_id);
		const QString sensor_type_name = define == nullptr ? type_id.trimmed() : define->type_name.trimmed();
		if (sensor_type_name.isEmpty() || define == nullptr || define->type == E_SENSOR_TYPE::UKN) {
			initial_situation_status = QStringLiteral("挂载失败：传感器类型不支持");
			emit_initial_changed(false, false, false, true);
			return false;
		}
		config->sensor_load[sensor_type_name] = std::max(1, config->sensor_load[sensor_type_name] + 1);
	}
	else if (kind == QStringLiteral("weapon")) {
		const S_WEAPON_DEFINE* define = find_initial_weapon_define(type_id);
		const QString weapon_type_name = define == nullptr ? type_id.trimmed() : define->type_name.trimmed();
		if (weapon_type_name.isEmpty() || define == nullptr || define->type == E_WEAPON_TYPE::UKN) {
			initial_situation_status = QStringLiteral("挂载失败：武器类型不支持");
			emit_initial_changed(false, false, false, true);
			return false;
		}
		config->weapon_load[weapon_type_name] = std::max(1, config->weapon_load[weapon_type_name] + 1);
	}
	else {
		initial_situation_status = QStringLiteral("挂载失败：装备类型不支持");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已为 %1 挂载 %2").arg(platform_id.trimmed(), type_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::addInitialWeaponMount(const QString& platform_id)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;

	QString weapon_type_name;
	for (const auto& item : sim_weapon_define_cache) {
		const QString candidate = item.second.type_name.trimmed();
		if (!candidate.isEmpty() && item.second.type != E_WEAPON_TYPE::UKN &&
			config->weapon_load.find(candidate) == config->weapon_load.end()) {
			weapon_type_name = candidate;
			break;
		}
	}
	if (weapon_type_name.isEmpty()) {
		initial_situation_status = QStringLiteral("新增武器挂载失败：无可用武器类型");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	config->weapon_load[weapon_type_name] = 1;
	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已新增 %1 的武器挂载").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::deleteInitialWeaponMount(const QString& platform_id, const QString& type_id)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;
	const auto erased = config->weapon_load.erase(type_id.trimmed());
	if (erased <= 0) return false;

	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已删除 %1 的武器挂载").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::updateInitialWeaponMount(const QString& platform_id,
	const QString& old_type_id, const QString& new_type_id, int quantity)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;
	const QString old_type = old_type_id.trimmed();
	const S_WEAPON_DEFINE* define = find_initial_weapon_define(new_type_id);
	const QString new_type = define == nullptr ? new_type_id.trimmed() : define->type_name.trimmed();
	if (new_type.isEmpty() || define == nullptr || define->type == E_WEAPON_TYPE::UKN || quantity < 0) return false;

	if (!old_type.isEmpty() && old_type != new_type) {
		config->weapon_load.erase(old_type);
	}
	config->weapon_load[new_type] = quantity;

	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已更新 %1 的武器挂载").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::addInitialSensorMount(const QString& platform_id)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;

	QString sensor_type_name;
	for (const auto& item : sim_sensor_define_cache) {
		const QString candidate = item.second.type_name.trimmed();
		if (!candidate.isEmpty() && item.second.type != E_SENSOR_TYPE::UKN &&
			config->sensor_load.find(candidate) == config->sensor_load.end()) {
			sensor_type_name = candidate;
			break;
		}
	}
	if (sensor_type_name.isEmpty()) {
		initial_situation_status = QStringLiteral("新增传感器挂载失败：无可用传感器类型");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	config->sensor_load[sensor_type_name] = 1;
	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已新增 %1 的传感器挂载").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::deleteInitialSensorMount(const QString& platform_id, const QString& type_id)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;
	const auto erased = config->sensor_load.erase(type_id.trimmed());
	if (erased <= 0) return false;

	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已删除 %1 的传感器挂载").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::updateInitialSensorMount(const QString& platform_id,
	const QString& old_type_id, const QString& new_type_id, int quantity)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;
	const QString old_type = old_type_id.trimmed();
	const S_SENSOR_DEFINE* define = find_initial_sensor_define(new_type_id);
	const QString new_type = define == nullptr ? new_type_id.trimmed() : define->type_name.trimmed();
	if (new_type.isEmpty() || define == nullptr || define->type == E_SENSOR_TYPE::UKN || quantity < 0) return false;

	if (!old_type.isEmpty() && old_type != new_type) {
		config->sensor_load.erase(old_type);
	}
	config->sensor_load[new_type] = quantity;

	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已更新 %1 的传感器挂载").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::updateInitialCatalogField(const QString& catalog_kind, const QString& type_id,
	const QString& field_path, const QString& value)
{
	if (catalog_kind == QStringLiteral("sensor")) {
		const QString original_id = type_id.trimmed();
		S_SENSOR_DEFINE* define = find_initial_sensor_define(type_id);
		if (define == nullptr) return false;
		const QString path = field_path.trimmed();
		const QString text = value.trimmed();
		bool ok = false;
		const double number = text.toDouble(&ok);

		if (path == QStringLiteral("type_name") || path == QStringLiteral("name")) {
			if (text.isEmpty()) return false;
			if (text != original_id && find_initial_sensor_define(text) != nullptr) return false;
			define->type_name = text;
		}
		else if (path == QStringLiteral("class") || path == QStringLiteral("type")) {
			const E_SENSOR_TYPE type = UtilEnum::trans_sensor_type(text);
			if (type == E_SENSOR_TYPE::UKN) return false;
			define->type = type;
		}
		else if ((path == QStringLiteral("reliability")) && ok) define->reliability = std::clamp(number, 0.0, 1.0);
		else if (path == QStringLiteral("act_domain_list")) {
			define->act_domain_list.clear();
			for (const QString& item : text.split(QRegularExpression(QStringLiteral(R"([,;，；\s]+)")),
				Qt::SkipEmptyParts)) {
				const E_DOMAIN domain = UtilEnum::trans_domain(item);
				if (domain != E_DOMAIN::UKN) define->act_domain_list.push_back(domain);
			}
		}
		else if (path.startsWith(QStringLiteral("task_mode_list."))) {
			const QStringList parts = path.split(QStringLiteral("."));
			if (parts.size() < 4) return false;
			const E_SENSOR_TASK task = UtilEnum::trans_sensor_task(parts.at(1));
			if (task == E_SENSOR_TASK::UKN) return false;
			S_SENSOR_TASK_MODE& mode = define->task_mode_list[task];
			const QString group = parts.at(2);
			const QString key = parts.at(3);

			if ((group == QStringLiteral("base")) && key == QStringLiteral("max_task_num") && ok) {
				mode.max_task_num = std::max(0, static_cast<int>(number));
			}
			else if (group == QStringLiteral("cap_env") && ok) {
				if (key == QStringLiteral("min_range_m")) mode.cap_env.min_range_m = number;
				else if (key == QStringLiteral("max_range_m")) mode.cap_env.max_range_m = number;
				else if (key == QStringLiteral("az_fov_deg")) mode.cap_env.az_fov_deg = number;
				else if (key == QStringLiteral("min_el_deg")) mode.cap_env.min_el_deg = number;
				else if (key == QStringLiteral("max_el_deg")) mode.cap_env.max_el_deg = number;
				else if (key == QStringLiteral("min_alt_m")) mode.cap_env.min_alt_m = number;
				else if (key == QStringLiteral("max_alt_m")) mode.cap_env.max_alt_m = number;
				else return false;
			}
			else if (group == QStringLiteral("cap_prob") && ok) {
				if (key == QStringLiteral("detection_prob")) mode.cap_prob.detection_prob = std::clamp(number, 0.0, 1.0);
				else if (key == QStringLiteral("false_alarm_prob")) mode.cap_prob.false_alarm_prob = std::clamp(number, 0.0, 1.0);
				else if (key == QStringLiteral("sensitive_slope")) mode.cap_prob.sensitive_slope = number;
				else return false;
			}
			else if (group == QStringLiteral("cap_acc") && ok) {
				if (key == QStringLiteral("range_sigma_m")) mode.cap_acc.range_sigma_m = number;
				else if (key == QStringLiteral("azimuth_sigma_deg")) mode.cap_acc.azimuth_sigma_deg = number;
				else if (key == QStringLiteral("elevation_sigma_deg")) mode.cap_acc.elevation_sigma_deg = number;
				else if (key == QStringLiteral("speed_sigma_ms")) mode.cap_acc.speed_sigma_ms = number;
				else return false;
			}
			else if (group == QStringLiteral("cap_time") && ok) {
				if (key == QStringLiteral("update_period_s") || key == QStringLiteral("scan_period_s")) {
					mode.cap_time.update_period_s = number;
				}
				else if (key == QStringLiteral("delay_ms")) mode.cap_time.delay_ms = number;
				else return false;
			}
			else return false;
		}
		else return false;

		if (define->type_name != original_id) {
			S_SENSOR_DEFINE moved_define = *define;
			sim_sensor_define_cache.erase(original_id);
			sim_sensor_define_cache[moved_define.type_name] = moved_define;
			deleted_initial_sensor_type_names << original_id;
			dirty_initial_sensor_type_names.erase(original_id);
		}

		initial_situation_status = QStringLiteral("已更新传感器类型 %1").arg(type_id.trimmed());
		mark_initial_sensor_type_dirty(define->type_name);
		rebuild_initial_situation_rows(true, true, true, true);
		return true;
	}

	if (catalog_kind == QStringLiteral("weapon")) {
		const QString original_id = type_id.trimmed();
		S_WEAPON_DEFINE* define = find_initial_weapon_define(type_id);
		if (define == nullptr) return false;
		const QString path = field_path.trimmed();
		const QString text = value.trimmed();
		bool ok = false;
		const double number = text.toDouble(&ok);

		if (path == QStringLiteral("type_name") || path == QStringLiteral("name")) {
			if (text.isEmpty()) return false;
			if (text != original_id && find_initial_weapon_define(text) != nullptr) return false;
			define->type_name = text;
		}
		else if (path == QStringLiteral("class") || path == QStringLiteral("type")) {
			const E_WEAPON_TYPE type = UtilEnum::trans_weapon_type(text);
			if (type == E_WEAPON_TYPE::UKN) return false;
			define->type = type;
		}
		else if (path == QStringLiteral("domain")) {
			const E_DOMAIN domain = UtilEnum::trans_domain(text);
			if (domain == E_DOMAIN::UKN) return false;
			define->domain = domain;
		}
		else if (path == QStringLiteral("mover")) {
			const E_MOVER mover = UtilEnum::trans_mover(text);
			if (mover == E_MOVER::UKN) return false;
			const bool type_changed = define->mover != mover;
			define->mover = mover;
			if (type_changed) define->mover_cap = default_mover_cap(mover);
		}
		else if ((path == QStringLiteral("launch_cap.max_range_m")) && ok) define->launch_cap.max_range_m = number;
		else if ((path == QStringLiteral("launch_cap.min_range_m")) && ok) define->launch_cap.min_range_m = number;
		else if ((path == QStringLiteral("launch_cap.firing_delay_s")) && ok) define->launch_cap.firing_delay_s = number;
		else if ((path == QStringLiteral("kill_cap.kill_range_m")) && ok) define->kill_cap.kill_range_m = number;
		else if ((path == QStringLiteral("kill_cap.kill_prob")) && ok) define->kill_cap.kill_prob = number;
		else if ((path == QStringLiteral("sign.radar_rcs_m2")) && ok) define->sign.radar_rcs_m2 = number;
		else if ((path == QStringLiteral("sign.opt_area_m2")) && ok) define->sign.opt_area_m2 = number;
		else if ((path == QStringLiteral("sign.infrared_wsr")) && ok) define->sign.infrared_wsr = number;
		else if ((path == QStringLiteral("mover_cap.mis_mover.min_speed_ms")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->min_speed_ms = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.max_speed_ms")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->max_speed_ms = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.min_alt_m")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->min_alt_m = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.max_alt_m")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->max_alt_m = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.max_linear_load_g")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->max_linear_load_g = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.max_radial_load_g")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->max_radial_load_g = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.max_climb_vel_ms")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->max_climb_vel_ms = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.max_drop_vel_ms")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->max_drop_vel_ms = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.life_time_s")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->life_time_s = number;
			else return false;
		}
		else if ((path == QStringLiteral("mover_cap.mis_mover.life_range_m")) && ok) {
			if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->mover_cap)) mis->life_range_m = number;
			else return false;
		}
		else return false;

		if (define->type_name != original_id) {
			S_WEAPON_DEFINE moved_define = *define;
			sim_weapon_define_cache.erase(original_id);
			sim_weapon_define_cache[moved_define.type_name] = moved_define;
			deleted_initial_weapon_type_names << original_id;
			dirty_initial_weapon_type_names.erase(original_id);
		}

		initial_situation_status = QStringLiteral("已更新武器类型 %1").arg(type_id.trimmed());
		mark_initial_weapon_type_dirty(define->type_name);
		rebuild_initial_situation_rows(true, true, true, true);
		return true;
	}

	if (catalog_kind != QStringLiteral("platform")) return false;
	const QString original_id = type_id.trimmed();
	S_PLAT_DEFINE* define = find_initial_platform_define(type_id);
	if (define == nullptr) return false;
	const QString path = field_path.trimmed();
	const QString text = value.trimmed();
	bool ok = false;
	const double number = text.toDouble(&ok);
	if (path == QStringLiteral("type_name")) {
		if (text.isEmpty()) return false;
		if (text != original_id && find_initial_platform_define(text) != nullptr) return false;
		define->type_name = text;
	}
	else if (path == QStringLiteral("icon") || path == QStringLiteral("icon_path")) {
		const QString icon = normalized_initial_platform_icon(text);
		if (!initial_platform_icon_options.contains(icon)) return false;
		define->icon_path = icon;
	}
	else if (path == QStringLiteral("domain")) {
		const E_DOMAIN domain = UtilEnum::trans_domain(text);
		if (domain == E_DOMAIN::UKN) return false;
		define->domain = domain;
	}
	else if (path == QStringLiteral("class") || path == QStringLiteral("type")) {
		const E_PLAT_TYPE type = UtilEnum::trans_type(text);
		if (type == E_PLAT_TYPE::UKN) return false;
		define->type = type;
	}
	else if (path == QStringLiteral("mover.type") || path == QStringLiteral("mover")) {
		const E_MOVER mover = UtilEnum::trans_mover(text);
		if (mover == E_MOVER::UKN) return false;
		const bool type_changed = define->mover != mover;
		define->mover = mover;
		if (type_changed) define->cap = default_mover_cap(mover);
	}
	else if ((path == QStringLiteral("capability_boundary.min_speed_ms") ||
		path == QStringLiteral("cap.air_mover.min_speed_ms") ||
		path == QStringLiteral("cap.sea_mover.min_speed_ms") ||
		path == QStringLiteral("cap.mis_mover.min_speed_ms")) && ok) {
		if (auto* air = std::get_if<S_AIR_MOVER_CAP>(&define->cap)) air->min_speed_ms = number;
		else if (auto* sea = std::get_if<S_SEA_MOVER_CAP>(&define->cap)) sea->min_speed_ms = number;
		else if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->min_speed_ms = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.max_speed_ms") ||
		path == QStringLiteral("cap.air_mover.max_speed_ms") ||
		path == QStringLiteral("cap.sea_mover.max_speed_ms") ||
		path == QStringLiteral("cap.mis_mover.max_speed_ms")) && ok) {
		if (auto* air = std::get_if<S_AIR_MOVER_CAP>(&define->cap)) air->max_speed_ms = number;
		else if (auto* sea = std::get_if<S_SEA_MOVER_CAP>(&define->cap)) sea->max_speed_ms = number;
		else if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->max_speed_ms = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.min_alt_m") ||
		path == QStringLiteral("cap.air_mover.min_alt_m") ||
		path == QStringLiteral("cap.mis_mover.min_alt_m")) && ok) {
		if (auto* air = std::get_if<S_AIR_MOVER_CAP>(&define->cap)) air->min_alt_m = number;
		else if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->min_alt_m = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.max_alt_m") ||
		path == QStringLiteral("cap.air_mover.max_alt_m") ||
		path == QStringLiteral("cap.mis_mover.max_alt_m")) && ok) {
		if (auto* air = std::get_if<S_AIR_MOVER_CAP>(&define->cap)) air->max_alt_m = number;
		else if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->max_alt_m = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.max_linear_load_g") ||
		path == QStringLiteral("cap.air_mover.max_linear_load_g") ||
		path == QStringLiteral("cap.mis_mover.max_linear_load_g")) && ok) {
		if (auto* air = std::get_if<S_AIR_MOVER_CAP>(&define->cap)) air->max_linear_load_g = number;
		else if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->max_linear_load_g = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.max_radial_load_g") ||
		path == QStringLiteral("cap.air_mover.max_radial_load_g") ||
		path == QStringLiteral("cap.mis_mover.max_radial_load_g")) && ok) {
		if (auto* air = std::get_if<S_AIR_MOVER_CAP>(&define->cap)) air->max_radial_load_g = number;
		else if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->max_radial_load_g = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.max_climb_vel_ms") ||
		path == QStringLiteral("cap.air_mover.max_climb_vel_ms") ||
		path == QStringLiteral("cap.mis_mover.max_climb_vel_ms")) && ok) {
		if (auto* air = std::get_if<S_AIR_MOVER_CAP>(&define->cap)) air->max_climb_vel_ms = number;
		else if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->max_climb_vel_ms = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.max_drop_vel_ms") ||
		path == QStringLiteral("cap.air_mover.max_drop_vel_ms") ||
		path == QStringLiteral("cap.mis_mover.max_drop_vel_ms")) && ok) {
		if (auto* air = std::get_if<S_AIR_MOVER_CAP>(&define->cap)) air->max_drop_vel_ms = number;
		else if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->max_drop_vel_ms = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.max_linear_acc_ms2") ||
		path == QStringLiteral("cap.sea_mover.max_linear_acc_ms2")) && ok) {
		if (auto* sea = std::get_if<S_SEA_MOVER_CAP>(&define->cap)) sea->max_linear_acc_ms2 = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.max_radial_acc_ms2") ||
		path == QStringLiteral("cap.sea_mover.max_radial_acc_ms2")) && ok) {
		if (auto* sea = std::get_if<S_SEA_MOVER_CAP>(&define->cap)) sea->max_radial_acc_ms2 = number;
		else return false;
	}
	else if ((path == QStringLiteral("capability_boundary.life_time_s") ||
		path == QStringLiteral("cap.mis_mover.life_time_s")) && ok) {
		if (auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define->cap)) mis->life_time_s = number;
		else return false;
	}
	else if ((path == QStringLiteral("detection_profile.radar_rcs_m2") ||
		path == QStringLiteral("sign.radar_rcs_m2")) && ok) define->sign.radar_rcs_m2 = number;
	else if ((path == QStringLiteral("detection_profile.opt_area_m2") ||
		path == QStringLiteral("sign.opt_area_m2")) && ok) define->sign.opt_area_m2 = number;
	else if ((path == QStringLiteral("detection_profile.infrared_wsr") ||
		path == QStringLiteral("sign.infrared_wsr")) && ok) define->sign.infrared_wsr = number;
	else return false;

	if (define->type_name != original_id) {
		S_PLAT_DEFINE moved_define = *define;
		sim_plat_define_cache.erase(original_id);
		sim_plat_define_cache[moved_define.type_name] = moved_define;
		deleted_initial_platform_type_names << original_id;
		dirty_initial_platform_type_names.erase(original_id);
	}

	initial_situation_status = QStringLiteral("已更新平台类型 %1").arg(type_id.trimmed());
	mark_initial_platform_type_dirty(define->type_name);
	rebuild_initial_situation_rows(true, true, true, true);
	return true;
}

bool UiICGAS::updateInitialScenarioPlatformField(const QString& platform_id,
	const QString& field_path, const QString& value)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;
	const QString path = field_path.trimmed();
	const QString text = value.trimmed();
	bool ok = false;
	const double number = text.toDouble(&ok);
	if (path == QStringLiteral("type_name")) config->type_name = text;
	else if (path == QStringLiteral("type_id")) {
		const S_PLAT_DEFINE* define = find_initial_platform_define(text);
		if (define == nullptr) return false;
		config->type_name = platform_config_name_for_type(define->type_name, config->plt_id);
	}
	else if (path == QStringLiteral("commander")) config->cmd_id = text;
	else if (path == QStringLiteral("side")) config->side = side_from_detail_value(text);
	else if (path == QStringLiteral("initial_position.lon_deg") && ok) {
		config->init_state.pos_lla.lon_rad = number DEG2RAD;
		set_initial_state_route_point(*config);
	}
	else if (path == QStringLiteral("initial_position.lat_deg") && ok) {
		config->init_state.pos_lla.lat_rad = number DEG2RAD;
		set_initial_state_route_point(*config);
	}
	else if (path == QStringLiteral("initial_position.altitude_m") && ok) {
		config->init_state.pos_lla.alt_m = number;
		set_initial_state_route_point(*config);
	}
	else if (path == QStringLiteral("initial_position.speed_mps") && ok) {
		config->init_state.vel_loc.speed_ms = number;
		set_initial_state_route_point(*config);
	}
	else if (path == QStringLiteral("initial_position.heading_deg") && ok) {
		config->init_state.vel_loc.track_ang_rad = UtilCoor::norm_rad_0_2pi(number DEG2RAD);
	}
	else if (path == QStringLiteral("initial_position.path_angle_deg") && ok) {
		config->init_state.vel_loc.path_ang_rad = number DEG2RAD;
	}
	else return false;
	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已更新平台 %1").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::updateInitialRoutePoint(const QString& platform_id, int route_index,
	const QString& field_path, const QString& value)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr || route_index <= 0 || route_index >= static_cast<int>(config->init_route.point_list.size())) return false;
	S_ROUTE_POINT& point = config->init_route.point_list[route_index];
	const QString path = field_path.trimmed();
	bool ok = false;
	const double number = value.trimmed().toDouble(&ok);
	if (!ok) return false;
	if (path == QStringLiteral("lon_deg")) point.pos_lla.lon_rad = number DEG2RAD;
	else if (path == QStringLiteral("lat_deg")) point.pos_lla.lat_rad = number DEG2RAD;
	else if (path == QStringLiteral("altitude_m")) point.pos_lla.alt_m = number;
	else if (path == QStringLiteral("speed_mps")) point.speed_ms = number;
	else return false;
	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已更新 %1 的航路点").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::setInitialRouteCirculate(const QString& platform_id, bool circulate)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;
	config->init_route.is_circulate = circulate;
	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已更新 %1 的循环航线").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::addInitialRoutePoint(const QString& platform_id)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr) return false;
	if (config->init_route.route_id.trimmed().isEmpty()) {
		config->init_route.route_id = config->plt_id.trimmed() + QStringLiteral("_route");
	}
	set_initial_state_route_point(*config);

	S_ROUTE_POINT point;
	if (!config->init_route.point_list.empty()) {
		point = config->init_route.point_list.back();
	}
	else {
		point.pos_lla = config->init_state.pos_lla;
		point.speed_ms = config->init_state.vel_loc.speed_ms;
	}
	config->init_route.point_list.push_back(point);
	renumber_route_points(config->init_route);
	set_initial_state_route_point(*config);

	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已新增 %1 的航路点").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::deleteInitialRoutePoint(const QString& platform_id, int route_index)
{
	S_PLAT_CONFIG* config = find_initial_platform_config(platform_id);
	if (config == nullptr || route_index <= 0 || route_index >= static_cast<int>(config->init_route.point_list.size())) return false;
	if (config->init_route.point_list.size() <= 2) {
		initial_situation_status = QStringLiteral("删除失败：至少保留 1 个用户航路点");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	config->init_route.point_list.erase(config->init_route.point_list.begin() + route_index);
	renumber_route_points(config->init_route);
	set_initial_state_route_point(*config);

	initial_selected_scenario_platform_id = platform_id.trimmed();
	mark_initial_platform_detail_dirty(selected_scenario);
	initial_situation_status = QStringLiteral("已删除 %1 的航路点").arg(platform_id.trimmed());
	rebuild_initial_situation_rows(false, true, true, true);
	return true;
}

bool UiICGAS::updateInitialSimCtrlField(const QString& field_path, const QString& value)
{
	const QString path = field_path.trimmed();
	const QString text = value.trimmed();
	bool ok = false;
	if (path == QStringLiteral("sim_iter_ms")) {
		const int sim_iter = text.toInt(&ok);
		if (!ok || sim_iter <= 0) {
			initial_sim_ctrl_save_status = QStringLiteral("步长必须大于0");
			emit_initial_changed(false, false, false, true);
			return false;
		}
		initial_sim_iter_ms = QString::number(sim_iter);
	}
	else if (path == QStringLiteral("time_acc_ratio")) {
		const double ratio = text.toDouble(&ok);
		if (!ok || ratio <= 0.0) {
			initial_sim_ctrl_save_status = QStringLiteral("倍速必须大于0");
			emit_initial_changed(false, false, false, true);
			return false;
		}
		initial_time_acc_ratio = compact_number_text(ratio);
	}
	else {
		return false;
	}

	initial_sim_ctrl_save_status = QStringLiteral("未保存");
	set_initial_sim_ctrl_dirty(true);
	emit_initial_changed(false, false, false, true);
	return true;
}

bool UiICGAS::saveInitialSimCtrlConfig()
{
	if (sim_controller == nullptr) {
		initial_sim_ctrl_save_status = QStringLiteral("保存失败：SimController 未就绪");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	bool iter_ok = false;
	bool ratio_ok = false;
	const int sim_iter = initial_sim_iter_ms.trimmed().toInt(&iter_ok);
	const double ratio = initial_time_acc_ratio.trimmed().toDouble(&ratio_ok);
	if (!iter_ok || sim_iter <= 0 || !ratio_ok || ratio <= 0.0) {
		initial_sim_ctrl_save_status = QStringLiteral("保存失败：参数无效");
		emit_initial_changed(false, false, false, true);
		return false;
	}

	initial_sim_ctrl_save_status = QStringLiteral("正在保存...");
	emit_initial_changed(false, false, false, true);

	SimController* controller = sim_controller;
	bool submitted = false;
	QMetaObject::invokeMethod(controller, [controller, sim_iter, ratio, &submitted]() {
		if (controller == nullptr) return;
		submitted = true;
		controller->save_sim_ctrl(sim_iter, ratio);
	}, Qt::BlockingQueuedConnection);
	if (!submitted) {
		initial_sim_ctrl_save_status = QStringLiteral("保存失败：请求未发出");
		emit_initial_changed(false, false, false, true);
	}
	return submitted;
}

bool UiICGAS::saveInitialSituationConfig()
{
	return saveInitialScenarioConfig();
}

bool UiICGAS::saveInitialScenarioConfig()
{
	const QString scenario_id = current_sim_scenario_id(sim_plat_config_cache, selected_scenario);
	if (scenario_id.isEmpty()) {
		initial_scenario_save_status = QStringLiteral("保存失败：未选择场景");
		emit_initial_changed(false, false, false, true);
		return false;
	}
	return submit_initial_scenario_save(scenario_id, QStringLiteral("scenario"));
}

bool UiICGAS::saveInitialSelectedPlatformConfig()
{
	const QString scenario_id = current_sim_scenario_id(sim_plat_config_cache, selected_scenario);
	if (scenario_id.isEmpty()) {
		initial_platform_detail_save_status = QStringLiteral("保存失败：未选择场景");
		emit_initial_changed(false, false, false, true);
		return false;
	}
	if (initial_selected_scenario_platform_id.trimmed().isEmpty()) {
		initial_platform_detail_save_status = QStringLiteral("保存失败：未选择平台");
		emit_initial_changed(false, false, false, true);
		return false;
	}
	return submit_initial_scenario_save(scenario_id, QStringLiteral("detail"));
}

bool UiICGAS::confirmConceptConfig()
{
	if (!applyPendingConceptDetail()) {
		concept_status = QStringLiteral("确认失败");
		rebuild_concept_state();
		sync_workflow_module_states();
		return false;
	}

	if (concept_dirty) {
		dirty_concept_scenarios.insert(selected_concept_scenario);
		concept_dirty = false;
	}

	std::set<QString> confirmed_scenarios;
	for (const QString& scenario : dirty_concept_scenarios) {
		auto graph_iter = concept_graph_cache.find(scenario);
		if (graph_iter == concept_graph_cache.end()) {
			concept_status = QStringLiteral("确认失败");
			rebuild_concept_state();
			sync_workflow_module_states();
			return false;
		}

		S_CONCEPT_GRAPH& graph = graph_iter->second;
		QString error_text;
		if (!validate_concept_graph(graph, &error_text)) {
			concept_status = error_text;
			rebuild_concept_state();
			sync_workflow_module_states();
			return false;
		}

		bool saved = false;
		InfoExpert* expert = InfoExpert::GetInstance();
		if (expert != nullptr) {
			const S_CONCEPT_GRAPH graph_snapshot = graph;
			saved = expert->save_concept_graph(graph_snapshot);
		}
		if (!saved) {
			concept_status = QStringLiteral("确认失败");
			rebuild_concept_state();
			sync_workflow_module_states();
			return false;
		}

		ModuleGraph::GetInstance()->update_concept_graph(graph);
		original_concept_graphs[scenario] = graph;
		confirmed_scenarios.insert(scenario);
	}

	for (const QString& scenario : confirmed_scenarios) {
		dirty_concept_scenarios.erase(scenario);
	}

	if (!dirty_concept_scenarios.empty()) {
		concept_status = QStringLiteral("确认失败");
		rebuild_concept_state();
		sync_workflow_module_states();
		return false;
	}

	concept_dirty = false;
	concept_detail_dirty = false;
	concept_config_confirmed = true;
	concept_config_locked = false;
	clear_concept_undo_state();
    concept_overview_apply_status = QStringLiteral("无修改");
    concept_node_apply_status = QStringLiteral("无修改");
	concept_status = QStringLiteral("确认成功");
	rebuild_concept_state();
	sync_workflow_module_states();
	return true;
}

bool UiICGAS::resetConceptConfig()
{
	const auto original_iter = original_concept_graphs.find(selected_concept_scenario);
	if (original_iter == original_concept_graphs.end()) {
		concept_status = QStringLiteral("重置失败");
		rebuild_concept_state();
		sync_workflow_module_states();
		return false;
	}

	S_CONCEPT_GRAPH& graph = concept_graph_cache[selected_concept_scenario];
	graph = original_iter->second;
	rebuild_concept_edge_index(graph);
	concept_dirty = false;
	dirty_concept_scenarios.erase(selected_concept_scenario);
	concept_detail_dirty = false;
	concept_config_confirmed = false;
	concept_config_locked = false;
	clear_concept_undo_state();
	clear_concept_focus_state();
    concept_overview_apply_status = QStringLiteral("无修改");
    concept_node_apply_status = QStringLiteral("无修改");
	concept_status = QStringLiteral("已重置");
	rebuild_concept_state();
	sync_workflow_module_states();
	return true;
}

bool UiICGAS::resetAllConceptConfigChanges()
{
	if (concept_detail_dirty) {
		discardPendingConceptDetail();
	}

	std::set<QString> scenarios = dirty_concept_scenarios;
	if (concept_dirty) {
		scenarios.insert(selected_concept_scenario);
	}

	if (scenarios.empty()) {
		concept_config_locked = false;
		concept_config_confirmed = false;
		clear_concept_undo_state();
		clear_concept_focus_state();
		concept_status = QStringLiteral("已重置");
		rebuild_concept_state();
		sync_workflow_module_states();
		return true;
	}

	for (const QString& scenario : scenarios) {
		const auto original_iter = original_concept_graphs.find(scenario);
		if (original_iter == original_concept_graphs.end()) {
			concept_status = QStringLiteral("重置失败");
			rebuild_concept_state();
			sync_workflow_module_states();
			return false;
		}

		S_CONCEPT_GRAPH& graph = concept_graph_cache[scenario];
		graph = original_iter->second;
		rebuild_concept_edge_index(graph);
		ModuleGraph::GetInstance()->update_concept_graph(graph);
	}

	dirty_concept_scenarios.clear();
	concept_dirty = false;
	concept_detail_dirty = false;
	concept_config_confirmed = false;
	concept_config_locked = false;
	clear_concept_undo_state();
	clear_concept_focus_state();
    concept_overview_apply_status = QStringLiteral("无修改");
    concept_node_apply_status = QStringLiteral("无修改");
	concept_status = QStringLiteral("已重置");
	rebuild_concept_state();
	sync_workflow_module_states();
	return true;
}

bool UiICGAS::applyPendingConceptDetail()
{
	if (!concept_detail_dirty) return true;
	if (concept_detail_mode == "overview") return applyConceptOverviewDetail();
	if (concept_detail_mode == "node") return applyConceptNodeDetail();
	return true;
}

void UiICGAS::discardPendingConceptDetail()
{
	if (!concept_detail_dirty) return;
	concept_detail_dirty = false;
	clear_concept_undo_state();
    concept_overview_apply_status = QStringLiteral("无修改");
    concept_node_apply_status = QStringLiteral("无修改");
	concept_status = conceptDirty() ? QStringLiteral("存在未确认修改") : QStringLiteral("无修改");
	rebuild_concept_state();
	sync_workflow_module_states();
}

UiICGAS::ConceptDetailDraftState UiICGAS::capture_concept_detail_draft() const
{
	ConceptDetailDraftState state;
	state.mode = concept_detail_mode;
	state.overview_type = concept_detail_type;
	state.node_id = concept_detail_node_id;
	state.node_name = concept_node_name;
	state.overview_status = concept_overview_apply_status;
	state.node_status = concept_node_apply_status;
	state.overview_rows = concept_overview_detail_rows;
	state.node_edge_rows = concept_node_edge_rows;
	return state;
}

void UiICGAS::restore_concept_detail_draft(const ConceptDetailDraftState& state)
{
	concept_detail_mode = state.mode;
	concept_detail_type = state.overview_type;
	concept_detail_node_id = state.node_id;
	concept_node_name = state.node_name;
	concept_overview_apply_status = state.overview_status;
	concept_node_apply_status = state.node_status;
	concept_overview_detail_rows = state.overview_rows;
	concept_node_edge_rows = state.node_edge_rows;
}

void UiICGAS::push_concept_undo_state()
{
}

void UiICGAS::clear_concept_undo_state()
{
}

void UiICGAS::mark_concept_detail_dirty()
{
	concept_config_confirmed = false;
	concept_detail_dirty = true;
	if (concept_detail_mode == "overview") concept_overview_apply_status = QStringLiteral("等待应用修改");
	else if (concept_detail_mode == "node") concept_node_apply_status = QStringLiteral("等待应用修改");
	sync_workflow_module_states();
}

void UiICGAS::clear_concept_focus_state()
{
	concept_focus_mode = false;
	concept_focus_activations.clear();
	concept_focus_parents.clear();
	concept_focus_order.clear();
	concept_focus_node_positions.clear();
}

UiICGAS::ConceptFocusActivation UiICGAS::make_concept_focus_activation(const S_CONCEPT_GRAPH& graph, const QString& node_id) const
{
	ConceptFocusActivation activation;
	const QString focus_id = node_id.trimmed();
	if (focus_id.isEmpty() || find_concept_node(graph, focus_id) == nullptr) return activation;

	activation.node_id = focus_id;
	for (const S_GRAPH_EDGE& edge : graph.edge_list) {
		if (edge.font_node_id == focus_id && edge.back_node_id != focus_id) {
			activation.inspired_node_ids.insert(edge.back_node_id);
			activation.inspired_edge_ids.insert(edge.id);
		}
		else if (edge.back_node_id == focus_id && edge.font_node_id != focus_id) {
			activation.inspired_node_ids.insert(edge.font_node_id);
			activation.inspired_edge_ids.insert(edge.id);
		}
	}
	return activation;
}

QPointF UiICGAS::default_concept_focus_node_position(const S_CONCEPT_GRAPH& graph, const QString& node_id) const
{
	constexpr int nodes_per_row = 6;
	constexpr double lane_x = 24.0;
	constexpr double lane_w = 1120.0;
	constexpr double lane_title_w = 108.0;
	constexpr double lane_content_padding = 16.0;
	constexpr double lane_vertical_padding = 8.0;
	constexpr double node_w = 148.0;
	constexpr double node_h = 62.0;
	constexpr double node_start_x = 160.0;
	constexpr double node_start_y = 18.0;
	constexpr double node_step_x = 158.0;
	constexpr double node_step_y = 90.0;
	constexpr double node_min_x = lane_x + lane_title_w + lane_content_padding;
	constexpr double node_max_x = lane_x + lane_w - node_w - lane_content_padding;

	const QString key = selected_concept_scenario + "|" + node_id.trimmed();
	const auto focus_pos_iter = concept_focus_node_positions.find(key);
	if (focus_pos_iter != concept_focus_node_positions.end()) {
		return focus_pos_iter->second;
	}

	const S_GRAPH_NODE* node = find_concept_node(graph, node_id.trimmed());
	if (node == nullptr) return QPointF(node_min_x, 64.0);

	int index = 0;
	for (const S_GRAPH_NODE& candidate : graph.node_list) {
		if (candidate.type != node->type) continue;
		if (candidate.id == node->id) break;
		++index;
	}

	QPointF relative_pos = concept_node_positions.count(key) > 0
		? concept_node_positions.at(key)
		: QPointF(node_start_x + (index % nodes_per_row) * node_step_x,
			node_start_y + (index / nodes_per_row) * node_step_y);

	const double lane_y = concept_graph_lane_y(graph, node->type);
	const double lane_h = concept_graph_lane_height(graph, node->type);
	relative_pos.setX(std::clamp(relative_pos.x(), node_min_x, node_max_x));
	relative_pos.setY(std::clamp(relative_pos.y(), lane_vertical_padding, lane_h - node_h - lane_vertical_padding));

	if (lane_y >= 0.0) {
		return QPointF(relative_pos.x(), lane_y + relative_pos.y());
	}

	const int lane_index = std::max(0, concept_lane_order_index(node->type));
	return QPointF(relative_pos.x(), 28.0 + lane_index * 126.0 + relative_pos.y());
}

QPointF UiICGAS::avoid_concept_focus_overlap(const QPointF& desired, const QString& node_id) const
{
	constexpr double node_w = 148.0;
	constexpr double node_h = 62.0;
	constexpr double padding = 18.0;
	constexpr double min_x = 12.0;
	constexpr double max_x = 1012.0;
	constexpr double min_y = 12.0;
	constexpr double max_y = 1800.0;

	QPointF resolved(
		std::clamp(desired.x(), min_x, max_x),
		std::clamp(desired.y(), min_y, max_y));
	const QString self_key = selected_concept_scenario + "|" + node_id.trimmed();

	for (int pass = 0; pass < 16; ++pass) {
		bool moved = false;
		for (const auto& item : concept_focus_node_positions) {
			if (item.first == self_key) continue;
			const QPointF other = item.second;
			const bool overlaps = std::abs((resolved.x() + node_w / 2.0) - (other.x() + node_w / 2.0)) < node_w + padding &&
				std::abs((resolved.y() + node_h / 2.0) - (other.y() + node_h / 2.0)) < node_h + padding;
			if (!overlaps) continue;

			const double dx = (resolved.x() + node_w / 2.0) - (other.x() + node_w / 2.0);
			const double dy = (resolved.y() + node_h / 2.0) - (other.y() + node_h / 2.0);
			const double push_x = std::abs(dx) < 1.0 ? padding : (dx > 0.0 ? padding : -padding);
			const double push_y = std::abs(dy) < 1.0 ? padding : (dy > 0.0 ? padding : -padding);
			resolved.setX(std::clamp(resolved.x() + push_x, min_x, max_x));
			resolved.setY(std::clamp(resolved.y() + push_y, min_y, max_y));
			moved = true;
		}
		if (!moved) break;
	}

	return resolved;
}

void UiICGAS::seed_concept_focus_positions(const S_CONCEPT_GRAPH& graph, const ConceptFocusActivation& activation)
{
	if (activation.node_id.isEmpty()) return;

	const QString focus_key = selected_concept_scenario + "|" + activation.node_id;
	if (concept_focus_node_positions.count(focus_key) == 0) {
		concept_focus_node_positions[focus_key] = avoid_concept_focus_overlap(
			default_concept_focus_node_position(graph, activation.node_id), activation.node_id);
	}

	const QPointF focus_pos = concept_focus_node_positions[focus_key];
	constexpr double node_w = 148.0;
	constexpr double node_h = 62.0;
	constexpr double min_x = 12.0;
	constexpr double min_y = 12.0;
	constexpr double max_x = 1012.0;
	constexpr double max_y = 1800.0;
	const double focus_center_x = focus_pos.x() + node_w / 2.0;
	const double focus_center_y = focus_pos.y() + node_h / 2.0;

	int index = 0;
	for (const QString& inspired_id : activation.inspired_node_ids) {
		if (inspired_id == activation.node_id) continue;
		const QString node_key = selected_concept_scenario + "|" + inspired_id;
		if (concept_focus_node_positions.count(node_key) > 0) {
			++index;
			continue;
		}

		const double ring = std::floor(index / 8.0);
		const double radius = 178.0 + ring * 76.0;
		constexpr double pi = 3.14159265358979323846;
		const double angle = (index % 8) * (pi / 4.0);
		QPointF desired(
			focus_center_x + std::cos(angle) * radius - node_w / 2.0,
			focus_center_y + std::sin(angle) * radius - node_h / 2.0);
		desired.setX(std::clamp(desired.x(), min_x, max_x));
		desired.setY(std::clamp(desired.y(), min_y, max_y));
		concept_focus_node_positions[node_key] = avoid_concept_focus_overlap(desired, inspired_id);
		++index;
	}
}

QString UiICGAS::concept_focus_parent_for(const QString& node_id) const
{
	for (auto iter = concept_focus_order.rbegin(); iter != concept_focus_order.rend(); ++iter) {
		const auto activation_iter = concept_focus_activations.find(*iter);
		if (activation_iter != concept_focus_activations.end() &&
			activation_iter->second.inspired_node_ids.count(node_id) > 0) {
			return *iter;
		}
	}
	return QString();
}

void UiICGAS::collapse_concept_focus_node(const QString& node_id)
{
	std::set<QString> subtree_nodes;
	collect_concept_focus_subtree(node_id, subtree_nodes);
	for (const QString& focus_id : subtree_nodes) {
		concept_focus_activations.erase(focus_id);
		concept_focus_parents.erase(focus_id);
	}
	concept_focus_order.erase(std::remove_if(concept_focus_order.begin(), concept_focus_order.end(),
		[&subtree_nodes](const QString& focus_id) { return subtree_nodes.count(focus_id) > 0; }),
		concept_focus_order.end());
}

void UiICGAS::collect_concept_focus_subtree(const QString& node_id, std::set<QString>& subtree_nodes) const
{
	if (subtree_nodes.count(node_id) > 0) return;
	subtree_nodes.insert(node_id);
	for (const auto& item : concept_focus_parents) {
		if (item.second == node_id) {
			collect_concept_focus_subtree(item.first, subtree_nodes);
		}
	}
}

void UiICGAS::setConceptNavMode(const QString& mode)
{
	const bool overview = mode.trimmed().toLower() != "graph";
	if (concept_overview_mode == overview) return;

	concept_overview_mode = overview;
	concept_detail_dirty = false;
	clear_concept_undo_state();
	if (concept_overview_mode) {
		concept_detail_mode = "empty";
		concept_detail_node_id.clear();
		clear_concept_focus_state();
	}
	rebuild_concept_state();
}

void UiICGAS::setConceptFocusMode(bool enabled)
{
	if (concept_overview_mode) enabled = false;
	if (concept_focus_mode == enabled) return;

	concept_focus_mode = enabled;
	if (!concept_focus_mode) {
		clear_concept_focus_state();
	}
	else {
		concept_type_filter_page = "node";
	}
	rebuild_concept_state();
}

void UiICGAS::toggleConceptFocusNode(const QString& node_id)
{
	if (concept_overview_mode) return;
	if (!concept_focus_mode) {
		concept_focus_mode = true;
		concept_type_filter_page = "node";
	}

	const S_CONCEPT_GRAPH* graph = current_concept_graph();
	const QString focus_id = node_id.trimmed();
	if (graph == nullptr || focus_id.isEmpty() || find_concept_node(*graph, focus_id) == nullptr) return;

	if (concept_focus_activations.count(focus_id) > 0) {
		std::set<QString> subtree_nodes;
		collect_concept_focus_subtree(focus_id, subtree_nodes);
		if (subtree_nodes.size() >= concept_focus_activations.size()) return;
		collapse_concept_focus_node(focus_id);
	}
	else {
		ConceptFocusActivation activation = make_concept_focus_activation(*graph, focus_id);
		if (activation.node_id.isEmpty()) return;

		if (!concept_focus_activations.empty()) {
			std::set<QString> visible_node_ids;
			std::set<QString> visible_edge_ids;
			for (const auto& item : concept_focus_activations) {
				const ConceptFocusActivation& current = item.second;
				visible_node_ids.insert(current.node_id);
				visible_node_ids.insert(current.inspired_node_ids.begin(), current.inspired_node_ids.end());
				visible_edge_ids.insert(current.inspired_edge_ids.begin(), current.inspired_edge_ids.end());
			}

			bool adds_visible_content = visible_node_ids.count(activation.node_id) == 0;
			for (const QString& inspired_id : activation.inspired_node_ids) {
				if (visible_node_ids.count(inspired_id) == 0) {
					adds_visible_content = true;
					break;
				}
			}
			if (!adds_visible_content) {
				for (const QString& edge_id : activation.inspired_edge_ids) {
					if (visible_edge_ids.count(edge_id) == 0) {
						adds_visible_content = true;
						break;
					}
				}
			}
			if (!adds_visible_content) return;
		}

		seed_concept_focus_positions(*graph, activation);
		const QString parent_id = concept_focus_parent_for(focus_id);
		concept_focus_activations[focus_id] = activation;
		concept_focus_parents[focus_id] = parent_id;
		if (std::find(concept_focus_order.begin(), concept_focus_order.end(), focus_id) == concept_focus_order.end()) {
			concept_focus_order.push_back(focus_id);
		}
	}

	rebuild_concept_state();
}

void UiICGAS::setConceptTypeFilterPage(const QString& page)
{
	const QString value = page.trimmed().toLower() == "edge" ? "edge" : "node";
	if (concept_type_filter_page == value) return;

	concept_type_filter_page = value;
	rebuild_concept_state();
}

void UiICGAS::setConceptNodeTypeChecked(const QString& type, bool checked)
{
	if (concept_focus_mode && !concept_overview_mode) return;

	const E_NODE_TYPE node_type = UtilEnum::trans_node_type(type);
	if (node_type == E_NODE_TYPE::UKN) return;

	if (checked) concept_user_node_types.insert(node_type);
	else concept_user_node_types.erase(node_type);

	if (concept_user_node_types.empty()) {
		concept_user_node_types.insert(node_type);
	}
	rebuild_concept_state();
}

void UiICGAS::setConceptFieldChecked(const QString& group, const QString& field, bool checked)
{
	const QString group_text = group.trimmed().toLower();
	const QString field_text = field.trimmed().toLower();

	if (group_text == "node" && field_text == "id") concept_show_node_id = checked;
	else if (group_text == "node" && field_text == "name") concept_show_node_name = checked;
	else if (group_text == "edge" && field_text == "id") concept_show_edge_id = checked;
	else if (group_text == "edge" && field_text == "name") concept_show_edge_name = checked;
	else if (group_text == "edge" && field_text == "weight") concept_show_edge_weight = checked;

	rebuild_concept_state();
}

void UiICGAS::showConceptOverviewDetail(const QString& type)
{
	concept_detail_dirty = false;
	clear_concept_undo_state();
	concept_detail_type = UtilEnum::trans_node_type(type);
	concept_detail_node_id.clear();
	concept_detail_mode = concept_detail_type == E_NODE_TYPE::UKN ? "empty" : "overview";
    concept_overview_apply_status = QStringLiteral("无修改");
	rebuild_concept_state();
}

void UiICGAS::showConceptNodeDetail(const QString& node_id)
{
	concept_detail_dirty = false;
	clear_concept_undo_state();
	concept_detail_node_id = node_id.trimmed();
	concept_detail_type = E_NODE_TYPE::UKN;
	concept_detail_mode = concept_detail_node_id.isEmpty() ? "empty" : "node";
    concept_node_apply_status = QStringLiteral("无修改");
	rebuild_concept_state();
}

QStringList UiICGAS::conceptNodeEdgeTypeOptions(const QString& direction, const QString& currentType) const
{
	QStringList values;
	const S_CONCEPT_GRAPH* graph = current_concept_graph();
	const S_GRAPH_NODE* current_node = graph == nullptr ? nullptr : find_concept_node(*graph, concept_detail_node_id);
	if (graph == nullptr || current_node == nullptr) return values;

	const E_NODE_TYPE current_type = UtilEnum::trans_node_type(currentType);
	const bool incoming = direction.trimmed().toLower() == "in" || direction.trimmed() == QStringLiteral("入边");
	const std::vector<E_NODE_TYPE> type_order = concept_node_type_order();

	for (const E_NODE_TYPE type : type_order) {
		bool has_candidate = false;
		for (const S_GRAPH_NODE& node : graph->node_list) {
			if (node.type != type || node.id == current_node->id) continue;
			const E_EDGE_TYPE candidate_type = incoming
				? UtilEnum::trans_edge_type(type, current_node->type)
				: UtilEnum::trans_edge_type(current_node->type, type);
			if (candidate_type == E_EDGE_TYPE::UKN) continue;
			has_candidate = true;
			break;
		}
		if (has_candidate) {
			values.push_back(UtilEnum::trans_node_type_en(type));
		}
	}

	if (current_type != E_NODE_TYPE::UKN && !values.contains(UtilEnum::trans_node_type_en(current_type))) {
		values.push_back(UtilEnum::trans_node_type_en(current_type));
	}
	return values;
}

QStringList UiICGAS::conceptNodeIdsByType(const QString& type) const
{
	QStringList values;
	const S_CONCEPT_GRAPH* graph = current_concept_graph();
	const E_NODE_TYPE node_type = UtilEnum::trans_node_type(type);
	if (graph == nullptr || node_type == E_NODE_TYPE::UKN) return values;

	for (const S_GRAPH_NODE& node : graph->node_list) {
		if (node.type == node_type && node.id != concept_detail_node_id) {
			values.push_back(node.id);
		}
	}
	std::sort(values.begin(), values.end(), [this](const QString& lhs, const QString& rhs) {
		const int left_dash = lhs.lastIndexOf('-');
		const int right_dash = rhs.lastIndexOf('-');
		bool left_ok = false;
		bool right_ok = false;
		const int left_num = lhs.mid(left_dash + 1).toInt(&left_ok);
		const int right_num = rhs.mid(right_dash + 1).toInt(&right_ok);
		if (left_ok && right_ok && left_num != right_num) return left_num < right_num;
		return lhs < rhs;
	});
	return values;
}

void UiICGAS::addConceptOverviewNode()
{
	if (concept_config_locked || concept_detail_mode != "overview") return;

	push_concept_undo_state();
	const QString prefix = UtilEnum::trans_node_type_prefix(concept_detail_type);
	int max_index = 0;
	for (const QVariant& value : concept_overview_detail_rows) {
		const QString id = value.toMap().value("id").toString();
		if (id.startsWith(prefix + "-")) {
			max_index = std::max(max_index, id.mid(prefix.size() + 1).toInt());
		}
	}

	QVariantMap row;
	row["id"] = QString("%1-%2").arg(prefix).arg(max_index + 1);
	row["nodeId"] = row.value("id");
	row["name"] = QStringLiteral("新增节点");
	concept_overview_detail_rows.push_back(row);
	mark_concept_detail_dirty();
	emit conceptChanged();
}

void UiICGAS::deleteConceptOverviewNode(int row)
{
	if (concept_config_locked || concept_detail_mode != "overview") return;
	if (row < 0 || row >= concept_overview_detail_rows.size()) return;

	push_concept_undo_state();
	concept_overview_detail_rows.removeAt(row);
	mark_concept_detail_dirty();
	emit conceptChanged();
}

void UiICGAS::setConceptOverviewNodeName(int row, const QString& name)
{
	if (concept_config_locked || concept_detail_mode != "overview") return;
	if (row < 0 || row >= concept_overview_detail_rows.size()) return;

	QVariantMap item = concept_overview_detail_rows.at(row).toMap();
	if (item.value("name").toString() == name) return;
	push_concept_undo_state();
	item["name"] = name;
	concept_overview_detail_rows[row] = item;
	mark_concept_detail_dirty();
	emit conceptChanged();
}

bool UiICGAS::applyConceptOverviewDetail()
{
	if (concept_config_locked || concept_detail_mode != "overview") return false;

	S_CONCEPT_GRAPH* graph = current_concept_graph();
	if (graph == nullptr || concept_detail_type == E_NODE_TYPE::UKN) return false;

	std::set<QString> keep_ids;
	std::vector<S_GRAPH_NODE> node_list;
	for (const S_GRAPH_NODE& node : graph->node_list) {
		if (node.type != concept_detail_type) {
			node_list.push_back(node);
			keep_ids.insert(node.id);
		}
	}

	for (const QVariant& value : concept_overview_detail_rows) {
		const QVariantMap item = value.toMap();
		S_GRAPH_NODE node;
		node.id		= item.value("id").toString().trimmed();
		node.name	= item.value("name").toString().trimmed();
		node.type	= concept_detail_type;
		node.active = true;
		if (!node.id.isEmpty()) {
			node_list.push_back(node);
			keep_ids.insert(node.id);
		}
	}

	graph->node_list = node_list;
	std::vector<S_GRAPH_EDGE> edge_list;
	for (const S_GRAPH_EDGE& edge : graph->edge_list) {
		if (keep_ids.count(edge.font_node_id) > 0 && keep_ids.count(edge.back_node_id) > 0) {
			edge_list.push_back(edge);
		}
	}
	graph->edge_list = edge_list;
	rebuild_concept_edge_index(*graph);

	concept_dirty = true;
	concept_config_confirmed = false;
	dirty_concept_scenarios.insert(selected_concept_scenario);
	concept_detail_dirty = false;
	clear_concept_undo_state();
    concept_overview_apply_status = QStringLiteral("修改已应用");
	concept_status = QStringLiteral("存在未确认修改");
	rebuild_concept_state();
	sync_workflow_module_states();
	return true;
}

void UiICGAS::setConceptNodeName(const QString& name)
{
	if (concept_config_locked || concept_detail_mode != "node") return;
	if (concept_node_name == name) return;

	push_concept_undo_state();
	concept_node_name = name;
	mark_concept_detail_dirty();
	emit conceptChanged();
}

void UiICGAS::addConceptNodeEdge(const QString& direction)
{
	if (concept_config_locked || concept_detail_mode != "node") return;

	const S_CONCEPT_GRAPH* graph = current_concept_graph();
	if (graph == nullptr) return;
	const S_GRAPH_NODE* node = find_concept_node(*graph, concept_detail_node_id);
	if (node == nullptr) return;

	push_concept_undo_state();
	const bool incoming = direction.trimmed().toLower() == "in";
	for (const S_GRAPH_NODE& other : graph->node_list) {
		if (other.id == node->id) continue;
		const E_NODE_TYPE source_type = incoming ? other.type : node->type;
		const E_NODE_TYPE target_type = incoming ? node->type : other.type;
		const E_EDGE_TYPE edge_type = UtilEnum::trans_edge_type(source_type, target_type);
		if (edge_type == E_EDGE_TYPE::UKN) continue;

		QVariantMap row;
		row["direction"] = incoming ? QStringLiteral("入边") : QStringLiteral("出边");
		row["edgeId"] = make_concept_edge_id(*graph);
		row["edgeType"] = UtilEnum::trans_edge_type_en(edge_type);
		row["otherType"] = UtilEnum::trans_node_type_en(other.type);
		row["otherNodeId"] = other.id;
        row["edgeName"] = QStringLiteral("新增边");
		row["weight"] = QStringLiteral("1.000");
		concept_node_edge_rows.push_back(row);
		mark_concept_detail_dirty();
		emit conceptChanged();
		return;
	}

	concept_node_apply_status = QStringLiteral("无可用节点");
	emit conceptChanged();
}

void UiICGAS::deleteConceptNodeEdge(int row)
{
	if (concept_config_locked || concept_detail_mode != "node") return;
	if (row < 0 || row >= concept_node_edge_rows.size()) return;

	push_concept_undo_state();
	concept_node_edge_rows.removeAt(row);
	mark_concept_detail_dirty();
	emit conceptChanged();
}

void UiICGAS::setConceptNodeEdgeOtherType(int row, const QString& type)
{
	if (concept_config_locked || concept_detail_mode != "node") return;
	if (row < 0 || row >= concept_node_edge_rows.size()) return;

	const S_CONCEPT_GRAPH* graph = current_concept_graph();
	if (graph == nullptr) return;
	QVariantMap item = concept_node_edge_rows.at(row).toMap();
	const E_NODE_TYPE other_type = UtilEnum::trans_node_type(type);
	for (const S_GRAPH_NODE& node : graph->node_list) {
		if (node.type == other_type && node.id != concept_detail_node_id) {
			if (item.value("otherType").toString() == UtilEnum::trans_node_type_en(other_type) &&
				item.value("otherNodeId").toString() == node.id) return;
			push_concept_undo_state();
			item["otherType"] = UtilEnum::trans_node_type_en(other_type);
			item["otherNodeId"] = node.id;
			concept_node_edge_rows[row] = item;
			mark_concept_detail_dirty();
			emit conceptChanged();
			return;
		}
	}
}

void UiICGAS::setConceptNodeEdgeOtherId(int row, const QString& node_id)
{
	if (concept_config_locked || concept_detail_mode != "node") return;
	if (row < 0 || row >= concept_node_edge_rows.size()) return;

	const S_CONCEPT_GRAPH* graph = current_concept_graph();
	if (graph == nullptr) return;
	const S_GRAPH_NODE* node = find_concept_node(*graph, node_id);
	if (node == nullptr) return;

	QVariantMap item = concept_node_edge_rows.at(row).toMap();
	if (item.value("otherNodeId").toString() == node->id) return;
	push_concept_undo_state();
	item["otherType"] = UtilEnum::trans_node_type_en(node->type);
	item["otherNodeId"] = node->id;
	concept_node_edge_rows[row] = item;
	mark_concept_detail_dirty();
	emit conceptChanged();
}

void UiICGAS::setConceptNodeEdgeName(int row, const QString& name)
{
	if (concept_config_locked || concept_detail_mode != "node") return;
	if (row < 0 || row >= concept_node_edge_rows.size()) return;

	QVariantMap item = concept_node_edge_rows.at(row).toMap();
	if (item.value("edgeName").toString() == name) return;
	push_concept_undo_state();
	item["edgeName"] = name;
	concept_node_edge_rows[row] = item;
	mark_concept_detail_dirty();
	emit conceptChanged();
}

void UiICGAS::setConceptNodeEdgeWeight(int row, const QString& weight)
{
	if (concept_config_locked || concept_detail_mode != "node") return;
	if (row < 0 || row >= concept_node_edge_rows.size()) return;

	QVariantMap item = concept_node_edge_rows.at(row).toMap();
	if (item.value("weight").toString() == weight) return;
	push_concept_undo_state();
	item["weight"] = weight;
	concept_node_edge_rows[row] = item;
	mark_concept_detail_dirty();
	emit conceptChanged();
}

bool UiICGAS::applyConceptNodeDetail()
{
	if (concept_config_locked || concept_detail_mode != "node") return false;

	S_CONCEPT_GRAPH* graph = current_concept_graph();
	if (graph == nullptr) return false;
	S_GRAPH_NODE* detail_node = find_concept_node(*graph, concept_detail_node_id);
	if (detail_node == nullptr) return false;

	detail_node->name = concept_node_name.trimmed();

	std::vector<S_GRAPH_EDGE> edge_list;
	for (const S_GRAPH_EDGE& edge : graph->edge_list) {
		if (edge.font_node_id != detail_node->id && edge.back_node_id != detail_node->id) {
			edge_list.push_back(edge);
		}
	}

	for (const QVariant& value : concept_node_edge_rows) {
		const QVariantMap item = value.toMap();
		const QString other_id = item.value("otherNodeId").toString().trimmed();
		const S_GRAPH_NODE* other_node = find_concept_node(*graph, other_id);
		if (other_node == nullptr) continue;

		const bool incoming = item.value("direction").toString() == QStringLiteral("入边");
		S_GRAPH_EDGE edge;
		edge.id				= item.value("edgeId").toString().trimmed();
		edge.name			= item.value("edgeName").toString().trimmed();
		edge.font_node_id	= incoming ? other_node->id : detail_node->id;
		edge.back_node_id	= incoming ? detail_node->id : other_node->id;
		edge.type			= UtilEnum::trans_edge_type(incoming ? other_node->type : detail_node->type,
			incoming ? detail_node->type : other_node->type);
		edge.weight			= item.value("weight").toString().toDouble();
		edge.active			= true;
		if (edge.id.isEmpty()) edge.id = make_concept_edge_id(*graph);
		edge_list.push_back(edge);
	}

	graph->edge_list = edge_list;
	rebuild_concept_edge_index(*graph);
	concept_dirty = true;
	concept_config_confirmed = false;
	dirty_concept_scenarios.insert(selected_concept_scenario);
	concept_detail_dirty = false;
	clear_concept_undo_state();
    concept_node_apply_status = QStringLiteral("修改已应用");
	concept_status = QStringLiteral("存在未确认修改");
	rebuild_concept_state();
	sync_workflow_module_states();
	return true;
}

void UiICGAS::setConceptGraphNodePosition(const QString& kind, const QString& node_id, double x, double y)
{
	if (kind.trimmed().toLower() != "overview" && kind.trimmed().toLower() != "node") return;
	const QString key = selected_concept_scenario + "|" + node_id.trimmed();
	if (kind.trimmed().toLower() == "overview") {
		concept_overview_positions[key] = QPointF(x, y);
	}
	else if (concept_focus_mode && !concept_overview_mode) {
		concept_focus_node_positions[key] = QPointF(x, y);
	}
	else {
		const S_CONCEPT_GRAPH* graph = current_concept_graph();
		const S_GRAPH_NODE* node = graph == nullptr ? nullptr : find_concept_node(*graph, node_id.trimmed());
		const double lane_y = node == nullptr ? -1.0 : concept_graph_lane_y(*graph, node->type);
		concept_node_positions[key] = QPointF(x, lane_y >= 0.0 ? y - lane_y : y);
	}
}

void UiICGAS::setConceptGraphNodePositions(const QVariantList& rows)
{
	for (const QVariant& value : rows) {
		const QVariantMap row = value.toMap();
		const QString kind = row.value(QStringLiteral("kind")).toString();
		const QString node_id = row.value(QStringLiteral("nodeId")).toString();
		if (node_id.trimmed().isEmpty()) continue;
		setConceptGraphNodePosition(kind, node_id,
			row.value(QStringLiteral("x")).toDouble(),
			row.value(QStringLiteral("y")).toDouble());
	}
}

void UiICGAS::moveConceptGraphLane(const QString& lane_type, int target_index)
{
	const E_NODE_TYPE type = UtilEnum::trans_node_type(lane_type);
	if (type == E_NODE_TYPE::UKN) return;

	auto iter = std::find(concept_lane_order.begin(), concept_lane_order.end(), type);
	if (iter == concept_lane_order.end()) return;

	concept_lane_order.erase(iter);
	target_index = std::clamp(target_index, 0, static_cast<int>(concept_lane_order.size()));
	concept_lane_order.insert(concept_lane_order.begin() + target_index, type);
	rebuild_concept_state();
}

void UiICGAS::refreshSituation()
{
	situation_refresh_dirty = false;
	if (ui_sit_monitor != nullptr) {
		ui_sit_monitor->refreshSituation();
	}
}

void UiICGAS::on_afs_client_log(const QString& info)
{
	if (append_log(info)) {
		emit logRowsChanged();
	}
	if (info.contains("[ERROR]", Qt::CaseInsensitive)) {
		afs_client_has_error = true;
		afs_client_error_text = info;
		emit workflowModuleStateChanged();
	}
}

void UiICGAS::on_afs_server_log(const QString& info)
{
	if (append_log(info)) {
		emit logRowsChanged();
	}
}

void UiICGAS::on_tac_server_log(const QString& info)
{
	if (append_log(info)) {
		emit logRowsChanged();
	}
}

void UiICGAS::on_timer_sync()
{
	const long long cur_time_ms = QDateTime::currentMSecsSinceEpoch();
	if (cur_time_ms - last_module_state_sync_time_ms >= 1000) {
		last_module_state_sync_time_ms = cur_time_ms;
		sync_workflow_module_states();
	}
	last_situation_refresh_time_ms = cur_time_ms;
	refreshSituation();
}

QObject* UiICGAS::root_object() const
{
	return engine.rootObjects().isEmpty() ? nullptr : engine.rootObjects().first();
}

void UiICGAS::center_on_primary_screen(QQuickWindow* window)
{
	if (window == nullptr) return;

	QScreen* screen = window->screen();
	const QList<QScreen*> screens = QGuiApplication::screens();
	if (screen == nullptr || !screens.contains(screen)) {
		screen = QGuiApplication::primaryScreen();
	}
	if (screen == nullptr) return;

	const QRect geometry = screen->availableGeometry();
	if (geometry.width() <= 0 || geometry.height() <= 0) return;

	if (window->screen() != screen) {
		window->setScreen(screen);
	}

	QSize target_size(qMax(1, window->width()), qMax(1, window->height()));
	if (target_size.width() > geometry.width() || target_size.height() > geometry.height()) {
		target_size.scale(geometry.size(), Qt::KeepAspectRatio);
		window->resize(qMax(1, target_size.width()), qMax(1, target_size.height()));
	}

	const int x = geometry.x() + qMax(0, (geometry.width() - window->width()) / 2);
	const int y = geometry.y() + qMax(0, (geometry.height() - window->height()) / 2);
	window->setPosition(x, y);
}

void UiICGAS::init_connect()
{
	connect(afs_client, &AfsimClient::sig_update_entity_state,
		module_db, &ModuleDatabase::on_afs_recv_entity_state, Qt::QueuedConnection);
	connect(afs_client, &AfsimClient::sig_update_entity_state, this,
		[this](const S_PLAT& plat) {
			Q_UNUSED(plat);
			afs_client_received_data = true;
		}, Qt::QueuedConnection);
	connect(afs_client, &AfsimClient::sig_afs_client_msg,
		this, &UiICGAS::on_afs_client_log);
	connect(afs_server, &AfsimServer::sig_afs_server_msg,
		this, &UiICGAS::on_afs_server_log);
	connect(tac_server, &TacviewServer::sig_tac_server_msg,
		this, &UiICGAS::on_tac_server_log);
	connect(sim_controller, &SimController::signal_platform_update,
		module_db, &ModuleDatabase::on_sim_platform_update, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_weapon_update,
		module_db, &ModuleDatabase::on_sim_weapon_update, Qt::QueuedConnection);
	connect(module_db, &ModuleDatabase::send_algo_plat,
		ui_sit_monitor, &UiSitMonitor::onAlgoPlatSnapshot, Qt::QueuedConnection);
	connect(module_db, &ModuleDatabase::send_algo_weapon,
		ui_sit_monitor, &UiSitMonitor::onAlgoWeaponSnapshot, Qt::QueuedConnection);
	connect(module_db, &ModuleDatabase::send_algo_fmt,
		ui_sit_monitor, &UiSitMonitor::onAlgoFmtSnapshot, Qt::QueuedConnection);
	connect(module_db, &ModuleDatabase::send_algo_coa,
		ui_sit_monitor, &UiSitMonitor::onAlgoCoaSnapshot, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_load_sim_ctrl, this,
		[this](bool is_success, int sim_iter_ms, double time_acc_ratio) {
			initial_sim_iter_ms = QString::number(std::max(1, sim_iter_ms));
			initial_time_acc_ratio = compact_number_text(time_acc_ratio);
			if (!is_success) {
				initial_sim_ctrl_save_status = QStringLiteral("使用默认值");
			}
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_load_plat_define, this,
		[this](bool is_success, SNAPSHOT_PLAT_DEFINE_MAP list_plat_define) {
			if (!is_success || list_plat_define.isNull()) return;
			sim_plat_define_cache = *list_plat_define;
			sim_plat_define_loaded = true;
			sim_config_loaded = sim_plat_define_loaded && sim_plat_config_loaded &&
				sim_weapon_define_loaded && sim_sensor_define_loaded;
			const int define_count = sim_define_count(sim_plat_define_cache);
			const int config_count = sim_config_count(sim_plat_config_cache);
			const int weapon_define_count = sim_weapon_define_count(sim_weapon_define_cache);
			const int sensor_define_count = sim_sensor_define_count(sim_sensor_define_cache);
			initial_situation_status = QStringLiteral("已从 SimController 加载 %1 个平台类型、%2 个武器类型、%3 个传感器类型、%4 个初始平台")
				.arg(define_count)
				.arg(weapon_define_count)
				.arg(sensor_define_count)
				.arg(config_count);
			rebuild_sim_scenario_names();
			rebuild_initial_situation_rows();
			refreshSituation();
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_load_plat_config, this,
		[this](bool is_success, SNAPSHOT_PLAT_CONFIG_MAP list_plat_config) {
			if (!is_success || list_plat_config.isNull()) return;
			sim_plat_config_cache = *list_plat_config;
			sim_plat_config_loaded = true;
			sim_config_loaded = sim_plat_define_loaded && sim_plat_config_loaded &&
				sim_weapon_define_loaded && sim_sensor_define_loaded;
			const int define_count = sim_define_count(sim_plat_define_cache);
			const int config_count = sim_config_count(sim_plat_config_cache);
			const int weapon_define_count = sim_weapon_define_count(sim_weapon_define_cache);
			const int sensor_define_count = sim_sensor_define_count(sim_sensor_define_cache);
			initial_situation_status = QStringLiteral("已从 SimController 加载 %1 个平台类型、%2 个武器类型、%3 个传感器类型、%4 个初始平台")
				.arg(define_count)
				.arg(weapon_define_count)
				.arg(sensor_define_count)
				.arg(config_count);
			rebuild_sim_scenario_names();
			rebuild_initial_situation_rows();
			refreshSituation();
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_load_weapon_define, this,
		[this](bool is_success, SNAPSHOT_WEAPON_DEFINE_MAP list_weapon_define) {
			if (!is_success || list_weapon_define.isNull()) return;
			sim_weapon_define_cache = *list_weapon_define;
			sim_weapon_define_loaded = true;
			sim_config_loaded = sim_plat_define_loaded && sim_plat_config_loaded &&
				sim_weapon_define_loaded && sim_sensor_define_loaded;
			rebuild_initial_situation_rows(true, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_load_sensor_define, this,
		[this](bool is_success, SNAPSHOT_SENSOR_DEFINE_MAP list_sensor_define) {
			if (!is_success || list_sensor_define.isNull()) return;
			sim_sensor_define_cache = *list_sensor_define;
			sim_sensor_define_loaded = true;
			sim_config_loaded = sim_plat_define_loaded && sim_plat_config_loaded &&
				sim_weapon_define_loaded && sim_sensor_define_loaded;
			rebuild_initial_situation_rows(true, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_save_plat_define, this,
		[this](bool is_success) {
			if (!pending_initial_platform_type_save_context.isEmpty()) {
				initial_platform_type_save_status = save_status_text(is_success,
					QStringLiteral("保存成功"),
					QStringLiteral("保存失败"));
				if (is_success) {
					deleted_initial_platform_type_names.clear();
					clear_initial_platform_type_dirty(pending_initial_platform_type_save_context);
				}
				else {
					pending_initial_platform_type_deleted_contexts.clear();
				}
				pending_initial_platform_type_save_context.clear();
			}
			else {
				initial_situation_status = is_success ?
					QStringLiteral("平台定义配置已写入 Config/ConfigSim") :
					QStringLiteral("平台定义配置写入失败");
			}
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_save_weapon_define, this,
		[this](bool is_success) {
			if (!pending_initial_weapon_type_save_context.isEmpty()) {
				initial_weapon_type_save_status = save_status_text(is_success,
					QStringLiteral("保存成功"),
					QStringLiteral("保存失败"));
				if (is_success) {
					deleted_initial_weapon_type_names.clear();
					clear_initial_weapon_type_dirty(pending_initial_weapon_type_save_context);
				}
				else {
					pending_initial_weapon_type_deleted_contexts.clear();
				}
				pending_initial_weapon_type_save_context.clear();
			}
			else {
				initial_situation_status = is_success ?
					QStringLiteral("武器定义配置已写入 Config/ConfigSim") :
					QStringLiteral("武器定义配置写入失败");
			}
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_save_sensor_define, this,
		[this](bool is_success) {
			if (!pending_initial_sensor_type_save_context.isEmpty()) {
				initial_sensor_type_save_status = save_status_text(is_success,
					QStringLiteral("保存成功"),
					QStringLiteral("保存失败"));
				if (is_success) {
					deleted_initial_sensor_type_names.clear();
					clear_initial_sensor_type_dirty(pending_initial_sensor_type_save_context);
				}
				else {
					pending_initial_sensor_type_deleted_contexts.clear();
				}
				pending_initial_sensor_type_save_context.clear();
			}
			else {
				initial_situation_status = is_success ?
					QStringLiteral("传感器定义配置已写入 Config/ConfigSim") :
					QStringLiteral("传感器定义配置写入失败");
			}
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_save_sim_ctrl, this,
		[this](bool is_success) {
			initial_sim_ctrl_save_status = save_status_text(is_success,
				QStringLiteral("保存成功"),
				QStringLiteral("保存失败"));
			if (is_success) {
				set_initial_sim_ctrl_dirty(false);
			}
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_save_plat_config, this,
		[this](bool is_success) {
			const QString saved_scenario_id = pending_initial_config_save_scenario_id;
			if (pending_initial_config_save_context == QStringLiteral("detail")) {
				initial_platform_detail_save_status = save_status_text(is_success,
					QStringLiteral("保存成功"),
					QStringLiteral("保存失败"));
				if (is_success) {
					clear_initial_platform_detail_dirty(saved_scenario_id);
					clear_initial_scenario_dirty(saved_scenario_id);
				}
				pending_initial_config_save_context.clear();
				pending_initial_config_save_scenario_id.clear();
			}
			else if (pending_initial_config_save_context == QStringLiteral("scenario")) {
				initial_scenario_save_status = save_status_text(is_success,
					QStringLiteral("保存成功"),
					QStringLiteral("保存失败"));
				if (is_success) {
					clear_initial_scenario_dirty(saved_scenario_id);
					clear_initial_platform_detail_dirty(saved_scenario_id);
				}
				pending_initial_config_save_context.clear();
				pending_initial_config_save_scenario_id.clear();
				rebuild_sim_scenario_names();
			}
			else {
				initial_situation_status = is_success ?
					QStringLiteral("初始态势配置已写入 Config/ConfigSim") :
					QStringLiteral("初始态势配置写入失败");
			}
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_rmov_plat_define, this,
		[this](bool is_success) {
			initial_situation_status = is_success ?
				QStringLiteral("平台定义配置已从 Config/ConfigSim 删除") :
				QStringLiteral("平台定义配置删除失败");
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_rmov_weapon_define, this,
		[this](bool is_success) {
			initial_situation_status = is_success ?
				QStringLiteral("武器定义配置已从 Config/ConfigSim 删除") :
				QStringLiteral("武器定义配置删除失败");
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_rmov_sensor_define, this,
		[this](bool is_success) {
			initial_situation_status = is_success ?
				QStringLiteral("传感器定义配置已从 Config/ConfigSim 删除") :
				QStringLiteral("传感器定义配置删除失败");
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
	connect(sim_controller, &SimController::signal_rmov_plat_config, this,
		[this](bool is_success) {
			if (pending_initial_config_delete_context == QStringLiteral("scenario")) {
				const QString deleted_id = pending_initial_config_delete_scenario_id;
				if (is_success) {
					sim_plat_config_cache.erase(deleted_id);
					if (selected_scenario == deleted_id) {
						selected_scenario = sim_plat_config_cache.empty()
							? QString()
							: sim_plat_config_cache.begin()->first;
						initial_selected_scenario_platform_id.clear();
						emit selectedScenarioChanged();
					}
					initial_scenario_save_status = QStringLiteral("删除成功");
					clear_initial_scenario_dirty(deleted_id);
					clear_initial_platform_detail_dirty(deleted_id);
					rebuild_sim_scenario_names();
					rebuild_initial_situation_rows(false, true, true, true);
				}
				else {
					initial_scenario_save_status = QStringLiteral("删除失败");
				}
				pending_initial_config_delete_context.clear();
				pending_initial_config_delete_scenario_id.clear();
			}
			else {
				initial_situation_status = is_success ?
					QStringLiteral("初始态势配置已从 Config/ConfigSim 删除") :
					QStringLiteral("初始态势配置删除失败");
			}
			emit_initial_changed(false, false, false, true);
		}, Qt::QueuedConnection);
}

void UiICGAS::refresh_sim_config_cache(bool force_reload)
{
	if (sim_controller == nullptr) return;
	if (!force_reload && sim_config_loaded) return;

	bool loaded = false;
	std::map<QString, S_PLAT_DEFINE> define_map;
	std::map<QString, std::vector<S_PLAT_CONFIG>> config_map;
	std::map<QString, S_WEAPON_DEFINE> weapon_define_map;
	std::map<QString, S_SENSOR_DEFINE> sensor_define_map;
	int sim_iter_ms = 1000;
	double time_acc_ratio = 1.0;
	SimController* controller = sim_controller;
	QMetaObject::invokeMethod(controller, [controller, &define_map, &config_map,
		&weapon_define_map, &sensor_define_map, &sim_iter_ms, &time_acc_ratio, &loaded]() {
		if (controller == nullptr) return;
		controller->load_sim_ctrl();
		if (controller->list_plat_define.empty()) controller->load_plat_define();
		if (controller->list_plat_config.empty()) controller->load_plat_config();
		if (controller->list_weapon_define.empty()) controller->load_weapon_define();
		if (controller->list_sensor_define.empty()) controller->load_sensor_define();
		define_map = controller->list_plat_define;
		config_map = controller->list_plat_config;
		weapon_define_map = controller->list_weapon_define;
		sensor_define_map = controller->list_sensor_define;
		sim_iter_ms = controller->sim_iter_ms;
		time_acc_ratio = controller->time_acc_ratio;
		loaded = true;
	}, Qt::BlockingQueuedConnection);
	if (loaded) {
		sim_plat_define_cache = std::move(define_map);
		sim_plat_config_cache = std::move(config_map);
		sim_weapon_define_cache = std::move(weapon_define_map);
		sim_sensor_define_cache = std::move(sensor_define_map);
		initial_sim_iter_ms = QString::number(std::max(1, sim_iter_ms));
		initial_time_acc_ratio = compact_number_text(time_acc_ratio);
	}
	sim_plat_define_loaded = loaded;
	sim_plat_config_loaded = loaded;
	sim_weapon_define_loaded = loaded;
	sim_sensor_define_loaded = loaded;
	sim_config_loaded = sim_plat_define_loaded && sim_plat_config_loaded &&
		sim_weapon_define_loaded && sim_sensor_define_loaded;
	rebuild_sim_scenario_names();
}

void UiICGAS::rebuild_sim_scenario_names()
{
	QStringList next_names;
	for (const auto& item : sim_plat_config_cache) {
		if (!item.first.trimmed().isEmpty()) {
			next_names << item.first;
		}
	}
	next_names.removeDuplicates();
	next_names.sort(Qt::CaseInsensitive);

	if (sim_scenario_name_list == next_names) return;
	sim_scenario_name_list = next_names;
	emit simScenarioNamesChanged();
}

void UiICGAS::refresh_database_config_cache()
{
	if (module_db == nullptr) return;

	int timeout_ms = database_plat_timeout_ms.toInt();
	int algo_step_ms = database_algo_step_ms.toInt();
	ModuleDatabase* database = module_db;
	QMetaObject::invokeMethod(database, [database, &timeout_ms, &algo_step_ms]() {
		if (database == nullptr) return;
		database->load_config();
		timeout_ms = database->plt_timeout_ms;
		algo_step_ms = database->algo_step_ms;
	}, Qt::BlockingQueuedConnection);

	database_plat_timeout_ms = QString::number(std::max(1, timeout_ms));
	database_algo_step_ms = QString::number(std::max(1, algo_step_ms));
	emit databaseConfigChanged();
}

bool UiICGAS::save_sim_config_cache(bool save_define, bool save_config)
{
	if (sim_controller == nullptr) return false;

	const std::map<QString, S_PLAT_DEFINE> define_map = sim_plat_define_cache;
	const std::map<QString, std::vector<S_PLAT_CONFIG>> config_map = sim_plat_config_cache;
	const QStringList deleted_define_names = deleted_initial_platform_type_names;
	bool ok = false;
	SimController* controller = sim_controller;
	QMetaObject::invokeMethod(controller, [controller, define_map, config_map, deleted_define_names,
		save_define, save_config, &ok]() mutable {
		if (controller == nullptr) return;
		ok = true;
		if (save_define) {
			for (const auto& item : define_map) {
				controller->save_plat_define(SNAPSHOT_PLAT_DEFINE_PAIR(
					new std::pair<QString, S_PLAT_DEFINE>(item)));
			}
			for (const QString& name : deleted_define_names) {
				controller->rmov_plat_define(SNAPSHOT_PLAT_DEFINE_PAIR(
					new std::pair<QString, S_PLAT_DEFINE>(name, S_PLAT_DEFINE())));
			}
		}
		if (save_config) {
			for (const auto& item : config_map) {
				controller->save_plat_config(SNAPSHOT_PLAT_CONFIG_PAIR(
					new std::pair<QString, std::vector<S_PLAT_CONFIG>>(item)));
			}
		}
	}, Qt::BlockingQueuedConnection);
	return ok;
}

bool UiICGAS::submit_initial_scenario_save(const QString& scenario_id, const QString& context)
{
	if (sim_controller == nullptr) {
		if (context == QStringLiteral("detail")) {
			initial_platform_detail_save_status = QStringLiteral("保存失败：SimController 未就绪");
		}
		else {
			initial_scenario_save_status = QStringLiteral("保存失败：SimController 未就绪");
		}
		emit_initial_changed(false, false, false, true);
		return false;
	}

	const QString id = scenario_id.trimmed();
	const auto iter = sim_plat_config_cache.find(id);
	if (id.isEmpty() || iter == sim_plat_config_cache.end()) {
		if (context == QStringLiteral("detail")) {
			initial_platform_detail_save_status = QStringLiteral("保存失败：场景不存在");
		}
		else {
			initial_scenario_save_status = QStringLiteral("保存失败：场景不存在");
		}
		emit_initial_changed(false, false, false, true);
		return false;
	}

	if (context == QStringLiteral("detail")) {
		initial_platform_detail_save_status = QStringLiteral("正在保存...");
	}
	else {
		initial_scenario_save_status = QStringLiteral("正在保存...");
	}
	pending_initial_config_save_context = context;
	pending_initial_config_save_scenario_id = id;
	emit_initial_changed(false, false, false, true);

	const std::pair<QString, std::vector<S_PLAT_CONFIG>> save_pair(id, iter->second);
	SimController* controller = sim_controller;
	bool submitted = false;
	QMetaObject::invokeMethod(controller, [controller, save_pair, &submitted]() {
		if (controller == nullptr) return;
		submitted = true;
		controller->save_plat_config(SNAPSHOT_PLAT_CONFIG_PAIR(
			new std::pair<QString, std::vector<S_PLAT_CONFIG>>(save_pair)));
	}, Qt::BlockingQueuedConnection);
	if (!submitted) {
		pending_initial_config_save_context.clear();
		pending_initial_config_save_scenario_id.clear();
		if (context == QStringLiteral("detail")) {
			initial_platform_detail_save_status = QStringLiteral("保存失败：请求未发出");
		}
		else {
			initial_scenario_save_status = QStringLiteral("保存失败：请求未发出");
		}
		emit_initial_changed(false, false, false, true);
	}
	return submitted;
}

void UiICGAS::set_module_database_running(bool running, bool clear_database)
{
	if (module_db == nullptr) return;

	bool timer_started = module_db_timer_started;
	ModuleDatabase* database = module_db;
	QMetaObject::invokeMethod(database, [database, running, clear_database, &timer_started]() {
		if (database == nullptr) return;
		database->is_running = running;
		if (clear_database) {
			database->clear_run_data();
		}
		if (running) {
			if (!timer_started) {
				database->on_sim_start();
				timer_started = true;
			}
		}
		else {
			database->on_sim_stop();
			timer_started = false;
		}
	}, Qt::BlockingQueuedConnection);
	module_db_timer_started = timer_started;
}

void UiICGAS::init_ui_state()
{
	init_workflow_pdu_log_rows();
	refresh_database_config_cache();
	InfoExpert* expert = InfoExpert::GetInstance();
	if (expert != nullptr) {
		scenario_name_list = expert->send_scenario_names();
		const QVariantMap loaded_comm_config = expert->send_comm_config();
		afs_client_addr = loaded_comm_config.value(QStringLiteral("afs_client_addr")).toString();
		afs_client_port = loaded_comm_config.value(QStringLiteral("afs_client_port")).toString();
		afs_server_addr = loaded_comm_config.value(QStringLiteral("afs_server_addr")).toString();
		afs_server_port = loaded_comm_config.value(QStringLiteral("afs_server_port")).toString();
		tac_server_addr = loaded_comm_config.value(QStringLiteral("tac_server_addr")).toString();
		tac_server_port = loaded_comm_config.value(QStringLiteral("tac_server_port")).toString();
		tac_server_user = loaded_comm_config.value(QStringLiteral("tac_server_user")).toString();
		tac_send_iter_ms = loaded_comm_config.value(QStringLiteral("tac_send_iter_ms")).toString();
		ui_sync_iter_ms = loaded_comm_config.value(QStringLiteral("ui_sync_iter_ms")).toString();
	}
	if (!sim_scenario_name_list.isEmpty()) {
		selected_scenario = sim_scenario_name_list.first();
	}
	else if (!scenario_name_list.isEmpty()) {
		selected_scenario = scenario_name_list.first();
	}

	initial_situation_status = QStringLiteral("初始态势加载中...");
	init_concept_state();
	init_workflow_module_states();
}

void UiICGAS::emit_initial_changed(bool catalog, bool scenario, bool selection, bool status)
{
	if (!catalog && !scenario && !selection && !status) return;
	if (catalog) emit initialCatalogChanged();
	if (scenario) emit initialScenarioChanged();
	if (selection) emit initialSelectionChanged();
	if (status) emit initialStatusChanged();
	emit initialSituationChanged();
}

void UiICGAS::rebuild_initial_catalog_rows()
{
	QVariantList next_platform_type_rows;
	QVariantList next_weapon_type_rows;
	QVariantList next_sensor_type_rows;
	rebuild_initial_platform_icon_options();
	normalize_initial_platform_define_icons();

	next_platform_type_rows.reserve(static_cast<int>(sim_plat_define_cache.size()));
	next_weapon_type_rows.reserve(static_cast<int>(sim_weapon_define_cache.size()));
	next_sensor_type_rows.reserve(static_cast<int>(sim_sensor_define_cache.size()));
	for (const auto& item : sim_plat_define_cache) {
		next_platform_type_rows.push_back(make_initial_platform_type_row(item.second));
	}
	for (const auto& item : sim_weapon_define_cache) {
		next_weapon_type_rows.push_back(make_initial_weapon_type_row(item.second));
	}
	for (const auto& item : sim_sensor_define_cache) {
		next_sensor_type_rows.push_back(make_initial_sensor_type_row(item.second));
	}

	initial_platform_type_rows = std::move(next_platform_type_rows);
	initial_weapon_type_rows = std::move(next_weapon_type_rows);
	initial_sensor_type_rows = std::move(next_sensor_type_rows);
}

void UiICGAS::rebuild_initial_platform_icon_options(bool force_reload)
{
	if (!force_reload && initial_platform_icon_options_loaded) return;

	initial_platform_icon_options.clear();
	const QDir mesh_dir(mesh_dir_path());
	const QFileInfoList files = mesh_dir.entryInfoList(QStringList() << QStringLiteral("*.obj"), QDir::Files, QDir::Name);
	initial_platform_icon_options.reserve(files.size());
	for (const QFileInfo& file : files) {
		initial_platform_icon_options << file.fileName();
	}
	initial_platform_icon_options.removeDuplicates();
	initial_platform_icon_options_loaded = true;
}

QString UiICGAS::normalized_initial_platform_icon(const QString& icon) const
{
	QString value = icon.trimmed();
	value.replace(QChar('\\'), QChar('/'));
	if (value.endsWith(QStringLiteral(".obj"), Qt::CaseInsensitive)) {
		value.chop(4);
	}
	const int slash = value.lastIndexOf(QChar('/'));
	if (slash >= 0) {
		value = value.mid(slash + 1);
	}

	if (value.compare(QStringLiteral("EA-18G"), Qt::CaseInsensitive) == 0 ||
		value.compare(QStringLiteral("EA18G"), Qt::CaseInsensitive) == 0) {
		value = QStringLiteral("FixedWing.F-18E");
	}
	else if (value.compare(QStringLiteral("DDG-51"), Qt::CaseInsensitive) == 0 ||
		value.compare(QStringLiteral("DDG51"), Qt::CaseInsensitive) == 0) {
		value = QStringLiteral("Watercraft.Type 052D");
	}
	else if (value.compare(QStringLiteral("Core.FixedWing"), Qt::CaseInsensitive) == 0 ||
		value.compare(QStringLiteral("FixedWing"), Qt::CaseInsensitive) == 0) {
		value = QStringLiteral("FixedWing.F-18E");
	}
	else if (value.compare(QStringLiteral("Core.Watercraft"), Qt::CaseInsensitive) == 0 ||
		value.compare(QStringLiteral("Watercraft"), Qt::CaseInsensitive) == 0) {
		value = QStringLiteral("Watercraft.Type 052D");
	}
	else if (value.compare(QStringLiteral("Core.AircraftCarrier"), Qt::CaseInsensitive) == 0 ||
		value.compare(QStringLiteral("AircraftCarrier"), Qt::CaseInsensitive) == 0) {
		value = QStringLiteral("Watercraft.CVN-59");
	}
	else if (value.compare(QStringLiteral("Core.Vehicle"), Qt::CaseInsensitive) == 0 ||
		value.compare(QStringLiteral("Vehicle"), Qt::CaseInsensitive) == 0) {
		value = QStringLiteral("Vehicle.Tank.M1");
	}
	else if (value.compare(QStringLiteral("Core.Rotorcraft"), Qt::CaseInsensitive) == 0 ||
		value.compare(QStringLiteral("Rotorcraft"), Qt::CaseInsensitive) == 0) {
		value = QStringLiteral("Rotorcraft.UH-60");
	}

	for (const QString& option : initial_platform_icon_options) {
		if (option.compare(value, Qt::CaseInsensitive) == 0 ||
			QFileInfo(option).completeBaseName().compare(value, Qt::CaseInsensitive) == 0) {
			return option;
		}
	}

	QString best;
	int best_score = -1;
	const QString key = value.toLower();
	for (const QString& option : initial_platform_icon_options) {
		const QString normalized_option = QFileInfo(option).completeBaseName().toLower();
		int score = -1;
		if (normalized_option.endsWith(QStringLiteral(".") + key)) score = 900;
		else if (normalized_option.endsWith(key)) score = 700;
		else if (normalized_option.contains(QStringLiteral(".") + key + QStringLiteral("."))) score = 650;
		else if (normalized_option.contains(key)) score = 500;
		if (score > best_score) {
			best_score = score;
			best = option;
		}
	}
	if (best_score >= 0) return best;
	if (key == QStringLiteral("cvn")) {
		for (const QString& option : initial_platform_icon_options) {
			if (option.compare(QStringLiteral("Core.AircraftCarrier.obj"), Qt::CaseInsensitive) == 0 ||
				option.compare(QStringLiteral("Watercraft.CVN-68.obj"), Qt::CaseInsensitive) == 0) {
				return option;
			}
		}
	}
	return initial_platform_icon_options.isEmpty() ? value : initial_platform_icon_options.first();
}

void UiICGAS::normalize_initial_platform_define_icons()
{
	for (auto& item : sim_plat_define_cache) {
		item.second.icon_path = normalized_initial_platform_icon(item.second.icon_path);
	}
}

void UiICGAS::rebuild_initial_scenario_rows()
{
	QVariantList next_rows;
	const std::vector<S_PLAT_CONFIG>* config_list = current_sim_config_list(sim_plat_config_cache, selected_scenario);
	if (config_list != nullptr) {
		next_rows.reserve(static_cast<int>(config_list->size()));
	}
	if (initial_selected_scenario_platform_id.isEmpty() && config_list != nullptr && !config_list->empty()) {
		initial_selected_scenario_platform_id = config_list->front().plt_id;
	}

	bool selected_exists = false;
	if (config_list != nullptr) {
		for (const S_PLAT_CONFIG& config : *config_list) {
			QVariantMap row = make_initial_platform_plot_row(config);
			const QString platform_id = row.value(QStringLiteral("platform_id")).toString();
			next_rows.push_back(row);
			if (platform_id == initial_selected_scenario_platform_id) {
				selected_exists = true;
			}
		}
	}

	if (!selected_exists && config_list != nullptr && !config_list->empty()) {
		initial_selected_scenario_platform_id = config_list->front().plt_id;
	}
	if (config_list == nullptr || config_list->empty()) {
		initial_selected_scenario_platform_id.clear();
	}

	initial_scenario_platform_rows = std::move(next_rows);
}

void UiICGAS::rebuild_initial_selected_platform()
{
	initial_selected_scenario_platform.clear();
	if (initial_selected_scenario_platform_id.isEmpty()) return;

	const std::vector<S_PLAT_CONFIG>* config_list = current_sim_config_list(sim_plat_config_cache, selected_scenario);
	if (config_list == nullptr) return;

	for (const S_PLAT_CONFIG& config : *config_list) {
		if (config.plt_id == initial_selected_scenario_platform_id) {
			initial_selected_scenario_platform = make_initial_platform_row(config, find_sim_platform_define(config));
			return;
		}
	}
}

void UiICGAS::rebuild_initial_situation_rows(bool catalog, bool scenario, bool selection, bool status)
{
	const bool may_update_selection = selection ||
		initial_selected_scenario_platform_id.isEmpty() ||
		initial_selected_scenario_platform.isEmpty();
	const QVariantList old_platform_type_rows = catalog ? initial_platform_type_rows : QVariantList();
	const QVariantList old_weapon_type_rows = catalog ? initial_weapon_type_rows : QVariantList();
	const QVariantList old_sensor_type_rows = catalog ? initial_sensor_type_rows : QVariantList();
	const QStringList old_icon_options = catalog ? initial_platform_icon_options : QStringList();
	const QVariantList old_scenario_platform_rows = scenario ? initial_scenario_platform_rows : QVariantList();
	const QVariantMap old_selected_platform = may_update_selection ? initial_selected_scenario_platform : QVariantMap();
	const QString old_selected_platform_id = initial_selected_scenario_platform_id;

	if (catalog) {
		rebuild_initial_catalog_rows();
	}
	if (scenario) {
		initial_scenario_platform_rows.clear();
	}
	if (selection) {
		initial_selected_scenario_platform.clear();
	}

	if (!sim_config_loaded && sim_plat_define_cache.empty() && sim_plat_config_cache.empty()) {
		initial_selected_scenario_platform_id.clear();
		if (scenario) {
			initial_scenario_platform_rows.clear();
		}
		if (selection) {
			initial_selected_scenario_platform.clear();
		}
		emit_initial_changed(catalog, scenario, selection, status);
		return;
	}

	if (scenario) {
		rebuild_initial_scenario_rows();
	}
	if (selection) {
		if (initial_selected_scenario_platform_id.isEmpty()) {
			initial_selected_scenario_platform.clear();
		}
		else {
			rebuild_initial_selected_platform();
		}
	}
	else if (initial_selected_scenario_platform_id.isEmpty()) {
		initial_selected_scenario_platform.clear();
		selection = true;
	}
	else if (initial_selected_scenario_platform.isEmpty()) {
		rebuild_initial_selected_platform();
		selection = true;
	}

	catalog = catalog && (initial_platform_type_rows != old_platform_type_rows ||
		initial_weapon_type_rows != old_weapon_type_rows ||
		initial_sensor_type_rows != old_sensor_type_rows ||
		initial_platform_icon_options != old_icon_options);
	scenario = scenario && initial_scenario_platform_rows != old_scenario_platform_rows;
	selection = selection && (initial_selected_scenario_platform != old_selected_platform ||
		initial_selected_scenario_platform_id != old_selected_platform_id);

	emit_initial_changed(catalog, scenario, selection, status);
}

QString UiICGAS::make_initial_scenario_id() const
{
	for (int index = 1; index < 1000; ++index) {
		const QString candidate = QStringLiteral("Scenario_%1").arg(index, 3, 10, QChar('0'));
		if (sim_plat_config_cache.find(candidate) == sim_plat_config_cache.end()) {
			return candidate;
		}
	}
	return QString();
}

QString UiICGAS::make_initial_platform_id(const QString& type_id) const
{
	Q_UNUSED(type_id);
	int max_id = 0;
	const std::vector<S_PLAT_CONFIG>* config_list = current_sim_config_list(sim_plat_config_cache, selected_scenario);
	if (config_list == nullptr) return QStringLiteral("00001");
	for (const S_PLAT_CONFIG& config : *config_list) {
		bool ok = false;
		const int id = config.plt_id.trimmed().toInt(&ok);
		if (ok) max_id = std::max(max_id, id);
	}
	return QStringLiteral("%1").arg(max_id + 1, 5, 10, QChar('0'));
}

S_PLAT_DEFINE* UiICGAS::find_initial_platform_define(const QString& type_id)
{
	const QString id = type_id.trimmed();
	auto map_iter = sim_plat_define_cache.find(id);
	if (map_iter != sim_plat_define_cache.end()) {
		return &map_iter->second;
	}
	for (auto& item : sim_plat_define_cache) {
		if (item.second.type_name == id) return &item.second;
	}
	return nullptr;
}

S_WEAPON_DEFINE* UiICGAS::find_initial_weapon_define(const QString& type_id)
{
	const QString id = type_id.trimmed();
	auto map_iter = sim_weapon_define_cache.find(id);
	if (map_iter != sim_weapon_define_cache.end()) {
		return &map_iter->second;
	}
	for (auto& item : sim_weapon_define_cache) {
		if (item.second.type_name == id) return &item.second;
	}
	return nullptr;
}

S_SENSOR_DEFINE* UiICGAS::find_initial_sensor_define(const QString& type_id)
{
	const QString id = type_id.trimmed();
	auto map_iter = sim_sensor_define_cache.find(id);
	if (map_iter != sim_sensor_define_cache.end()) {
		return &map_iter->second;
	}
	for (auto& item : sim_sensor_define_cache) {
		if (item.second.type_name == id) return &item.second;
	}
	return nullptr;
}

S_PLAT_CONFIG* UiICGAS::find_initial_platform_config(const QString& platform_id)
{
	const QString id = platform_id.trimmed();
	std::vector<S_PLAT_CONFIG>* config_list = current_sim_config_list(sim_plat_config_cache, selected_scenario);
	if (config_list == nullptr) return nullptr;
	for (S_PLAT_CONFIG& config : *config_list) {
		if (config.plt_id == id) return &config;
	}
	return nullptr;
}

void UiICGAS::start_module_initialization()
{
	prepare_workflow_module_states();
	emit workflowModuleStateChanged();

	QPointer<UiICGAS> self(this);
	QThreadPool::globalInstance()->start([self]() {
		if (!self.isNull()) {
			self->initialize_workflow_module_instances();
		}

		if (self.isNull()) return;
		QMetaObject::invokeMethod(self.data(), [self]() {
			if (self.isNull()) return;
			self->mark_workflow_modules_ready();
			self->sync_workflow_module_states();
		}, Qt::QueuedConnection);
	});
}

void UiICGAS::schedule_deferred_startup()
{
	if (deferred_startup_done || deferred_startup_scheduled) {
		return;
	}

	deferred_startup_scheduled = true;
	QPointer<UiICGAS> self(this);
	QTimer::singleShot(120, this, [self]() {
		if (!self.isNull()) {
			self->run_deferred_startup();
		}
	});
}

void UiICGAS::run_deferred_startup()
{
	if (deferred_startup_done) {
		return;
	}

	deferred_startup_scheduled = false;
	deferred_startup_done = true;

	refresh_sim_config_cache(true);
	if (workflow_icgas_mode() &&
		exact_sim_config_list(sim_plat_config_cache, selected_scenario) == nullptr) {
		const QString scenario_id = first_loaded_sim_scenario_id(sim_scenario_name_list, sim_plat_config_cache);
		if (!scenario_id.isEmpty()) {
			setSelectedScenario(scenario_id);
		}
	}
	const int define_count = sim_define_count(sim_plat_define_cache);
	const int config_count = sim_config_count(sim_plat_config_cache);
	const int weapon_define_count = sim_weapon_define_count(sim_weapon_define_cache);
	const int sensor_define_count = sim_sensor_define_count(sim_sensor_define_cache);
	initial_situation_status = QStringLiteral("已从 SimController 加载 %1 个平台类型、%2 个武器类型、%3 个传感器类型、%4 个初始平台")
		.arg(define_count)
		.arg(weapon_define_count)
		.arg(sensor_define_count)
		.arg(config_count);
	rebuild_initial_situation_rows();
	refreshSituation();
	start_module_initialization();
	sync_workflow_module_states();
}

QString UiICGAS::replay_log_root_path() const
{
	QStringList start_path_list;
	start_path_list << QDir::currentPath();

	const QString app_dir = QCoreApplication::applicationDirPath();
	if (!app_dir.isEmpty() && !start_path_list.contains(app_dir)) {
		start_path_list << app_dir;
	}

	for (const QString& start_path : start_path_list) {
		QDir dir(start_path);
		for (int i = 0; i < 8; ++i) {
			const QString log_path = dir.absoluteFilePath("LogFile");
			if (QDir(log_path).exists()) {
				return QDir(log_path).absolutePath();
			}

			const QString project_log_path = dir.absoluteFilePath("ICGAS/LogFile");
			if (QDir(project_log_path).exists()) {
				return QDir(project_log_path).absolutePath();
			}

			if (!dir.cdUp()) break;
		}
	}

	return QDir("LogFile").absolutePath();
}

void UiICGAS::init_concept_state()
{
	InfoExpert* expert = InfoExpert::GetInstance();
	if (expert != nullptr) {
		concept_graph_cache = expert->send_concept_graphs();
	}

	concept_scenario_names.clear();
	for (const auto& item : concept_graph_cache) {
		concept_scenario_names << item.first;
		if (original_concept_graphs.find(item.first) == original_concept_graphs.end()) {
			original_concept_graphs[item.first] = item.second;
		}
	}

	if (selected_concept_scenario.isEmpty() && !concept_scenario_names.isEmpty()) {
		selected_concept_scenario = concept_scenario_names.first();
	}

	rebuild_concept_state();
}

void UiICGAS::rebuild_concept_state()
{
	const bool preserve_detail_draft = concept_detail_dirty;
	const ConceptDetailDraftState detail_draft = preserve_detail_draft
		? capture_concept_detail_draft()
		: ConceptDetailDraftState();

	concept_scenario_rows.clear();
	concept_node_type_rows.clear();
	concept_edge_type_rows.clear();
	concept_field_rows.clear();
	concept_graph_lanes.clear();
	concept_graph_nodes.clear();
	concept_graph_edges.clear();
	concept_overview_detail_rows.clear();
	concept_node_edge_rows.clear();

	const std::vector<E_NODE_TYPE> node_type_order = concept_node_type_order();

	const auto graph_iter = concept_graph_cache.find(selected_concept_scenario);
	if (graph_iter == concept_graph_cache.end()) {
		concept_status = QStringLiteral("读取失败");
		emit conceptChanged();
		return;
	}

	const S_CONCEPT_GRAPH& graph = graph_iter->second;
	concept_scenario_rows.push_back(make_pair_row(QStringLiteral("场景ID"), graph.scenario_id));
	concept_scenario_rows.push_back(make_pair_row(QStringLiteral("场景类型"), UtilEnum::trans_scenario_type_en(graph.scenario_type)));
	concept_scenario_rows.push_back(make_pair_row(QStringLiteral("场景名称"), graph.scenario_name));
	concept_scenario_rows.push_back(make_pair_row(QStringLiteral("节点数量"), QString::number(static_cast<int>(graph.node_list.size()))));
	concept_scenario_rows.push_back(make_pair_row(QStringLiteral("边数量"), QString::number(static_cast<int>(graph.edge_list.size()))));

	std::map<E_NODE_TYPE, int> node_type_count;
	std::map<E_EDGE_TYPE, int> edge_type_count;
	std::map<QString, const S_GRAPH_NODE*> node_map;
	for (const S_GRAPH_NODE& node : graph.node_list) {
		node_type_count[node.type]++;
		node_map[node.id] = &node;
	}

	for (const S_GRAPH_EDGE& edge : graph.edge_list) {
		edge_type_count[edge.type]++;
	}

	if (concept_overview_mode && concept_focus_mode) {
		clear_concept_focus_state();
	}

	if (concept_user_node_types.empty()) {
		concept_user_node_types = { E_NODE_TYPE::ACT, E_NODE_TYPE::EFT, E_NODE_TYPE::DEC, E_NODE_TYPE::OBJ };
	}

	const bool focus_graph_mode = concept_focus_mode && !concept_overview_mode;
	if (focus_graph_mode) {
		std::map<QString, ConceptFocusActivation> rebuilt_activations;
		std::vector<QString> rebuilt_order;
		for (const QString& focus_id : concept_focus_order) {
			if (rebuilt_activations.count(focus_id) > 0) continue;
			ConceptFocusActivation activation = make_concept_focus_activation(graph, focus_id);
			if (activation.node_id.isEmpty()) continue;
			rebuilt_activations[focus_id] = activation;
			rebuilt_order.push_back(focus_id);
		}
		concept_focus_activations = rebuilt_activations;
		concept_focus_order = rebuilt_order;
		for (auto iter = concept_focus_parents.begin(); iter != concept_focus_parents.end(); ) {
			if (concept_focus_activations.count(iter->first) == 0 ||
				(!iter->second.isEmpty() && concept_focus_activations.count(iter->second) == 0)) {
				iter = concept_focus_parents.erase(iter);
			}
			else {
				++iter;
			}
		}
	}

	const bool focus_active = focus_graph_mode && !concept_focus_activations.empty();
	std::set<QString> focus_visible_node_ids;
	std::set<QString> focus_visible_edge_ids;
	if (focus_active) {
		for (const auto& item : concept_focus_activations) {
			const ConceptFocusActivation& activation = item.second;
			focus_visible_node_ids.insert(activation.node_id);
			focus_visible_node_ids.insert(activation.inspired_node_ids.begin(), activation.inspired_node_ids.end());
			focus_visible_edge_ids.insert(activation.inspired_edge_ids.begin(), activation.inspired_edge_ids.end());
		}
	}

	std::set<E_EDGE_TYPE> visible_edge_types;
	for (const S_GRAPH_EDGE& edge : graph.edge_list) {
		const auto font_iter = node_map.find(edge.font_node_id);
		const auto back_iter = node_map.find(edge.back_node_id);
		if (font_iter == node_map.end() || back_iter == node_map.end()) continue;
		const bool visible_edge = concept_overview_mode
			|| (focus_active
				? focus_visible_edge_ids.count(edge.id) > 0
				: (concept_user_node_types.count(font_iter->second->type) > 0 &&
					concept_user_node_types.count(back_iter->second->type) > 0));
		if (visible_edge) {
			visible_edge_types.insert(edge.type);
		}
	}

	for (const E_NODE_TYPE type : node_type_order) {
		QVariantMap row;
		row["showChecked"] = focus_active || concept_user_node_types.count(type) > 0;
		row["showEnabled"] = !concept_overview_mode && !focus_graph_mode;
		row["type"] = UtilEnum::trans_node_type_en(type);
		row["name"] = UtilEnum::trans_node_type_en(type);
		row["sourceType"] = QString();
		row["targetType"] = QString();
		row["count"] = node_type_count[type];
		concept_node_type_rows.push_back(row);
	}

	for (const E_EDGE_TYPE type : concept_edge_type_order()) {
		const auto endpoints = concept_edge_node_types(type);
		QVariantMap row;
		row["showChecked"] = visible_edge_types.count(type) > 0;
		row["showEnabled"] = false;
		row["type"] = UtilEnum::trans_edge_type_en(type);
		row["name"] = UtilEnum::trans_edge_type_ch(type);
		row["sourceType"] = UtilEnum::trans_node_type_en(endpoints.first);
		row["targetType"] = UtilEnum::trans_node_type_en(endpoints.second);
		row["count"] = edge_type_count[type];
		concept_edge_type_rows.push_back(row);
	}

	auto add_field_row = [this](const QString& label, const QString& field,
		bool node_checked, bool node_enabled, bool edge_checked, bool edge_enabled) {
		QVariantMap row;
		row["label"] = label;
		row["field"] = field;
		row["nodeChecked"] = node_checked;
		row["nodeEnabled"] = node_enabled && !concept_overview_mode;
		row["edgeChecked"] = edge_checked;
		row["edgeEnabled"] = edge_enabled && !concept_overview_mode;
		concept_field_rows.push_back(row);
	};
	add_field_row(QStringLiteral("ID"), "id", concept_show_node_id, true, concept_show_edge_id, true);
	add_field_row(QStringLiteral("名称"), "name", concept_show_node_name, true, concept_show_edge_name, true);
	add_field_row(QStringLiteral("权重"), "weight", false, false, concept_show_edge_weight, true);

	concept_mission_text = QStringLiteral("Mission 内容");
	for (const S_GRAPH_NODE& node : graph.node_list) {
		if (node.type == E_NODE_TYPE::MIS) {
			concept_mission_text = node.name.isEmpty() ? node.id : node.name;
			break;
		}
	}

    concept_graph_title = concept_overview_mode
		? QStringLiteral("总览导航")
		: (focus_graph_mode ? QStringLiteral("聚焦模式") : QStringLiteral("概念图谱"));
    concept_detail_title = QStringLiteral("属性编辑");
	const bool detail_editable = !concept_config_locked;

	auto node_visual = [](const E_NODE_TYPE type)->QVariantMap {
		QVariantMap visual;
		switch (type) {
		case E_NODE_TYPE::MIS:
			visual["laneColor"] = "#EAF3FB";
			visual["nodeColor"] = "#BFD7F2";
			visual["strokeColor"] = "#4E79A7";
			break;
		case E_NODE_TYPE::TSK:
			visual["laneColor"] = "#E8F6F8";
			visual["nodeColor"] = "#B9E3EA";
			visual["strokeColor"] = "#2F8F9D";
			break;
		case E_NODE_TYPE::ACT:
			visual["laneColor"] = "#EEF9EF";
			visual["nodeColor"] = "#C5EBC8";
			visual["strokeColor"] = "#4B9A55";
			break;
		case E_NODE_TYPE::OBJ:
			visual["laneColor"] = "#ECF8F2";
			visual["nodeColor"] = "#C7EAD9";
			visual["strokeColor"] = "#4E9F7D";
			break;
		case E_NODE_TYPE::COG:
			visual["laneColor"] = "#F1F7EC";
			visual["nodeColor"] = "#D5E6C4";
			visual["strokeColor"] = "#7A9E5B";
			break;
		case E_NODE_TYPE::DEC:
			visual["laneColor"] = "#FCF7E8";
			visual["nodeColor"] = "#F2E2B3";
			visual["strokeColor"] = "#B38B2D";
			break;
		case E_NODE_TYPE::EFT:
			visual["laneColor"] = "#FDF2E8";
			visual["nodeColor"] = "#F4D1AE";
			visual["strokeColor"] = "#C97A2B";
			break;
		case E_NODE_TYPE::LOO:
			visual["laneColor"] = "#FCEEEE";
			visual["nodeColor"] = "#F1C2C2";
			visual["strokeColor"] = "#B85C5C";
			break;
		case E_NODE_TYPE::COA:
			visual["laneColor"] = "#F2EFF8";
			visual["nodeColor"] = "#D5CCE9";
			visual["strokeColor"] = "#7B6AAE";
			break;
		case E_NODE_TYPE::PLT:
		default:
			visual["laneColor"] = "#EEF2F6";
			visual["nodeColor"] = "#CAD5E2";
			visual["strokeColor"] = "#5C728A";
			break;
		}
		visual["textColor"] = "#0f172a";
		return visual;
	};

	if (concept_overview_mode) {
		constexpr double overview_node_w = 150.0;
		constexpr double overview_node_h = 92.0;
		const std::map<E_NODE_TYPE, QPointF> pos_map = {
			{ E_NODE_TYPE::PLT, QPointF(50 ,  34) },
			{ E_NODE_TYPE::MIS, QPointF(732,  34) },
			{ E_NODE_TYPE::TSK, QPointF(50 , 248) },
			{ E_NODE_TYPE::COG, QPointF(352, 248) },
			{ E_NODE_TYPE::OBJ, QPointF(732, 248) },
			{ E_NODE_TYPE::LOO, QPointF(952, 248) },
			{ E_NODE_TYPE::ACT, QPointF(50 , 518) },
			{ E_NODE_TYPE::EFT, QPointF(352, 518) },
			{ E_NODE_TYPE::DEC, QPointF(732, 518) },
			{ E_NODE_TYPE::COA, QPointF(952, 518) },
		};

		std::map<E_NODE_TYPE, QPointF> center_map;
		for (const E_NODE_TYPE type : node_type_order) {
			const QString type_text = UtilEnum::trans_node_type_en(type);
			const QString pos_key = selected_concept_scenario + "|" + type_text;
			const QPointF pos = concept_overview_positions.count(pos_key) > 0
				? concept_overview_positions[pos_key]
				: pos_map.at(type);
			const QVariantMap visual = node_visual(type);
			const QPointF center(pos.x() + overview_node_w / 2.0, pos.y() + overview_node_h / 2.0);
			concept_graph_nodes.push_back(make_concept_graph_node(type_text, type_text,
				QString("%1 / %2").arg(type_text, UtilEnum::trans_node_type_ch(type)),
				QStringLiteral("节点数：%1").arg(node_type_count[type]),
				pos.x(), pos.y(), overview_node_w, overview_node_h,
				visual.value("nodeColor").toString(),
				visual.value("strokeColor").toString(),
				visual.value("textColor").toString(),
				"overview", detail_editable, true));
			center_map[type] = center;
		}

		std::map<std::pair<E_NODE_TYPE, E_NODE_TYPE>, int> overview_edges;
		for (const S_GRAPH_EDGE& edge : graph.edge_list) {
			const auto font_iter = node_map.find(edge.font_node_id);
			const auto back_iter = node_map.find(edge.back_node_id);
			if (font_iter == node_map.end() || back_iter == node_map.end()) continue;
			overview_edges[{ font_iter->second->type, back_iter->second->type }]++;
		}
		for (const auto& item : overview_edges) {
			const QString source_id = UtilEnum::trans_node_type_en(item.first.first);
			const QString target_id = UtilEnum::trans_node_type_en(item.first.second);
			concept_graph_edges.push_back(make_concept_graph_edge(source_id, target_id,
				QString("%1\n%2").arg(UtilEnum::trans_edge_type_ch(UtilEnum::trans_edge_type(item.first.first, item.first.second)),
					QStringLiteral("边数量：%1").arg(item.second)),
				node_visual(item.first.first).value("strokeColor").toString()));
		}
	}
	else {
		constexpr int nodes_per_row = 6;
		constexpr double lane_x = 24.0;
		constexpr double lane_w = 1120.0;
		constexpr double lane_title_w = 108.0;
		constexpr double lane_content_padding = 16.0;
		constexpr double lane_vertical_padding = 8.0;
		constexpr double node_w = 148.0;
		constexpr double node_h = 62.0;
		constexpr double node_start_x = 160.0;
		constexpr double node_start_y = 18.0;
		constexpr double node_step_x = 158.0;
		constexpr double node_step_y = 90.0;
		constexpr double node_min_x = lane_x + lane_title_w + lane_content_padding;
		constexpr double node_max_x = lane_x + lane_w - node_w - lane_content_padding;
		double y = 28.0;
		for (const E_NODE_TYPE type : concept_lane_order) {
			if (!focus_active && concept_user_node_types.count(type) == 0) continue;

			std::vector<const S_GRAPH_NODE*> nodes;
			for (const S_GRAPH_NODE& node : graph.node_list) {
				if (node.type != type) continue;
				if (focus_active && focus_visible_node_ids.count(node.id) == 0) continue;
				nodes.push_back(&node);
			}
			if (focus_active && nodes.empty()) continue;

			const double lane_h = std::max(104.0,
				30.0 + std::ceil(nodes.size() / static_cast<double>(nodes_per_row)) * 96.0);
			QVariantMap lane;
			const QVariantMap visual = node_visual(type);
			lane["laneType"] = UtilEnum::trans_node_type_en(type);
			lane["laneName"] = UtilEnum::trans_node_type_ch(type);
			lane["laneX"] = lane_x;
			lane["laneY"] = y;
			lane["laneW"] = lane_w;
			lane["laneH"] = lane_h;
			lane["laneTitleW"] = lane_title_w;
			lane["laneContentPadding"] = lane_content_padding;
			lane["laneVerticalPadding"] = lane_vertical_padding;
			lane["fillColor"] = visual.value("laneColor");
			lane["borderColor"] = visual.value("strokeColor");
			concept_graph_lanes.push_back(lane);

			for (int index = 0; index < static_cast<int>(nodes.size()); ++index) {
				const S_GRAPH_NODE* node = nodes[index];
				const QString node_key = selected_concept_scenario + "|" + node->id;
				QPointF saved_pos;
				const auto focus_pos_iter = concept_focus_node_positions.find(node_key);
				if (focus_graph_mode && focus_pos_iter != concept_focus_node_positions.end()) {
					saved_pos = focus_pos_iter->second;
				}
				else {
					QPointF relative_pos = concept_node_positions.count(node_key) > 0
						? concept_node_positions[node_key]
						: QPointF(node_start_x + (index % nodes_per_row) * node_step_x,
							node_start_y + (index / nodes_per_row) * node_step_y);
					relative_pos.setX(std::clamp(relative_pos.x(), node_min_x, node_max_x));
					relative_pos.setY(std::clamp(relative_pos.y(), lane_vertical_padding, lane_h - node_h - lane_vertical_padding));
					saved_pos = QPointF(relative_pos.x(), y + relative_pos.y());
				}
				const QVariantMap visual = node_visual(type);
				QStringList lines;
				if (concept_show_node_id) lines << QString("ID: %1").arg(node->id);
				if (concept_show_node_name) lines << node->name;
				const bool editable_node = detail_editable;
				const bool focused_node = focus_graph_mode && concept_focus_activations.count(node->id) > 0;
				concept_graph_nodes.push_back(make_concept_graph_node(node->id, UtilEnum::trans_node_type_en(type),
					node->id, lines.join("\n"),
					saved_pos.x(), saved_pos.y(), node_w, node_h,
					visual.value("nodeColor").toString(),
					visual.value("strokeColor").toString(),
					visual.value("textColor").toString(),
					"node", editable_node, true, focused_node));
			}
			y += lane_h + 22.0;
		}

		for (const S_GRAPH_EDGE& edge : graph.edge_list) {
			const S_GRAPH_NODE* source_node = find_concept_node(graph, edge.font_node_id);
			const S_GRAPH_NODE* target_node = find_concept_node(graph, edge.back_node_id);
			if (source_node == nullptr || target_node == nullptr) continue;
			if (focus_active) {
				if (focus_visible_edge_ids.count(edge.id) == 0) continue;
			}
			else if (concept_user_node_types.count(source_node->type) == 0 ||
				concept_user_node_types.count(target_node->type) == 0) {
				continue;
			}

			QStringList labels;
			if (concept_show_edge_id) labels << edge.id;
			if (concept_show_edge_name) labels << edge.name;
			if (concept_show_edge_weight) labels << QString::number(edge.weight, 'f', 2);
			const QVariantMap visual = source_node != nullptr ? node_visual(source_node->type) : QVariantMap();
			const QString edge_color = visual.value("strokeColor", QStringLiteral("#64748b")).toString();
			concept_graph_edges.push_back(make_concept_graph_edge(edge.font_node_id, edge.back_node_id, labels.join("\n"), edge_color));
		}
	}

	if (concept_detail_mode == "overview" && concept_detail_type != E_NODE_TYPE::UKN) {
		concept_detail_title = QStringLiteral("类型详情");
		concept_overview_type = UtilEnum::trans_node_type_en(concept_detail_type);
		concept_overview_type_name = UtilEnum::trans_node_type_ch(concept_detail_type);
		for (const S_GRAPH_NODE& node : graph.node_list) {
			if (node.type != concept_detail_type) continue;
			QVariantMap row;
			row["id"] = node.id;
			row["nodeId"] = node.id;
			row["name"] = node.name;
			concept_overview_detail_rows.push_back(row);
		}
	}
	else if (concept_detail_mode == "node" && !concept_detail_node_id.isEmpty()) {
		const S_GRAPH_NODE* node = find_concept_node(graph, concept_detail_node_id);
		if (node == nullptr) {
			concept_detail_mode = "empty";
			concept_detail_node_id.clear();
		}
		else {
			concept_detail_title = QStringLiteral("节点详情");
			concept_node_type_line = QString("%1 / %2").arg(UtilEnum::trans_node_type_en(node->type), UtilEnum::trans_node_type_ch(node->type));
			concept_node_id_line = node->id;
			concept_node_name = node->name;

			for (const S_GRAPH_EDGE& edge : graph.edge_list) {
				if (edge.font_node_id != node->id && edge.back_node_id != node->id) continue;
				const bool incoming = edge.back_node_id == node->id;
				const QString other_id = incoming ? edge.font_node_id : edge.back_node_id;
				const S_GRAPH_NODE* other = find_concept_node(graph, other_id);
				if (other == nullptr) continue;

				QVariantMap row;
				row["direction"] = incoming ? QStringLiteral("入边") : QStringLiteral("出边");
				row["edgeId"] = edge.id;
				row["edgeType"] = UtilEnum::trans_edge_type_en(edge.type);
				row["otherType"] = UtilEnum::trans_node_type_en(other->type);
				row["otherNodeId"] = other->id;
				row["edgeName"] = edge.name;
				row["weight"] = QString::number(edge.weight, 'f', 3);
				concept_node_edge_rows.push_back(row);
			}
		}
	}

	if (concept_detail_mode == "empty") {
        concept_detail_title = QStringLiteral("属性编辑");
		concept_overview_type.clear();
		concept_overview_type_name.clear();
		concept_node_type_line.clear();
		concept_node_id_line.clear();
		concept_node_name.clear();
		concept_overview_apply_status = QStringLiteral("无修改");
		concept_node_apply_status = QStringLiteral("无修改");
	}

	if (preserve_detail_draft && detail_draft.mode == concept_detail_mode) {
		const bool same_overview = detail_draft.mode == "overview" && detail_draft.overview_type == concept_detail_type;
		const bool same_node = detail_draft.mode == "node" && detail_draft.node_id == concept_detail_node_id;
		if (same_overview || same_node) {
			restore_concept_detail_draft(detail_draft);
		}
	}

	ModuleGraph::GetInstance()->update_concept_graph(graph);
	if (concept_status.isEmpty()) {
		concept_status = QStringLiteral("读取成功");
	}
	emit conceptChanged();
}

S_CONCEPT_GRAPH* UiICGAS::current_concept_graph()
{
	const auto iter = concept_graph_cache.find(selected_concept_scenario);
	return iter == concept_graph_cache.end() ? nullptr : &iter->second;
}

const S_CONCEPT_GRAPH* UiICGAS::current_concept_graph() const
{
	const auto iter = concept_graph_cache.find(selected_concept_scenario);
	return iter == concept_graph_cache.end() ? nullptr : &iter->second;
}

S_GRAPH_NODE* UiICGAS::find_concept_node(S_CONCEPT_GRAPH& graph, const QString& node_id) const
{
	for (S_GRAPH_NODE& node : graph.node_list) {
		if (node.id == node_id) return &node;
	}
	return nullptr;
}

const S_GRAPH_NODE* UiICGAS::find_concept_node(const S_CONCEPT_GRAPH& graph, const QString& node_id) const
{
	for (const S_GRAPH_NODE& node : graph.node_list) {
		if (node.id == node_id) return &node;
	}
	return nullptr;
}

void UiICGAS::rebuild_concept_edge_index(S_CONCEPT_GRAPH& graph) const
{
	graph.back_edges.clear();
	graph.font_edges.clear();
	for (const S_GRAPH_EDGE& edge : graph.edge_list) {
		graph.back_edges.insert(edge.font_node_id, edge.id);
		graph.font_edges.insert(edge.back_node_id, edge.id);
	}
}

std::vector<E_NODE_TYPE> UiICGAS::concept_node_type_order() const
{
	return {
		E_NODE_TYPE::MIS, E_NODE_TYPE::COG, E_NODE_TYPE::OBJ, E_NODE_TYPE::LOO, E_NODE_TYPE::COA,
		E_NODE_TYPE::DEC, E_NODE_TYPE::EFT, E_NODE_TYPE::ACT, E_NODE_TYPE::TSK, E_NODE_TYPE::PLT
	};
}

std::vector<E_EDGE_TYPE> UiICGAS::concept_edge_type_order() const
{
	return {
		E_EDGE_TYPE::MIS_TO_COG, E_EDGE_TYPE::MIS_TO_OBJ, E_EDGE_TYPE::OBJ_TO_LOO,
		E_EDGE_TYPE::LOO_TO_COA, E_EDGE_TYPE::COG_TO_DEC, E_EDGE_TYPE::OBJ_TO_DEC,
		E_EDGE_TYPE::DEC_TO_DEC, E_EDGE_TYPE::DEC_TO_EFT, E_EDGE_TYPE::EFT_TO_ACT,
		E_EDGE_TYPE::ACT_TO_ACT, E_EDGE_TYPE::ACT_TO_TSK, E_EDGE_TYPE::TSK_TO_PLT
	};
}

std::pair<E_NODE_TYPE, E_NODE_TYPE> UiICGAS::concept_edge_node_types(E_EDGE_TYPE type) const
{
	switch (type) {
	case E_EDGE_TYPE::MIS_TO_COG:	return { E_NODE_TYPE::MIS, E_NODE_TYPE::COG };
	case E_EDGE_TYPE::MIS_TO_OBJ:	return { E_NODE_TYPE::MIS, E_NODE_TYPE::OBJ };
	case E_EDGE_TYPE::OBJ_TO_LOO:	return { E_NODE_TYPE::OBJ, E_NODE_TYPE::LOO };
	case E_EDGE_TYPE::LOO_TO_COA:	return { E_NODE_TYPE::LOO, E_NODE_TYPE::COA };
	case E_EDGE_TYPE::COG_TO_DEC:	return { E_NODE_TYPE::COG, E_NODE_TYPE::DEC };
	case E_EDGE_TYPE::OBJ_TO_DEC:	return { E_NODE_TYPE::OBJ, E_NODE_TYPE::DEC };
	case E_EDGE_TYPE::DEC_TO_DEC:	return { E_NODE_TYPE::DEC, E_NODE_TYPE::DEC };
	case E_EDGE_TYPE::DEC_TO_EFT:	return { E_NODE_TYPE::DEC, E_NODE_TYPE::EFT };
	case E_EDGE_TYPE::EFT_TO_ACT:	return { E_NODE_TYPE::EFT, E_NODE_TYPE::ACT };
	case E_EDGE_TYPE::ACT_TO_ACT:	return { E_NODE_TYPE::ACT, E_NODE_TYPE::ACT };
	case E_EDGE_TYPE::ACT_TO_TSK:	return { E_NODE_TYPE::ACT, E_NODE_TYPE::TSK };
	case E_EDGE_TYPE::TSK_TO_PLT:	return { E_NODE_TYPE::TSK, E_NODE_TYPE::PLT };
	default:						return { E_NODE_TYPE::UKN, E_NODE_TYPE::UKN };
	}
}

void UiICGAS::sync_comm_config_cache()
{
	InfoExpert* expert = InfoExpert::GetInstance();
	if (expert == nullptr) return;

	const QString next_afs_client_addr = afs_client_addr;
	const QString next_afs_client_port = afs_client_port;
	const QString next_afs_server_addr = afs_server_addr;
	const QString next_afs_server_port = afs_server_port;
	const QString next_tac_server_addr = tac_server_addr;
	const QString next_tac_server_port = tac_server_port;
	const QString next_tac_server_user = tac_server_user;
	const QString next_tac_send_iter_ms = tac_send_iter_ms;
	expert->update_comm_config_cache(
		next_afs_client_addr, next_afs_client_port,
		next_afs_server_addr, next_afs_server_port,
		next_tac_server_addr, next_tac_server_port,
		next_tac_server_user, next_tac_send_iter_ms);
}

void UiICGAS::set_workflow_run_state(const QString& state)
{
	const QString next_state = state.trimmed();
	if (workflow_run_state == next_state) return;
	workflow_run_state = next_state;
	emit workflowRunStateChanged();
	sync_workflow_module_states();
}

void UiICGAS::sync_workflow_module_states()
{
	if (workflow_module_states.empty()) {
		init_workflow_module_states();
	}

	update_concept_graph_workflow_state();
	update_mission_workflow_state();
	update_afs_client_workflow_state();
	update_internal_sim_workflow_state();
	set_workflow_module_state("input-replay",
		workflow_replay_mode() ? E_MODULE_RUN_STATE::READY : E_MODULE_RUN_STATE::DISABLED,
		workflow_replay_mode() ? QStringLiteral("回放模式输入已就绪") : QStringLiteral("当前输入源关闭回放输入"));
	set_workflow_module_state("database",
		workflowRunning() ? E_MODULE_RUN_STATE::RUNNING :
		workflowPaused() ? E_MODULE_RUN_STATE::WAITING : E_MODULE_RUN_STATE::READY,
		workflowRunning() ? QStringLiteral("数据库管理运行中") :
		workflowPaused() ? QStringLiteral("流程暂停，数据库内存保留") : QStringLiteral("数据库管理已就绪"));
	set_workflow_module_state("afsim-server",
		afs_server == nullptr ? E_MODULE_RUN_STATE::NOT_READY :
		afs_server_running ? E_MODULE_RUN_STATE::RUNNING : E_MODULE_RUN_STATE::DISABLED,
		afs_server == nullptr ? QStringLiteral("AFSIM服务器模块未实例化") :
		afs_server_running ? QStringLiteral("AFSIM控制出口运行中") : QStringLiteral("AFSIM服务器默认关闭"));
	set_workflow_module_state("tacview-server",
		tac_server == nullptr ? E_MODULE_RUN_STATE::NOT_READY :
		tac_server_running ? E_MODULE_RUN_STATE::RUNNING : E_MODULE_RUN_STATE::READY,
		tac_server == nullptr ? QStringLiteral("Tacview服务器模块未实例化") :
		tac_server_running ? QStringLiteral("Tacview同步服务运行中") : QStringLiteral("Tacview服务器已就绪"));

	recompute_workflow_module_states();
	QByteArray next_signature;
	QDataStream signature_stream(&next_signature, QIODevice::WriteOnly);
	signature_stream << make_workflow_module_state_rows();
	if (next_signature == workflow_module_state_signature) return;
	workflow_module_state_signature = next_signature;
	emit workflowModuleStateChanged();
}

namespace {
const std::vector<QString>& workflow_instance_module_ids()
{
	static const std::vector<QString> ids = {
		QStringLiteral("kg-parse"),
		QStringLiteral("bvr-solver"),
		QStringLiteral("enemy-group"),
		QStringLiteral("friendly-group"),
		QStringLiteral("enemy-intent"),
		QStringLiteral("friendly-intent"),
		QStringLiteral("enemy-threat"),
		QStringLiteral("friendly-threat"),
		QStringLiteral("friendly-cog"),
		QStringLiteral("enemy-cog-template"),
		QStringLiteral("loo"),
		QStringLiteral("behavior-template"),
		QStringLiteral("coa"),
		QStringLiteral("enemy-coa")
	};
	return ids;
}

bool workflow_concept_graph_ready(const S_CONCEPT_GRAPH& graph)
{
	return !graph.scenario_id.trimmed().isEmpty() && !graph.node_list.empty();
}
}

void UiICGAS::init_workflow_module_states()
{
	workflow_module_states.clear();

	auto add_module = [this](const QString& id, const QString& name,
		const E_MODULE_RUN_STATE state = E_MODULE_RUN_STATE::READY)->void {
		S_MODULE_STATE module_state;
		module_state.module_id = id;
		module_state.module_name = name;
		module_state.self_state = state;
		module_state.effective_state = state;
		module_state.update_time_ms = QDateTime::currentMSecsSinceEpoch();
		workflow_module_states[id] = module_state;
	};

	add_module("input-afsim", QStringLiteral("AFSIM客户端"), E_MODULE_RUN_STATE::DISABLED);
	add_module("input-internal-sim", QStringLiteral("内置仿真"));
	add_module("input-replay", QStringLiteral("回放模式"), E_MODULE_RUN_STATE::DISABLED);
	add_module("input-concept-graph", QStringLiteral("概念图谱"));
	add_module("input-command", QStringLiteral("上级指令"), E_MODULE_RUN_STATE::DONE);
	add_module("database", QStringLiteral("数据库管理"));
	add_module("kg-parse", QStringLiteral("概念图谱解析"));
	add_module("bvr-solver", QStringLiteral("超视距解算"));
	add_module("enemy-group", QStringLiteral("敌方目标分群"));
	add_module("friendly-group", QStringLiteral("我方分群识别"));
	add_module("enemy-intent", QStringLiteral("敌方意图分析"));
	add_module("friendly-intent", QStringLiteral("我方意图识别"));
	add_module("enemy-threat", QStringLiteral("我方威胁评估"));
	add_module("friendly-threat", QStringLiteral("我方威胁判断"));
	add_module("friendly-cog", QStringLiteral("我方COG分析"));
	add_module("enemy-cog-template", QStringLiteral("敌方COG模板匹配"));
	add_module("loo", QStringLiteral("LOO作战线生成"));
	add_module("behavior-template", QStringLiteral("我方LOO模板匹配"));
	add_module("coa", QStringLiteral("我方COA生成"));
	add_module("enemy-coa", QStringLiteral("敌方COA生成"));
	add_module("afsim-server", QStringLiteral("AFSIM服务器"));
	add_module("tacview-server", QStringLiteral("Tacview服务器"));

	workflow_module_states["database"].prerequisite_module_ids = { "kg-parse", "bvr-solver" };
	workflow_module_states["database"].prerequisite_any_module_id_groups = { { "input-afsim", "input-internal-sim", "input-replay" } };
	workflow_module_states["kg-parse"].prerequisite_module_ids = { "input-command", "input-concept-graph" };
	workflow_module_states["enemy-group"].prerequisite_module_ids = { "database" };
	workflow_module_states["friendly-group"].prerequisite_module_ids = { "database" };
	workflow_module_states["enemy-intent"].prerequisite_module_ids = { "enemy-group" };
	workflow_module_states["friendly-intent"].prerequisite_module_ids = { "friendly-group" };
	workflow_module_states["enemy-threat"].prerequisite_module_ids = { "enemy-intent", "enemy-group" };
	workflow_module_states["friendly-threat"].prerequisite_module_ids = { "friendly-group", "friendly-intent" };
	workflow_module_states["friendly-cog"].prerequisite_module_ids = { "enemy-threat" };
	workflow_module_states["loo"].prerequisite_module_ids = { "enemy-threat", "friendly-cog" };
	workflow_module_states["enemy-cog-template"].prerequisite_module_ids = { "friendly-threat" };
	workflow_module_states["behavior-template"].prerequisite_module_ids = { "friendly-threat", "enemy-cog-template" };
	workflow_module_states["coa"].prerequisite_module_ids = { "loo" };
	workflow_module_states["enemy-coa"].prerequisite_module_ids = { "behavior-template" };

	update_concept_graph_workflow_state();
	update_mission_workflow_state();
	update_afs_client_workflow_state();
	update_internal_sim_workflow_state();
	recompute_workflow_module_states();
}

void UiICGAS::prepare_workflow_module_states()
{
	if (workflow_module_states.empty()) {
		init_workflow_module_states();
	}

	for (const QString& module_id : workflow_instance_module_ids()) {
		auto iter = workflow_module_states.find(module_id);
		if (iter == workflow_module_states.end()) continue;
		if (iter->second.self_state == E_MODULE_RUN_STATE::DISABLED ||
			iter->second.self_state == E_MODULE_RUN_STATE::RUNNING ||
			iter->second.self_state == E_MODULE_RUN_STATE::DONE) {
			continue;
		}
		iter->second.self_state = E_MODULE_RUN_STATE::NOT_READY;
		iter->second.effective_state = E_MODULE_RUN_STATE::NOT_READY;
		iter->second.detail = QStringLiteral("%1模块初始化中").arg(iter->second.module_name);
		iter->second.update_time_ms = QDateTime::currentMSecsSinceEpoch();
	}
	recompute_workflow_module_states();
}

void UiICGAS::initialize_workflow_module_instances()
{
	(void)ModuleGraph::GetInstance();
	(void)ModuleBvrSolver::GetInstance();
	(void)ModuleEmyGroup::GetInstance();
	(void)ModuleOurGroup::GetInstance();
	(void)ModuleEmyIntent::GetInstance();
	(void)ModuleOurIntent::GetInstance();
	(void)ModuleEmyThreat::GetInstance();
	(void)ModuleOurThreat::GetInstance();
	(void)ModuleOurCOG::GetInstance();
	(void)ModuleEmyCOG::GetInstance();
	(void)ModuleOurLOO::GetInstance();
	(void)ModuleEmyLOO::GetInstance();
	(void)ModuleOurCOA::GetInstance();
	(void)ModuleEmyCOA::GetInstance();
}

void UiICGAS::mark_workflow_modules_ready()
{
	const long long update_time_ms = QDateTime::currentMSecsSinceEpoch();
	for (const QString& module_id : workflow_instance_module_ids()) {
		auto iter = workflow_module_states.find(module_id);
		if (iter == workflow_module_states.end()) continue;
		if (iter->second.self_state == E_MODULE_RUN_STATE::FAILED ||
			iter->second.self_state == E_MODULE_RUN_STATE::RUNNING ||
			iter->second.self_state == E_MODULE_RUN_STATE::DONE ||
			iter->second.self_state == E_MODULE_RUN_STATE::DISABLED) {
			continue;
		}
		iter->second.self_state = E_MODULE_RUN_STATE::READY;
		iter->second.detail = QStringLiteral("%1模块已就绪").arg(iter->second.module_name);
		iter->second.error_text.clear();
		iter->second.update_time_ms = update_time_ms;
	}
	recompute_workflow_module_states();
}

void UiICGAS::update_afs_client_workflow_state()
{
	E_MODULE_RUN_STATE state = E_MODULE_RUN_STATE::READY;
	QString detail = QStringLiteral("AFSIM客户端已就绪");
	QString error_text;

	if (!workflow_afsim_mode()) {
		state = E_MODULE_RUN_STATE::DISABLED;
		detail = QStringLiteral("当前输入源关闭AFSIM客户端");
	}
	else if (workflowPaused()) {
		state = E_MODULE_RUN_STATE::WAITING;
		detail = QStringLiteral("流程暂停，等待继续接收");
	}
	else if (afs_client_config_dirty) {
		state = E_MODULE_RUN_STATE::NOT_READY;
		detail = QStringLiteral("AFSIM客户端配置未保存");
	}
	else if (afs_client_has_error) {
		state = E_MODULE_RUN_STATE::FAILED;
		detail = QStringLiteral("AFSIM客户端报错");
		error_text = afs_client_error_text;
	}
	else if (!workflowRunning()) {
		state = E_MODULE_RUN_STATE::READY;
		detail = QStringLiteral("流程未运行");
	}
	else if (afs_client_received_data) {
		state = E_MODULE_RUN_STATE::RUNNING;
		detail = QStringLiteral("正在接收AFSIM数据");
	}
	else if (afs_client_comm_started) {
		state = E_MODULE_RUN_STATE::WAITING;
		detail = QStringLiteral("已开始连接，等待AFSIM数据");
	}

	set_workflow_module_state("input-afsim", state, detail, error_text);
}

void UiICGAS::update_internal_sim_workflow_state()
{
	E_MODULE_RUN_STATE state = E_MODULE_RUN_STATE::READY;
	QString detail = QStringLiteral("内置仿真已就绪");

	if (!workflow_icgas_mode()) {
		state = E_MODULE_RUN_STATE::DISABLED;
		detail = QStringLiteral("当前输入源关闭内置仿真");
	}
	else if (!sim_config_loaded) {
		state = E_MODULE_RUN_STATE::NOT_READY;
		detail = QStringLiteral("仿真场景配置加载中");
	}
	else if (selected_scenario.trimmed().isEmpty()) {
		state = E_MODULE_RUN_STATE::NOT_READY;
		detail = QStringLiteral("未选择仿真场景");
	}
	else if (const std::vector<S_PLAT_CONFIG>* config_list =
		exact_sim_config_list(sim_plat_config_cache, selected_scenario);
		config_list == nullptr) {
		state = E_MODULE_RUN_STATE::NOT_READY;
		detail = QStringLiteral("当前仿真场景未加载");
	}
	else if (config_list->empty()) {
		state = E_MODULE_RUN_STATE::NOT_READY;
		detail = QStringLiteral("当前仿真场景没有平台");
	}
	else if (workflowPaused()) {
		state = E_MODULE_RUN_STATE::WAITING;
		detail = QStringLiteral("流程暂停，内置仿真保持场景");
	}
	else if (internal_sim_running) {
		state = E_MODULE_RUN_STATE::RUNNING;
		detail = QStringLiteral("内置仿真运行中");
	}

	set_workflow_module_state("input-internal-sim", state, detail);
}

void UiICGAS::update_concept_graph_workflow_state()
{
	const S_CONCEPT_GRAPH* graph = current_concept_graph();
	const bool ready = graph != nullptr && workflow_concept_graph_ready(*graph);
	E_MODULE_RUN_STATE state = ready ? E_MODULE_RUN_STATE::READY : E_MODULE_RUN_STATE::NOT_READY;
	QString detail = ready ? QStringLiteral("概念图谱已就绪") : QStringLiteral("概念图谱为空或解析失败");

	if (ready && workflowRunning()) {
		state = E_MODULE_RUN_STATE::DONE;
		detail = QStringLiteral("流程运行中，配置已确认");
	}
	else if (ready && (conceptDirty() || conceptPendingDetailChanges())) {
		state = E_MODULE_RUN_STATE::WAITING;
		detail = QStringLiteral("存在未保存内容");
	}
	else if (ready && concept_config_confirmed) {
		state = E_MODULE_RUN_STATE::DONE;
		detail = QStringLiteral("配置已确认");
	}

	set_workflow_module_state("input-concept-graph", state, detail);
}

void UiICGAS::update_mission_workflow_state()
{
	if (module_mission == nullptr) {
		set_workflow_module_state("input-command", E_MODULE_RUN_STATE::DONE,
			QStringLiteral("上级指令默认完成"));
		return;
	}

	const S_MISSION mission = module_mission->mission_snapshot();
	const QString command_text = mission.source_text.trimmed();
	if (command_text.isEmpty()) {
		set_workflow_module_state("input-command", E_MODULE_RUN_STATE::DONE,
			QStringLiteral("上级指令默认完成"));
		return;
	}

	const QString mission_id = mission.mission_id.trimmed();
	set_workflow_module_state("input-command", E_MODULE_RUN_STATE::DONE,
		mission_id.isEmpty()
			? QStringLiteral("上级指令内容已输入")
			: QStringLiteral("上级指令内容已输入：%1").arg(mission_id));
}

void UiICGAS::set_workflow_module_state(const QString& module_id, E_MODULE_RUN_STATE state,
	const QString& detail, const QString& error_text)
{
	auto iter = workflow_module_states.find(module_id);
	if (iter == workflow_module_states.end()) {
		S_MODULE_STATE module_state;
		module_state.module_id = module_id;
		module_state.module_name = module_id;
		workflow_module_states[module_id] = module_state;
		iter = workflow_module_states.find(module_id);
	}

	if (iter->second.self_state == state &&
		iter->second.detail == detail &&
		iter->second.error_text == error_text) {
		return;
	}

	iter->second.self_state = state;
	iter->second.effective_state = state;
	iter->second.detail = detail;
	iter->second.error_text = error_text;
	iter->second.update_time_ms = QDateTime::currentMSecsSinceEpoch();
}

QVariantList UiICGAS::make_workflow_module_state_rows() const
{
	QVariantList rows;
	for (const auto& item : workflow_module_states) {
		const S_MODULE_STATE& state = item.second;
		QVariantMap row;
		row["id"] = state.module_id;
		row["type_name"] = state.module_name;
		row["selfState"] = workflow_module_state_key(state.self_state);
		row["parentState"] = workflow_module_state_key(state.parent_state);
		row["effectiveState"] = workflow_module_state_key(state.effective_state);
		row["selfLabel"] = workflow_module_state_label(state.self_state);
		row["parentLabel"] = workflow_module_state_label(state.parent_state);
		row["effectiveLabel"] = workflow_module_state_label(state.effective_state);
		row["label"] = workflow_module_state_label(state.effective_state);
		row["detail"] = state.detail;
		row["errorText"] = state.error_text;
		row["required"] = state.required;
		row["updateTimeMs"] = state.update_time_ms;
		QStringList prerequisites;
		for (const QString& id : state.prerequisite_module_ids) {
			prerequisites.push_back(id);
		}
		for (const auto& any_group : state.prerequisite_any_module_id_groups) {
			for (const QString& id : any_group) {
				prerequisites.push_back(id);
			}
		}
		row["prerequisites"] = prerequisites;
		rows.push_back(row);
	}
	return rows;
}

bool UiICGAS::workflow_module_blocks(const S_MODULE_STATE& state) const
{
	if (!state.required || state.self_state == E_MODULE_RUN_STATE::DISABLED ||
		state.module_id == QStringLiteral("afsim-server")) {
		return false;
	}
	return state.self_state == E_MODULE_RUN_STATE::NOT_READY ||
		state.self_state == E_MODULE_RUN_STATE::UNKNOWN ||
		state.self_state == E_MODULE_RUN_STATE::FAILED ||
		state.effective_state == E_MODULE_RUN_STATE::NOT_READY ||
		state.effective_state == E_MODULE_RUN_STATE::UNKNOWN ||
		state.effective_state == E_MODULE_RUN_STATE::FAILED;
}

QString UiICGAS::first_not_ready_workflow_module_name() const
{
	for (const auto& item : workflow_module_states) {
		const S_MODULE_STATE& state = item.second;
		if (!state.required || state.self_state == E_MODULE_RUN_STATE::DISABLED ||
			state.module_id == QStringLiteral("afsim-server")) {
			continue;
		}
		if (state.self_state == E_MODULE_RUN_STATE::NOT_READY ||
			state.self_state == E_MODULE_RUN_STATE::UNKNOWN ||
			state.self_state == E_MODULE_RUN_STATE::FAILED) {
			return state.module_name.trimmed().isEmpty()
				? state.module_id
				: state.module_name;
		}
	}

	for (const auto& item : workflow_module_states) {
		if (workflow_module_blocks(item.second)) {
			return item.second.module_name.trimmed().isEmpty()
				? item.second.module_id
				: item.second.module_name;
		}
	}
	return QString();
}

QString UiICGAS::workflow_module_state_key(E_MODULE_RUN_STATE state) const
{
	switch (state) {
	case E_MODULE_RUN_STATE::NOT_READY: return "not_ready";
	case E_MODULE_RUN_STATE::READY: return "ready";
	case E_MODULE_RUN_STATE::WAITING: return "wait";
	case E_MODULE_RUN_STATE::RUNNING: return "run";
	case E_MODULE_RUN_STATE::DONE: return "done";
	case E_MODULE_RUN_STATE::FAILED: return "fail";
	case E_MODULE_RUN_STATE::DISABLED: return "disabled";
	default: return "unknown";
	}
}

QString UiICGAS::workflow_module_state_label(E_MODULE_RUN_STATE state) const
{
	switch (state) {
	case E_MODULE_RUN_STATE::NOT_READY: return QStringLiteral("未就绪");
	case E_MODULE_RUN_STATE::READY: return QStringLiteral("已就绪");
	case E_MODULE_RUN_STATE::WAITING: return QStringLiteral("等待中");
	case E_MODULE_RUN_STATE::RUNNING: return QStringLiteral("运行中");
	case E_MODULE_RUN_STATE::DONE: return QStringLiteral("完成");
	case E_MODULE_RUN_STATE::FAILED: return QStringLiteral("失败");
	case E_MODULE_RUN_STATE::DISABLED: return QStringLiteral("关闭");
	default: return QStringLiteral("未知");
	}
}

E_MODULE_RUN_STATE UiICGAS::aggregate_workflow_parent_state(const S_MODULE_STATE& state) const
{
	if (state.prerequisite_module_ids.empty() &&
		state.prerequisite_any_module_id_groups.empty()) {
		return E_MODULE_RUN_STATE::READY;
	}

	bool has_failed = false;
	bool has_unknown = false;
	bool has_not_ready = false;
	bool has_waiting = false;
	bool has_running = false;
	bool has_ready = false;
	bool has_done = false;
	bool all_done = true;

	auto accept_state = [&](const E_MODULE_RUN_STATE prerequisite_state)->void {
		if (prerequisite_state == E_MODULE_RUN_STATE::FAILED) has_failed = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::UNKNOWN) has_unknown = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::NOT_READY ||
			prerequisite_state == E_MODULE_RUN_STATE::DISABLED) has_not_ready = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::WAITING) has_waiting = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::RUNNING) has_running = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::READY) has_ready = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::DONE) has_done = true;

		all_done = all_done && prerequisite_state == E_MODULE_RUN_STATE::DONE;
	};

	for (const QString& prerequisite_id : state.prerequisite_module_ids) {
		const auto prerequisite_iter = workflow_module_states.find(prerequisite_id);
		if (prerequisite_iter == workflow_module_states.end()) {
			accept_state(E_MODULE_RUN_STATE::UNKNOWN);
		}
		else if (prerequisite_iter->second.self_state != E_MODULE_RUN_STATE::DISABLED) {
			accept_state(prerequisite_iter->second.effective_state);
		}
	}
	for (const auto& any_group : state.prerequisite_any_module_id_groups) {
		accept_state(aggregate_workflow_any_group(any_group));
	}

	if (has_failed) return E_MODULE_RUN_STATE::FAILED;
	if (state.dependency_mode == E_MODULE_DEPENDENCY_MODE::ANY_READY) {
		if (has_running) return E_MODULE_RUN_STATE::RUNNING;
		if (has_ready) return E_MODULE_RUN_STATE::READY;
		if (has_done) return E_MODULE_RUN_STATE::DONE;
		if (has_not_ready || has_unknown) return E_MODULE_RUN_STATE::NOT_READY;
		return E_MODULE_RUN_STATE::WAITING;
	}

	if (has_not_ready || has_unknown) return E_MODULE_RUN_STATE::NOT_READY;
	if (has_waiting) return E_MODULE_RUN_STATE::WAITING;
	if (has_running) return E_MODULE_RUN_STATE::RUNNING;
	if (all_done) return E_MODULE_RUN_STATE::DONE;
	return E_MODULE_RUN_STATE::READY;
}

E_MODULE_RUN_STATE UiICGAS::aggregate_workflow_any_group(
	const std::vector<QString>& prerequisite_ids) const
{
	if (prerequisite_ids.empty()) {
		return E_MODULE_RUN_STATE::READY;
	}

	bool has_failed = false;
	bool has_unknown = false;
	bool has_not_ready = false;
	bool has_waiting = false;
	bool has_running = false;
	bool has_ready = false;
	bool has_done = false;

	for (const QString& prerequisite_id : prerequisite_ids) {
		const auto prerequisite_iter = workflow_module_states.find(prerequisite_id);
		const E_MODULE_RUN_STATE prerequisite_state = prerequisite_iter == workflow_module_states.end()
			? E_MODULE_RUN_STATE::UNKNOWN
			: prerequisite_iter->second.effective_state;
		if (prerequisite_iter != workflow_module_states.end() &&
			prerequisite_iter->second.self_state == E_MODULE_RUN_STATE::DISABLED) {
			continue;
		}

		if (prerequisite_state == E_MODULE_RUN_STATE::FAILED) has_failed = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::UNKNOWN) has_unknown = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::NOT_READY ||
			prerequisite_state == E_MODULE_RUN_STATE::DISABLED) has_not_ready = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::WAITING) has_waiting = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::RUNNING) has_running = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::READY) has_ready = true;
		else if (prerequisite_state == E_MODULE_RUN_STATE::DONE) has_done = true;
	}

	if (has_failed) return E_MODULE_RUN_STATE::FAILED;
	if (has_running) return E_MODULE_RUN_STATE::RUNNING;
	if (has_ready) return E_MODULE_RUN_STATE::READY;
	if (has_done) return E_MODULE_RUN_STATE::DONE;
	if (has_not_ready || has_unknown) return E_MODULE_RUN_STATE::NOT_READY;
	return has_waiting ? E_MODULE_RUN_STATE::WAITING : E_MODULE_RUN_STATE::UNKNOWN;
}

E_MODULE_RUN_STATE UiICGAS::combine_workflow_module_state(E_MODULE_RUN_STATE self_state,
	E_MODULE_RUN_STATE parent_state) const
{
	if (self_state == E_MODULE_RUN_STATE::DISABLED) return E_MODULE_RUN_STATE::DISABLED;
	if (self_state == E_MODULE_RUN_STATE::FAILED || parent_state == E_MODULE_RUN_STATE::FAILED) {
		return E_MODULE_RUN_STATE::FAILED;
	}
	if (parent_state == E_MODULE_RUN_STATE::UNKNOWN) {
		parent_state = E_MODULE_RUN_STATE::NOT_READY;
	}

	switch (self_state) {
	case E_MODULE_RUN_STATE::READY:
		return (parent_state == E_MODULE_RUN_STATE::READY || parent_state == E_MODULE_RUN_STATE::DONE)
			? E_MODULE_RUN_STATE::READY : E_MODULE_RUN_STATE::WAITING;
	case E_MODULE_RUN_STATE::NOT_READY:
	case E_MODULE_RUN_STATE::UNKNOWN:
		return E_MODULE_RUN_STATE::NOT_READY;
	case E_MODULE_RUN_STATE::DONE:
		return (parent_state == E_MODULE_RUN_STATE::READY || parent_state == E_MODULE_RUN_STATE::DONE)
			? E_MODULE_RUN_STATE::DONE : E_MODULE_RUN_STATE::WAITING;
	case E_MODULE_RUN_STATE::RUNNING:
		return parent_state == E_MODULE_RUN_STATE::NOT_READY
			? E_MODULE_RUN_STATE::WAITING : E_MODULE_RUN_STATE::RUNNING;
	case E_MODULE_RUN_STATE::WAITING:
		return E_MODULE_RUN_STATE::WAITING;
	default:
		return E_MODULE_RUN_STATE::UNKNOWN;
	}
}

void UiICGAS::recompute_workflow_module_states()
{
	for (auto& item : workflow_module_states) {
		item.second.parent_state = E_MODULE_RUN_STATE::READY;
		item.second.effective_state = item.second.self_state;
	}

	for (int pass = 0; pass < 4; ++pass) {
		for (auto& item : workflow_module_states) {
			S_MODULE_STATE& state = item.second;
			state.parent_state = aggregate_workflow_parent_state(state);
			state.effective_state = combine_workflow_module_state(state.self_state, state.parent_state);
		}
	}
}

bool UiICGAS::start_afs_client_comm()
{
	if (afs_client == nullptr || module_db == nullptr) return false;
	if (!workflow_afsim_mode()) return true;
	if (afs_client_running) return true;

	afs_client_has_error = false;
	afs_client_received_data = false;
	afs_client_error_text.clear();
	if (selected_scenario.trimmed().isEmpty()) {
		if (append_log(QString("[%1] [ERROR] [AfSim Client] AFSim scene not selected")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
			emit logRowsChanged();
		}
		afs_client_has_error = true;
		afs_client_error_text = QStringLiteral("AFSim scene not selected");
		return false;
	}

	sync_comm_config_cache();
	set_module_database_running(true, false);
	afs_client_comm_started = true;
	if (!afs_client->start_afs_client()) {
		afs_client_has_error = true;
		afs_client_error_text = QStringLiteral("AFSIM客户端启动失败");
		afs_client_comm_started = false;
		set_module_database_running(false, false);
		afs_client_received_data = false;
		return false;
	}

	afs_client_running = true;
	return true;
}

bool UiICGAS::start_internal_sim_comm()
{
	if (module_db == nullptr || sim_controller == nullptr) return false;
	if (internal_sim_running) return true;
	if (!sim_config_loaded) {
		refresh_sim_config_cache(true);
	}
	if (selected_scenario.trimmed().isEmpty()) {
		if (append_log(QString("[%1] [ERROR] [ICGAS] Simulation scene not selected")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")))) {
			emit logRowsChanged();
		}
		return false;
	}

	QString scenario_name = selected_scenario.trimmed();
	const std::vector<S_PLAT_CONFIG>* config_list = exact_sim_config_list(sim_plat_config_cache, scenario_name);
	if (config_list == nullptr) {
		const QString fallback_scenario = first_loaded_sim_scenario_id(sim_scenario_name_list, sim_plat_config_cache);
		if (!fallback_scenario.isEmpty()) {
			setSelectedScenario(fallback_scenario);
			scenario_name = fallback_scenario;
			config_list = exact_sim_config_list(sim_plat_config_cache, scenario_name);
		}
	}
	if (config_list == nullptr) {
		if (append_log(QString("[%1] [ERROR] [ICGAS] Simulation scene %2 not loaded")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
			.arg(selected_scenario.trimmed()))) {
			emit logRowsChanged();
		}
		return false;
	}
	if (config_list->empty()) {
		if (append_log(QString("[%1] [ERROR] [ICGAS] Simulation scene %2 has no platforms")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
			.arg(scenario_name))) {
			emit logRowsChanged();
		}
		return false;
	}

	platform_state_cache.clear();
	set_module_database_running(true, false);

	bool started = false;
	SimController* controller = sim_controller;
	QMetaObject::invokeMethod(controller, [controller, scenario_name, &started]() {
		if (controller == nullptr) return;
		started = controller->sim_start(scenario_name);
	}, Qt::BlockingQueuedConnection);
	if (!started) {
		set_module_database_running(false, false);
		if (append_log(QString("[%1] [ERROR] [ICGAS] Simulation scene %2 start failed")
			.arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
			.arg(scenario_name))) {
			emit logRowsChanged();
		}
		return false;
	}

	internal_sim_running = true;
	situation_refresh_dirty = true;
	return true;
}

bool UiICGAS::start_afs_server_comm()
{
	if (afs_server == nullptr) return false;
	if (afs_server_running) return true;

	sync_comm_config_cache();
	if (!afs_server->start_afs_server()) {
		return false;
	}
	afs_server_running = true;
	return true;
}

bool UiICGAS::start_tac_server_comm()
{
	if (tac_server == nullptr) return false;
	if (tac_server_running) return true;

	sync_comm_config_cache();
	if (!tac_server->start_tac_server()) {
		return false;
	}
	tac_server_running = true;
	return true;
}

void UiICGAS::stop_afs_client_comm(bool clear_database)
{
	if (afs_client_running && afs_client != nullptr) {
		afs_client->stop_afs_client();
		afs_client_running = false;
	}
	if (module_db != nullptr) {
		afs_client_comm_started = false;
		afs_client_received_data = false;
		set_module_database_running(false, clear_database);
		if (clear_database) platform_state_cache.clear();
	}
}

void UiICGAS::stop_internal_sim_comm(bool clear_database)
{
	const bool was_running = internal_sim_running;
	internal_sim_running = false;
	if (was_running && sim_controller != nullptr) {
		SimController* controller = sim_controller;
		QMetaObject::invokeMethod(controller, [controller]() {
			if (controller == nullptr) return;
			controller->sim_stop();
		}, Qt::BlockingQueuedConnection);
	}
	if (module_db != nullptr) {
		set_module_database_running(false, clear_database);
	}
	if (clear_database) platform_state_cache.clear();
}

void UiICGAS::stop_afs_server_comm()
{
	if (!afs_server_running || afs_server == nullptr) return;
	afs_server->stop_afs_server();
	afs_server_running = false;
}

void UiICGAS::stop_tac_server_comm()
{
	if (!tac_server_running || tac_server == nullptr) return;
	tac_server->stop_tac_server();
	tac_server_running = false;
}

void UiICGAS::stop_workflow_comm(bool clear_database)
{
	stop_tac_server_comm();
	stop_afs_server_comm();
	stop_afs_client_comm(clear_database);
	stop_internal_sim_comm(clear_database);
}

bool UiICGAS::refresh_detail()
{
	QVariantList new_primary_rows;
	QVariantList new_secondary_rows;
	QVariantList new_relative_rows;

	if (!primary_plat_id.isEmpty()) {
		const auto state_iter = monitor_platform_cache.find(primary_plat_id);
		if (state_iter != monitor_platform_cache.end()) {
			new_primary_rows = make_plat_detail_rows(state_iter->second);
		}
		else if (const auto weapon_iter = monitor_weapon_cache.find(primary_plat_id);
			weapon_iter != monitor_weapon_cache.end()) {
			new_primary_rows = make_weapon_detail_rows(weapon_iter->second);
		}
		else if (const auto format_iter = monitor_format_cache.find(primary_plat_id);
			format_iter != monitor_format_cache.end()) {
			new_primary_rows = make_format_detail_rows(format_iter->second);
		}
		else if (const S_PLAT_CONFIG* config = find_sim_platform_config(primary_plat_id)) {
			new_primary_rows = make_plat_detail_rows(make_sim_plat_state(*config, find_sim_platform_define(*config)));
		}
	}

	if (!secondary_plat_id.isEmpty()) {
		const auto state_iter = monitor_platform_cache.find(secondary_plat_id);
		if (state_iter != monitor_platform_cache.end()) {
			new_secondary_rows = make_plat_detail_rows(state_iter->second);
		}
		else if (const auto weapon_iter = monitor_weapon_cache.find(secondary_plat_id);
			weapon_iter != monitor_weapon_cache.end()) {
			new_secondary_rows = make_weapon_detail_rows(weapon_iter->second);
		}
		else if (const auto format_iter = monitor_format_cache.find(secondary_plat_id);
			format_iter != monitor_format_cache.end()) {
			new_secondary_rows = make_format_detail_rows(format_iter->second);
		}
		else if (const S_PLAT_CONFIG* config = find_sim_platform_config(secondary_plat_id)) {
			new_secondary_rows = make_plat_detail_rows(make_sim_plat_state(*config, find_sim_platform_define(*config)));
		}
	}

	if (!primary_plat_id.isEmpty() && !secondary_plat_id.isEmpty()) {
		new_relative_rows.push_back(make_pair_row(QStringLiteral("状态"), QStringLiteral("相对信息解算已移出ModuleDatabase")));
	}

	const bool detail_changed = primary_rows != new_primary_rows ||
		secondary_rows != new_secondary_rows ||
		relative_rows != new_relative_rows;
	if (detail_changed) {
		primary_rows = std::move(new_primary_rows);
		secondary_rows = std::move(new_secondary_rows);
		relative_rows = std::move(new_relative_rows);
	}
	return detail_changed;
}

bool UiICGAS::append_log(const QString& info)
{
	return update_workflow_pdu_log_row(info);
}

void UiICGAS::init_workflow_pdu_log_rows()
{
	workflow_log_rows.clear();
	for (const int pdu_type : { 1, 2, 3, 23 }) {
		workflow_log_rows.push_back(workflow_pdu_log_line(
			pdu_type, false, QStringLiteral("--:--:--.---"), 0));
		workflow_log_rows.push_back(workflow_pdu_log_line(
			pdu_type, true, QStringLiteral("--:--:--.---"), 0));
	}
}

void UiICGAS::reset_workflow_pdu_log_rows(bool send_side)
{
	if (workflow_log_rows.size() != 8) init_workflow_pdu_log_rows();
	for (const int pdu_type : { 1, 2, 3, 23 }) {
		const int slot = workflow_pdu_log_slot(pdu_type, send_side);
		if (slot >= 0 && slot < workflow_log_rows.size()) {
			workflow_log_rows[slot] = workflow_pdu_log_line(
				pdu_type, send_side, QStringLiteral("--:--:--.---"), 0);
		}
	}
}

bool UiICGAS::update_workflow_pdu_log_row(const QString& info)
{
	if (workflow_log_rows.size() != 8) init_workflow_pdu_log_rows();

	static const QRegularExpression bracket_reg(QStringLiteral("^\\[([^\\]]+)\\] \\[([^\\]]+)\\] \\[([^\\]]+)\\] (.+)$"));
	const QRegularExpressionMatch bracket_match = bracket_reg.match(info);
	if (!bracket_match.hasMatch()) return false;

	const QString stage = bracket_match.captured(2);
	const QString module = bracket_match.captured(3);
	const QString message = bracket_match.captured(4);
	const QString time_text = bracket_match.captured(1);

	if (stage == "START") {
		if (module == "AFS Client" || module == "AfSim Client") {
			reset_workflow_pdu_log_rows(false);
			return true;
		}
		if (module == "AFS Server" || module == "AfSim Server") {
			reset_workflow_pdu_log_rows(true);
			return true;
		}
		return false;
	}

	static const QRegularExpression pdu_reg(QStringLiteral("^DIS PDU-([0-9]+) (recv|drop|send)\\. Num ([0-9]+)\\.$"));
	const QRegularExpressionMatch pdu_match = pdu_reg.match(message);
	if (!pdu_match.hasMatch()) return false;

	const int pdu_type = pdu_match.captured(1).toInt();
	const QString action = pdu_match.captured(2);
	const int count = pdu_match.captured(3).toInt();
	const bool send_side = stage == "SEND" || action == "send";
	if (!send_side && stage != "RECV") return false;

	const int slot = workflow_pdu_log_slot(pdu_type, send_side);
	if (slot < 0 || slot >= workflow_log_rows.size()) return false;
	workflow_log_rows[slot] = QString("[%1] DIS PDU-%2 %3. Num %4.")
		.arg(time_text)
		.arg(pdu_type, 2, 10, QChar('0'))
		.arg(action)
		.arg(count);
	return true;
}

int UiICGAS::workflow_pdu_log_slot(int pdu_type, bool send_side) const
{
	int row = 0;
	for (const int known_type : { 1, 2, 3, 23 }) {
		if (pdu_type == known_type) return row * 2 + (send_side ? 1 : 0);
		row++;
	}
	return -1;
}

QString UiICGAS::workflow_pdu_log_line(int pdu_type, bool send_side,
	const QString& time_text, int count) const
{
	return QString("[%1] DIS PDU-%2 %3. Num %4.")
		.arg(time_text)
		.arg(pdu_type, 2, 10, QChar('0'))
		.arg(send_side ? QStringLiteral("send") : QStringLiteral("recv"))
		.arg(count);
}

const S_PLAT_DEFINE* UiICGAS::find_sim_platform_define(const S_PLAT_CONFIG& config) const
{
	const QString type_name = config.type_name.trimmed();
	if (!type_name.isEmpty()) {
		const auto exact_iter = sim_plat_define_cache.find(type_name);
		if (exact_iter != sim_plat_define_cache.end()) {
			return &exact_iter->second;
		}
		for (const auto& item : sim_plat_define_cache) {
			const S_PLAT_DEFINE& define = item.second;
			if (define.type_name.compare(type_name, Qt::CaseInsensitive) == 0) {
				return &define;
			}
		}
	}

	const QString config_name = config.type_name.trimmed().toUpper();
	const QString platform_id = config.plt_id.trimmed().toUpper();
	auto find_by_type = [&](E_PLAT_TYPE type, E_DOMAIN domain) -> const S_PLAT_DEFINE* {
		for (const auto& item : sim_plat_define_cache) {
			const S_PLAT_DEFINE& define = item.second;
			if (define.type == type && define.domain == domain) return &define;
		}
		return nullptr;
	};
	if (config_name.startsWith(QStringLiteral("F-18E_")) ||
		config_name.startsWith(QStringLiteral("RED_ELC_")) ||
		config_name.startsWith(QStringLiteral("BLUE_ELC_")) ||
		platform_id.startsWith(QStringLiteral("RED_ELC_")) ||
		platform_id.startsWith(QStringLiteral("BLUE_ELC_"))) {
		return find_by_type(E_PLAT_TYPE::ELC, E_DOMAIN::AIR);
	}
	if (config_name.startsWith(QStringLiteral("F-15_")) ||
		config_name.startsWith(QStringLiteral("RED_FLT_")) ||
		config_name.startsWith(QStringLiteral("BLUE_FLT_")) ||
		platform_id.startsWith(QStringLiteral("RED_FLT_")) ||
		platform_id.startsWith(QStringLiteral("BLUE_FLT_"))) {
		return find_by_type(E_PLAT_TYPE::FLT, E_DOMAIN::AIR);
	}
	if (config_name.startsWith(QStringLiteral("E-3_")) ||
		config_name.startsWith(QStringLiteral("RED_WAN_")) ||
		config_name.startsWith(QStringLiteral("BLUE_WAN_")) ||
		platform_id.startsWith(QStringLiteral("RED_WAN_")) ||
		platform_id.startsWith(QStringLiteral("BLUE_WAN_"))) {
		return find_by_type(E_PLAT_TYPE::WAN, E_DOMAIN::AIR);
	}
	if (config_name.startsWith(QStringLiteral("B-52_")) ||
		config_name.startsWith(QStringLiteral("BLUE_BOM_")) ||
		platform_id.startsWith(QStringLiteral("BLUE_BOM_"))) {
		return find_by_type(E_PLAT_TYPE::BOM, E_DOMAIN::AIR);
	}
	if (config_name.startsWith(QStringLiteral("CVN-59_")) ||
		config_name.startsWith(QStringLiteral("RED_CAR_")) ||
		platform_id.startsWith(QStringLiteral("RED_CAR_"))) {
		return find_by_type(E_PLAT_TYPE::CAR, E_DOMAIN::SEA);
	}
	if (config_name.startsWith(QStringLiteral("052D_")) ||
		config_name.startsWith(QStringLiteral("RED_DST_")) ||
		platform_id.startsWith(QStringLiteral("RED_DST_"))) {
		return find_by_type(E_PLAT_TYPE::DST, E_DOMAIN::SEA);
	}
	return nullptr;
}

const S_PLAT_CONFIG* UiICGAS::find_sim_platform_config(const QString& platform_id) const
{
	const QString id = platform_id.trimmed();
	const std::vector<S_PLAT_CONFIG>* selected_config_list = current_sim_config_list(sim_plat_config_cache, selected_scenario);
	if (selected_config_list != nullptr) {
		for (const S_PLAT_CONFIG& config : *selected_config_list) {
			if (config.plt_id == id) return &config;
		}
	}
	for (const auto& scenario_config : sim_plat_config_cache) {
		if (selected_config_list != nullptr && &scenario_config.second == selected_config_list) continue;
		for (const S_PLAT_CONFIG& config : scenario_config.second) {
			if (config.plt_id == id) return &config;
		}
	}
	return nullptr;
}

QString UiICGAS::situation_platform_icon(const S_PLAT& plat) const
{
	const QString type_name = plat.type_name.trimmed();
	if (!type_name.isEmpty()) {
		const auto exact_iter = sim_plat_define_cache.find(type_name);
		if (exact_iter != sim_plat_define_cache.end() &&
			!exact_iter->second.icon_path.trimmed().isEmpty()) {
			return exact_iter->second.icon_path;
		}
		for (const auto& item : sim_plat_define_cache) {
			const S_PLAT_DEFINE& define = item.second;
			if (define.type_name.compare(type_name, Qt::CaseInsensitive) == 0 &&
				!define.icon_path.trimmed().isEmpty()) {
				return define.icon_path;
			}
		}
	}

	if (const S_PLAT_CONFIG* config = find_sim_platform_config(plat.plt_id)) {
		if (const S_PLAT_DEFINE* define = find_sim_platform_define(*config)) {
			if (!define->icon_path.trimmed().isEmpty()) return define->icon_path;
		}
	}

	S_PLAT_CONFIG config;
	config.type_name = plat.type_name;
	config.plt_id = plat.plt_id;
	if (const S_PLAT_DEFINE* define = find_sim_platform_define(config)) {
		if (!define->icon_path.trimmed().isEmpty()) return define->icon_path;
	}

	return plat.type_name;
}

QString UiICGAS::situation_weapon_icon(const S_WEAPON& weapon) const
{
	const QString type_name = weapon.type_name.trimmed();
	if (!type_name.isEmpty()) {
		const auto exact_iter = sim_weapon_define_cache.find(type_name);
		if (exact_iter != sim_weapon_define_cache.end() &&
			!exact_iter->second.icon_path.trimmed().isEmpty()) {
			return exact_iter->second.icon_path;
		}
		for (const auto& item : sim_weapon_define_cache) {
			const S_WEAPON_DEFINE& define = item.second;
			if (define.type_name.compare(type_name, Qt::CaseInsensitive) == 0 &&
				!define.icon_path.trimmed().isEmpty()) {
				return define.icon_path;
			}
		}
	}
	return QStringLiteral("Core.Missile.obj");
}

QString UiICGAS::situation_format_icon(const S_FORMAT& format) const
{
	(void)format;
	return QStringLiteral("Core.Sphere.obj");
}

S_PLAT UiICGAS::make_sim_plat_state(const S_PLAT_CONFIG& config, const S_PLAT_DEFINE* define) const
{
	S_PLAT plat;
	plat.plt_id = config.plt_id;
	plat.fmt_id = config.fmt_id;
	plat.cmd_id = config.cmd_id.trimmed().isEmpty() ? QStringLiteral("SELF") : config.cmd_id.trimmed();
	plat.type_name = define == nullptr ? config.type_name : define->type_name;
	plat.side = config.side;
	plat.type = define == nullptr ? E_PLAT_TYPE::UKN : define->type;
	plat.valid = true;

	S_MOTION_FRAME motion;
	motion.pos_lla = config.init_state.pos_lla;
	motion.vel_loc = config.init_state.vel_loc;
	motion.pos_ecef = UtilCoor::pos_lla2ecef(motion.pos_lla);
	motion.vel_ecef = UtilCoor::vel_loc2ecef(motion.vel_loc, motion.pos_lla);
	motion.update_time_ms = 0;
	plat.list_motion.push_back(motion);
	return plat;
}

QVariantMap UiICGAS::make_initial_platform_type_row(const S_PLAT_DEFINE& define) const
{
	QVariantMap row;
	row["type_id"] = define.type_name;
	row["type_name"] = define.type_name;
	row["icon"] = define.icon_path;
	row["icon_path"] = define.icon_path;
	row["class"] = UtilEnum::trans_type_en(define.type);
	row["type"] = UtilEnum::trans_type_en(define.type);
	row["domain"] = UtilEnum::trans_domain_en(define.domain);
	row["mover"] = UtilEnum::trans_mover_en(define.mover);

	QVariantMap capability;
	QVariantMap cap;
	if (const auto* air = std::get_if<S_AIR_MOVER_CAP>(&define.cap)) {
		capability["min_speed_ms"] = air->min_speed_ms;
		capability["max_speed_ms"] = air->max_speed_ms;
		capability["min_alt_m"] = air->min_alt_m;
		capability["max_alt_m"] = air->max_alt_m;
		capability["max_linear_load_g"] = air->max_linear_load_g;
		capability["max_radial_load_g"] = air->max_radial_load_g;
		capability["max_climb_vel_ms"] = air->max_climb_vel_ms;
		capability["max_drop_vel_ms"] = air->max_drop_vel_ms;
		cap["air_mover"] = capability;
	}
	else if (const auto* sea = std::get_if<S_SEA_MOVER_CAP>(&define.cap)) {
		capability["min_speed_ms"] = sea->min_speed_ms;
		capability["max_speed_ms"] = sea->max_speed_ms;
		capability["max_linear_acc_ms2"] = sea->max_linear_acc_ms2;
		capability["max_radial_acc_ms2"] = sea->max_radial_acc_ms2;
		cap["sea_mover"] = capability;
	}
	else if (const auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define.cap)) {
		capability["min_speed_ms"] = mis->min_speed_ms;
		capability["max_speed_ms"] = mis->max_speed_ms;
		capability["min_alt_m"] = mis->min_alt_m;
		capability["max_alt_m"] = mis->max_alt_m;
		capability["max_linear_load_g"] = mis->max_linear_load_g;
		capability["max_radial_load_g"] = mis->max_radial_load_g;
		capability["max_climb_vel_ms"] = mis->max_climb_vel_ms;
		capability["max_drop_vel_ms"] = mis->max_drop_vel_ms;
		capability["life_range_m"] = mis->life_range_m;
		capability["life_time_s"] = mis->life_time_s;
		cap["mis_mover"] = capability;
	}
	row["capability_boundary"] = capability;
	row["cap"] = cap;

	QVariantMap detection;
	detection["radar_rcs_m2"] = define.sign.radar_rcs_m2;
	detection["opt_area_m2"] = define.sign.opt_area_m2;
	detection["infrared_wsr"] = define.sign.infrared_wsr;
	row["detection_profile"] = detection;
	row["sign"] = detection;
	return row;
}

QVariantMap UiICGAS::make_initial_weapon_type_row(const S_WEAPON_DEFINE& define) const
{
	QVariantMap row;
	row["type_id"] = define.type_name;
	row["type_name"] = define.type_name;
	row["name"] = define.type_name;
	row["icon"] = define.icon_path;
	row["icon_path"] = define.icon_path;
	row["class"] = UtilEnum::trans_weapon_type_en(define.type);
	row["type"] = UtilEnum::trans_weapon_type_en(define.type);
	row["domain"] = UtilEnum::trans_domain_en(define.domain);
	row["mover"] = UtilEnum::trans_mover_en(define.mover);

	QVariantMap launch_cap;
	launch_cap["max_range_m"] = define.launch_cap.max_range_m;
	launch_cap["min_range_m"] = define.launch_cap.min_range_m;
	launch_cap["firing_delay_s"] = define.launch_cap.firing_delay_s;
	row["launch_cap"] = launch_cap;

	QVariantMap mover_cap;
	if (const auto* mis = std::get_if<S_MIS_MOVER_CAP>(&define.mover_cap)) {
		QVariantMap cap;
		cap["min_speed_ms"] = mis->min_speed_ms;
		cap["max_speed_ms"] = mis->max_speed_ms;
		cap["min_alt_m"] = mis->min_alt_m;
		cap["max_alt_m"] = mis->max_alt_m;
		cap["max_linear_load_g"] = mis->max_linear_load_g;
		cap["max_radial_load_g"] = mis->max_radial_load_g;
		cap["max_climb_vel_ms"] = mis->max_climb_vel_ms;
		cap["max_drop_vel_ms"] = mis->max_drop_vel_ms;
		cap["life_range_m"] = mis->life_range_m;
		cap["life_time_s"] = mis->life_time_s;
		mover_cap["mis_mover"] = cap;
	}
	row["mover_cap"] = mover_cap;

	QVariantMap kill_cap;
	kill_cap["kill_range_m"] = define.kill_cap.kill_range_m;
	kill_cap["kill_prob"] = define.kill_cap.kill_prob;
	row["kill_cap"] = kill_cap;

	QVariantMap sign;
	sign["radar_rcs_m2"] = define.sign.radar_rcs_m2;
	sign["opt_area_m2"] = define.sign.opt_area_m2;
	sign["infrared_wsr"] = define.sign.infrared_wsr;
	row["sign"] = sign;
	return row;
}

QVariantMap UiICGAS::make_initial_sensor_type_row(const S_SENSOR_DEFINE& define) const
{
	QVariantMap row;
	row["type_id"] = define.type_name;
	row["type_name"] = define.type_name;
	row["name"] = define.type_name;
	row["class"] = UtilEnum::trans_sensor_type_en(define.type);
	row["type"] = UtilEnum::trans_sensor_type_en(define.type);
	row["reliability"] = define.reliability;

	QStringList domain_list;
	for (const E_DOMAIN domain : define.act_domain_list) {
		const QString domain_text = UtilEnum::trans_domain_en(domain);
		if (domain_text != QStringLiteral("UKN")) domain_list << domain_text;
	}
	row["act_domain_list"] = domain_list.join(QStringLiteral(","));

	double max_range_m = 0.0;
	QVariantMap task_mode_list;
	for (const auto& item : define.task_mode_list) {
		if (item.first == E_SENSOR_TASK::UKN) continue;
		const S_SENSOR_TASK_MODE& mode = item.second;
		max_range_m = std::max(max_range_m, mode.cap_env.max_range_m);

		QVariantMap task_mode;
		QVariantMap base;
		base["max_task_num"] = mode.max_task_num;
		task_mode["base"] = base;

		QVariantMap cap_env;
		cap_env["min_range_m"] = mode.cap_env.min_range_m;
		cap_env["max_range_m"] = mode.cap_env.max_range_m;
		cap_env["az_fov_deg"] = mode.cap_env.az_fov_deg;
		cap_env["min_el_deg"] = mode.cap_env.min_el_deg;
		cap_env["max_el_deg"] = mode.cap_env.max_el_deg;
		cap_env["min_alt_m"] = mode.cap_env.min_alt_m;
		cap_env["max_alt_m"] = mode.cap_env.max_alt_m;
		task_mode["cap_env"] = cap_env;

		QVariantMap cap_prob;
		cap_prob["detection_prob"] = mode.cap_prob.detection_prob;
		cap_prob["false_alarm_prob"] = mode.cap_prob.false_alarm_prob;
		cap_prob["sensitive_slope"] = mode.cap_prob.sensitive_slope;
		task_mode["cap_prob"] = cap_prob;

		QVariantMap cap_acc;
		cap_acc["range_sigma_m"] = mode.cap_acc.range_sigma_m;
		cap_acc["azimuth_sigma_deg"] = mode.cap_acc.azimuth_sigma_deg;
		cap_acc["elevation_sigma_deg"] = mode.cap_acc.elevation_sigma_deg;
		cap_acc["speed_sigma_ms"] = mode.cap_acc.speed_sigma_ms;
		task_mode["cap_acc"] = cap_acc;

		QVariantMap cap_time;
		cap_time["update_period_s"] = mode.cap_time.update_period_s;
		cap_time["delay_ms"] = mode.cap_time.delay_ms;
		task_mode["cap_time"] = cap_time;

		task_mode_list[UtilEnum::trans_sensor_task_en(item.first)] = task_mode;
	}
	row["task_mode_list"] = task_mode_list;
	row["max_range_m"] = max_range_m;
	return row;
}

QVariantMap UiICGAS::make_initial_platform_plot_row(const S_PLAT_CONFIG& config) const
{
	QVariantMap row;
	const S_PLAT_DEFINE* define = find_sim_platform_define(config);
	row[QStringLiteral("platform_id")] = config.plt_id;
	row[QStringLiteral("commander")] = config.cmd_id;
	row[QStringLiteral("side")] = config.side == E_SIDE::RED ? QStringLiteral("red") :
		config.side == E_SIDE::BLUE ? QStringLiteral("blue") :
		config.side == E_SIDE::WHITE ? QStringLiteral("white") : QStringLiteral("unknown");
	row[QStringLiteral("icon")] = define == nullptr ? QString() : define->icon_path;

	QVariantList route_points;
	row[QStringLiteral("lon_deg")] = config.init_state.pos_lla.lon_rad RAD2DEG;
	row[QStringLiteral("lat_deg")] = config.init_state.pos_lla.lat_rad RAD2DEG;
	row[QStringLiteral("altitude_m")] = config.init_state.pos_lla.alt_m;
	row[QStringLiteral("speed_mps")] = config.init_state.vel_loc.speed_ms;
	row[QStringLiteral("heading_deg")] = UtilCoor::norm_rad_0_2pi(config.init_state.vel_loc.track_ang_rad) RAD2DEG;
	row[QStringLiteral("path_angle_deg")] = config.init_state.vel_loc.path_ang_rad RAD2DEG;
	for (const S_ROUTE_POINT& point : config.init_route.point_list) {
		QVariantMap route_point;
		route_point[QStringLiteral("lon_deg")] = point.pos_lla.lon_rad RAD2DEG;
		route_point[QStringLiteral("lat_deg")] = point.pos_lla.lat_rad RAD2DEG;
		route_point[QStringLiteral("altitude_m")] = point.pos_lla.alt_m;
		route_point[QStringLiteral("speed_mps")] = point.speed_ms;
		route_points.push_back(route_point);
	}
	row[QStringLiteral("route_is_circulate")] = config.init_route.is_circulate;
	row[QStringLiteral("route_points")] = route_points;
	double max_sensor_range_m = 0.0;
	for (const auto& load : config.sensor_load) {
		const auto define_iter = sim_sensor_define_cache.find(load.first);
		if (define_iter == sim_sensor_define_cache.end()) continue;
		for (const auto& mode : define_iter->second.task_mode_list) {
			max_sensor_range_m = std::max(max_sensor_range_m, mode.second.cap_env.max_range_m);
		}
	}
	double max_weapon_range_m = 0.0;
	for (const auto& load : config.weapon_load) {
		const auto define_iter = sim_weapon_define_cache.find(load.first);
		if (define_iter == sim_weapon_define_cache.end()) continue;
		max_weapon_range_m = std::max(max_weapon_range_m, define_iter->second.launch_cap.max_range_m);
	}
	row[QStringLiteral("max_sensor_range_m")] = max_sensor_range_m;
	row[QStringLiteral("max_weapon_range_m")] = max_weapon_range_m;
	return row;
}

QVariantMap UiICGAS::make_initial_platform_row(const S_PLAT_CONFIG& config, const S_PLAT_DEFINE* define) const
{
	QVariantMap row;
	row["platform_id"] = config.plt_id;
	row["type_name"] = config.type_name;
	row["type_id"] = define == nullptr ? config.type_name : define->type_name;
	row["commander"] = config.cmd_id;
	row["side"] = config.side == E_SIDE::RED ? QStringLiteral("red") :
		config.side == E_SIDE::BLUE ? QStringLiteral("blue") :
		config.side == E_SIDE::WHITE ? QStringLiteral("white") : QStringLiteral("unknown");
	row["icon"] = define == nullptr ? QString() : define->icon_path;

	QVariantMap position;
	QVariantList route_points;
	position[QStringLiteral("lon_deg")] = config.init_state.pos_lla.lon_rad RAD2DEG;
	position[QStringLiteral("lat_deg")] = config.init_state.pos_lla.lat_rad RAD2DEG;
	position[QStringLiteral("altitude_m")] = config.init_state.pos_lla.alt_m;
	position[QStringLiteral("speed_mps")] = config.init_state.vel_loc.speed_ms;
	position[QStringLiteral("heading_deg")] = UtilCoor::norm_rad_0_2pi(config.init_state.vel_loc.track_ang_rad) RAD2DEG;
	position[QStringLiteral("path_angle_deg")] = config.init_state.vel_loc.path_ang_rad RAD2DEG;
	for (const S_ROUTE_POINT& point : config.init_route.point_list) {
		QVariantMap route_point;
		route_point[QStringLiteral("point_id")] = point.point_id;
		route_point[QStringLiteral("waypoint_id")] = static_cast<int>(route_points.size()) + 1;
		route_point[QStringLiteral("lon_deg")] = point.pos_lla.lon_rad RAD2DEG;
		route_point[QStringLiteral("lat_deg")] = point.pos_lla.lat_rad RAD2DEG;
		route_point[QStringLiteral("altitude_m")] = point.pos_lla.alt_m;
		route_point[QStringLiteral("speed_mps")] = point.speed_ms;
		route_points.push_back(route_point);
	}
	row["initial_position"] = position;
	row["route_is_circulate"] = config.init_route.is_circulate;
	row["route_points"] = route_points;
	row["lon_deg"] = position.value(QStringLiteral("lon_deg"));
	row["lat_deg"] = position.value(QStringLiteral("lat_deg"));
	row["altitude_m"] = position.value(QStringLiteral("altitude_m"));
	row["speed_mps"] = position.value(QStringLiteral("speed_mps"));
	row["heading_deg"] = position.value(QStringLiteral("heading_deg"));
	row["path_angle_deg"] = position.value(QStringLiteral("path_angle_deg"));

	QVariantList mounted_weapons;
	double max_weapon_range_m = 0.0;
	for (const auto& item : config.weapon_load) {
		const QString type_name = item.first;
		QVariantMap mount;
		mount[QStringLiteral("mount_name")] = type_name;
		mount[QStringLiteral("type_id")] = type_name;
		mount[QStringLiteral("quantity")] = item.second;
		mounted_weapons.push_back(mount);
		const auto define_iter = sim_weapon_define_cache.find(type_name);
		if (define_iter != sim_weapon_define_cache.end()) {
			max_weapon_range_m = std::max(max_weapon_range_m, define_iter->second.launch_cap.max_range_m);
		}
	}
	QVariantList mounted_sensors;
	double max_sensor_range_m = 0.0;
	for (const auto& item : config.sensor_load) {
		const QString type_name = item.first;
		QVariantMap mount;
		mount[QStringLiteral("mount_name")] = type_name;
		mount[QStringLiteral("type_id")] = type_name;
		mount[QStringLiteral("quantity")] = item.second;
		mounted_sensors.push_back(mount);
		const auto define_iter = sim_sensor_define_cache.find(type_name);
		if (define_iter != sim_sensor_define_cache.end()) {
			for (const auto& mode : define_iter->second.task_mode_list) {
				max_sensor_range_m = std::max(max_sensor_range_m, mode.second.cap_env.max_range_m);
			}
		}
	}
	row["mounted_weapons"] = mounted_weapons;
	row["mounted_sensors"] = mounted_sensors;
	row["max_sensor_range_m"] = max_sensor_range_m;
	row["max_weapon_range_m"] = max_weapon_range_m;
	row["source_file"] = QStringLiteral("SimController::list_plat_config");
	return row;
}

QVariantMap UiICGAS::make_pair_row(const QString& name, const QString& value) const
{
	QVariantMap row;
	row["name"] = name;
	row["value"] = value;
	return row;
}

QVariantMap UiICGAS::make_plat_row(const S_PLAT& plat) const
{
	QVariantMap row;
	const S_MOTION_FRAME motion_default{};
	const S_MOTION_FRAME* motion_ptr = latest_motion(plat);
	const S_MOTION_FRAME& motion = motion_ptr == nullptr ? motion_default : *motion_ptr;
	const double lon_deg = show_zero_num(motion.pos_lla.lon_rad RAD2DEG);
	const double lat_deg = show_zero_num(motion.pos_lla.lat_rad RAD2DEG);
	const double alt_m = show_zero_num(motion.pos_lla.alt_m);
	const double theta_deg = show_zero_num(UtilCoor::norm_rad_0_2pi(motion.vel_loc.track_ang_rad) RAD2DEG);
	const double gamma_deg = show_zero_num(motion.vel_loc.path_ang_rad RAD2DEG);

	row["platId"] = plat.plt_id;
	row["displayId"] = plat.plt_id;
	const QString commander_id = plat.cmd_id.trimmed().isEmpty() ? QStringLiteral("SELF") : plat.cmd_id.trimmed();
	row["cmdId"] = commander_id;
	row["fmtId"] = plat.fmt_id;
	row["memberText"] = QString();
	row["sideKey"] = side_map_key(plat.side);
	row["sideText"] = UtilEnum::trans_side_ch(plat.side);
	row["entityKind"] = QStringLiteral("platform");
	row["entityKindText"] = QStringLiteral("平台");
	row["type"] = UtilEnum::trans_type_ch(plat.type);
	row["typeName"] = plat.type_name;
	row["lonDeg"] = numberText(lon_deg, 6);
	row["latDeg"] = numberText(lat_deg, 6);
	row["altM"] = numberText(alt_m, 1);
	row["speedMS"] = numberText(show_zero_num(motion.vel_loc.speed_ms), 1);
	row["thetaDeg"] = numberText(theta_deg, 1);
	row["gammaDeg"] = numberText(gamma_deg, 1);
	row["updateTime"] = timeText(motion.update_time_ms);
	append_threat_fields(row, plat.algo_threat);
	row[QStringLiteral("platform_id")] = plat.plt_id;
	row[QStringLiteral("commander")] = commander_id;
	row[QStringLiteral("side")] = side_map_key(plat.side);
	row[QStringLiteral("icon")] = situation_platform_icon(plat);
	row[QStringLiteral("entity_kind")] = QStringLiteral("platform");
	row[QStringLiteral("lon_deg")] = lon_deg;
	row[QStringLiteral("lat_deg")] = lat_deg;
	row[QStringLiteral("altitude_m")] = alt_m;
	row[QStringLiteral("heading_deg")] = theta_deg;
	row[QStringLiteral("path_angle_deg")] = gamma_deg;
	row[QStringLiteral("max_sensor_range_m")] = 0.0;
	row[QStringLiteral("max_weapon_range_m")] = 0.0;
	return row;
}

QVariantMap UiICGAS::make_weapon_row(const S_WEAPON& weapon) const
{
	QVariantMap row;
	const S_MOTION_FRAME motion_default{};
	const S_MOTION_FRAME* motion_ptr = latest_motion(weapon);
	const S_MOTION_FRAME& motion = motion_ptr == nullptr ? motion_default : *motion_ptr;
	const double lon_deg = show_zero_num(motion.pos_lla.lon_rad RAD2DEG);
	const double lat_deg = show_zero_num(motion.pos_lla.lat_rad RAD2DEG);
	const double alt_m = show_zero_num(motion.pos_lla.alt_m);
	const double theta_deg = show_zero_num(UtilCoor::norm_rad_0_2pi(motion.vel_loc.track_ang_rad) RAD2DEG);
	const double gamma_deg = show_zero_num(motion.vel_loc.path_ang_rad RAD2DEG);

	row["platId"] = weapon.weapon_id;
	row["displayId"] = weapon.weapon_id;
	row["cmdId"] = weapon.own_plat_id;
	row["fmtId"] = QString();
	row["memberText"] = QString();
	row["side"] = side_map_key(weapon.side);
	row["sideKey"] = side_map_key(weapon.side);
	row["sideText"] = UtilEnum::trans_side_ch(weapon.side);
	row["entityKind"] = QStringLiteral("weapon");
	row["entityKindText"] = QStringLiteral("武器");
	row["type"] = weapon_type_text(weapon.type);
	row["typeName"] = weapon.type_name;
	row["lonDeg"] = numberText(lon_deg, 6);
	row["latDeg"] = numberText(lat_deg, 6);
	row["altM"] = numberText(alt_m, 1);
	row["speedMS"] = numberText(show_zero_num(motion.vel_loc.speed_ms), 1);
	row["thetaDeg"] = numberText(theta_deg, 1);
	row["gammaDeg"] = numberText(gamma_deg, 1);
	row["updateTime"] = timeText(motion.update_time_ms);
	append_threat_fields(row, weapon.algo_threat);
	row[QStringLiteral("platform_id")] = weapon.weapon_id;
	row[QStringLiteral("commander")] = weapon.own_plat_id;
	row[QStringLiteral("icon")] = situation_weapon_icon(weapon);
	row[QStringLiteral("entity_kind")] = QStringLiteral("weapon");
	row[QStringLiteral("lon_deg")] = lon_deg;
	row[QStringLiteral("lat_deg")] = lat_deg;
	row[QStringLiteral("altitude_m")] = alt_m;
	row[QStringLiteral("heading_deg")] = theta_deg;
	row[QStringLiteral("path_angle_deg")] = gamma_deg;
	row[QStringLiteral("max_sensor_range_m")] = 0.0;
	row[QStringLiteral("max_weapon_range_m")] = 0.0;
	return row;
}

QVariantMap UiICGAS::make_format_row(const S_FORMAT& format) const
{
	QVariantMap row;
	const S_MOTION_FRAME motion_default{};
	const S_MOTION_FRAME* motion_ptr = latest_motion(format);
	const S_MOTION_FRAME& motion = motion_ptr == nullptr ? motion_default : *motion_ptr;
	const double lon_deg = show_zero_num(motion.pos_lla.lon_rad RAD2DEG);
	const double lat_deg = show_zero_num(motion.pos_lla.lat_rad RAD2DEG);
	const double alt_m = show_zero_num(motion.pos_lla.alt_m);
	const double theta_deg = show_zero_num(UtilCoor::norm_rad_0_2pi(motion.vel_loc.track_ang_rad) RAD2DEG);
	const double gamma_deg = show_zero_num(motion.vel_loc.path_ang_rad RAD2DEG);
	const double fmt_range_m = std::max(0.0, show_zero_num(format.fmt_range_m));
	const double formation_range_m = fmt_range_m + FMT_OUT_RANGE_M;
	const QString monitor_id = format_monitor_id(format);
	const QString member_text = format_member_text(format);

	row["platId"] = monitor_id;
	row["displayId"] = format.fmt_id;
	row["cmdId"] = QStringLiteral("SELF");
	row["fmtId"] = format.fmt_id;
	row["memberText"] = member_text;
	row["sideKey"] = side_map_key(format.side);
	row["sideText"] = UtilEnum::trans_side_ch(format.side);
	row["entityKind"] = QStringLiteral("format");
	row["entityKindText"] = QStringLiteral("编队");
	row["type"] = QStringLiteral("编队");
	row["typeName"] = QString();
	row["lonDeg"] = numberText(lon_deg, 6);
	row["latDeg"] = numberText(lat_deg, 6);
	row["altM"] = numberText(alt_m, 1);
	row["speedMS"] = numberText(show_zero_num(motion.vel_loc.speed_ms), 1);
	row["thetaDeg"] = numberText(theta_deg, 1);
	row["gammaDeg"] = numberText(gamma_deg, 1);
	row["updateTime"] = timeText(motion.update_time_ms);
	append_threat_fields(row, format.algo_threat);
	append_cog_fields(row, format.algo_cog);
	row[QStringLiteral("fmtRangeText")] = numberText(fmt_range_m, 1);
	row[QStringLiteral("formationRangeText")] = numberText(formation_range_m, 1);
	row[QStringLiteral("platform_id")] = monitor_id;
	row[QStringLiteral("commander")] = QStringLiteral("SELF");
	row[QStringLiteral("side")] = side_map_key(format.side);
	row[QStringLiteral("icon")] = situation_format_icon(format);
	row[QStringLiteral("entity_kind")] = QStringLiteral("format");
	row[QStringLiteral("lon_deg")] = lon_deg;
	row[QStringLiteral("lat_deg")] = lat_deg;
	row[QStringLiteral("altitude_m")] = alt_m;
	row[QStringLiteral("heading_deg")] = theta_deg;
	row[QStringLiteral("path_angle_deg")] = gamma_deg;
	row[QStringLiteral("fmt_range_m")] = fmt_range_m;
	row[QStringLiteral("formation_range_m")] = formation_range_m;
	row[QStringLiteral("max_sensor_range_m")] = 0.0;
	row[QStringLiteral("max_weapon_range_m")] = 0.0;
	return row;
}

QVariantList UiICGAS::make_plat_detail_rows(const S_PLAT& plat) const
{
	QVariantList rows;
	const S_MOTION_FRAME motion_default{};
	const S_MOTION_FRAME* motion_ptr = latest_motion(plat);
	const S_MOTION_FRAME& motion = motion_ptr == nullptr ? motion_default : *motion_ptr;

	rows.push_back(make_pair_row(QStringLiteral("平台ID"), plat.plt_id));
	rows.push_back(make_pair_row(QStringLiteral("编队ID"), plat.fmt_id));
	rows.push_back(make_pair_row(QStringLiteral("指挥关系"), plat.cmd_id.trimmed().isEmpty() ? QStringLiteral("SELF") : plat.cmd_id.trimmed()));
	rows.push_back(make_pair_row(QStringLiteral("阵营"), UtilEnum::trans_side_ch(plat.side)));
	rows.push_back(make_pair_row(QStringLiteral("类型"), UtilEnum::trans_type_ch(plat.type)));
	rows.push_back(make_pair_row(QStringLiteral("威胁值"), threat_value_text(plat.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("威胁等级"), threat_level_text(plat.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("威胁排序"), threat_sort_text(plat.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("经度(deg)"), numberText(motion.pos_lla.lon_rad RAD2DEG, 6)));
	rows.push_back(make_pair_row(QStringLiteral("纬度(deg)"), numberText(motion.pos_lla.lat_rad RAD2DEG, 6)));
	rows.push_back(make_pair_row(QStringLiteral("高度(m)"), numberText(motion.pos_lla.alt_m, 1)));
	rows.push_back(make_pair_row(QStringLiteral("速度(m/s)"), numberText(motion.vel_loc.speed_ms, 1)));
	rows.push_back(make_pair_row(QStringLiteral("航向角(deg)"), numberText(UtilCoor::norm_rad_0_2pi(motion.vel_loc.track_ang_rad) RAD2DEG, 1)));
	rows.push_back(make_pair_row(QStringLiteral("俯仰角(deg)"), numberText(motion.vel_loc.path_ang_rad RAD2DEG, 1)));
	rows.push_back(make_pair_row(QStringLiteral("更新时间"), timeText(motion.update_time_ms)));
	return rows;
}

QVariantList UiICGAS::make_weapon_detail_rows(const S_WEAPON& weapon) const
{
	QVariantList rows;
	const S_MOTION_FRAME motion_default{};
	const S_MOTION_FRAME* motion_ptr = latest_motion(weapon);
	const S_MOTION_FRAME& motion = motion_ptr == nullptr ? motion_default : *motion_ptr;

	rows.push_back(make_pair_row(QStringLiteral("武器ID"), weapon.weapon_id));
	rows.push_back(make_pair_row(QStringLiteral("发射平台"), weapon.own_plat_id));
	rows.push_back(make_pair_row(QStringLiteral("目标平台"), weapon.tgt_plat_id));
	rows.push_back(make_pair_row(QStringLiteral("阵营"), UtilEnum::trans_side_ch(weapon.side)));
	rows.push_back(make_pair_row(QStringLiteral("类型"), weapon_type_text(weapon.type)));
	rows.push_back(make_pair_row(QStringLiteral("型号"), weapon.type_name));
	rows.push_back(make_pair_row(QStringLiteral("威胁值"), threat_value_text(weapon.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("威胁等级"), threat_level_text(weapon.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("威胁排序"), threat_sort_text(weapon.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("经度(deg)"), numberText(motion.pos_lla.lon_rad RAD2DEG, 6)));
	rows.push_back(make_pair_row(QStringLiteral("纬度(deg)"), numberText(motion.pos_lla.lat_rad RAD2DEG, 6)));
	rows.push_back(make_pair_row(QStringLiteral("高度(m)"), numberText(motion.pos_lla.alt_m, 1)));
	rows.push_back(make_pair_row(QStringLiteral("速度(m/s)"), numberText(motion.vel_loc.speed_ms, 1)));
	rows.push_back(make_pair_row(QStringLiteral("航向角(deg)"), numberText(UtilCoor::norm_rad_0_2pi(motion.vel_loc.track_ang_rad) RAD2DEG, 1)));
	rows.push_back(make_pair_row(QStringLiteral("俯仰角(deg)"), numberText(motion.vel_loc.path_ang_rad RAD2DEG, 1)));
	rows.push_back(make_pair_row(QStringLiteral("更新时间"), timeText(motion.update_time_ms)));
	return rows;
}

QVariantList UiICGAS::make_format_detail_rows(const S_FORMAT& format) const
{
	QVariantList rows;
	const S_MOTION_FRAME motion_default{};
	const S_MOTION_FRAME* motion_ptr = latest_motion(format);
	const S_MOTION_FRAME& motion = motion_ptr == nullptr ? motion_default : *motion_ptr;
	const QString member_text = format_member_text(format);

	rows.push_back(make_pair_row(QStringLiteral("编队ID"), format.fmt_id));
	rows.push_back(make_pair_row(QStringLiteral("成员数量"), QString::number(static_cast<int>(format.plat_id_list.size()))));
	rows.push_back(make_pair_row(QStringLiteral("成员平台"), member_text.isEmpty() ? QStringLiteral("--") : member_text));
	rows.push_back(make_pair_row(QStringLiteral("编队半径(m)"), numberText(format.fmt_range_m, 1)));
	rows.push_back(make_pair_row(QStringLiteral("显示半径(m)"), numberText(std::max(0.0, format.fmt_range_m) + FMT_OUT_RANGE_M, 1)));
	rows.push_back(make_pair_row(QStringLiteral("阵营"), UtilEnum::trans_side_ch(format.side)));
	rows.push_back(make_pair_row(QStringLiteral("威胁值"), threat_value_text(format.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("威胁等级"), threat_level_text(format.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("威胁排序"), threat_sort_text(format.algo_threat)));
	rows.push_back(make_pair_row(QStringLiteral("COG值"), cog_value_text(format.algo_cog)));
	rows.push_back(make_pair_row(QStringLiteral("COG等级"), cog_level_text(format.algo_cog)));
	rows.push_back(make_pair_row(QStringLiteral("COG排序"), cog_sort_text(format.algo_cog)));
	rows.push_back(make_pair_row(QStringLiteral("经度(deg)"), numberText(motion.pos_lla.lon_rad RAD2DEG, 6)));
	rows.push_back(make_pair_row(QStringLiteral("纬度(deg)"), numberText(motion.pos_lla.lat_rad RAD2DEG, 6)));
	rows.push_back(make_pair_row(QStringLiteral("高度(m)"), numberText(motion.pos_lla.alt_m, 1)));
	rows.push_back(make_pair_row(QStringLiteral("速度(m/s)"), numberText(motion.vel_loc.speed_ms, 1)));
	rows.push_back(make_pair_row(QStringLiteral("航向角(deg)"), numberText(UtilCoor::norm_rad_0_2pi(motion.vel_loc.track_ang_rad) RAD2DEG, 1)));
	rows.push_back(make_pair_row(QStringLiteral("俯仰角(deg)"), numberText(motion.vel_loc.path_ang_rad RAD2DEG, 1)));
	rows.push_back(make_pair_row(QStringLiteral("更新时间"), timeText(motion.update_time_ms)));
	return rows;
}

bool UiICGAS::validate_concept_graph(const S_CONCEPT_GRAPH& graph, QString* error_text) const
{
	if (graph.node_list.empty()) {
		if (error_text != nullptr) *error_text = QStringLiteral("节点为空");
		return false;
	}

	std::set<QString> node_ids;
	for (const S_GRAPH_NODE& node : graph.node_list) {
		const QString prefix = UtilEnum::trans_node_type_prefix(node.type);
		const QRegularExpression reg(QString("^%1-[A-Za-z0-9_]+$").arg(prefix));
		if (node.id.trimmed().isEmpty() || !reg.match(node.id).hasMatch()) {
			if (error_text != nullptr) *error_text = QStringLiteral("节点ID格式错误：%1").arg(node.id);
			return false;
		}
		if (node_ids.count(node.id) > 0) {
			if (error_text != nullptr) *error_text = QStringLiteral("节点ID重复：%1").arg(node.id);
			return false;
		}
		node_ids.insert(node.id);
	}

	std::set<QString> edge_ids;
	for (const S_GRAPH_EDGE& edge : graph.edge_list) {
		if (edge.id.trimmed().isEmpty() || !QRegularExpression("^EDGE-[0-9]+$").match(edge.id).hasMatch()) {
			if (error_text != nullptr) *error_text = QStringLiteral("边ID格式错误：%1").arg(edge.id);
			return false;
		}
		if (edge_ids.count(edge.id) > 0) {
			if (error_text != nullptr) *error_text = QStringLiteral("边ID重复：%1").arg(edge.id);
			return false;
		}
		if (node_ids.count(edge.font_node_id) == 0 || node_ids.count(edge.back_node_id) == 0) {
			if (error_text != nullptr) *error_text = QStringLiteral("边连接节点不存在：%1").arg(edge.id);
			return false;
		}

		const S_GRAPH_NODE* font_node = find_concept_node(graph, edge.font_node_id);
		const S_GRAPH_NODE* back_node = find_concept_node(graph, edge.back_node_id);
		if (font_node == nullptr || back_node == nullptr ||
			UtilEnum::trans_edge_type(font_node->type, back_node->type) != edge.type) {
			if (error_text != nullptr) *error_text = QStringLiteral("边类型不匹配：%1").arg(edge.id);
			return false;
		}
		edge_ids.insert(edge.id);
	}

	if (error_text != nullptr) *error_text = QStringLiteral("校验通过");
	return true;
}

QString UiICGAS::make_concept_edge_id(const S_CONCEPT_GRAPH& graph) const
{
	int max_index = 0;
	for (const S_GRAPH_EDGE& edge : graph.edge_list) {
		if (edge.id.startsWith("EDGE-")) {
			max_index = std::max(max_index, edge.id.mid(5).toInt());
		}
	}
	for (const QVariant& value : concept_node_edge_rows) {
		const QString id = value.toMap().value("edgeId").toString();
		if (id.startsWith("EDGE-")) {
			max_index = std::max(max_index, id.mid(5).toInt());
		}
	}
	return QString("EDGE-%1").arg(max_index + 1, 3, 10, QChar('0'));
}

int UiICGAS::concept_lane_order_index(const E_NODE_TYPE type) const
{
	const auto iter = std::find(concept_lane_order.begin(), concept_lane_order.end(), type);
	return iter == concept_lane_order.end() ? -1 : static_cast<int>(std::distance(concept_lane_order.begin(), iter));
}

double UiICGAS::concept_graph_lane_height(const S_CONCEPT_GRAPH& graph, E_NODE_TYPE type) const
{
	int node_count = 0;
	for (const S_GRAPH_NODE& node : graph.node_list) {
		if (node.type == type) ++node_count;
	}
	constexpr int nodes_per_row = 6;
	return std::max(104.0, 30.0 + std::ceil(node_count / static_cast<double>(nodes_per_row)) * 96.0);
}

double UiICGAS::concept_graph_lane_y(const S_CONCEPT_GRAPH& graph, E_NODE_TYPE type) const
{
	double y = 28.0;
	for (const E_NODE_TYPE lane_type : concept_lane_order) {
		if (concept_user_node_types.count(lane_type) == 0) continue;
		if (lane_type == type) return y;
		y += concept_graph_lane_height(graph, lane_type) + 22.0;
	}
	return -1.0;
}

QVariantMap UiICGAS::make_concept_graph_node(const QString& id, const QString& type, const QString& title,
	const QString& name, double x, double y, double w, double h, const QString& fill_color,
	const QString& border_color, const QString& text_color, const QString& kind, bool editable,
	bool draggable, bool focus_active) const
{
	QVariantMap row;
	row["nodeId"] = id;
	row["nodeType"] = type;
	row["titleText"] = title;
	row["bodyText"] = name;
	row["nodeX"] = x;
	row["nodeY"] = y;
	row["nodeW"] = w;
	row["nodeH"] = h;
	row["fillColor"] = fill_color;
	row["borderColor"] = border_color;
	row["textColor"] = text_color;
	row["kind"] = kind;
	row["editable"] = editable;
	row["draggable"] = draggable;
	row["focusActive"] = focus_active;
	return row;
}

QVariantMap UiICGAS::make_concept_graph_edge(const QString& source_id, const QString& target_id,
	const QString& label, const QString& color) const
{
	QVariantMap row;
	row["sourceId"] = source_id;
	row["targetId"] = target_id;
	row["label"] = label;
	row["lineColor"] = color;
	return row;
}

QString UiICGAS::numberText(double value, int precision) const
{
	return QString::number(show_zero_num(value), 'f', std::min(precision, 4));
}

QString UiICGAS::timeText(const long long time_ms) const
{
	if (time_ms <= 0) return QStringLiteral("--");
	return QDateTime::fromMSecsSinceEpoch(time_ms).toString("hh:mm:ss.zzz");
}

double UiICGAS::show_zero_num(double value) const
{
	return std::abs(value) < 1e-8 ? 0.0 : value;
}
