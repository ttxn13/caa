#pragma once
#include "pch.hpp"
#include "../../Core/IncCore.hpp"

#include "../../Protocol/AfsimServer.hpp"
#include "../../Protocol/AfsimClient.hpp"
#include "../../Protocol/TacviewServer.hpp"

class AfsimServer;
class AfsimClient;
class ModuleDatabase;
class ModuleMission;
class QTimer;
class SimController;
class TacviewServer;
class UiAFSimTest;
class UiGraph;
class UiScenario;
class UiSitMonitor;
class UiWorkflow;

class UiICGAS : public QObject
{
	Q_OBJECT
	friend class UiSitMonitor;

	Q_PROPERTY(QStringList scenarioNames READ scenarioNames NOTIFY scenarioNamesChanged)
	Q_PROPERTY(QStringList simScenarioNames READ simScenarioNames NOTIFY simScenarioNamesChanged)
	Q_PROPERTY(QStringList replayLogNames READ replayLogNames CONSTANT)
	Q_PROPERTY(QString selectedScenario READ selectedScenario WRITE setSelectedScenario NOTIFY selectedScenarioChanged)
	Q_PROPERTY(QString workflowInputMode READ workflowInputMode WRITE setWorkflowInputMode NOTIFY workflowInputModeChanged)

	Q_PROPERTY(QString afsClientAddr READ afsClientAddr WRITE setAfsClientAddr NOTIFY commConfigChanged)
	Q_PROPERTY(QString afsClientPort READ afsClientPort WRITE setAfsClientPort NOTIFY commConfigChanged)
	Q_PROPERTY(bool afsClientConfigDirty READ afsClientConfigDirty NOTIFY afsClientConfigDirtyChanged)
	Q_PROPERTY(QString afsServerAddr READ afsServerAddr WRITE setAfsServerAddr NOTIFY commConfigChanged)
	Q_PROPERTY(QString afsServerPort READ afsServerPort WRITE setAfsServerPort NOTIFY commConfigChanged)
	Q_PROPERTY(QString tacServerAddr READ tacServerAddr WRITE setTacServerAddr NOTIFY commConfigChanged)
	Q_PROPERTY(QString tacServerPort READ tacServerPort WRITE setTacServerPort NOTIFY commConfigChanged)
	Q_PROPERTY(QString tacServerUser READ tacServerUser WRITE setTacServerUser NOTIFY commConfigChanged)
	Q_PROPERTY(QString tacSendIterMs READ tacSendIterMs WRITE setTacSendIterMs NOTIFY commConfigChanged)
	Q_PROPERTY(QString uiSyncIterMs READ uiSyncIterMs CONSTANT)
	Q_PROPERTY(QString databasePlatTimeoutMs READ databasePlatTimeoutMs NOTIFY databaseConfigChanged)
	Q_PROPERTY(QString databaseAlgoStepMs READ databaseAlgoStepMs NOTIFY databaseConfigChanged)
	Q_PROPERTY(bool databaseConfigDirty READ databaseConfigDirty NOTIFY databaseConfigDirtyChanged)

	Q_PROPERTY(bool afsClientRunning READ afsClientRunning NOTIFY commStateChanged)
	Q_PROPERTY(bool afsServerRunning READ afsServerRunning NOTIFY commStateChanged)
	Q_PROPERTY(bool tacServerRunning READ tacServerRunning NOTIFY commStateChanged)
	Q_PROPERTY(bool workflowRunning READ workflowRunning NOTIFY workflowRunStateChanged)
	Q_PROPERTY(bool workflowPaused READ workflowPaused NOTIFY workflowRunStateChanged)
	Q_PROPERTY(QString defaultMissionId READ defaultMissionId CONSTANT)
	Q_PROPERTY(QString missionId READ missionId WRITE setMissionId NOTIFY missionChanged)
	Q_PROPERTY(QString missionSourceText READ missionSourceText WRITE setMissionSourceText NOTIFY missionChanged)
	Q_PROPERTY(bool missionConfigDirty READ missionConfigDirty NOTIFY missionConfigDirtyChanged)
	Q_PROPERTY(QStringList workflowLogRows READ workflowLogRows NOTIFY logRowsChanged)

	Q_PROPERTY(QVariantList platformRows READ platformRows NOTIFY situationChanged)
	Q_PROPERTY(QVariantList redSituationRows READ redSituationRows NOTIFY situationChanged)
	Q_PROPERTY(QVariantList blueSituationRows READ blueSituationRows NOTIFY situationChanged)
	Q_PROPERTY(QVariantList primaryRows READ primaryRows NOTIFY detailChanged)
	Q_PROPERTY(QVariantList secondaryRows READ secondaryRows NOTIFY detailChanged)
	Q_PROPERTY(QVariantList relativeRows READ relativeRows NOTIFY detailChanged)
	Q_PROPERTY(QString primaryPlatId READ primaryPlatId NOTIFY detailChanged)
	Q_PROPERTY(QString secondaryPlatId READ secondaryPlatId NOTIFY detailChanged)

