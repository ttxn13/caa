#pragma once
#include "pch.hpp"
#include "../Comm/CoreGraph.hpp"

// 可配置专家知识
class InfoExpert
{
public:	// 公有函数
	static InfoExpert*	GetInstance();
	static void			DstInstance();
	InfoExpert();
	~InfoExpert();

	void init_config_common()	;	// 初始化基本配置信息
	void init_concept_graph_config();	// 初始化知识图谱配置

	QStringList send_scenario_names();	// 发送场景列表
	S_CONCEPT_GRAPH send_concept_graph();	// 发送当前知识图谱
	std::map<QString, S_CONCEPT_GRAPH> send_concept_graphs();	// 发送知识图谱列表
	bool save_concept_graph(const S_CONCEPT_GRAPH& graph);	// 更新并写入知识图谱配置
	QVariantMap send_comm_config();	// 发送通信配置缓存
	bool update_comm_config_cache(const QString& afs_client_addr, const QString& afs_client_port,
		const QString& afs_server_addr, const QString& afs_server_port,
		const QString& tac_server_addr, const QString& tac_server_port,
		const QString& tac_server_user, const QString& tac_send_iter_ms);
	bool save_comm_config(const QString& afs_client_addr, const QString& afs_client_port,
		const QString& afs_server_addr, const QString& afs_server_port,
		const QString& tac_server_addr, const QString& tac_server_port,
		const QString& tac_server_user, const QString& tac_send_iter_ms);

private: // 私有函数
	void rebuild_concept_edge_index(S_CONCEPT_GRAPH& graph) const;
	std::map<QString, S_CONCEPT_GRAPH> read_concept_graph_config() const;
	bool write_concept_graph_config(const S_CONCEPT_GRAPH& graph) const;

	QStringList scenario_name_list() const;				// 获取Scenario下的场景列表
	QString scenario_root_path() const;															// Scenario根目录


public:
	// ---------- Admin参数 ----------
	int admin_mode			;	// 管理员模式		，1-免登录 0-正常模式
	int embed_mode			;	// 嵌入模式			，1-嵌入   0-不嵌入
	int database_mode		;	// 回放模式			，0-回放   1-正常模式
	int sim_iter_ms			;	// 仿真步长			, ms

	// ---------- 用户登录信息 ----------
	std::map<QString, QString> login_info;	// 用户名、密码

	// ---------- 通信配置信息 ----------
	QString afs_client_addr		;	// AFSIM   client IP地址
	QString afs_client_port		;	// AFSIM   client 端口号
	QString afs_server_addr		;	// AFSIM   server IP地址
	QString afs_server_port		;	// AFSIM   server 端口号
	QString tac_server_addr		;	// Tacview server IP地址
	QString tac_server_port		;	// Tacview server 端口号
	QString tac_server_user		;	// Tacview server 用户名

	// ---------- 线程刷新频率 ----------
	int afs_recv_iter_ms;	// AFSIM   接收线程间隔, ms
	int afs_send_iter_ms;	// AFSIM   发送线程间隔, ms
	int tac_recv_iter_ms;	// Tacview 接收线程间隔, ms
	int tac_send_iter_ms;	// Tacview 发送线程间隔, ms

	// ---------- UI刷新频率 ----------
	int ui_sync_iter_ms;	// UI统一刷新间隔, ms

	// ---------- 静态知识/指挥配置 ----------
	S_CONCEPT_GRAPH							concept_graph		;	// 知识图谱
	std::map<QString, S_CONCEPT_GRAPH>		list_concept_graph	;	// 知识图谱列表

private:
	static QMutex mutex;
	static InfoExpert* instance;
	mutable std::map<QString, QString> concept_graph_config_paths;
	mutable QMutex data_mutex;
};
