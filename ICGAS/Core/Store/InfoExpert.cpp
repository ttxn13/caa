#include "pch.hpp"
#include "InfoExpert.hpp"
#include "../Util/UtilEnum.hpp"
#include "../Util/UtilJson.hpp"

#include <QCoreApplication>

QMutex InfoExpert::mutex;
InfoExpert* InfoExpert::instance = NULL;

InfoExpert* InfoExpert::GetInstance()
{
	if (instance == NULL) {
		mutex.lock();
		instance = new InfoExpert();
		mutex.unlock();
	}
	return instance;
}

void InfoExpert::DstInstance()
{
	delete instance;
	instance = NULL;
}

InfoExpert::InfoExpert()
{
	init_config_common();
	init_concept_graph_config();
}

InfoExpert::~InfoExpert()
{
}

// 初始化基本配置信息
void InfoExpert::init_config_common()
{
	login_info.clear();
	admin_mode		= 0;
	embed_mode		= 0;
	database_mode	= 1;
	sim_iter_ms		= 40;
	afs_recv_iter_ms = afs_send_iter_ms = 100;
	ui_sync_iter_ms = 500;

	const QString name_config_comm = "Config/ConfigComm.json";
	QJsonObject config_common;
	if (!UtilJson::read_json_object_file(name_config_comm, config_common)) return;

	// ---------- Admin参数 ----------
	const QJsonObject admin_obj = config_common.value("ADMIN").toObject();
	this->admin_mode	= UtilJson::json_text(admin_obj, "admin_mode"		, "0"	).toInt();
	this->embed_mode	= UtilJson::json_text(admin_obj, "embed_mode"		, "0"	).toInt();
	this->database_mode = UtilJson::json_text(admin_obj, "database_mode"	, "1"	).toInt();
	this->sim_iter_ms	= UtilJson::json_text(admin_obj, "sim_iter_ms"		, "40"	).toInt();

	// ---------- 用户登录信息 ----------
	const QJsonArray log_info_list = config_common.value("LOG_INFO").toArray();
	for (const QJsonValue& log_info_value : log_info_list) {
		const QJsonObject log_info_item = log_info_value.toObject();
		const QString user = UtilJson::json_text(log_info_item, "user");
		const QString pass = UtilJson::json_text(log_info_item, "pass");
		if (!user.isEmpty()) {
			this->login_info[user] = pass;
		}
	}

	// ---------- 通信配置信息 ----------
	const QJsonObject socket_obj = config_common.value("SOCKET").toObject();
	const QJsonObject afs_client_socket_item = socket_obj.value("afsim_client").toObject();
	this->afs_client_addr = UtilJson::json_text(afs_client_socket_item, "addr");
	this->afs_client_port = UtilJson::json_text(afs_client_socket_item, "port");
	const QJsonObject afs_server_socket_item = socket_obj.value("afsim_server").toObject();
	this->afs_server_addr = UtilJson::json_text(afs_server_socket_item, "addr");
	this->afs_server_port = UtilJson::json_text(afs_server_socket_item, "port");
	const QJsonObject tac_server_socket_item = socket_obj.value("tacview_server").toObject();
	this->tac_server_addr = UtilJson::json_text(tac_server_socket_item, "addr");
	this->tac_server_port = UtilJson::json_text(tac_server_socket_item, "port");
	this->tac_server_user = UtilJson::json_text(tac_server_socket_item, "user");

	// ---------- 线程刷新频率 ----------
	const QJsonObject thread_iter_obj = config_common.value("THREAD_ITER").toObject();
	const QJsonObject afs_iter_item = thread_iter_obj.value("afs").toObject();
	this->afs_recv_iter_ms = UtilJson::json_text(afs_iter_item, "recv_iter_ms", "100").toInt();
	this->afs_send_iter_ms = UtilJson::json_text(afs_iter_item, "send_iter_ms", "100").toInt();
	const QJsonObject tac_iter_item = thread_iter_obj.value("tac").toObject();
	this->tac_recv_iter_ms = UtilJson::json_text(tac_iter_item, "recv_iter_ms", "100").toInt();
	this->tac_send_iter_ms = UtilJson::json_text(tac_iter_item, "send_iter_ms", "100").toInt();

	// ---------- UI刷新频率 ----------
	const QJsonObject ui_iter_obj = config_common.value("UI_ITER").toObject();
	this->ui_sync_iter_ms = std::max(1,
		UtilJson::json_text(ui_iter_obj, "sync_icgas_iter_ms",
			UtilJson::json_text(ui_iter_obj, "sync_iter_ms", "500")).toInt());
}