	Q_PROPERTY(QVariantList initialPlatformTypeRows READ initialPlatformTypeRows NOTIFY initialCatalogChanged)
	Q_PROPERTY(QVariantList initialWeaponTypeRows READ initialWeaponTypeRows NOTIFY initialCatalogChanged)
	Q_PROPERTY(QVariantList initialSensorTypeRows READ initialSensorTypeRows NOTIFY initialCatalogChanged)
	Q_PROPERTY(QStringList initialPlatformIconOptions READ initialPlatformIconOptions NOTIFY initialCatalogChanged)
	Q_PROPERTY(QStringList initialMoverTypeOptions READ initialMoverTypeOptions CONSTANT)
	Q_PROPERTY(QStringList initialDomainOptions READ initialDomainOptions CONSTANT)
	Q_PROPERTY(QStringList initialPlatformClassOptions READ initialPlatformClassOptions CONSTANT)
	Q_PROPERTY(QStringList initialWeaponClassOptions READ initialWeaponClassOptions CONSTANT)
	Q_PROPERTY(QStringList initialSensorClassOptions READ initialSensorClassOptions CONSTANT)
	Q_PROPERTY(QVariantList initialScenarioPlatformRows READ initialScenarioPlatformRows NOTIFY initialScenarioChanged)
	Q_PROPERTY(QString initialSelectedScenarioPlatformId READ initialSelectedScenarioPlatformId NOTIFY initialSelectionChanged)
	Q_PROPERTY(QVariantMap initialSelectedScenarioPlatform READ initialSelectedScenarioPlatform NOTIFY initialSelectionChanged)
	Q_PROPERTY(QString initialSimIterMs READ initialSimIterMs NOTIFY initialStatusChanged)
	Q_PROPERTY(QString initialTimeAccRatio READ initialTimeAccRatio NOTIFY initialStatusChanged)
	Q_PROPERTY(QString initialSituationStatus READ initialSituationStatus NOTIFY initialStatusChanged)
	Q_PROPERTY(QString initialSimCtrlSaveStatus READ initialSimCtrlSaveStatus NOTIFY initialStatusChanged)
	Q_PROPERTY(QString initialPlatformTypeSaveStatus READ initialPlatformTypeSaveStatus NOTIFY initialStatusChanged)
	Q_PROPERTY(QString initialWeaponTypeSaveStatus READ initialWeaponTypeSaveStatus NOTIFY initialStatusChanged)
	Q_PROPERTY(QString initialSensorTypeSaveStatus READ initialSensorTypeSaveStatus NOTIFY initialStatusChanged)
	Q_PROPERTY(QString initialScenarioSaveStatus READ initialScenarioSaveStatus NOTIFY initialStatusChanged)
	Q_PROPERTY(QString initialPlatformDetailSaveStatus READ initialPlatformDetailSaveStatus NOTIFY initialStatusChanged)

	Q_PROPERTY(QStringList conceptScenarioNames READ conceptScenarioNames NOTIFY conceptChanged)
	Q_PROPERTY(QString selectedConceptScenario READ selectedConceptScenario WRITE setSelectedConceptScenario NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptStatus READ conceptStatus NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptScenarioRows READ conceptScenarioRows NOTIFY conceptChanged)
	Q_PROPERTY(bool conceptOverviewMode READ conceptOverviewMode NOTIFY conceptChanged)
	Q_PROPERTY(bool conceptFocusMode READ conceptFocusMode NOTIFY conceptChanged)
	Q_PROPERTY(bool conceptFocusActive READ conceptFocusActive NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptTypeFilterPage READ conceptTypeFilterPage NOTIFY conceptChanged)
	Q_PROPERTY(bool conceptConfigLocked READ conceptConfigLocked NOTIFY conceptChanged)
	Q_PROPERTY(bool conceptDirty READ conceptDirty NOTIFY conceptChanged)
	Q_PROPERTY(bool conceptPendingDetailChanges READ conceptPendingDetailChanges NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptMissionText READ conceptMissionText NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptGraphTitle READ conceptGraphTitle NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptDetailMode READ conceptDetailMode NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptDetailTitle READ conceptDetailTitle NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptOverviewType READ conceptOverviewType NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptOverviewTypeName READ conceptOverviewTypeName NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptNodeTypeLine READ conceptNodeTypeLine NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptNodeIdLine READ conceptNodeIdLine NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptNodeName READ conceptNodeName NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptOverviewApplyStatus READ conceptOverviewApplyStatus NOTIFY conceptChanged)
	Q_PROPERTY(QString conceptNodeApplyStatus READ conceptNodeApplyStatus NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptNodeTypeRows READ conceptNodeTypeRows NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptEdgeTypeRows READ conceptEdgeTypeRows NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptFieldRows READ conceptFieldRows NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptGraphLanes READ conceptGraphLanes NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptGraphNodes READ conceptGraphNodes NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptGraphEdges READ conceptGraphEdges NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptOverviewDetailRows READ conceptOverviewDetailRows NOTIFY conceptChanged)
	Q_PROPERTY(QVariantList conceptNodeEdgeRows READ conceptNodeEdgeRows NOTIFY conceptChanged)

public:		// 公有函数
	static UiICGAS*	GetInstance();
	explicit UiICGAS(QObject* parent = nullptr);
	~UiICGAS();

	void show();
	void setWindowIcon(const QIcon& icon);
	void shutdown();

	QStringList scenarioNames() const			{ return scenario_name_list; }
	QStringList simScenarioNames() const			{ return sim_scenario_name_list; }
	QStringList replayLogNames() const;
	QString selectedScenario() const			{ return selected_scenario; }
	QString workflowInputMode() const			{ return workflow_input_mode; }
	QString afsClientAddr() const				{ return afs_client_addr; }
	QString afsClientPort() const				{ return afs_client_port; }
	bool afsClientConfigDirty() const			{ return afs_client_config_dirty; }
	QString afsServerAddr() const				{ return afs_server_addr; }
	QString afsServerPort() const				{ return afs_server_port; }
	QString tacServerAddr() const				{ return tac_server_addr; }
	QString tacServerPort() const				{ return tac_server_port; }
	QString tacServerUser() const				{ return tac_server_user; }
	QString tacSendIterMs() const				{ return tac_send_iter_ms; }
	QString uiSyncIterMs() const				{ return ui_sync_iter_ms; }
	QString databasePlatTimeoutMs() const		{ return database_plat_timeout_ms; }
	QString databaseAlgoStepMs() const			{ return database_algo_step_ms; }
	bool databaseConfigDirty() const			{ return database_config_dirty; }
	bool afsClientRunning() const				{ return afs_client_running; }
	bool afsServerRunning() const				{ return afs_server_running; }
	bool tacServerRunning() const				{ return tac_server_running; }
	bool workflowRunning() const				{ return workflow_run_state == "running"; }
	bool workflowPaused() const					{ return workflow_run_state == "paused"; }
	QString defaultMissionId() const;
	QString missionId() const;
	QString missionSourceText() const;
	bool missionConfigDirty() const			{ return mission_config_dirty; }
	QStringList workflowLogRows() const			{ return workflow_log_rows; }
	QVariantList platformRows() const			{ return platform_rows; }
	QVariantList redSituationRows() const;
	QVariantList blueSituationRows() const;
	QVariantList primaryRows() const			{ return primary_rows; }
	QVariantList secondaryRows() const			{ return secondary_rows; }
	QVariantList relativeRows() const			{ return relative_rows; }
	QString primaryPlatId() const				{ return primary_plat_id; }
	QString secondaryPlatId() const				{ return secondary_plat_id; }
	QVariantList initialPlatformTypeRows() const		{ return initial_platform_type_rows; }
	QVariantList initialWeaponTypeRows() const		{ return initial_weapon_type_rows; }
	QVariantList initialSensorTypeRows() const		{ return initial_sensor_type_rows; }
	QStringList initialPlatformIconOptions() const	{ return initial_platform_icon_options; }
	QStringList initialMoverTypeOptions() const;
	QStringList initialDomainOptions() const;
	QStringList initialPlatformClassOptions() const;
	QStringList initialWeaponClassOptions() const;
	QStringList initialSensorClassOptions() const;
	QVariantList initialScenarioPlatformRows() const	{ return initial_scenario_platform_rows; }
	QString initialSelectedScenarioPlatformId() const	{ return initial_selected_scenario_platform_id; }
	QVariantMap initialSelectedScenarioPlatform() const	{ return initial_selected_scenario_platform; }
	QString initialSimIterMs() const					{ return initial_sim_iter_ms; }
	QString initialTimeAccRatio() const				{ return initial_time_acc_ratio; }
	QString initialSituationStatus() const			{ return initial_situation_status; }
	QString initialSimCtrlSaveStatus() const		{ return initial_sim_ctrl_save_status; }
	QString initialPlatformTypeSaveStatus() const	{ return initial_platform_type_save_status; }
	QString initialWeaponTypeSaveStatus() const		{ return initial_weapon_type_save_status; }
	QString initialSensorTypeSaveStatus() const		{ return initial_sensor_type_save_status; }
	QString initialScenarioSaveStatus() const		{ return initial_scenario_save_status; }
	QString initialPlatformDetailSaveStatus() const	{ return initial_platform_detail_save_status; }
	QStringList conceptScenarioNames() const	{ return concept_scenario_names; }
	QString selectedConceptScenario() const		{ return selected_concept_scenario; }
	QString conceptStatus() const				{ return concept_status; }
	QVariantList conceptScenarioRows() const	{ return concept_scenario_rows; }
	bool conceptOverviewMode() const			{ return concept_overview_mode; }
	bool conceptFocusMode() const				{ return concept_focus_mode; }
	bool conceptFocusActive() const				{ return concept_focus_mode && !concept_focus_activations.empty(); }
	QString conceptTypeFilterPage() const		{ return concept_type_filter_page; }
	bool conceptConfigLocked() const			{ return concept_config_locked; }
	bool conceptDirty() const					{ return concept_dirty || !dirty_concept_scenarios.empty(); }
	bool conceptPendingDetailChanges() const	{ return concept_detail_dirty; }
	QString conceptMissionText() const			{ return concept_mission_text; }
	QString conceptGraphTitle() const			{ return concept_graph_title; }
	QString conceptDetailMode() const			{ return concept_detail_mode; }
	QString conceptDetailTitle() const			{ return concept_detail_title; }
	QString conceptOverviewType() const			{ return concept_overview_type; }
	QString conceptOverviewTypeName() const		{ return concept_overview_type_name; }
	QString conceptNodeTypeLine() const			{ return concept_node_type_line; }
	QString conceptNodeIdLine() const			{ return concept_node_id_line; }
	QString conceptNodeName() const				{ return concept_node_name; }
	QString conceptOverviewApplyStatus() const	{ return concept_overview_apply_status; }
	QString conceptNodeApplyStatus() const		{ return concept_node_apply_status; }
	QVariantList conceptNodeTypeRows() const	{ return concept_node_type_rows; }
	QVariantList conceptEdgeTypeRows() const	{ return concept_edge_type_rows; }
	QVariantList conceptFieldRows() const		{ return concept_field_rows; }
	QVariantList conceptGraphLanes() const		{ return concept_graph_lanes; }
	QVariantList conceptGraphNodes() const		{ return concept_graph_nodes; }
	QVariantList conceptGraphEdges() const		{ return concept_graph_edges; }
	QVariantList conceptOverviewDetailRows() const	{ return concept_overview_detail_rows; }
	QVariantList conceptNodeEdgeRows() const		{ return concept_node_edge_rows; }