void InfoExpert::init_concept_graph_config()
{
	std::map<QString, S_CONCEPT_GRAPH> graphs = read_concept_graph_config();

	QMutexLocker locker(&data_mutex);
	list_concept_graph = graphs;
	concept_graph = list_concept_graph.empty()
		? S_CONCEPT_GRAPH()
		: list_concept_graph.begin()->second;
}

QStringList InfoExpert::send_scenario_names()
{
	return scenario_name_list();
}

S_CONCEPT_GRAPH InfoExpert::send_concept_graph()
{
	QMutexLocker locker(&data_mutex);
	return concept_graph;
}

std::map<QString, S_CONCEPT_GRAPH> InfoExpert::send_concept_graphs()
{
	{
		QMutexLocker locker(&data_mutex);
		if (!list_concept_graph.empty()) return list_concept_graph;
	}

	init_concept_graph_config();

	QMutexLocker locker(&data_mutex);
	return list_concept_graph;
}

bool InfoExpert::save_concept_graph(const S_CONCEPT_GRAPH& graph)
{
	if (graph.scenario_id.trimmed().isEmpty()) return false;

	S_CONCEPT_GRAPH stored_graph = graph;
	rebuild_concept_edge_index(stored_graph);
	const bool is_success = write_concept_graph_config(stored_graph);
	if (!is_success) return false;

	QMutexLocker locker(&data_mutex);
	list_concept_graph[stored_graph.scenario_id] = stored_graph;
	concept_graph = stored_graph;
	return true;
}

QVariantMap InfoExpert::send_comm_config()
{
	QVariantMap config;
	QMutexLocker locker(&data_mutex);
	config[QStringLiteral("afs_client_addr")] = afs_client_addr;
	config[QStringLiteral("afs_client_port")] = afs_client_port;
	config[QStringLiteral("afs_server_addr")] = afs_server_addr;
	config[QStringLiteral("afs_server_port")] = afs_server_port;
	config[QStringLiteral("tac_server_addr")] = tac_server_addr;
	config[QStringLiteral("tac_server_port")] = tac_server_port;
	config[QStringLiteral("tac_server_user")] = tac_server_user;
	config[QStringLiteral("tac_send_iter_ms")] = QString::number(tac_send_iter_ms);
	config[QStringLiteral("ui_sync_iter_ms")] = QString::number(ui_sync_iter_ms);
	return config;
}

bool InfoExpert::update_comm_config_cache(const QString& afs_client_addr, const QString& afs_client_port,
	const QString& afs_server_addr, const QString& afs_server_port,
	const QString& tac_server_addr, const QString& tac_server_port,
	const QString& tac_server_user, const QString& tac_send_iter_ms)
{
	bool tac_iter_ok = false;
	const int tac_send_iter = std::max(1, tac_send_iter_ms.trimmed().toInt(&tac_iter_ok));
	if (!tac_iter_ok) return false;

	QMutexLocker locker(&data_mutex);
	this->afs_client_addr = afs_client_addr.trimmed();
	this->afs_client_port = afs_client_port.trimmed();
	this->afs_server_addr = afs_server_addr.trimmed();
	this->afs_server_port = afs_server_port.trimmed();
	this->tac_server_addr = tac_server_addr.trimmed();
	this->tac_server_port = tac_server_port.trimmed();
	this->tac_server_user = tac_server_user.trimmed();
	this->tac_send_iter_ms = tac_send_iter;
	return true;
}

bool InfoExpert::save_comm_config(const QString& afs_client_addr, const QString& afs_client_port,
	const QString& afs_server_addr, const QString& afs_server_port,
	const QString& tac_server_addr, const QString& tac_server_port,
	const QString& tac_server_user, const QString& tac_send_iter_ms)
{
	if (!update_comm_config_cache(afs_client_addr, afs_client_port,
		afs_server_addr, afs_server_port,
		tac_server_addr, tac_server_port,
		tac_server_user, tac_send_iter_ms)) {
		return false;
	}

	QString next_afs_client_addr;
	QString next_afs_client_port;
	QString next_afs_server_addr;
	QString next_afs_server_port;
	QString next_tac_server_addr;
	QString next_tac_server_port;
	QString next_tac_server_user;
	int next_tac_send_iter_ms = 1;
	{
		QMutexLocker locker(&data_mutex);
		next_afs_client_addr = this->afs_client_addr;
		next_afs_client_port = this->afs_client_port;
		next_afs_server_addr = this->afs_server_addr;
		next_afs_server_port = this->afs_server_port;
		next_tac_server_addr = this->tac_server_addr;
		next_tac_server_port = this->tac_server_port;
		next_tac_server_user = this->tac_server_user;
		next_tac_send_iter_ms = this->tac_send_iter_ms;
	}

	const QString name_config_comm = "Config/ConfigComm.json";
	QJsonObject config_obj;
	if (!UtilJson::read_json_object_file(name_config_comm, config_obj)) return false;

	QJsonObject socket_obj = config_obj.value("SOCKET").toObject();
	QJsonObject afs_client_obj = socket_obj.value("afsim_client").toObject();
	afs_client_obj["addr"] = next_afs_client_addr;
	afs_client_obj["port"] = next_afs_client_port;
	socket_obj["afsim_client"] = afs_client_obj;

	QJsonObject afs_server_obj = socket_obj.value("afsim_server").toObject();
	afs_server_obj["addr"] = next_afs_server_addr;
	afs_server_obj["port"] = next_afs_server_port;
	socket_obj["afsim_server"] = afs_server_obj;

	QJsonObject tac_server_obj = socket_obj.value("tacview_server").toObject();
	tac_server_obj["addr"] = next_tac_server_addr;
	tac_server_obj["port"] = next_tac_server_port;
	tac_server_obj["user"] = next_tac_server_user;
	socket_obj["tacview_server"] = tac_server_obj;
	config_obj["SOCKET"] = socket_obj;

	QJsonObject thread_iter_obj = config_obj.value("THREAD_ITER").toObject();
	QJsonObject tac_iter_obj = thread_iter_obj.value("tac").toObject();
	tac_iter_obj["send_iter_ms"] = QString::number(next_tac_send_iter_ms);
	thread_iter_obj["tac"] = tac_iter_obj;
	config_obj["THREAD_ITER"] = thread_iter_obj;

	return UtilJson::write_json_object_file(name_config_comm, config_obj);
}

void InfoExpert::rebuild_concept_edge_index(S_CONCEPT_GRAPH& graph) const
{
	graph.back_edges.clear();
	graph.font_edges.clear();
	for (const S_GRAPH_EDGE& edge : graph.edge_list) {
		graph.back_edges.insert(edge.font_node_id, edge.id);
		graph.font_edges.insert(edge.back_node_id, edge.id);
	}
}