	void setSelectedScenario(const QString& scenario);
	void setAfsClientAddr(const QString& value);
	void setAfsClientPort(const QString& value);
	void setAfsServerAddr(const QString& value);
	void setAfsServerPort(const QString& value);
	void setTacServerAddr(const QString& value);
	void setTacServerPort(const QString& value);
	void setTacServerUser(const QString& value);
	void setTacSendIterMs(const QString& value);
	void setMissionId(const QString& value);
	void setMissionSourceText(const QString& value);
	void setSelectedConceptScenario(const QString& scenario);

	Q_INVOKABLE void toggleAfsServer();
	Q_INVOKABLE void toggleTacServer();
	Q_INVOKABLE void runWorkflow();
	Q_INVOKABLE bool hasUnsavedWorkflowChanges() const;
	Q_INVOKABLE QString unsavedWorkflowMessage() const;
	Q_INVOKABLE void pauseWorkflow();
	Q_INVOKABLE void stopWorkflow();
	Q_INVOKABLE bool saveWorkflowConfig();
	Q_INVOKABLE bool saveAfsClientConfig(const QString& addr, const QString& port);
	Q_INVOKABLE bool saveDatabaseConfig(const QString& plt_timeout_ms, const QString& algo_step_ms);
	Q_INVOKABLE void setDatabaseConfigDraftDirty(bool dirty);
	Q_INVOKABLE void resetAfsClientConfigDraft();
	Q_INVOKABLE void setAfsClientConfigDraftDirty(bool dirty);
	Q_INVOKABLE bool saveMissionConfig(const QString& mission_id, const QString& source_text);
	Q_INVOKABLE void setMissionConfigDraftDirty(bool dirty);
	Q_INVOKABLE void setWorkflowInputMode(const QString& input_mode);
	Q_INVOKABLE void setWorkflowRealtimeMode(bool realtime_mode);
	Q_INVOKABLE QString workflowModuleStateKind(const QString& module_id) const;
	Q_INVOKABLE void selectPrimaryPlat(const QString& plat_id);
	Q_INVOKABLE void selectSecondaryPlat(const QString& plat_id);
	Q_INVOKABLE void selectInitialScenarioPlatform(const QString& platform_id);
	Q_INVOKABLE QString addInitialPlatformType();
	Q_INVOKABLE bool deleteInitialPlatformType(const QString& type_id);
	Q_INVOKABLE bool saveInitialPlatformType(const QString& type_id);
	Q_INVOKABLE QString addInitialWeaponType();
	Q_INVOKABLE bool deleteInitialWeaponType(const QString& type_id);
	Q_INVOKABLE bool saveInitialWeaponType(const QString& type_id);
	Q_INVOKABLE QString addInitialSensorType();
	Q_INVOKABLE bool deleteInitialSensorType(const QString& type_id);
	Q_INVOKABLE bool saveInitialSensorType(const QString& type_id);
	Q_INVOKABLE QString addInitialScenario();
	Q_INVOKABLE QString createInitialScenario(const QString& scenario_id, bool copy_current);
	Q_INVOKABLE bool deleteInitialScenario(const QString& scenario_id);
	Q_INVOKABLE bool resetInitialScenarioConfig();
	Q_INVOKABLE bool addInitialScenarioPlatform(const QString& type_id, double lon_deg, double lat_deg);
	Q_INVOKABLE bool mountInitialEquipment(const QString& platform_id, const QString& equipment_kind, const QString& type_id);
	Q_INVOKABLE bool addInitialWeaponMount(const QString& platform_id);
	Q_INVOKABLE bool deleteInitialWeaponMount(const QString& platform_id, const QString& type_id);
	Q_INVOKABLE bool updateInitialWeaponMount(const QString& platform_id,
		const QString& old_type_id, const QString& new_type_id, int quantity);
	Q_INVOKABLE bool addInitialSensorMount(const QString& platform_id);
	Q_INVOKABLE bool deleteInitialSensorMount(const QString& platform_id, const QString& type_id);
	Q_INVOKABLE bool updateInitialSensorMount(const QString& platform_id,
		const QString& old_type_id, const QString& new_type_id, int quantity);
	Q_INVOKABLE bool updateInitialCatalogField(const QString& catalog_kind, const QString& type_id,
		const QString& field_path, const QString& value);
	Q_INVOKABLE bool updateInitialScenarioPlatformField(const QString& platform_id,
		const QString& field_path, const QString& value);
	Q_INVOKABLE bool updateInitialRoutePoint(const QString& platform_id, int route_index,
		const QString& field_path, const QString& value);
	Q_INVOKABLE bool setInitialRouteCirculate(const QString& platform_id, bool circulate);
	Q_INVOKABLE bool addInitialRoutePoint(const QString& platform_id);
	Q_INVOKABLE bool deleteInitialRoutePoint(const QString& platform_id, int route_index);
	Q_INVOKABLE bool updateInitialSimCtrlField(const QString& field_path, const QString& value);
	Q_INVOKABLE bool saveInitialSimCtrlConfig();
	Q_INVOKABLE bool saveInitialSituationConfig();
	Q_INVOKABLE bool saveInitialScenarioConfig();
	Q_INVOKABLE bool saveInitialSelectedPlatformConfig();
	Q_INVOKABLE bool confirmConceptConfig();
	Q_INVOKABLE bool resetConceptConfig();
	Q_INVOKABLE bool resetAllConceptConfigChanges();
	Q_INVOKABLE bool applyPendingConceptDetail();
	Q_INVOKABLE void discardPendingConceptDetail();
	Q_INVOKABLE void refreshSituation();
	Q_INVOKABLE void setConceptNavMode(const QString& mode);
	Q_INVOKABLE void setConceptFocusMode(bool enabled);
	Q_INVOKABLE void toggleConceptFocusNode(const QString& node_id);
	Q_INVOKABLE void setConceptTypeFilterPage(const QString& page);
	Q_INVOKABLE void setConceptNodeTypeChecked(const QString& type, bool checked);
	Q_INVOKABLE void setConceptFieldChecked(const QString& group, const QString& field, bool checked);
	Q_INVOKABLE void showConceptOverviewDetail(const QString& type);
	Q_INVOKABLE void showConceptNodeDetail(const QString& node_id);
	Q_INVOKABLE QStringList conceptNodeEdgeTypeOptions(const QString& direction, const QString& currentType) const;
	Q_INVOKABLE QStringList conceptNodeIdsByType(const QString& type) const;
	Q_INVOKABLE void addConceptOverviewNode();
	Q_INVOKABLE void deleteConceptOverviewNode(int row);
	Q_INVOKABLE void setConceptOverviewNodeName(int row, const QString& name);
	Q_INVOKABLE bool applyConceptOverviewDetail();
	Q_INVOKABLE void setConceptNodeName(const QString& name);
	Q_INVOKABLE void addConceptNodeEdge(const QString& direction);
	Q_INVOKABLE void deleteConceptNodeEdge(int row);
	Q_INVOKABLE void setConceptNodeEdgeOtherType(int row, const QString& type);
	Q_INVOKABLE void setConceptNodeEdgeOtherId(int row, const QString& node_id);
	Q_INVOKABLE void setConceptNodeEdgeName(int row, const QString& name);
	Q_INVOKABLE void setConceptNodeEdgeWeight(int row, const QString& weight);
	Q_INVOKABLE bool applyConceptNodeDetail();
	Q_INVOKABLE void setConceptGraphNodePosition(const QString& kind, const QString& node_id, double x, double y);
	Q_INVOKABLE void setConceptGraphNodePositions(const QVariantList& rows);
	Q_INVOKABLE void moveConceptGraphLane(const QString& lane_type, int target_index);

signals:
	void scenarioNamesChanged();
	void simScenarioNamesChanged();
	void selectedScenarioChanged();
	void workflowInputModeChanged();
	void commConfigChanged();
	void databaseConfigChanged();
	void databaseConfigDirtyChanged();
	void afsClientConfigDirtyChanged();
	void commStateChanged();
	void workflowRunStateChanged();
	void workflowModuleStateChanged();
	void missionChanged();
	void missionConfigDirtyChanged();
	void logRowsChanged();
	void situationChanged();
	void detailChanged();
	void initialSituationChanged();
	void initialCatalogChanged();
	void initialScenarioChanged();
	void initialSelectionChanged();
	void initialStatusChanged();
	void conceptChanged();

private slots:
	void on_afs_client_log(const QString& info);	// AFSim客户端日志更新
	void on_afs_server_log(const QString& info);	// AFSim服务端日志更新
	void on_tac_server_log(const QString& info);	// Tacview服务端日志更新
	void on_timer_sync();							// 显示定时刷新

private:	// 私有函数
	QObject* root_object() const;
	void center_on_primary_screen(QQuickWindow* window);
	void init_connect();
	void init_ui_state();
	void refresh_sim_config_cache(bool force_reload = false);
	bool save_sim_config_cache(bool save_define, bool save_config);
	void set_module_database_running(bool running, bool clear_database);
	void emit_initial_changed(bool catalog, bool scenario, bool selection, bool status);
	void rebuild_initial_catalog_rows();
	void rebuild_initial_scenario_rows();
	void rebuild_initial_selected_platform();
	void rebuild_initial_situation_rows(bool catalog = true, bool scenario = true,
		bool selection = true, bool status = true);
	void rebuild_initial_platform_icon_options(bool force_reload = false);
	QString normalized_initial_platform_icon(const QString& icon) const;
	void normalize_initial_platform_define_icons();
	void init_concept_state();
	void start_module_initialization();
	void schedule_deferred_startup();
	void run_deferred_startup();
	QString make_initial_scenario_id() const;
	QString make_initial_platform_id(const QString& type_id) const;
	bool submit_initial_scenario_save(const QString& scenario_id, const QString& context);
	S_PLAT_DEFINE* find_initial_platform_define(const QString& type_id);
	S_WEAPON_DEFINE* find_initial_weapon_define(const QString& type_id);
	S_SENSOR_DEFINE* find_initial_sensor_define(const QString& type_id);
	S_PLAT_CONFIG* find_initial_platform_config(const QString& platform_id);
	QString replay_log_root_path() const;
	void rebuild_concept_state();
	void rebuild_concept_edge_index(S_CONCEPT_GRAPH& graph) const;
	std::vector<E_NODE_TYPE> concept_node_type_order() const;
	std::vector<E_EDGE_TYPE> concept_edge_type_order() const;
	std::pair<E_NODE_TYPE, E_NODE_TYPE> concept_edge_node_types(E_EDGE_TYPE type) const;
	void rebuild_sim_scenario_names();
	void refresh_database_config_cache();
	void sync_comm_config_cache();
	void set_workflow_run_state(const QString& state);
	void sync_workflow_module_states();
	void init_workflow_module_states();
	void prepare_workflow_module_states();
	void initialize_workflow_module_instances();
	void mark_workflow_modules_ready();
	bool workflow_afsim_mode() const;
	bool workflow_icgas_mode() const;
	bool workflow_replay_mode() const;
	QString normalized_workflow_input_mode(const QString& input_mode) const;
	QStringList unsaved_workflow_changes() const;
	void set_comm_config_dirty(bool dirty);
	void set_initial_sim_ctrl_dirty(bool dirty);
	void set_initial_platform_type_dirty(bool dirty);
	void set_initial_weapon_type_dirty(bool dirty);
	void set_initial_sensor_type_dirty(bool dirty);
	void set_initial_scenario_dirty(bool dirty);
	void set_initial_platform_detail_dirty(bool dirty);
	void mark_initial_platform_type_dirty(const QString& type_id);
	void clear_initial_platform_type_dirty(const QString& type_id);
	void mark_initial_weapon_type_dirty(const QString& type_id);
	void clear_initial_weapon_type_dirty(const QString& type_id);
	void mark_initial_sensor_type_dirty(const QString& type_id);
	void clear_initial_sensor_type_dirty(const QString& type_id);
	void mark_initial_scenario_dirty(const QString& scenario_id);
	void clear_initial_scenario_dirty(const QString& scenario_id);
	void mark_initial_platform_detail_dirty(const QString& scenario_id);
	void clear_initial_platform_detail_dirty(const QString& scenario_id);
	void update_afs_client_workflow_state();
	void update_internal_sim_workflow_state();
	void update_concept_graph_workflow_state();
	void update_mission_workflow_state();
	void set_workflow_module_state(const QString& module_id, E_MODULE_RUN_STATE state,
		const QString& detail = QString(), const QString& error_text = QString());
	QVariantList make_workflow_module_state_rows() const;
	bool workflow_module_blocks(const S_MODULE_STATE& state) const;
	bool workflowRunnable() const;
	QString first_not_ready_workflow_module_name() const;
	QString workflow_module_state_key(E_MODULE_RUN_STATE state) const;
	QString workflow_module_state_label(E_MODULE_RUN_STATE state) const;
	E_MODULE_RUN_STATE aggregate_workflow_parent_state(const S_MODULE_STATE& state) const;
	E_MODULE_RUN_STATE aggregate_workflow_any_group(const std::vector<QString>& prerequisite_ids) const;
	E_MODULE_RUN_STATE combine_workflow_module_state(E_MODULE_RUN_STATE self_state,
		E_MODULE_RUN_STATE parent_state) const;
	void recompute_workflow_module_states();
	bool start_afs_client_comm();
	bool start_internal_sim_comm();
	bool start_afs_server_comm();
	bool start_tac_server_comm();
	void stop_afs_client_comm(bool clear_database);
	void stop_internal_sim_comm(bool clear_database);
	void stop_afs_server_comm();
	void stop_tac_server_comm();
	void stop_workflow_comm(bool clear_database);
	bool refresh_detail();
	bool append_log(const QString& info);
	void init_workflow_pdu_log_rows();
	void reset_workflow_pdu_log_rows(bool send_side);
	bool update_workflow_pdu_log_row(const QString& info);
	int workflow_pdu_log_slot(int pdu_type, bool send_side) const;
	QString workflow_pdu_log_line(int pdu_type, bool send_side,
		const QString& time_text, int count) const;
	const S_PLAT_DEFINE* find_sim_platform_define(const S_PLAT_CONFIG& config) const;
	const S_PLAT_CONFIG* find_sim_platform_config(const QString& platform_id) const;
	QString situation_platform_icon(const S_PLAT& plat) const;
	QString situation_weapon_icon(const S_WEAPON& weapon) const;
	QString situation_format_icon(const S_FORMAT& format) const;
	S_PLAT make_sim_plat_state(const S_PLAT_CONFIG& config, const S_PLAT_DEFINE* define) const;
	QVariantMap make_initial_platform_type_row(const S_PLAT_DEFINE& define) const;
	QVariantMap make_initial_weapon_type_row(const S_WEAPON_DEFINE& define) const;
	QVariantMap make_initial_sensor_type_row(const S_SENSOR_DEFINE& define) const;
	QVariantMap make_initial_platform_plot_row(const S_PLAT_CONFIG& config) const;
	QVariantMap make_initial_platform_row(const S_PLAT_CONFIG& config, const S_PLAT_DEFINE* define) const;