std::map<QString, S_CONCEPT_GRAPH> InfoExpert::read_concept_graph_config() const
{
	std::map<QString, S_CONCEPT_GRAPH> graphs;
	std::map<QString, QString> graph_paths;
	const QString dir_config_graph = "Config/ConceptGraph";
	QDir dir(dir_config_graph);
	if (!dir.exists()) {
		qDebug() << "Error: ConceptGraph config directory does not exist:" + dir_config_graph;
		return graphs;
	}

	const QFileInfoList config_files = dir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Name);
	for (const QFileInfo& file_info : config_files) {
		QJsonObject graph_obj;
		if (!UtilJson::read_json_object_file(file_info.absoluteFilePath(), graph_obj)) continue;
		if (graph_obj.isEmpty()) {
			qDebug() << "Error: ConceptGraph config root is empty:" + file_info.absoluteFilePath();
			continue;
		}

		S_CONCEPT_GRAPH graph;
		graph.scenario_id = UtilJson::json_text(graph_obj, "scenario_id");
		graph.scenario_name = UtilJson::json_text(graph_obj, "scenario_name");
		graph.scenario_type = UtilEnum::trans_scenario_type(UtilJson::json_text(graph_obj, "scenario_type"));
		if (graph.scenario_id.isEmpty()) graph.scenario_id = file_info.completeBaseName();

		const QJsonArray node_list = graph_obj.value("node_list").toArray();
		for (const QJsonValue& node_value : node_list) {
			const QJsonObject node_obj = node_value.toObject();
			S_GRAPH_NODE node;
			const QString node_type_value = UtilJson::json_text(node_obj, "type");
			node.id = UtilJson::json_text(node_obj, "id");
			node.name = UtilJson::json_text(node_obj, "name");
			node.type = UtilEnum::trans_node_type(node_type_value);
			if (node.type == E_NODE_TYPE::UKN) {
				const int dash_index = node.id.indexOf('-');
				node.type = UtilEnum::trans_node_type(dash_index > 0 ? node.id.left(dash_index) : node.id);
			}
			if (node.id.isEmpty()) {
				qDebug() << "Warning: ConceptGraph node id is empty:" << file_info.fileName();
				continue;
			}
			graph.node_list.push_back(node);
		}

		const QJsonArray edge_list = graph_obj.value("edge_list").toArray();
		for (const QJsonValue& edge_value : edge_list) {
			const QJsonObject edge_obj = edge_value.toObject();
			S_GRAPH_EDGE edge;
			edge.id = UtilJson::json_text(edge_obj, "id");
			edge.name = UtilJson::json_text(edge_obj, "name");
			edge.type = UtilEnum::trans_edge_type(UtilJson::json_text(edge_obj, "type"));
			edge.font_node_id = UtilJson::json_text(edge_obj, "font_node_id");
			edge.back_node_id = UtilJson::json_text(edge_obj, "back_node_id");

			bool weight_ok = false;
			const double weight = UtilJson::json_text(edge_obj, "weight", "1.0").toDouble(&weight_ok);
			edge.weight = weight_ok ? weight : 1.0;

			if (edge.id.isEmpty()) {
				qDebug() << "Warning: ConceptGraph edge id is empty:" << file_info.fileName();
				continue;
			}

			graph.edge_list.push_back(edge);
		}

		rebuild_concept_edge_index(graph);
		graphs[graph.scenario_id] = graph;
		graph_paths[graph.scenario_id] = file_info.absoluteFilePath();
	}

	QMutexLocker locker(&data_mutex);
	concept_graph_config_paths = graph_paths;
	return graphs;
}

bool InfoExpert::write_concept_graph_config(const S_CONCEPT_GRAPH& graph) const
{
	if (graph.scenario_id.trimmed().isEmpty()) return false;

	QDir dir("Config/ConceptGraph");
	if (!dir.exists() && !dir.mkpath(".")) return false;

	QJsonObject obj;
	QString graph_path;
	{
		QMutexLocker locker(&data_mutex);
		const auto path_iter = concept_graph_config_paths.find(graph.scenario_id);
		if (path_iter != concept_graph_config_paths.end()) {
			graph_path = path_iter->second;
		}
	}
	if (graph_path.trimmed().isEmpty()) {
		graph_path = dir.filePath(graph.scenario_id + ".json");
	}
	if (QFileInfo::exists(graph_path)) {
		UtilJson::read_json_object_file(graph_path, obj);
	}

	const std::vector<E_NODE_TYPE> node_type_order = {
		E_NODE_TYPE::MIS, E_NODE_TYPE::TSK, E_NODE_TYPE::ACT, E_NODE_TYPE::OBJ,
		E_NODE_TYPE::COG, E_NODE_TYPE::DEC, E_NODE_TYPE::EFT,
		E_NODE_TYPE::LOO, E_NODE_TYPE::COA, E_NODE_TYPE::PLT
	};
	const std::vector<E_EDGE_TYPE> edge_type_order = {
		E_EDGE_TYPE::MIS_TO_COG, E_EDGE_TYPE::MIS_TO_OBJ, E_EDGE_TYPE::OBJ_TO_LOO,
		E_EDGE_TYPE::LOO_TO_COA, E_EDGE_TYPE::COG_TO_DEC, E_EDGE_TYPE::OBJ_TO_DEC,
		E_EDGE_TYPE::DEC_TO_DEC, E_EDGE_TYPE::DEC_TO_EFT, E_EDGE_TYPE::EFT_TO_ACT,
		E_EDGE_TYPE::ACT_TO_ACT, E_EDGE_TYPE::ACT_TO_TSK, E_EDGE_TYPE::TSK_TO_PLT
	};

	QJsonArray node_type_list;
	for (const E_NODE_TYPE type : node_type_order) {
		node_type_list.append(UtilEnum::trans_node_type_en(type));
	}
	QJsonArray edge_type_list;
	for (const E_EDGE_TYPE type : edge_type_order) {
		edge_type_list.append(UtilEnum::trans_edge_type_en(type));
	}

	std::map<QString, QJsonObject> old_nodes;
	for (const QJsonValue& value : obj.value("node_list").toArray()) {
		const QJsonObject item = value.toObject();
		const QString id = item.value("id").toString().trimmed();
		if (!id.isEmpty()) old_nodes[id] = item;
	}

	std::map<QString, QJsonObject> old_edges;
	for (const QJsonValue& value : obj.value("edge_list").toArray()) {
		const QJsonObject item = value.toObject();
		const QString id = item.value("id").toString().trimmed();
		if (!id.isEmpty()) old_edges[id] = item;
	}

	obj["scenario_id"] = graph.scenario_id;
	obj["scenario_name"] = graph.scenario_name;
	obj["scenario_type"] = UtilEnum::trans_scenario_type_en(graph.scenario_type);
	obj["node_type_list"] = node_type_list;
	obj["edge_type_list"] = edge_type_list;

	QJsonArray node_list;
	for (const S_GRAPH_NODE& node : graph.node_list) {
		const auto iter = old_nodes.find(node.id);
		QJsonObject item = iter == old_nodes.end() ? QJsonObject() : iter->second;
		item["id"] = node.id;
		item["type"] = UtilEnum::trans_node_type_en(node.type);
		item["name"] = node.name;
		node_list.append(item);
	}
	obj["node_list"] = node_list;

	QJsonArray edge_list;
	for (const S_GRAPH_EDGE& edge : graph.edge_list) {
		const auto iter = old_edges.find(edge.id);
		QJsonObject item = iter == old_edges.end() ? QJsonObject() : iter->second;
		item["id"] = edge.id;
		item["type"] = UtilEnum::trans_edge_type_en(edge.type);
		item["name"] = edge.name;
		item["font_node_id"] = edge.font_node_id;
		item["back_node_id"] = edge.back_node_id;
		item["weight"] = QString::number(edge.weight, 'f', 3);
		edge_list.append(item);
	}
	obj["edge_list"] = edge_list;

	return UtilJson::write_json_object_file(graph_path, obj);
}

QStringList InfoExpert::scenario_name_list() const
{
	QStringList name_list;

	QDir root_dir(scenario_root_path());
	if (!root_dir.exists()) return name_list;

	const QFileInfoList scene_list = root_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
	for (const QFileInfo& scene_info : scene_list) {
		name_list << scene_info.fileName();
	}

	return name_list;
}

QString InfoExpert::scenario_root_path() const
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
			const QString scenario_path = dir.absoluteFilePath("Scenario");
			if (QDir(scenario_path).exists()) {
				return QDir(scenario_path).absolutePath();
			}
			if (!dir.cdUp()) break;
		}
	}

	return QDir("Scenario").absolutePath();
}