	struct ConceptDetailDraftState {
		QString mode;
		E_NODE_TYPE overview_type = E_NODE_TYPE::UKN;
		QString node_id;
		QString node_name;
		QString overview_status;
		QString node_status;
		QVariantList overview_rows;
		QVariantList node_edge_rows;
	};

	struct ConceptFocusActivation {
		QString node_id;
		std::set<QString> inspired_node_ids;
		std::set<QString> inspired_edge_ids;
	};

	S_CONCEPT_GRAPH* current_concept_graph();
	const S_CONCEPT_GRAPH* current_concept_graph() const;
	ConceptDetailDraftState capture_concept_detail_draft() const;
	void restore_concept_detail_draft(const ConceptDetailDraftState& state);
	void push_concept_undo_state();
	void clear_concept_undo_state();
	void mark_concept_detail_dirty();
	void clear_concept_focus_state();
	ConceptFocusActivation make_concept_focus_activation(const S_CONCEPT_GRAPH& graph, const QString& node_id) const;
	QPointF default_concept_focus_node_position(const S_CONCEPT_GRAPH& graph, const QString& node_id) const;
	QPointF avoid_concept_focus_overlap(const QPointF& desired, const QString& node_id) const;
	void seed_concept_focus_positions(const S_CONCEPT_GRAPH& graph, const ConceptFocusActivation& activation);
	QString concept_focus_parent_for(const QString& node_id) const;
	void collapse_concept_focus_node(const QString& node_id);
	void collect_concept_focus_subtree(const QString& node_id, std::set<QString>& subtree_nodes) const;
	S_GRAPH_NODE* find_concept_node(S_CONCEPT_GRAPH& graph, const QString& node_id) const;
	const S_GRAPH_NODE* find_concept_node(const S_CONCEPT_GRAPH& graph, const QString& node_id) const;
	QVariantMap make_pair_row(const QString& name, const QString& value) const;
	QVariantMap make_plat_row(const S_PLAT& plat) const;
	QVariantMap make_weapon_row(const S_WEAPON& weapon) const;
	QVariantMap make_format_row(const S_FORMAT& format) const;
	QVariantList make_plat_detail_rows(const S_PLAT& plat) const;
	QVariantList make_weapon_detail_rows(const S_WEAPON& weapon) const;
	QVariantList make_format_detail_rows(const S_FORMAT& format) const;
	bool validate_concept_graph(const S_CONCEPT_GRAPH& graph, QString* error_text) const;
	QString make_concept_edge_id(const S_CONCEPT_GRAPH& graph) const;
	int concept_lane_order_index(const E_NODE_TYPE type) const;
	double concept_graph_lane_height(const S_CONCEPT_GRAPH& graph, E_NODE_TYPE type) const;
	double concept_graph_lane_y(const S_CONCEPT_GRAPH& graph, E_NODE_TYPE type) const;
	QVariantMap make_concept_graph_node(const QString& id, const QString& type, const QString& title,
		const QString& name, double x, double y, double w, double h, const QString& fill_color,
		const QString& border_color, const QString& text_color, const QString& kind, bool editable,
		bool draggable, bool focus_active = false) const;
	QVariantMap make_concept_graph_edge(const QString& source_id, const QString& target_id,
		const QString& label, const QString& color) const;
	QString numberText(double value, int precision = 3) const;
	QString timeText(const long long time_ms) const;
	double show_zero_num(double value) const;

private:	// 私有变量
	static QMutex	mutex		;
	static UiICGAS*	instance	;

	QQmlApplicationEngine	engine		;
	QIcon					window_icon	;
	UiAFSimTest*			afsim_test	= nullptr	;
	UiScenario*				ui_scenario	= nullptr	;
	UiGraph*				ui_graph	= nullptr	;
	UiSitMonitor*			ui_sit_monitor = nullptr;
	UiWorkflow*				ui_workflow	= nullptr	;
	ModuleDatabase*			module_db	= nullptr	;	// 态势数据库
	ModuleMission*			module_mission = nullptr;	// 上级指令解析
	SimController*			sim_controller = nullptr;	// 原生仿真控制器
	QTimer*					timer_sync	= nullptr	;	// 显示定时刷新
	AfsimClient*			afs_client	= nullptr	;	// AFSIM客户端
	AfsimServer*			afs_server	= nullptr	;	// AFSIM服务端
	TacviewServer*			tac_server	= nullptr	;	// Tacview服务端

	bool afs_client_running	= false	;	// afs_client运行状态
	bool internal_sim_running = false	;	// 内置仿真运行状态
	bool afs_server_running	= false	;	// afs_server运行状态
	bool tac_server_running	= false	;	// tac_server运行状态
	QString workflow_run_state = "stopped";	// 工作流运行状态
	bool workflow_realtime_mode = true;	// 兼容旧实时/回放接口
	QString workflow_input_mode = QStringLiteral("icgas");	// afsim / icgas / replay
	bool shutdown_done		= false	;	// 退出清理状态
	bool deferred_startup_scheduled = false;
	bool deferred_startup_done = false;

	QStringList scenario_name_list	;
	QStringList sim_scenario_name_list	;
	QString selected_scenario		;
	bool afs_client_comm_started = false;
	bool afs_client_has_error = false;
	bool afs_client_received_data = false;
	QString afs_client_error_text;
	bool mission_config_dirty		= false	;
	QString afs_client_addr			;
	QString afs_client_port			;
	bool afs_client_config_dirty		= false	;
	bool comm_config_dirty			= false	;
	QString afs_server_addr			;
	QString afs_server_port			;
	QString tac_server_addr			;
	QString tac_server_port			;
	QString tac_server_user			;
	QString tac_send_iter_ms		;
	QString ui_sync_iter_ms		;
	QString database_plat_timeout_ms	= QStringLiteral("10000");
	QString database_algo_step_ms	= QStringLiteral("1000");
	bool database_config_dirty		= false	;

	QStringList workflow_log_rows	;
	std::map<QString, S_MODULE_STATE> workflow_module_states;
	QByteArray workflow_module_state_signature;
	long long last_situation_refresh_time_ms = 0;
	long long last_module_state_sync_time_ms = 0;
	bool situation_refresh_dirty = false;
	bool module_db_timer_started = false;
	bool sim_config_loaded = false;
	bool sim_plat_define_loaded = false;
	bool sim_plat_config_loaded = false;
	bool sim_weapon_define_loaded = false;
	bool sim_sensor_define_loaded = false;
	std::map<QString, S_PLAT_DEFINE> sim_plat_define_cache;
	std::map<QString, std::vector<S_PLAT_CONFIG>> sim_plat_config_cache;
	std::map<QString, S_WEAPON_DEFINE> sim_weapon_define_cache;
	std::map<QString, S_SENSOR_DEFINE> sim_sensor_define_cache;
	std::map<QString, S_PLAT> platform_state_cache;
	std::map<QString, S_PLAT> monitor_platform_cache;
	std::map<QString, S_WEAPON> monitor_weapon_cache;
	std::map<QString, S_FORMAT> monitor_format_cache;
	QVariantList platform_rows		;
	QVariantList primary_rows		;
	QVariantList secondary_rows		;
	QVariantList relative_rows		;
	QString primary_plat_id			;
	QString secondary_plat_id		;

	QVariantList initial_platform_type_rows;
	QVariantList initial_weapon_type_rows;
	QVariantList initial_sensor_type_rows;
	QStringList initial_platform_icon_options;
	bool initial_platform_icon_options_loaded = false;
	QStringList deleted_initial_platform_type_names;
	QStringList deleted_initial_weapon_type_names;
	QStringList deleted_initial_sensor_type_names;
	QVariantList initial_scenario_platform_rows;
	QVariantMap initial_selected_scenario_platform;
	QString initial_selected_scenario_platform_id;
	QString initial_sim_iter_ms = QStringLiteral("1000");
	QString initial_time_acc_ratio = QStringLiteral("1");
	QString initial_situation_status = QStringLiteral("未加载");
	QString initial_sim_ctrl_save_status;
	QString initial_platform_type_save_status;
	QString initial_weapon_type_save_status;
	QString initial_sensor_type_save_status;
	QString initial_scenario_save_status;
	QString initial_platform_detail_save_status;
	bool initial_sim_ctrl_dirty = false;
	bool initial_platform_type_dirty = false;
	bool initial_weapon_type_dirty = false;
	bool initial_sensor_type_dirty = false;
	bool initial_scenario_dirty = false;
	bool initial_platform_detail_dirty = false;
	std::set<QString> dirty_initial_platform_type_names;
	std::set<QString> dirty_initial_weapon_type_names;
	std::set<QString> dirty_initial_sensor_type_names;
	std::set<QString> dirty_initial_scenario_names;
	std::set<QString> dirty_initial_platform_detail_scenario_names;
	QString pending_initial_platform_type_save_context;
	QStringList pending_initial_platform_type_deleted_contexts;
	QString pending_initial_weapon_type_save_context;
	QStringList pending_initial_weapon_type_deleted_contexts;
	QString pending_initial_sensor_type_save_context;
	QStringList pending_initial_sensor_type_deleted_contexts;
	QString pending_initial_config_save_context;
	QString pending_initial_config_save_scenario_id;
	QString pending_initial_config_delete_context;
	QString pending_initial_config_delete_scenario_id;

	QStringList concept_scenario_names	;
	QString selected_concept_scenario	;
	QString concept_status				;
	QVariantList concept_scenario_rows	;
	bool concept_overview_mode			= true	;
	bool concept_focus_mode				= false	;
	QString concept_type_filter_page	= "node";
	bool concept_config_locked			= false	;
	bool concept_config_confirmed		= false	;
	bool concept_dirty					= false	;
	bool concept_detail_dirty			= false	;
	QString concept_mission_text		;
	QString concept_graph_title			;
	QString concept_detail_mode			= "empty";
	QString concept_detail_title		;
	QString concept_overview_type		;
	QString concept_overview_type_name	;
	QString concept_node_type_line		;
	QString concept_node_id_line		;
	QString concept_node_name			;
	QString concept_overview_apply_status = "无修改";
	QString concept_node_apply_status	= "无修改";
	QVariantList concept_node_type_rows	;
	QVariantList concept_edge_type_rows	;
	QVariantList concept_field_rows		;
	QVariantList concept_graph_lanes	;
	QVariantList concept_graph_nodes	;
	QVariantList concept_graph_edges	;
	QVariantList concept_overview_detail_rows;
	QVariantList concept_node_edge_rows	;
	std::set<E_NODE_TYPE> concept_user_node_types = {
		E_NODE_TYPE::ACT, E_NODE_TYPE::EFT, E_NODE_TYPE::DEC, E_NODE_TYPE::OBJ
	};
	bool concept_show_node_id			= false	;
	bool concept_show_node_name			= true	;
	bool concept_show_edge_id			= false	;
	bool concept_show_edge_name			= false	;
	bool concept_show_edge_weight		= true	;
	E_NODE_TYPE concept_detail_type		= E_NODE_TYPE::UKN;
	QString concept_detail_node_id		;
	std::map<QString, S_CONCEPT_GRAPH> concept_graph_cache;
	std::map<QString, S_CONCEPT_GRAPH> original_concept_graphs;
	std::set<QString> dirty_concept_scenarios;
	std::map<QString, QPointF> concept_overview_positions;
	std::map<QString, QPointF> concept_node_positions;
	std::map<QString, QPointF> concept_focus_node_positions;
	std::map<QString, ConceptFocusActivation> concept_focus_activations;
	std::map<QString, QString> concept_focus_parents;
	std::vector<QString> concept_focus_order;
	std::vector<E_NODE_TYPE> concept_lane_order = {
		E_NODE_TYPE::MIS, E_NODE_TYPE::COG, E_NODE_TYPE::OBJ, E_NODE_TYPE::LOO, E_NODE_TYPE::COA,
		E_NODE_TYPE::DEC, E_NODE_TYPE::EFT, E_NODE_TYPE::ACT, E_NODE_TYPE::TSK, E_NODE_TYPE::PLT
	};
};
