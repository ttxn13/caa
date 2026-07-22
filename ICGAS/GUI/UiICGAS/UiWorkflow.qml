import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Window 6.8
import ICGAS.Ui 1.0
pragma ComponentBehavior: Bound

Item {
id: workflowPage
property var appRoot: null
property var controller: uiWorkflow
readonly property var backend: controller && controller.backend ? controller.backend : uiICGAS
    
function workflowModuleDefinitionsData() {
    return [
        { "id": "input-afsim", "title": "AFSim客户端", "lane": "input", "meta": "AFSim场景输入", "inputs": ["PDU 实体", "DIS PDU 02", "DIS PDU 23"], "outputs": ["平台态势", "武器事件", "传感器状态"], "source": true, "defaultStatus": "ready", "defaultSchedule": "停用模块", "defaultX": 130, "defaultY": 18 },
        { "id": "input-internal-sim", "title": "内置仿真", "lane": "input", "meta": "ICGAS仿真场景输入", "inputs": ["仿真场景", "平台配置", "初始态势"], "outputs": ["平台态势", "武器事件", "传感器状态"], "source": true, "defaultStatus": "ready", "defaultSchedule": "实时刷新", "defaultX": 359, "defaultY": 18 },
        { "id": "input-replay", "title": "回放模式", "lane": "input", "meta": "回放数据加载", "inputs": ["回放 PDU 01", "回放 PDU 02", "回放 PDU 23"], "outputs": ["平台态势", "武器事件", "传感器状态"], "source": true, "defaultStatus": "ready", "defaultSchedule": "停用模块", "defaultX": 587, "defaultY": 18 },
        { "id": "input-concept-graph", "title": "概念图谱", "lane": "input", "meta": "概念图谱数据输入", "inputs": ["json配置文件"], "outputs": ["概念图谱"], "source": true, "defaultStatus": "ready", "defaultSchedule": "自动触发", "defaultX": 816, "defaultY": 18 },
        { "id": "input-command", "title": "上级指令", "lane": "input", "meta": "上级作战指令输入", "inputs": ["作战指令"], "outputs": ["指令语义"], "source": true, "defaultStatus": "done", "defaultSchedule": "手动运行", "defaultX": 1044, "defaultY": 18 },
        { "id": "database", "title": "数据库管理", "lane": "data", "meta": "实时刷新 | 必须运行 | 限本层移动", "inputs": ["敌我态势"], "outputs": ["态势快照", "知识图谱数据"], "fixed": true, "defaultStatus": "run", "defaultSchedule": "实时刷新", "defaultX": 359, "defaultY": 116 },
        { "id": "kg-parse", "title": "概念图谱解析", "lane": "data", "meta": "输入知识图谱，生成 DEC / EFT / ACT", "inputs": ["知识图谱"], "outputs": ["DEC", "EFT", "ACT"], "defaultStatus": "ready", "defaultSchedule": "自动触发", "defaultX": 816, "defaultY": 116 },
        { "id": "bvr-solver", "title": "超视距解算", "lane": "data", "meta": "超视距态势与目标关系解算", "inputs": ["敌我态势", "传感器数据"], "outputs": ["超视距解算结果"], "defaultStatus": "ready", "defaultSchedule": "自动触发", "defaultX": 130, "defaultY": 116 },
        { "id": "enemy-group", "title": "敌方目标分群", "lane": "analysis", "meta": "输入敌方态势，写入目标分群", "inputs": ["敌方态势"], "outputs": ["目标分群"], "defaultStatus": "ready", "defaultSchedule": "实时刷新", "defaultX": 138, "defaultY": 212 },
        { "id": "friendly-group", "title": "我方分群识别", "lane": "analysis", "meta": "我方态势分群识别", "inputs": ["我方态势"], "outputs": ["我方分群"], "defaultStatus": "ready", "defaultSchedule": "自动触发", "defaultX": 760, "defaultY": 212 },
        { "id": "enemy-intent", "title": "敌方意图分析", "lane": "analysis", "meta": "输入目标分群，写入敌方意图", "inputs": ["目标分群"], "outputs": ["敌方意图"], "defaultStatus": "ready", "defaultSchedule": "实时刷新", "defaultX": 392, "defaultY": 212 },
        { "id": "friendly-intent", "title": "我方意图识别", "lane": "analysis", "meta": "输入我方分群，识别我方意图", "inputs": ["我方分群"], "outputs": ["我方意图"], "defaultStatus": "ready", "defaultSchedule": "自动触发", "defaultX": 1000, "defaultY": 212 },
        { "id": "enemy-threat", "title": "我方威胁评估", "lane": "analysis", "meta": "态势 + 意图 + 分群，输出评分与排序", "inputs": ["敌方态势", "敌方意图", "目标分群"], "outputs": ["目标威胁评分", "分群威胁排序"], "defaultStatus": "wait", "defaultSchedule": "实时刷新", "defaultX": 276, "defaultY": 286 },
        { "id": "friendly-threat", "title": "我方威胁判断", "lane": "analysis", "meta": "分群 + 意图，完成威胁判断", "inputs": ["我方分群", "我方意图"], "outputs": ["我方威胁"], "defaultStatus": "wait", "defaultSchedule": "自动触发", "defaultX": 880, "defaultY": 286 },
        { "id": "friendly-cog", "title": "我方COG分析", "lane": "solution", "meta": "态势 + 分群 + 威胁，生成我方COG", "inputs": ["敌方态势", "目标分群", "威胁评估"], "outputs": ["我方COG"], "defaultStatus": "ready", "defaultSchedule": "自动触发", "defaultX": 138, "defaultY": 398 },
        { "id": "enemy-cog-template", "title": "敌方COG模板匹配", "lane": "solution", "meta": "威胁评估驱动敌方COG模板匹配", "inputs": ["威胁评估", "敌方COG模板库"], "outputs": ["敌方COG模板"], "defaultStatus": "ready", "defaultSchedule": "自动触发", "defaultX": 640, "defaultY": 398 },
        { "id": "loo", "title": "LOO作战线生成", "lane": "solution", "meta": "KG解析 + COG + 分群，输出 LOO", "inputs": ["KG解析结果", "我方COG", "目标分群", "敌我态势"], "outputs": ["LOO"], "defaultStatus": "wait", "defaultSchedule": "自动触发", "defaultX": 392, "defaultY": 398 },
        { "id": "behavior-template", "title": "我方LOO模板匹配", "lane": "solution", "meta": "敌方COG模板牵引我方LOO模板匹配", "inputs": ["我方威胁", "敌方COG模板", "LOO模板库"], "outputs": ["LOO模板"], "defaultStatus": "ready", "defaultSchedule": "自动触发", "defaultX": 880, "defaultY": 398 },
        { "id": "coa", "title": "我方COA生成", "lane": "solution", "meta": "汇总上述结果，生成 COA", "inputs": ["LOO", "威胁评估"], "outputs": ["COA"], "defaultStatus": "wait", "defaultSchedule": "自动触发", "defaultX": 378, "defaultY": 472 },
        { "id": "enemy-coa", "title": "敌方COA生成", "lane": "solution", "meta": "预测敌方COA", "inputs": ["LOO模板", "我方威胁"], "outputs": ["敌方COA"], "defaultStatus": "wait", "defaultSchedule": "自动触发", "defaultX": 880, "defaultY": 472 },
        { "id": "afsim-server", "title": "AFSim服务器", "lane": "command", "meta": "发送生成指令 | 控制作战进程", "inputs": ["COA", "控制指令"], "outputs": ["AFSim控制"], "server": true, "fixed": true, "defaultStatus": "wait", "defaultSchedule": "实时刷新", "defaultX": 138, "defaultY": 560 },
        { "id": "tacview-server", "title": "Tacview服务器", "lane": "command", "meta": "同步态势数据 | 三维态势展示", "inputs": ["态势快照", "COA", "敌方COA"], "outputs": ["Tacview显示"], "server": true, "fixed": true, "defaultStatus": "wait", "defaultSchedule": "实时刷新", "defaultX": 520, "defaultY": 560 }
    ]
}

function defaultPlacedModulesData() {
    return [
        { "id": "input-afsim", "lane": "input", "slot": "input-0-0", "row": 0, "col": 0, "schedule": "停用模块" },
        { "id": "input-internal-sim", "lane": "input", "slot": "input-0-2", "row": 0, "col": 2, "schedule": "实时刷新" },
        { "id": "input-replay", "lane": "input", "slot": "input-0-4", "row": 0, "col": 4, "schedule": "停用模块" },
        { "id": "input-concept-graph", "lane": "input", "slot": "input-0-6", "row": 0, "col": 6, "schedule": "自动触发" },
        { "id": "input-command", "lane": "input", "slot": "input-0-8", "row": 0, "col": 8, "schedule": "手动运行" },
        { "id": "database", "lane": "data", "slot": "data-0-2", "row": 0, "col": 2, "schedule": "实时刷新" },
        { "id": "kg-parse", "lane": "data", "slot": "data-0-6", "row": 0, "col": 6, "schedule": "自动触发" },
        { "id": "bvr-solver", "lane": "data", "slot": "data-0-0", "row": 0, "col": 0, "schedule": "自动触发" },
        { "id": "enemy-group", "lane": "analysis", "slot": "analysis-0-1", "row": 0, "col": 1, "schedule": "实时刷新" },
        { "id": "enemy-intent", "lane": "analysis", "slot": "analysis-0-3", "row": 0, "col": 3, "schedule": "实时刷新" },
        { "id": "enemy-threat", "lane": "analysis", "slot": "analysis-1-2", "row": 1, "col": 2, "schedule": "实时刷新" },
        { "id": "friendly-group", "lane": "analysis", "slot": "analysis-0-5", "row": 0, "col": 5, "schedule": "自动触发" },
        { "id": "friendly-intent", "lane": "analysis", "slot": "analysis-0-7", "row": 0, "col": 7, "schedule": "自动触发" },
        { "id": "friendly-threat", "lane": "analysis", "slot": "analysis-1-6", "row": 1, "col": 6, "schedule": "自动触发" },
        { "id": "friendly-cog", "lane": "solution", "slot": "solution-0-1", "row": 0, "col": 1, "schedule": "自动触发" },
        { "id": "loo", "lane": "solution", "slot": "solution-0-3", "row": 0, "col": 3, "schedule": "自动触发" },
        { "id": "coa", "lane": "solution", "slot": "solution-1-2", "row": 1, "col": 2, "schedule": "自动触发" },
        { "id": "enemy-cog-template", "lane": "solution", "slot": "solution-0-5", "row": 0, "col": 5, "schedule": "自动触发" },
        { "id": "behavior-template", "lane": "solution", "slot": "solution-0-7", "row": 0, "col": 7, "schedule": "自动触发" },
        { "id": "enemy-coa", "lane": "solution", "slot": "solution-1-6", "row": 1, "col": 6, "schedule": "自动触发" },
        { "id": "afsim-server", "lane": "command", "slot": "command-0-3", "row": 0, "col": 3, "schedule": "实时刷新" },
        { "id": "tacview-server", "lane": "command", "slot": "command-0-5", "row": 0, "col": 5, "schedule": "实时刷新" }
    ]
}

function workflowEdgesData() {
    return [
        { "sourceId": "input-afsim", "targetId": "database" },
        { "sourceId": "input-internal-sim", "targetId": "database" },
        { "sourceId": "input-replay", "targetId": "database" },
        { "sourceId": "input-command", "targetId": "kg-parse" },
        { "sourceId": "input-concept-graph", "targetId": "kg-parse" },
        { "sourceId": "bvr-solver", "targetId": "database" },
        { "sourceId": "database", "targetId": "enemy-group" },
        { "sourceId": "enemy-group", "targetId": "enemy-intent" },
        { "sourceId": "database", "targetId": "friendly-group" },
        { "sourceId": "friendly-group", "targetId": "friendly-intent" },
        { "sourceId": "enemy-intent", "targetId": "enemy-threat" },
        { "sourceId": "enemy-group", "targetId": "enemy-threat" },
        { "sourceId": "enemy-threat", "targetId": "friendly-cog" },
        { "sourceId": "enemy-threat", "targetId": "loo" },
        { "sourceId": "friendly-cog", "targetId": "loo" },
        { "sourceId": "kg-parse", "targetId": "database" },
        { "sourceId": "loo", "targetId": "coa" },
        { "sourceId": "friendly-group", "targetId": "friendly-threat" },
        { "sourceId": "friendly-intent", "targetId": "friendly-threat" },
        { "sourceId": "friendly-threat", "targetId": "behavior-template" },
        { "sourceId": "friendly-threat", "targetId": "enemy-cog-template" },
        { "sourceId": "enemy-cog-template", "targetId": "behavior-template" },
        { "sourceId": "behavior-template", "targetId": "enemy-coa" },
        { "sourceId": "coa", "targetId": "tacview-server" },
        { "sourceId": "coa", "targetId": "afsim-server" },
        { "sourceId": "enemy-coa", "targetId": "tacview-server" }
    ]
}
readonly property string uiFontFamily: appRoot ? appRoot.uiFontFamily : "Microsoft YaHei UI"
readonly property int uiFontSize: appRoot ? appRoot.uiFontSize : 13
readonly property int uiSmallFontSize: appRoot ? appRoot.uiSmallFontSize : 12
readonly property color uiFocusColor: appRoot ? appRoot.uiFocusColor : "#2f80ed"
readonly property color uiBorderColor: appRoot ? appRoot.uiBorderColor : "#c9d2dd"
readonly property var workflowStatusLegendRows: appRoot ? appRoot.workflowStatusLegendRows : []
    
function workflowStatusColor(statusKind) {
    return appRoot ? appRoot.workflowStatusColor(statusKind) : "#64748b"
}
    
property string inputMode: appRoot ? appRoot.workflowInputMode : "icgas"
property bool realtimeMode: inputMode !== "replay"
property int workflowSubTab: 0
property string selectedModuleId: ""
readonly property bool hasSelectedModule: selectedModuleId.length > 0
readonly property string selectedModule: hasSelectedModule ? moduleTitle(selectedModuleId) : "未选择模块"
readonly property int workflowNodeHeight: 64
readonly property int workflowNodeWidth: 176
readonly property int workflowDesignWidth: 1260
readonly property int workflowDesignHeight: 650
readonly property int workflowGridLeft: 104
readonly property int workflowGridRightMargin: 14
readonly property int workflowMajorColumns: 5
readonly property int workflowSnapColumns: 9
readonly property int workflowLaneGap: 10
readonly property int workflowTopMargin: 8
readonly property int workflowBottomMargin: 10
property int workflowCanvasWidthHint: 1260
property int workflowCanvasHeightHint: 650
readonly property var workflowLanes: makeWorkflowLanes(workflowCanvasHeightHint)
property string draggingModuleId: ""
property real draggingModuleX: 0
property real draggingModuleY: 0
property bool draggingSnapPreviewActive: false
property real draggingSnapPreviewX: 0
property real draggingSnapPreviewY: 0
property int moduleStatusRefreshTick: 0
readonly property bool moduleSettingsLocked: backend.workflowRunning || backend.workflowPaused
property string afsClientDraftAddr: backend.afsClientAddr
property string afsClientDraftPort: backend.afsClientPort
property string databasePlatTimeoutDraft: backend.databasePlatTimeoutMs
property string databaseAlgoStepDraft: backend.databaseAlgoStepMs
readonly property bool databaseDraftDirty: databasePlatTimeoutDraft !== backend.databasePlatTimeoutMs ||
                                           databaseAlgoStepDraft !== backend.databaseAlgoStepMs
readonly property string missionCommittedId: backend.missionId.length > 0 ? backend.missionId : backend.defaultMissionId
property string missionIdDraft: missionCommittedId
property string missionSourceDraft: backend.missionSourceText
readonly property string missionNormalizedDraftId: missionIdDraft.trim().length > 0 ? missionIdDraft.trim() : backend.defaultMissionId
readonly property bool missionDraftDirty: missionNormalizedDraftId !== missionCommittedId || missionSourceDraft !== backend.missionSourceText
property real replaySpeedValue: 1.0
property real replaySpeedDraft: replaySpeedValue
readonly property bool replaySpeedDirty: Math.abs(replaySpeedDraft - replaySpeedValue) > 0.0001
onMissionDraftDirtyChanged: backend.setMissionConfigDraftDirty(missionDraftDirty)
readonly property var workflowModuleDefinitions: workflowModuleDefinitionsData()
property var placedModules: defaultPlacedModulesData()
readonly property var workflowEdges: workflowEdgesData()
readonly property var workflowSnapSlots: makeWorkflowSnapSlots()
    
onRealtimeModeChanged: {
    syncInputSourceModeFromRealtime()
    workflowConnections.requestPaint()
}
onInputModeChanged: {
    syncInputSourceModeFromRealtime()
    workflowConnections.requestPaint()
}
onAfsClientDraftAddrChanged: markAfsClientDraftDirty()
onAfsClientDraftPortChanged: markAfsClientDraftDirty()
onDatabaseDraftDirtyChanged: {
    backend.setDatabaseConfigDraftDirty(databaseDraftDirty)
    workflowConnections.requestPaint()
}
onWorkflowCanvasWidthHintChanged: workflowConnections.requestPaint()
onWorkflowCanvasHeightHintChanged: workflowConnections.requestPaint()
    
function definitionForId(moduleId) {
    for (var i = 0; i < workflowModuleDefinitions.length; ++i) {
        if (workflowModuleDefinitions[i].id === moduleId) {
            return workflowModuleDefinitions[i]
        }
    }
    return ({})
}
    
function laneForId(laneId) {
    for (var i = 0; i < workflowLanes.length; ++i) {
        if (workflowLanes[i].id === laneId) {
            return workflowLanes[i]
        }
    }
    return workflowLanes[2]
}
    
function laneTitle(laneId) {
    return laneForId(laneId).title
}
    
function moduleDefinitionsForLane(laneId) {
    var modules = []
    for (var i = 0; i < workflowModuleDefinitions.length; ++i) {
        if (workflowModuleDefinitions[i].lane === laneId) {
            modules.push(workflowModuleDefinitions[i])
        }
    }
    return modules
}
    
function workflowModuleStatusGroups() {
    var groups = []
    var laneOrder = ["input", "data", "analysis", "solution", "command"]
    for (var i = 0; i < laneOrder.length; ++i) {
        var laneId = laneOrder[i]
        groups.push({
            "id": laneId,
            "title": laneTitle(laneId),
            "modules": moduleDefinitionsForLane(laneId)
        })
    }
    return groups
}
    
function moduleTitle(moduleId) {
    var def = definitionForId(moduleId)
    return def.title || ""
}
    
function moduleWidth(module) {
    return workflowNodeWidth
}
    
function placedModule(moduleId) {
    for (var i = 0; i < placedModules.length; ++i) {
        if (placedModules[i].id === moduleId) {
            return placedModules[i]
        }
    }
    return null
}
    
function isPlaced(moduleId) {
    return placedModule(moduleId) !== null
}
    
function moduleDisabled(module) {
    if (isInputSourceModule(module.id)) {
        return module.id !== activeInputModuleId()
    }
    var stateKind = backend.workflowModuleStateKind(module.id)
    if (stateKind === "disabled") {
        return true
    }
    var schedule = moduleSchedule(module)
    if ((module.id === "bvr-solver" ||
         module.id === "friendly-group" ||
         module.id === "friendly-intent" ||
         module.id === "friendly-threat") &&
        schedule === "停用模块") {
        return true
    }
    if (module.id === "input-afsim") {
        return schedule === "停用模块"
    }
    if (module.id === "input-internal-sim") {
        return schedule === "停用模块"
    }
    if (module.id === "input-replay") {
        return schedule === "停用模块"
    }
    if (module.id === "afsim-server") {
        return schedule === "停用模块"
    }
    if (module.id === "tacview-server") {
        return schedule === "停用模块"
    }
    return false
}
    
function moduleMeta(module) {
    var def = definitionForId(module.id)
    if (module.id === "input-afsim") {
        return moduleDisabled(module) ? "AFSim输入 | 已停用" : "AFSim输入 | 已启用"
    }
    if (module.id === "input-internal-sim") {
        return moduleDisabled(module) ? "ICGAS仿真场景输入 | 已停用" : "ICGAS仿真场景输入 | 已启用"
    }
    if (module.id === "input-replay") {
        return moduleDisabled(module) ? "回放数据加载 | 已停用" : "回放数据加载 | 已启用"
    }
    if (module.id === "tacview-server") {
        return moduleDisabled(module) ? "同步态势数据 | 已停用" : "同步态势数据 | 实时刷新"
    }
    if (module.id === "bvr-solver") {
        return "超视距态势与目标关系解算"
    }
    if (module.id === "friendly-intent") {
        return "输入我方分群，识别我方意图"
    }
    if (module.id === "behavior-template") {
        return "敌方COG模板牵引我方LOO模板匹配"
    }
    return def.meta || ""
}
    
function moduleStatus(module, refreshTick) {
    var def = definitionForId(module.id)
    if (moduleDisabled(module)) {
        return "disabled"
    }
    if (module.id === "input-afsim" && backend.afsClientConfigDirty) {
        return "not_ready"
    }
    var stateKind = backend.workflowModuleStateKind(module.id)
    if (stateKind && stateKind.length > 0 && stateKind !== "unknown") {
        return stateKind
    }
    return def.defaultStatus || "ready"
}
    
function moduleStatusLabel(statusKind) {
    if (statusKind === "run") {
        return "运行中"
    }
    if (statusKind === "done" || statusKind === "complete" || statusKind === "completed") {
        return "完成"
    }
    if (statusKind === "wait") {
        return "等待中"
    }
    if (statusKind === "not_ready") {
        return "未就绪"
    }
    if (statusKind === "disabled") {
        return "关闭"
    }
    if (statusKind === "unknown") {
        return "未知"
    }
    if (statusKind === "fail" || statusKind === "failed") {
        return "失败"
    }
    return "已就绪"
}
    
function moduleStatusDetail(module) {
    if (module.id === "input-afsim" && backend.afsClientConfigDirty) {
        return "配置未保存"
    }
    return moduleSchedule(module)
}
    
function moduleSchedule(module) {
    if (!module || !module.id) {
        return ""
    }
    if (module.schedule && module.schedule.length > 0) {
        return module.schedule
    }
    var placed = placedModule(module.id)
    if (placed && placed.schedule && placed.schedule.length > 0) {
        return placed.schedule
    }
    var def = definitionForId(module.id)
    return def.defaultSchedule || "手动运行"
}
    
function moduleStatusSummary(module, refreshTick) {
    var label = moduleStatusLabel(moduleStatus(module, refreshTick))
    var detail = moduleStatusDetail(module)
    return detail.length > 0 ? label + " · " + detail : label
}
    
function selectedServerRunning() {
    if (selectedModuleId === "afsim-server") {
        return backend.afsServerRunning
    }
    if (selectedModuleId === "tacview-server") {
        return backend.tacServerRunning
    }
    return false
}
    
function selectedServerStateText() {
    return selectedServerRunning() ? "运行中" : "已关闭"
}
    
function toggleSelectedServer() {
    if (moduleSettingsLocked) {
        return
    }
    if (selectedModuleId === "afsim-server") {
        backend.toggleAfsServer()
        workflowConnections.requestPaint()
    } else if (selectedModuleId === "tacview-server") {
        backend.toggleTacServer()
    }
}
    
function selectedIsServerModule() {
    return hasSelectedModule && selectedDefinition().server === true
}
    
function selectedIsAfsClientModule() {
    return selectedModuleId === "input-afsim"
}

function selectedIsInternalSimModule() {
    return selectedModuleId === "input-internal-sim"
}
    
function selectedIsReplayModule() {
    return selectedModuleId === "input-replay"
}
    
function selectedIsConceptGraphModule() {
    return selectedModuleId === "input-concept-graph"
}
    
function selectedIsAfsServerModule() {
    return selectedModuleId === "afsim-server"
}
    
function selectedIsTacviewServerModule() {
    return selectedModuleId === "tacview-server"
}
    
function selectedIsCommModule() {
    return selectedIsAfsClientModule() || selectedIsAfsServerModule() || selectedIsTacviewServerModule()
}
    
function selectedIsMissionModule() {
    return selectedModuleId === "input-command"
}

function selectedIsDatabaseModule() {
    return selectedModuleId === "database"
}
    
function resetMissionDraft() {
    missionIdDraft = missionCommittedId
    missionSourceDraft = backend.missionSourceText
    backend.setMissionConfigDraftDirty(false)
}
    
function clearMissionDraft() {
    if (moduleSettingsLocked) {
        return
    }
    missionIdDraft = backend.defaultMissionId
    missionSourceDraft = ""
    backend.setMissionConfigDraftDirty(missionDraftDirty)
}
    
function saveMissionDraft() {
    if (moduleSettingsLocked) {
        return
    }
    var savedId = missionNormalizedDraftId
    var savedText = missionSourceDraft
    if (backend.saveMissionConfig(savedId, savedText)) {
        missionIdDraft = savedId
        missionSourceDraft = savedText
        backend.setMissionConfigDraftDirty(false)
    }
}
    
function markAfsClientDraftDirty() {
    if (!selectedIsAfsClientModule() || moduleSettingsLocked || backend.afsClientRunning) {
        return
    }
    backend.setAfsClientConfigDraftDirty(afsClientDraftAddr !== backend.afsClientAddr ||
                                        afsClientDraftPort !== backend.afsClientPort)
}
    
function resetAfsClientDraft() {
    afsClientDraftAddr = backend.afsClientAddr
    afsClientDraftPort = backend.afsClientPort
    backend.resetAfsClientConfigDraft()
}
    
function saveAfsClientDraft() {
    if (moduleSettingsLocked || backend.afsClientRunning) {
        return
    }
    if (backend.saveAfsClientConfig(afsClientDraftAddr, afsClientDraftPort)) {
        afsClientDraftAddr = backend.afsClientAddr
        afsClientDraftPort = backend.afsClientPort
    }
}

function resetDatabaseDraft() {
    databasePlatTimeoutDraft = backend.databasePlatTimeoutMs
    databaseAlgoStepDraft = backend.databaseAlgoStepMs
}

function saveDatabaseDraft() {
    if (moduleSettingsLocked) {
        return
    }
    if (backend.saveDatabaseConfig(databasePlatTimeoutDraft, databaseAlgoStepDraft)) {
        resetDatabaseDraft()
    }
}
    
function replaySpeedText(value) {
    var fixed = Number(value).toFixed(1)
    return fixed.replace(/\.0$/, "") + "x"
}
    
function confirmReplaySpeed() {
    if (moduleSettingsLocked) {
        return
    }
    replaySpeedValue = Math.max(0.1, Math.min(100, replaySpeedDraft))
    replaySpeedDraft = replaySpeedValue
}
    
function resetReplaySpeed() {
    replaySpeedDraft = replaySpeedValue
}
    
function openConceptGraphPage() {
    workflowPage.appRoot.activeMainTabIndex = 1
    backend.setConceptNavMode("graph")
}

function openAFSimTestPage() {
    workflowPage.appRoot.setWorkflowInputMode("afsim")
    workflowPage.appRoot.requestMainTabChange(0)
}
    
function isInputSourceModule(moduleId) {
    return moduleId === "input-afsim" || moduleId === "input-internal-sim" || moduleId === "input-replay"
}
    
function scheduleOptionEnabled(moduleId, schedule) {
    if (moduleSettingsLocked) {
        return false
    }
    if (!selectedScheduleMutable()) {
        return false
    }
    return scheduleOptionsForModule(moduleId).indexOf(schedule) >= 0
}
    
function scheduleOptionsForModule(moduleId) {
    if (isInputSourceModule(moduleId)) {
        return ["实时刷新", "停用模块"]
    }
    if (moduleId === "afsim-server") {
        return ["实时刷新", "停用模块"]
    }
    if (moduleId === "kg-parse") {
        return ["自动触发", "手动运行"]
    }
    if (moduleId === "bvr-solver") {
        return ["实时刷新", "自动触发", "停用模块"]
    }
    if (moduleId === "database") {
        return ["实时刷新"]
    }
    if (moduleId === "enemy-group" || moduleId === "enemy-intent" || moduleId === "enemy-threat") {
        return ["实时刷新", "手动运行"]
    }
    if (moduleId === "friendly-group" || moduleId === "friendly-intent" || moduleId === "friendly-threat") {
        return ["实时刷新", "手动运行", "停用模块"]
    }
    if (moduleId === "input-concept-graph") {
        return ["自动触发", "手动运行"]
    }
    if (moduleId === "input-command") {
        return ["手动运行", "停用模块"]
    }
    if (moduleId === "tacview-server") {
        return ["实时刷新", "停用模块"]
    }
    return ["实时刷新", "自动触发", "手动运行", "停用模块"]
}
    
function updateModuleSchedule(next, moduleId, schedule) {
    for (var i = 0; i < next.length; ++i) {
        if (next[i].id === moduleId) {
            next[i] = {
                "id": next[i].id,
                "lane": next[i].lane,
                "slot": next[i].slot,
                "row": next[i].row,
                "col": next[i].col,
                "schedule": schedule
            }
            return true
        }
    }
    return false
}

function inputModeForModule(moduleId) {
    if (moduleId === "input-afsim") {
        return "afsim"
    }
    if (moduleId === "input-replay") {
        return "replay"
    }
    return "icgas"
}

function activeInputModuleId() {
    if (workflowPage.inputMode === "afsim") {
        return "input-afsim"
    }
    if (workflowPage.inputMode === "replay") {
        return "input-replay"
    }
    return "input-internal-sim"
}
    
function setInputSourceMode(activeModuleId) {
    var next = placedModules.slice()
    var changed = false
    var ids = ["input-afsim", "input-internal-sim", "input-replay"]
    for (var i = 0; i < ids.length; ++i) {
        changed = updateModuleSchedule(next, ids[i], ids[i] === activeModuleId ? "实时刷新" : "停用模块") || changed
    }
    workflowPage.appRoot.setWorkflowInputMode(inputModeForModule(activeModuleId))
    workflowPage.inputMode = inputModeForModule(activeModuleId)
    if (changed) {
        setPlacedModules(next)
    }
    workflowPage.moduleStatusRefreshTick += 1
}
    
function syncInputSourceModeFromRealtime() {
    var activeModuleId = activeInputModuleId()
    var next = placedModules.slice()
    var changed = false
    var ids = ["input-afsim", "input-internal-sim", "input-replay"]
    for (var i = 0; i < ids.length; ++i) {
        changed = updateModuleSchedule(next, ids[i], ids[i] === activeModuleId ? "实时刷新" : "停用模块") || changed
    }
    if (changed) {
        setPlacedModules(next)
    }
}
    
function workflowMainLogRows(logRows) {
    var rows = logRows || []
    var defaults = [
        "[--:--:--.---] DIS PDU-01 recv. Num 0.",
        "[--:--:--.---] DIS PDU-01 send. Num 0.",
        "[--:--:--.---] DIS PDU-02 recv. Num 0.",
        "[--:--:--.---] DIS PDU-02 send. Num 0.",
        "[--:--:--.---] DIS PDU-03 recv. Num 0.",
        "[--:--:--.---] DIS PDU-03 send. Num 0.",
        "[--:--:--.---] DIS PDU-23 recv. Num 0.",
        "[--:--:--.---] DIS PDU-23 send. Num 0."
    ]
    var result = []
    for (var i = 0; i < defaults.length; ++i) {
        result.push(i < rows.length && rows[i] ? rows[i] : defaults[i])
    }
    return result
    /*
    if (rows.length === 0) {
        return ["[日志] 等待通信与流程事件"]
    }
    return rows
    */
}
    
function workflowTimelineRows(refreshTick) {
    var inputTitle = workflowPage.inputMode === "afsim" ? "AFSim场景输入" :
                     workflowPage.inputMode === "replay" ? "回放场景输入" : "ICGAS仿真场景输入"
    var inputDetail = workflowPage.inputMode === "replay" ? workflowPage.appRoot.selectedWorkflowReplayVenue : backend.selectedScenario
    return [
        {
            "time": "Tick " + refreshTick,
            "title": inputTitle,
            "detail": inputDetail,
            "status": workflowPage.inputMode === "replay" ? "ready" : "run"
        },
        {
            "time": "输入",
            "title": "AFSim DIS接收",
            "detail": backend.afsClientRunning ? "接收中 · " + backend.afsClientAddr + ":" + backend.afsClientPort : "等待启动",
            "status": backend.afsClientRunning ? "run" : "wait"
        },
        {
            "time": "同步",
            "title": "态势数据写入",
            "detail": backend.afsClientRunning ? "态势帧持续刷新，数据库模块待消费" : "等待上游态势帧",
            "status": backend.afsClientRunning ? "run" : "wait"
        },
        {
            "time": "控制",
            "title": "AFSim控制出口",
            "detail": backend.afsServerRunning ? "控制出口已开启" : "等待COA输出控制指令",
            "status": backend.afsServerRunning ? "run" : "wait"
        },
        {
            "time": "展示",
            "title": "Tacview同步",
            "detail": backend.tacServerRunning ? "服务运行 · " + backend.tacServerAddr + ":" + backend.tacServerPort : "等待启动",
            "status": backend.tacServerRunning ? "run" : "wait"
        }
    ]
}
    
function selectedDefinition() {
    if (!hasSelectedModule) {
        return ({})
    }
    return definitionForId(selectedModuleId)
}
    
function selectedPlacedModule() {
    if (!hasSelectedModule) {
        return null
    }
    return placedModule(selectedModuleId)
}
    
function selectedInputs() {
    var def = selectedDefinition()
    return def.inputs || []
}
    
function selectedOutputs() {
    var def = selectedDefinition()
    return def.outputs || []
}
    
function selectedLaneTitle() {
    if (!hasSelectedModule) {
        return ""
    }
    var module = selectedPlacedModule()
    return laneTitle(module ? module.lane : selectedDefinition().lane)
}
    
function selectedSchedule() {
    if (!hasSelectedModule) {
        return ""
    }
    return moduleSchedule(selectedPlacedModule() || selectedDefinition())
}
    
function setSelectedSchedule(schedule) {
    if (!hasSelectedModule || !selectedScheduleMutable()) {
        return
    }
    if (isInputSourceModule(selectedModuleId)) {
        if (schedule === "实时刷新") {
            setInputSourceMode(selectedModuleId)
        } else if (schedule === "停用模块") {
            setInputSourceMode("input-internal-sim")
        }
        return
    }
    if (!scheduleOptionEnabled(selectedModuleId, schedule)) {
        return
    }
    var next = placedModules.slice()
    if (updateModuleSchedule(next, selectedModuleId, schedule)) {
        setPlacedModules(next)
    }
}
    
function selectedDraggableText() {
    if (!hasSelectedModule) {
        return ""
    }
    return "是（限本层）"
}
    
function selectedScheduleMutable() {
    if (moduleSettingsLocked) {
        return false
    }
    if (selectedModuleId === "afsim-server") {
        return workflowPage.inputMode === "afsim"
    }
    if (!hasSelectedModule) {
        return false
    }
    if (isInputSourceModule(selectedModuleId)) {
        return true
    }
    if (selectedModuleId === "input-command" || selectedModuleId === "input-concept-graph") {
        return true
    }
    if (selectedModuleId === "tacview-server" || selectedModuleId === "kg-parse" ||
        selectedModuleId === "bvr-solver" || selectedModuleId === "database" ||
        selectedModuleId === "enemy-group" || selectedModuleId === "enemy-intent" ||
        selectedModuleId === "enemy-threat" || selectedModuleId === "friendly-group" ||
        selectedModuleId === "friendly-intent" || selectedModuleId === "friendly-threat") {
        return true
    }
    return !moduleLaneLocked(selectedDefinition())
}
    
function moduleLaneLocked(def) {
    return def.fixed === true || def.source === true || def.server === true
}
    
function moduleDragLaneId(moduleId, fallbackLane) {
    var def = definitionForId(moduleId)
    return def.id ? def.lane : (fallbackLane || "analysis")
}
    
function dragSourceText(source, fieldName) {
    return source ? String(source[fieldName]) : ""
}
    
function activateModule(moduleId) {
    selectedModuleId = moduleId
    workflowConnections.requestPaint()
}
    
function setDraggingModule(moduleId, nodeX, nodeY) {
    draggingModuleId = moduleId || ""
    draggingModuleX = nodeX
    draggingModuleY = nodeY
    updateDraggingSnapPreview(moduleId, nodeX, nodeY)
    workflowConnections.requestPaint()
}
    
function clearSelectedModule() {
    selectedModuleId = ""
    workflowConnections.requestPaint()
}
    
function clearDraggingModule(moduleId) {
    if (draggingModuleId === "" || draggingModuleId === moduleId) {
        draggingModuleId = ""
        draggingSnapPreviewActive = false
        workflowConnections.requestPaint()
    }
}
    
function setPlacedModules(nextModules) {
    placedModules = nextModules
    workflowConnections.requestPaint()
}
    
function makeWorkflowLanes(canvasHeight) {
    var laneGap = workflowLaneGap
    var inputBaseHeight = 74
    var dataBaseHeight = 76
    var analysisBaseHeight = 178
    var solutionBaseHeight = 178
    var commandBaseHeight = 86
    var baseHeight = inputBaseHeight + dataBaseHeight + analysisBaseHeight + solutionBaseHeight + commandBaseHeight
    var availableHeight = Math.max(baseHeight,
                                   Math.max(workflowDesignHeight, canvasHeight) - workflowTopMargin - workflowBottomMargin - laneGap * 4)
    var extraHeight = availableHeight - baseHeight
    var inputHeight = Math.round(inputBaseHeight + extraHeight * 0.08)
    var dataHeight = Math.round(dataBaseHeight + extraHeight * 0.10)
    var analysisHeight = Math.round(analysisBaseHeight + extraHeight * 0.31)
    var solutionHeight = Math.round(solutionBaseHeight + extraHeight * 0.34)
    var commandHeight = Math.max(commandBaseHeight, availableHeight - inputHeight - dataHeight - analysisHeight - solutionHeight)
    var y = workflowTopMargin
    var lanes = [
        { "id": "input", "title": "输入源", "y": y, "height": inputHeight }
    ]
    y += inputHeight + laneGap
    lanes.push({ "id": "data", "title": "数据层", "y": y, "height": dataHeight })
    y += dataHeight + laneGap
    lanes.push({ "id": "analysis", "title": "分析层", "y": y, "height": analysisHeight })
    y += analysisHeight + laneGap
    lanes.push({ "id": "solution", "title": "方案层", "y": y, "height": solutionHeight })
    y += solutionHeight + laneGap
    lanes.push({ "id": "command", "title": "指令层", "y": y, "height": commandHeight })
    return lanes
}
    
function makeWorkflowSnapSlots() {
    var slots = []
    for (var laneIndex = 0; laneIndex < workflowLanes.length; ++laneIndex) {
        var lane = workflowLanes[laneIndex]
        var rowCount = laneRowCount(lane.id)
        for (var row = 0; row < rowCount; ++row) {
            for (var col = 0; col < workflowSnapColumns; ++col) {
                slots.push({
                    "id": lane.id + "-" + row + "-" + col,
                    "lane": lane.id,
                    "row": row,
                    "col": col
                })
            }
        }
    }
    return slots
}
    
function laneRowCount(laneId) {
    return (laneId === "analysis" || laneId === "solution") ? 2 : 1
}
    
function clampValue(value, minValue, maxValue) {
    return Math.max(minValue, Math.min(maxValue, value))
}
    
function normalizedColumn(col) {
    return clampValue(Math.round(col), 0, workflowSnapColumns - 1)
}
    
function normalizedRow(laneId, row) {
    return clampValue(Math.round(row), 0, laneRowCount(laneId) - 1)
}
    
function gridCellWidth() {
    var availableWidth = Math.max(workflowMajorColumns * workflowNodeWidth, workflowCanvasWidthHint - workflowGridLeft - workflowGridRightMargin)
    return availableWidth / workflowMajorColumns
}
    
function gridXForColumn(col) {
    var normalizedCol = normalizedColumn(col)
    var halfCellWidth = gridCellWidth() / 2
    return Math.round(workflowGridLeft + normalizedCol * halfCellWidth + (gridCellWidth() - workflowNodeWidth) / 2)
}
    
function gridYForRow(laneId, row) {
    var lane = laneForId(laneId)
    var normalizedRowValue = normalizedRow(laneId, row)
    var rowCount = laneRowCount(laneId)
    if (rowCount === 1) {
        return Math.round(lane.y + (lane.height - workflowNodeHeight) / 2)
    }
    var topY = lane.y + 14
    var bottomY = lane.y + lane.height - workflowNodeHeight - 14
    return Math.round(topY + (bottomY - topY) * normalizedRowValue / (rowCount - 1))
}
    
function slotX(slot) {
    return gridXForColumn(slot.col)
}
    
function slotY(slot) {
    return gridYForRow(slot.lane, slot.row)
}
    
function workflowRoutingColumns() {
    var columns = []
    for (var col = 0; col < workflowSnapColumns; ++col) {
        columns.push(gridXForColumn(col) + workflowNodeWidth / 2)
    }
    return columns
}
    
function workflowRoutingRows() {
    var centers = []
    for (var laneIndex = 0; laneIndex < workflowLanes.length; ++laneIndex) {
        var lane = workflowLanes[laneIndex]
        for (var row = 0; row < laneRowCount(lane.id); ++row) {
            centers.push(gridYForRow(lane.id, row) + workflowNodeHeight / 2)
        }
    }
    centers.sort(function(a, b) { return a - b })
    
    var rows = []
    if (centers.length > 1) {
        rows.push(Math.round(Math.max(0, centers[0] - (centers[1] - centers[0]) / 2)))
    }
    for (var i = 0; i < centers.length; ++i) {
        rows.push(Math.round(centers[i]))
        if (i + 1 < centers.length) {
            rows.push(Math.round((centers[i] + centers[i + 1]) / 2))
        }
    }
    if (centers.length > 1) {
        var last = centers.length - 1
        rows.push(Math.round(Math.min(workflowCanvasHeightHint, centers[last] + (centers[last] - centers[last - 1]) / 2)))
    }
    return rows
}
    
function slotForModuleId(moduleId) {
    for (var i = 0; i < workflowSnapSlots.length; ++i) {
        if (workflowSnapSlots[i].id === moduleId) {
            return workflowSnapSlots[i]
        }
    }
    return null
}
    
function slotOccupied(slotId, exceptId) {
    for (var i = 0; i < placedModules.length; ++i) {
        var module = placedModules[i]
        var moduleCurrentSlot = moduleSlot(module)
        if (module.id !== exceptId && moduleCurrentSlot && moduleCurrentSlot.id === slotId) {
            return true
        }
    }
    return false
}
    
function nearestFreeSlot(x, y, laneId, exceptId) {
    var bestSlot = null
    var bestDistance = Number.MAX_VALUE
    var centerX = x + workflowNodeWidth / 2
    var centerY = y + workflowNodeHeight / 2
    for (var i = 0; i < workflowSnapSlots.length; ++i) {
        var slot = workflowSnapSlots[i]
        if (laneId && slot.lane !== laneId) {
            continue
        }
        if (slotOccupied(slot.id, exceptId)) {
            continue
        }
        var slotCenterX = slotX(slot) + workflowNodeWidth / 2
        var slotCenterY = slotY(slot) + workflowNodeHeight / 2
        var distance = Math.abs(centerX - slotCenterX) + Math.abs(centerY - slotCenterY)
        if (distance < bestDistance) {
            bestDistance = distance
            bestSlot = slot
        }
    }
    return bestSlot
}
    
function nearestSlot(x, y, laneId) {
    var bestSlot = null
    var bestDistance = Number.MAX_VALUE
    var centerX = x + workflowNodeWidth / 2
    var centerY = y + workflowNodeHeight / 2
    for (var i = 0; i < workflowSnapSlots.length; ++i) {
        var slot = workflowSnapSlots[i]
        if (laneId && slot.lane !== laneId) {
            continue
        }
        var slotCenterX = slotX(slot) + workflowNodeWidth / 2
        var slotCenterY = slotY(slot) + workflowNodeHeight / 2
        var distance = Math.abs(centerX - slotCenterX) + Math.abs(centerY - slotCenterY)
        if (distance < bestDistance) {
            bestDistance = distance
            bestSlot = slot
        }
    }
    return bestSlot
}
    
function moduleSlot(module) {
    if (!module) {
        return null
    }
    if (module.slot) {
        var slot = slotForModuleId(module.slot)
        if (slot) {
            return slot
        }
    }
    var laneId = module.lane || "analysis"
    var row = normalizedRow(laneId, module.row || 0)
    var col = normalizedColumn(module.col || 0)
    return { "id": laneId + "-" + row + "-" + col, "lane": laneId, "row": row, "col": col }
}
    
function moduleIdAtSlot(slotId, exceptId) {
    for (var i = 0; i < placedModules.length; ++i) {
        var module = placedModules[i]
        var slot = moduleSlot(module)
        if (module.id !== exceptId && slot && slot.id === slotId) {
            return module.id
        }
    }
    return ""
}
    
function moduleCanUseSlot(moduleId, slot) {
    var def = definitionForId(moduleId)
    return slot && def.id && slot.lane === moduleDragLaneId(moduleId, "analysis")
}
    
function moduleX(module) {
    var slot = moduleSlot(module)
    return slot ? slotX(slot) : 0
}
    
function moduleY(module) {
    var slot = moduleSlot(module)
    return slot ? slotY(slot) : 0
}
    
function gridSlotForPosition(x, y, laneId, exceptId) {
    var targetLaneId = laneId || laneForY(y + workflowNodeHeight / 2, "analysis")
    return nearestFreeSlot(x, y, targetLaneId, exceptId)
}
    
function snapSlotForPosition(x, y, laneId) {
    var targetLaneId = laneId || laneForY(y + workflowNodeHeight / 2, "analysis")
    return nearestSlot(x, y, targetLaneId)
}
    
function updateDraggingSnapPreview(moduleId, nodeX, nodeY) {
    var current = placedModule(moduleId)
    var def = definitionForId(moduleId)
    if (!current || !def.id) {
        draggingSnapPreviewActive = false
        return
    }
    var laneId = moduleDragLaneId(moduleId, current.lane)
    var slot = snapSlotForPosition(nodeX, nodeY, laneId)
    if (!slot || !moduleCanUseSlot(moduleId, slot)) {
        slot = moduleSlot(current)
    }
    if (!slot) {
        draggingSnapPreviewActive = false
        return
    }
    draggingSnapPreviewX = slotX(slot)
    draggingSnapPreviewY = slotY(slot)
    draggingSnapPreviewActive = true
}
    
function scheduleForX(x) {
    var centerX = x + workflowNodeWidth / 2
    if (centerX < gridXForColumn(3) + workflowNodeWidth / 2) {
        return "实时刷新"
    }
    if (centerX < gridXForColumn(7) + workflowNodeWidth / 2) {
        return "自动触发"
    }
    return "手动运行"
}
    
function scheduleForPlacement(moduleId, existingModule, slot) {
    if (existingModule && existingModule.schedule && existingModule.schedule.length > 0) {
        return existingModule.schedule
    }
    var def = definitionForId(moduleId)
    if (def.defaultSchedule && def.defaultSchedule.length > 0) {
        return def.defaultSchedule
    }
    return slot ? scheduleForX(slotX(slot)) : "手动运行"
}
    
function laneForY(y, fallbackLane) {
    for (var i = 0; i < workflowLanes.length; ++i) {
        var lane = workflowLanes[i]
        if (y >= lane.y && y <= lane.y + lane.height) {
            return lane.id
        }
    }
    var nearest = laneForId(fallbackLane || "analysis")
    var bestDistance = Math.abs(y - (nearest.y + nearest.height / 2))
    for (var j = 0; j < workflowLanes.length; ++j) {
        var candidate = workflowLanes[j]
        var distance = Math.abs(y - (candidate.y + candidate.height / 2))
        if (distance < bestDistance) {
            nearest = candidate
            bestDistance = distance
        }
    }
    return nearest.id
}
    
function addOrMoveModule(moduleId, dropX, dropY) {
    var def = definitionForId(moduleId)
    if (!def.id || def.fixed || def.server) {
        return
    }
    var laneId = moduleDragLaneId(moduleId, def.lane)
    var slot = gridSlotForPosition(dropX - workflowNodeWidth / 2, dropY - workflowNodeHeight / 2, laneId, moduleId)
    if (!slot) {
        return
    }
    var next = placedModules.slice()
    var found = false
    for (var i = 0; i < next.length; ++i) {
        if (next[i].id === moduleId) {
            var schedule = scheduleForPlacement(moduleId, next[i], slot)
            next[i] = { "id": moduleId, "lane": slot.lane, "slot": slot.id, "row": slot.row, "col": slot.col, "schedule": schedule }
            found = true
            break
        }
    }
    if (!found) {
        var initialSchedule = scheduleForPlacement(moduleId, null, slot)
        next.push({ "id": moduleId, "lane": slot.lane, "slot": slot.id, "row": slot.row, "col": slot.col, "schedule": initialSchedule })
    }
    setPlacedModules(next)
    activateModule(moduleId)
}
    
function movePlacedModule(moduleId, nodeX, nodeY) {
    var current = placedModule(moduleId)
    var def = definitionForId(moduleId)
    if (!current || !def.id) {
        return
    }
    var laneId = moduleDragLaneId(moduleId, current.lane)
    var currentSlot = moduleSlot(current)
    var slot = snapSlotForPosition(nodeX, nodeY, laneId)
    if (!slot || !moduleCanUseSlot(moduleId, slot)) {
        slot = currentSlot
    }
    if (!slot) {
        return
    }
    var occupantId = moduleIdAtSlot(slot.id, moduleId)
    if (occupantId.length > 0 && !moduleCanUseSlot(occupantId, currentSlot)) {
        slot = moduleSlot(current)
        occupantId = ""
    }
    var next = placedModules.slice()
    var schedule = scheduleForPlacement(moduleId, current, slot)
    for (var i = 0; i < next.length; ++i) {
        if (next[i].id === moduleId) {
            next[i] = {
                "id": moduleId,
                "lane": slot.lane,
                "slot": slot.id,
                "row": slot.row,
                "col": slot.col,
                "schedule": schedule
            }
            break
        }
    }
    if (occupantId.length > 0 && currentSlot) {
        var occupantDef = definitionForId(occupantId)
        var occupantSchedule = ""
        for (var j = 0; j < next.length; ++j) {
            if (next[j].id === occupantId) {
                occupantSchedule = scheduleForPlacement(occupantId, next[j], currentSlot)
                next[j] = {
                    "id": occupantId,
                    "lane": currentSlot.lane,
                    "slot": currentSlot.id,
                    "row": currentSlot.row,
                    "col": currentSlot.col,
                    "schedule": occupantSchedule
                }
                break
            }
        }
    }
    setPlacedModules(next)
    activateModule(moduleId)
}
    
Connections {
    target: backend
    function onWorkflowModuleStateChanged() {
        workflowPage.moduleStatusRefreshTick += 1
    }
    function onCommConfigChanged() {
        if (!backend.afsClientConfigDirty) {
            workflowPage.afsClientDraftAddr = backend.afsClientAddr
            workflowPage.afsClientDraftPort = backend.afsClientPort
        }
    }
    function onMissionChanged() {
        if (!backend.missionConfigDirty) {
            workflowPage.resetMissionDraft()
        }
    }
    function onWorkflowInputModeChanged() {
        workflowPage.appRoot.workflowInputMode = backend.workflowInputMode
        workflowPage.appRoot.workflowRealtimeMode = backend.workflowInputMode !== "replay"
        workflowPage.inputMode = backend.workflowInputMode
        workflowPage.moduleStatusRefreshTick += 1
    }
}
    
function nodeInputPoint(moduleId) {
    var module = placedModule(moduleId)
    if (!module) {
        return null
    }
    if (draggingModuleId === moduleId) {
        return { "x": draggingModuleX + moduleWidth(module) / 2, "y": draggingModuleY }
    }
    return { "x": moduleX(module) + moduleWidth(module) / 2, "y": moduleY(module) }
}
    
function nodeOutputPoint(moduleId) {
    var module = placedModule(moduleId)
    if (!module) {
        return null
    }
    if (draggingModuleId === moduleId) {
        return { "x": draggingModuleX + moduleWidth(module) / 2, "y": draggingModuleY + workflowNodeHeight }
    }
    return { "x": moduleX(module) + moduleWidth(module) / 2, "y": moduleY(module) + workflowNodeHeight }
}
    
ColumnLayout {
    anchors.fill: parent
    spacing: 8
    
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 14
    
        WorkflowPanel {
            Layout.preferredWidth: 360
            Layout.minimumWidth: 360
            Layout.maximumWidth: 360
            Layout.fillHeight: true
            title: "模块状态监视"
            statusLegendVisible: true
    
            Flickable {
                anchors.fill: parent
                anchors.margins: 12
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                contentWidth: width
                contentHeight: leftStatusColumn.implicitHeight
    
                ColumnLayout {
                    id: leftStatusColumn
                    width: parent.width
                    spacing: 14
    
                    Repeater {
                        model: workflowPage.workflowModuleStatusGroups()
                        delegate: ColumnLayout {
                            required property var modelData
    
                            Layout.fillWidth: true
                            spacing: 7
    
                            Label {
                                Layout.fillWidth: true
                                text: modelData.title
                                color: "#20242a"
                                font.family: workflowPage.appRoot.uiFontFamily
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                            }
    
                            GridLayout {
                                id: moduleStatusGrid
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: 8
                                rowSpacing: 8
    
                                Repeater {
                                    model: modelData.modules
                                    delegate: WorkflowStatusCard {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: Math.max(0, (moduleStatusGrid.width - moduleStatusGrid.columnSpacing) / 2)
                                        compact: true
                                        title: modelData.title
                                        text: workflowPage.moduleStatusSummary(modelData, workflowPage.moduleStatusRefreshTick)
                                        statusKind: workflowPage.moduleStatus(modelData, workflowPage.moduleStatusRefreshTick)
                                        selected: workflowPage.selectedModuleId === modelData.id
                                        clickable: true
                                        onClicked: workflowPage.activateModule(modelData.id)
                                    }
                                }
                            }
                        }
                    }
    
                }
            }
        }
    
        WorkflowPanel {
            id: centerWorkflowPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "工作流程编排"
    
            Flickable {
                id: workflowCanvasScroll
                anchors.fill: parent
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                contentWidth: width
                contentHeight: height
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
    
                Item {
                    id: workflowCanvas
                    width: workflowCanvasScroll.contentWidth
                    height: workflowCanvasScroll.contentHeight
                    onWidthChanged: workflowPage.workflowCanvasWidthHint = width
                    onHeightChanged: workflowPage.workflowCanvasHeightHint = height
                    Component.onCompleted: {
                        workflowPage.workflowCanvasWidthHint = width
                        workflowPage.workflowCanvasHeightHint = height
                        workflowBackground.requestPaint()
                        workflowConnections.requestPaint()
                    }
    
                    Canvas {
                        id: workflowBackground
                        anchors.fill: parent
                        renderStrategy: Canvas.Threaded
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.fillStyle = "#fbfcfe"
                            ctx.fillRect(0, 0, width, height)
    
                            for (var li = 0; li < workflowPage.workflowLanes.length; ++li) {
                                var lane = workflowPage.workflowLanes[li]
                                ctx.fillStyle = li % 2 === 0 ? "#f8fbff" : "#ffffff"
                                ctx.fillRect(0, lane.y, width, lane.height)
                                ctx.strokeStyle = "#d9e1ea"
                                ctx.lineWidth = 1
                                ctx.strokeRect(0.5, lane.y + 0.5, width - 1, lane.height)
                            }
    
                            ctx.fillStyle = "#e4e9ef"
                            for (var gx = 0; gx < width; gx += 28) {
                                for (var gy = 0; gy < height; gy += 28) {
                                    ctx.fillRect(gx, gy, 1, 1)
                                }
                            }
                        }
                    }
    
                    Canvas {
                        id: workflowConnections
                        anchors.fill: parent
                        property var modulePorts: ({})
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.lineCap = "round"
                            var routeContext = makeRouteContext()
                            var routedEdges = makeRoutedEdges(routeContext)
                            var portLayout = makePortLayout(routedEdges)
                            var nextModulePorts = portLayout.modulePorts
                            for (var edgeIndex = 0; edgeIndex < workflowPage.workflowEdges.length; ++edgeIndex) {
                                drawModuleEdge(ctx, routeContext, portLayout.edgePorts, routedEdges[edgeIndex])
                            }
                            modulePorts = nextModulePorts
                        }
    
                        function drawModuleEdge(ctx, routeContext, edgePorts, routedEdge) {
                            if (!routedEdge || routedEdge.route.length < 2) {
                                return
                            }
                            var edgePort = edgePorts[routedEdge.id]
                            var route = adjustedRouteForPorts(routedEdge.route, edgePort)
                            var fromModule = routeContext.moduleById[routedEdge.sourceId]
                            var toModule = routeContext.moduleById[routedEdge.targetId]
                            var off = (fromModule && workflowPage.moduleDisabled(fromModule)) || (toModule && workflowPage.moduleDisabled(toModule))
                            ctx.beginPath()
                            ctx.strokeStyle = off ? "#b9c1cc" : edgeColor(routedEdge.sourceId)
                            ctx.lineWidth = 2
                            ctx.moveTo(route[0].x, route[0].y)
                            for (var i = 1; i < route.length; ++i) {
                                ctx.lineTo(route[i].x, route[i].y)
                            }
                            ctx.stroke()
                        }
    
                        function modulePortConnections(moduleId) {
                            return modulePorts[moduleId] || { "top": [], "right": [], "bottom": [], "left": [] }
                        }
    
                        function makeRoutedEdges(routeContext) {
                            var routedEdges = []
                            for (var edgeIndex = 0; edgeIndex < workflowPage.workflowEdges.length; ++edgeIndex) {
                                var edge = workflowPage.workflowEdges[edgeIndex]
                                var route = moduleEdgeRoute(routeContext, edge.sourceId, edge.targetId)
                                if (route.length < 2) {
                                    routedEdges.push(null)
                                    continue
                                }
                                routedEdges.push({
                                    "id": edge.sourceId + "->" + edge.targetId,
                                    "sourceId": edge.sourceId,
                                    "targetId": edge.targetId,
                                    "route": route,
                                    "sourceSide": sideFromSegment(route[0], route[1], true),
                                    "targetSide": sideFromSegment(route[route.length - 2], route[route.length - 1], false)
                                })
                            }
                            return routedEdges
                        }
    
                        function makePortLayout(routedEdges) {
                            var targetPortGroups = ({})
                            var sourcePortKeys = ({})
                            var edgePorts = ({})
                            for (var edgeIndex = 0; edgeIndex < routedEdges.length; ++edgeIndex) {
                                var routedEdge = routedEdges[edgeIndex]
                                if (!routedEdge) {
                                    continue
                                }
                                registerSourcePortUse(sourcePortKeys, edgePorts, routedEdge)
                                registerTargetPortUse(targetPortGroups, routedEdge)
                            }
    
                            var nextModulePorts = ({})
                            for (var sourceKey in sourcePortKeys) {
                                var sourceParts = sourceKey.split(":")
                                if (sourceParts.length === 2) {
                                    appendModulePort(nextModulePorts, sourceParts[0], sourceParts[1], 0)
                                }
                            }
                            for (var key in targetPortGroups) {
                                var group = targetPortGroups[key]
                                group.edgeIds.sort()
                                if (group.moduleId === "kg-parse" && group.side === "top") {
                                    group.edgeIds.reverse()
                                }
                                var offsets = portOffsets(group.edgeIds.length)
                                for (var i = 0; i < group.edgeIds.length; ++i) {
                                    var edgeId = group.edgeIds[i]
                                    var offset = offsets[i]
                                    appendModulePort(nextModulePorts, group.moduleId, group.side, offset)
                                    if (!edgePorts[edgeId]) {
                                        edgePorts[edgeId] = ({})
                                    }
                                    edgePorts[edgeId].targetSide = group.side
                                    edgePorts[edgeId].targetOffset = offset
                                }
                            }
                            return { "modulePorts": nextModulePorts, "edgePorts": edgePorts }
                        }
    
                        function registerSourcePortUse(sourcePortKeys, edgePorts, routedEdge) {
                            if (routedEdge.sourceSide.length === 0) {
                                return
                            }
                            sourcePortKeys[routedEdge.sourceId + ":" + routedEdge.sourceSide] = true
                            if (!edgePorts[routedEdge.id]) {
                                edgePorts[routedEdge.id] = ({})
                            }
                            edgePorts[routedEdge.id].sourceSide = routedEdge.sourceSide
                            edgePorts[routedEdge.id].sourceOffset = 0
                        }
    
                        function registerTargetPortUse(portGroups, routedEdge) {
                            if (routedEdge.targetSide.length === 0) {
                                return
                            }
                            var moduleId = routedEdge.targetId
                            var side = routedEdge.targetSide
                            var edgeId = routedEdge.id
                            var key = moduleId + ":" + side
                            var group = portGroups[key]
                            if (!group) {
                                group = { "moduleId": moduleId, "side": side, "edgeIds": [] }
                                portGroups[key] = group
                            }
                            group.edgeIds.push(edgeId)
                        }
    
                        function appendModulePort(nextModulePorts, moduleId, side, offset) {
                            var ports = nextModulePorts[moduleId]
                            if (!ports) {
                                ports = { "top": [], "right": [], "bottom": [], "left": [] }
                                nextModulePorts[moduleId] = ports
                            }
                            ports[side].push(offset)
                        }
    
                        function portOffsets(count) {
                            var offsets = []
                            var spacing = 12
                            var start = -(count - 1) * spacing / 2
                            for (var i = 0; i < count; ++i) {
                                offsets.push(start + i * spacing)
                            }
                            return offsets
                        }
    
                        function adjustedRouteForPorts(route, edgePort) {
                            if (!edgePort) {
                                return route.slice(0)
                            }
                            var sourceOffset = edgePort.sourceOffset || 0
                            var targetOffset = edgePort.targetOffset || 0
                            var adjusted = route.slice(0)
                            adjusted[0] = shiftedPoint(route[0], edgePort.sourceSide, sourceOffset)
                            adjusted[adjusted.length - 1] = shiftedPoint(route[route.length - 1], edgePort.targetSide, targetOffset)
                            keepEndSegmentOrthogonal(adjusted)
                            return compactRoute(adjusted)
                        }
    
                        function keepEndSegmentOrthogonal(route) {
                            if (route.length < 2) {
                                return
                            }
                            keepAdjacentSegmentOrthogonal(route, 0, 1)
                            keepAdjacentSegmentOrthogonal(route, route.length - 1, route.length - 2)
                        }
    
                        function keepAdjacentSegmentOrthogonal(route, anchorIndex, neighborIndex) {
                            var anchor = route[anchorIndex]
                            var neighbor = route[neighborIndex]
                            if (!anchor || !neighbor) {
                                return
                            }
                            var dx = Math.abs(anchor.x - neighbor.x)
                            var dy = Math.abs(anchor.y - neighbor.y)
                            if (dx <= dy) {
                                route[neighborIndex] = { "x": anchor.x, "y": neighbor.y }
                            } else {
                                route[neighborIndex] = { "x": neighbor.x, "y": anchor.y }
                            }
                        }
    
                        function shiftedPoint(point, side, offset) {
                            if (side === "top" || side === "bottom") {
                                return { "x": point.x + offset, "y": point.y }
                            }
                            if (side === "left" || side === "right") {
                                return { "x": point.x, "y": point.y + offset }
                            }
                            return point
                        }
    
                        function sideFromSegment(a, b, leavingSource) {
                            var dx = b.x - a.x
                            var dy = b.y - a.y
                            if (dx === 0 && dy === 0) {
                                return ""
                            }
                            if (Math.abs(dx) >= Math.abs(dy)) {
                                if (leavingSource) {
                                    return dx >= 0 ? "right" : "left"
                                }
                                return dx >= 0 ? "left" : "right"
                            }
                            if (leavingSource) {
                                return dy >= 0 ? "bottom" : "top"
                            }
                            return dy >= 0 ? "top" : "bottom"
                        }
    
                        function makeRouteContext() {
                            var moduleById = ({})
                            var boxes = ({})
                            for (var i = 0; i < workflowPage.placedModules.length; ++i) {
                                var module = workflowPage.placedModules[i]
                                moduleById[module.id] = module
                                boxes[module.id] = moduleBox(module)
                            }
                            return {
                                "modules": workflowPage.placedModules,
                                "moduleById": moduleById,
                                "boxes": boxes,
                                "rows": workflowPage.workflowRoutingRows(),
                                "columns": workflowPage.workflowRoutingColumns()
                            }
                        }
    
                        function moduleEdgeRoute(routeContext, fromId, toId) {
                            var fromBox = routeContext.boxes[fromId]
                            var toBox = routeContext.boxes[toId]
                            if (!fromBox || !toBox) {
                                return []
                            }
                            var start = routingPointForBox(routeContext, fromBox)
                            var end = routingPointForBox(routeContext, toBox)
                            var verticalRelation = start.y !== end.y
                            var horizontalRelation = start.x !== end.x
                            var fallback = []
                            var route = []
    
                            if (verticalRelation) {
                                if (!horizontalRelation) {
                                    route = compactRoute([start, end])
                                    fallback = route
                                    if (!routeBlocked(routeContext, route, fromId, toId)) {
                                        return route
                                    }
                                }
                                var rowCandidates = routingRowsBetween(routeContext, start.y, end.y)
                                for (var rowIndex = 0; rowIndex < rowCandidates.length; ++rowIndex) {
                                    var rowY = rowCandidates[rowIndex]
                                    route = compactRoute([start, { "x": start.x, "y": rowY }, { "x": end.x, "y": rowY }, end])
                                    if (fallback.length === 0) {
                                        fallback = route
                                    }
                                    if (!routeBlocked(routeContext, route, fromId, toId)) {
                                        return route
                                    }
                                }
                                var columnCandidates = routingColumnsAround(routeContext, (start.x + end.x) / 2)
                                for (var colIndex = 0; colIndex < columnCandidates.length; ++colIndex) {
                                    var columnX = columnCandidates[colIndex]
                                    if (columnX === start.x || columnX === end.x) {
                                        continue
                                    }
                                    route = compactRoute([start, { "x": columnX, "y": start.y }, { "x": columnX, "y": end.y }, end])
                                    if (fallback.length === 0) {
                                        fallback = route
                                    }
                                    if (!routeBlocked(routeContext, route, fromId, toId)) {
                                        return route
                                    }
                                }
                            } else if (horizontalRelation) {
                                route = compactRoute([start, end])
                                fallback = route
                                if (!routeBlocked(routeContext, route, fromId, toId)) {
                                    return route
                                }
                                var avoidRows = routingRowsAround(routeContext, start.y, true)
                                var avoidColumns = routingColumnsBetween(routeContext, start.x, end.x)
                                for (var avoidRowIndex = 0; avoidRowIndex < avoidRows.length; ++avoidRowIndex) {
                                    var avoidY = avoidRows[avoidRowIndex]
                                    for (var avoidColIndex = 0; avoidColIndex < avoidColumns.length; ++avoidColIndex) {
                                        var avoidX = avoidColumns[avoidColIndex]
                                        if (avoidX === start.x || avoidX === end.x) {
                                            continue
                                        }
                                        route = compactRoute([start, { "x": avoidX, "y": start.y }, { "x": avoidX, "y": avoidY }, { "x": end.x, "y": avoidY }, end])
                                        if (!routeBlocked(routeContext, route, fromId, toId)) {
                                            return route
                                        }
                                    }
                                }
                            } else {
                                fallback = compactRoute([start, end])
                            }
    
                            return fallback
                        }
                        function moduleBox(module) {
                            var x = workflowPage.moduleX(module)
                            var y = workflowPage.moduleY(module)
                            if (workflowPage.draggingModuleId === module.id) {
                                x = workflowPage.draggingModuleX
                                y = workflowPage.draggingModuleY
                            }
                            return {
                                "left": x,
                                "right": x + workflowPage.moduleWidth(module),
                                "top": y,
                                "bottom": y + workflowPage.workflowNodeHeight
                            }
                        }
    
                        function boxCenter(box) {
                            return {
                                "x": Math.round((box.left + box.right) / 2),
                                "y": Math.round((box.top + box.bottom) / 2)
                            }
                        }
    
                        function routingPointForBox(routeContext, box) {
                            var center = boxCenter(box)
                            return {
                                "x": nearestValue(routeContext.columns, center.x),
                                "y": nearestValue(routeContext.rows, center.y)
                            }
                        }
    
                        function routingRowsBetween(routeContext, y1, y2) {
                            var rows = routeContext.rows
                            var minY = Math.min(y1, y2)
                            var maxY = Math.max(y1, y2)
                            var middleY = (y1 + y2) / 2
                            var between = []
                            for (var i = 0; i < rows.length; ++i) {
                                if (rows[i] > minY && rows[i] < maxY) {
                                    between.push(rows[i])
                                }
                            }
                            return sortValuesByDistance(between.length > 0 ? between : rows, middleY)
                        }
    
                        function routingRowsAround(routeContext, y, excludeExact) {
                            var rows = routeContext.rows
                            var candidates = []
                            for (var i = 0; i < rows.length; ++i) {
                                if (!excludeExact || rows[i] !== y) {
                                    candidates.push(rows[i])
                                }
                            }
                            return sortValuesByDistance(candidates, y)
                        }
    
                        function routingColumnsBetween(routeContext, x1, x2) {
                            var columns = routeContext.columns
                            var minX = Math.min(x1, x2)
                            var maxX = Math.max(x1, x2)
                            var middleX = (x1 + x2) / 2
                            var between = []
                            for (var i = 0; i < columns.length; ++i) {
                                if (columns[i] >= minX && columns[i] <= maxX) {
                                    between.push(columns[i])
                                }
                            }
                            return sortValuesByDistance(between.length > 0 ? between : columns, middleX)
                        }
    
                        function routingColumnsAround(routeContext, x) {
                            return sortValuesByDistance(routeContext.columns, x)
                        }
    
                        function sortValuesByDistance(values, target) {
                            var sorted = values.slice(0)
                            sorted.sort(function(a, b) {
                                var distanceA = Math.abs(a - target)
                                var distanceB = Math.abs(b - target)
                                if (distanceA === distanceB) {
                                    return a - b
                                }
                                return distanceA - distanceB
                            })
                            return sorted
                        }
    
                        function nearestValue(values, target) {
                            if (values.length === 0) {
                                return Math.round(target)
                            }
                            var best = values[0]
                            var bestDistance = Math.abs(best - target)
                            for (var i = 1; i < values.length; ++i) {
                                var distance = Math.abs(values[i] - target)
                                if (distance < bestDistance) {
                                    best = values[i]
                                    bestDistance = distance
                                }
                            }
                            return best
                        }
    
                        function routeBlocked(routeContext, route, fromId, toId) {
                            for (var i = 1; i < route.length; ++i) {
                                if (segmentBlocked(routeContext, route[i - 1], route[i], fromId, toId)) {
                                    return true
                                }
                            }
                            return false
                        }
    
                        function segmentBlocked(routeContext, a, b, fromId, toId) {
                            if (a.x !== b.x && a.y !== b.y) {
                                return true
                            }
                            for (var i = 0; i < routeContext.modules.length; ++i) {
                                var module = routeContext.modules[i]
                                if (module.id === fromId || module.id === toId) {
                                    continue
                                }
                                var box = routeContext.boxes[module.id]
                                if (a.y === b.y && horizontalSegmentIntersectsBox(a, b, box)) {
                                    return true
                                }
                                if (a.x === b.x && verticalSegmentIntersectsBox(a, b, box)) {
                                    return true
                                }
                            }
                            return false
                        }
    
                        function horizontalSegmentIntersectsBox(a, b, box) {
                            var left = Math.min(a.x, b.x)
                            var right = Math.max(a.x, b.x)
                            return a.y > box.top && a.y < box.bottom && right > box.left && left < box.right
                        }
    
                        function verticalSegmentIntersectsBox(a, b, box) {
                            var top = Math.min(a.y, b.y)
                            var bottom = Math.max(a.y, b.y)
                            return a.x > box.left && a.x < box.right && bottom > box.top && top < box.bottom
                        }
    
                        function compactRoute(route) {
                            var compact = []
                            for (var i = 0; i < route.length; ++i) {
                                var point = { "x": Math.round(route[i].x), "y": Math.round(route[i].y) }
                                if (compact.length === 0 || compact[compact.length - 1].x !== point.x || compact[compact.length - 1].y !== point.y) {
                                    compact.push(point)
                                }
                            }
                            return compact
                        }
    
                        function edgeColor(fromId) {
                            var sourceLane = workflowPage.definitionForId(fromId).lane || ""
                            if (sourceLane === "input") return "#4b7fd8"
                            if (sourceLane === "data") return "#74d18b"
                            if (sourceLane === "analysis") return "#ff7571"
                            if (sourceLane === "solution") return "#f6b26b"
                            if (sourceLane === "command") return "#dc2626"
                            return "#2b2f36"
                        }
                    }
    
                    DropArea {
                        anchors.fill: parent
                        keys: ["workflow-module"]
                        enabled: !workflowPage.moduleSettingsLocked
                        onDropped: function(drop) {
                            if (drop.source) {
                                workflowPage.addOrMoveModule(workflowPage.dragSourceText(drop.source, "moduleId"),
                                                             drop.x + workflowCanvasScroll.contentX,
                                                             drop.y + workflowCanvasScroll.contentY)
                            }
                        }
                    }
    
                    MouseArea {
                        anchors.fill: parent
                        z: 1
                        acceptedButtons: Qt.LeftButton
                        onClicked: workflowPage.clearSelectedModule()
                    }
    
                    Repeater {
                        model: workflowPage.workflowLanes
                        delegate: WorkflowLaneLabel {
                            required property var modelData
                            x: 18
                            y: modelData.y + 8
                            text: modelData.title
                        }
                    }
    
                    WorkflowNode {
                        id: workflowSnapPreview
                        visible: workflowPage.draggingSnapPreviewActive
                        baseX: workflowPage.draggingSnapPreviewX
                        baseY: workflowPage.draggingSnapPreviewY
                        width: workflowPage.workflowNodeWidth
                        moduleId: workflowPage.draggingModuleId
                        title: workflowPage.moduleTitle(workflowPage.draggingModuleId)
                        meta: workflowPage.moduleMeta({ "id": workflowPage.draggingModuleId })
                        schedule: ""
                        selected: true
                        removable: false
                        opacity: 0.36
                        z: 4
                        enabled: false
                    }
    
                    Repeater {
                        model: workflowPage.placedModules
                        delegate: WorkflowNode {
                            required property var modelData
                            readonly property var moduleDef: workflowPage.definitionForId(modelData.id)
                            readonly property var portConnections: workflowConnections.modulePortConnections(modelData.id)
                            baseX: workflowPage.moduleX(modelData)
                            baseY: workflowPage.moduleY(modelData)
                            width: workflowPage.moduleWidth(modelData)
                            moduleId: modelData.id
                            title: moduleDef.title
                            meta: workflowPage.moduleMeta(modelData)
                            lane: modelData.lane
                            schedule: workflowPage.moduleSchedule(modelData)
                            source: moduleDef.source === true
                            fixed: moduleDef.fixed === true
                            server: moduleDef.server === true
                            disabledNode: workflowPage.moduleDisabled(modelData)
                            statusKind: workflowPage.moduleStatus(modelData, workflowPage.moduleStatusRefreshTick)
                            selected: workflowPage.selectedModuleId === moduleId
                            warningNode: modelData.id === "input-afsim" && backend.afsClientConfigDirty
                            topPorts: portConnections.top
                            rightPorts: portConnections.right
                            bottomPorts: portConnections.bottom
                            leftPorts: portConnections.left
                            removable: !(moduleDef.fixed === true || moduleDef.source === true || moduleDef.server === true)
                            onClicked: workflowPage.activateModule(moduleId)
                            onDragPreview: function(nodeX, nodeY) { workflowPage.setDraggingModule(moduleId, nodeX, nodeY) }
                            onDragCleared: workflowPage.clearDraggingModule(moduleId)
                            onMoved: function(nodeX, nodeY) { workflowPage.movePlacedModule(moduleId, nodeX, nodeY) }
                        }
                    }
                }
            }
        }
    
        WorkflowPanel {
            Layout.preferredWidth: 344
            Layout.minimumWidth: 344
            Layout.maximumWidth: 344
            Layout.fillHeight: true
            title: "模块配置"
            actionText: "≡"
    
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
    
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 76
                    color: "#f7fbff"
                    border.color: "#e1e5ea"
                    border.width: 1
    
                    Column {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        anchors.topMargin: 12
                        spacing: 6
                        Label { text: "当前选中模块"; color: "#687380"; font.family: workflowPage.appRoot.uiFontFamily; font.pixelSize: 13 }
                        Label { width: parent.width; text: workflowPage.selectedModule; color: "#1477d4"; font.family: workflowPage.appRoot.uiFontFamily; font.pixelSize: 20; font.bold: true; elide: Text.ElideRight }
                    }
                }
    
                Label {
                    visible: !workflowPage.hasSelectedModule
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 16
                    text: "点击画布模块查看配置"
                    color: "#687380"
                    font.family: workflowPage.appRoot.uiFontFamily
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
    
                Flickable {
                    visible: workflowPage.hasSelectedModule
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: propertyColumn.implicitHeight
    
                    ColumnLayout {
                        id: propertyColumn
                        width: parent.width
                        spacing: 0
    
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 8
                            Label { text: "运行方式"; color: "#20242a"; font.family: workflowPage.appRoot.uiFontFamily; font.pixelSize: 15; font.bold: true }
                            WorkflowUiButton {
                                visible: workflowPage.selectedIsServerModule() && !workflowPage.selectedIsCommModule()
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                text: workflowPage.selectedServerRunning() ? "关闭服务" : "开启服务"
                                visualChecked: workflowPage.selectedServerRunning()
                                onClicked: workflowPage.toggleSelectedServer()
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
    
                                WorkflowScheduleButton {
                                    Layout.preferredWidth: 72
                                    Layout.minimumWidth: 72
                                    Layout.maximumWidth: 72
                                    Layout.preferredHeight: 32
                                    text: "实时刷新"
                                    enabled: workflowPage.scheduleOptionEnabled(workflowPage.selectedModuleId, text)
                                    visualChecked: workflowPage.selectedSchedule() === text
                                    onClicked: workflowPage.setSelectedSchedule(text)
                                }
                                WorkflowScheduleButton {
                                    Layout.preferredWidth: 72
                                    Layout.minimumWidth: 72
                                    Layout.maximumWidth: 72
                                    Layout.preferredHeight: 32
                                    text: "自动触发"
                                    enabled: workflowPage.scheduleOptionEnabled(workflowPage.selectedModuleId, text)
                                    visualChecked: workflowPage.selectedSchedule() === text
                                    onClicked: workflowPage.setSelectedSchedule(text)
                                }
                                WorkflowScheduleButton {
                                    Layout.preferredWidth: 72
                                    Layout.minimumWidth: 72
                                    Layout.maximumWidth: 72
                                    Layout.preferredHeight: 32
                                    text: "手动运行"
                                    enabled: workflowPage.scheduleOptionEnabled(workflowPage.selectedModuleId, text)
                                    visualChecked: workflowPage.selectedSchedule() === text
                                    onClicked: workflowPage.setSelectedSchedule(text)
                                }
                                WorkflowScheduleButton {
                                    Layout.preferredWidth: 72
                                    Layout.minimumWidth: 72
                                    Layout.maximumWidth: 72
                                    Layout.preferredHeight: 32
                                    text: "停用模块"
                                    enabled: workflowPage.scheduleOptionEnabled(workflowPage.selectedModuleId, text)
                                    visualChecked: workflowPage.selectedSchedule() === text
                                    onClicked: workflowPage.setSelectedSchedule(text)
                                }
                            }
                        }
    
                        Rectangle { visible: workflowPage.selectedIsConceptGraphModule(); Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }
    
                        ColumnLayout {
                            visible: workflowPage.selectedIsConceptGraphModule()
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 8
    
                            Label {
                                Layout.fillWidth: true
                                text: "当前场景"
                                color: "#20242a"
                                font.family: workflowPage.appRoot.uiFontFamily
                                font.pixelSize: 15
                                font.bold: true
                            }
    
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
    
                                WorkflowUiComboBox {
                                    Layout.fillWidth: true
                                    model: backend.conceptScenarioNames
                                    currentIndex: Math.max(0, backend.conceptScenarioNames.indexOf(backend.selectedConceptScenario))
                                    enabled: false
                                }
    
                                WorkflowUiButton {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 32
                                    text: "图谱修改"
                                    onClicked: workflowPage.openConceptGraphPage()
                                }
                            }
                        }
    
                        Rectangle { visible: workflowPage.selectedIsReplayModule(); Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }
    
                        ColumnLayout {
                            visible: workflowPage.selectedIsReplayModule()
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 8
    
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
    
                                Label {
                                    Layout.fillWidth: true
                                    text: "回放倍速"
                                    color: "#20242a"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 15
                                    font.bold: true
                                }
                                Label {
                                    Layout.preferredWidth: 56
                                    text: workflowPage.replaySpeedText(workflowPage.replaySpeedDraft)
                                    color: workflowPage.replaySpeedDirty ? "#b45309" : "#475569"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 14
                                    font.bold: workflowPage.replaySpeedDirty
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
    
                            Slider {
                                Layout.fillWidth: true
                                from: 0.1
                                to: 100
                                stepSize: 0.1
                                snapMode: Slider.SnapAlways
                                value: workflowPage.replaySpeedDraft
                                enabled: !workflowPage.moduleSettingsLocked
                                onMoved: workflowPage.replaySpeedDraft = value
                            }
    
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
    
                                Label {
                                    Layout.fillWidth: true
                                    text: "0.1x"
                                    color: "#687380"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 12
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: "100x"
                                    color: "#687380"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
    
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
    
                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "确认"
                                    enabled: !workflowPage.moduleSettingsLocked && workflowPage.replaySpeedDirty
                                    onClicked: workflowPage.confirmReplaySpeed()
                                }
                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "重置"
                                    enabled: !workflowPage.moduleSettingsLocked && workflowPage.replaySpeedDirty
                                    onClicked: workflowPage.resetReplaySpeed()
                                }
                            }
                        }
    
                        Rectangle { visible: workflowPage.selectedIsCommModule(); Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }
    
                        ColumnLayout {
                            visible: workflowPage.selectedIsCommModule()
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 8
    
                            Label { text: "通信配置"; color: "#20242a"; font.family: workflowPage.appRoot.uiFontFamily; font.pixelSize: 15; font.bold: true }
    
                            ColumnLayout {
                                visible: workflowPage.selectedIsAfsClientModule()
                                Layout.fillWidth: true
                                spacing: 8
    
                                WorkflowLabeledEdit {
                                    Layout.fillWidth: true
                                    label: "IP"
                                    textValue: workflowPage.afsClientDraftAddr
                                    warn: backend.afsClientConfigDirty
                                    enabled: !workflowPage.moduleSettingsLocked && !backend.afsClientRunning
                                    onTextEdited: function(text) { workflowPage.afsClientDraftAddr = text }
                                    onEditFinished: function(text) { workflowPage.afsClientDraftAddr = text }
                                }
                                WorkflowLabeledEdit {
                                    Layout.fillWidth: true
                                    label: "端口"
                                    textValue: workflowPage.afsClientDraftPort
                                    warn: backend.afsClientConfigDirty
                                    enabled: !workflowPage.moduleSettingsLocked && !backend.afsClientRunning
                                    onTextEdited: function(text) { workflowPage.afsClientDraftPort = text }
                                    onEditFinished: function(text) { workflowPage.afsClientDraftPort = text }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
    
                                    WorkflowUiButton {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        text: "保存"
                                        enabled: !workflowPage.moduleSettingsLocked && !backend.afsClientRunning && backend.afsClientConfigDirty
                                        onClicked: workflowPage.saveAfsClientDraft()
                                    }
                                    WorkflowUiButton {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        text: "重置"
                                        enabled: !workflowPage.moduleSettingsLocked && !backend.afsClientRunning && backend.afsClientConfigDirty
                                        onClicked: workflowPage.resetAfsClientDraft()
                                    }
                                }
                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "测试AFSim"
                                    enabled: !workflowPage.moduleSettingsLocked
                                    onClicked: workflowPage.openAFSimTestPage()
                                }
                            }
    
                            ColumnLayout {
                                visible: workflowPage.selectedIsAfsServerModule()
                                Layout.fillWidth: true
                                spacing: 8
    
                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: backend.afsServerRunning ? "关闭控制出口" : "开启控制出口"
                                    visualChecked: backend.afsServerRunning
                                    enabled: !workflowPage.moduleSettingsLocked
                                    onClicked: workflowPage.toggleSelectedServer()
                                }
                                WorkflowLabeledEdit {
                                    Layout.fillWidth: true
                                    label: "IP"
                                    textValue: backend.afsServerAddr
                                    enabled: !workflowPage.moduleSettingsLocked && !backend.afsServerRunning
                                    onEditFinished: function(text) { backend.afsServerAddr = text }
                                }
                                WorkflowLabeledEdit {
                                    Layout.fillWidth: true
                                    label: "端口"
                                    textValue: backend.afsServerPort
                                    enabled: !workflowPage.moduleSettingsLocked && !backend.afsServerRunning
                                    onEditFinished: function(text) { backend.afsServerPort = text }
                                }
                            }
    
                            ColumnLayout {
                                visible: workflowPage.selectedIsTacviewServerModule()
                                Layout.fillWidth: true
                                spacing: 8
    
                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: backend.tacServerRunning ? "停止服务" : "启动服务"
                                    visualChecked: backend.tacServerRunning
                                    enabled: !workflowPage.moduleSettingsLocked
                                    onClicked: backend.toggleTacServer()
                                }
                                WorkflowLabeledEdit {
                                    Layout.fillWidth: true
                                    label: "用户"
                                    textValue: backend.tacServerUser
                                    enabled: !workflowPage.moduleSettingsLocked && !backend.tacServerRunning
                                    onEditFinished: function(text) { backend.tacServerUser = text }
                                }
                                WorkflowLabeledEdit {
                                    Layout.fillWidth: true
                                    label: "间隔"
                                    textValue: backend.tacSendIterMs
                                    suffix: "ms"
                                    enabled: !workflowPage.moduleSettingsLocked && !backend.tacServerRunning
                                    onEditFinished: function(text) { backend.tacSendIterMs = text }
                                }
                                WorkflowLabeledEdit {
                                    Layout.fillWidth: true
                                    label: "IP"
                                    textValue: backend.tacServerAddr
                                    enabled: !workflowPage.moduleSettingsLocked && !backend.tacServerRunning
                                    onEditFinished: function(text) { backend.tacServerAddr = text }
                                }
                                WorkflowLabeledEdit {
                                    Layout.fillWidth: true
                                    label: "端口"
                                    textValue: backend.tacServerPort
                                    enabled: !workflowPage.moduleSettingsLocked && !backend.tacServerRunning
                                    onEditFinished: function(text) { backend.tacServerPort = text }
                                }
                            }
                        }
    
                        Rectangle { visible: workflowPage.selectedIsMissionModule(); Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }

                        ColumnLayout {
                            visible: workflowPage.selectedIsMissionModule()
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 8
    
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
    
                                Label {
                                    Layout.fillWidth: true
                                    text: "上级指令"
                                    color: "#20242a"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 15
                                    font.bold: true
                                }
                                Label {
                                    visible: workflowPage.missionDraftDirty
                                    text: "未保存"
                                    color: "#b45309"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                            }
    
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5
    
                                Label {
                                    Layout.fillWidth: true
                                    text: "指令ID"
                                    color: workflowPage.missionDraftDirty ? "#b45309" : "#475569"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 13
                                    font.bold: workflowPage.missionDraftDirty
                                }
                                WorkflowUiTextField {
                                    Layout.fillWidth: true
                                    text: workflowPage.missionIdDraft
                                    warn: workflowPage.missionDraftDirty
                                    enabled: !workflowPage.moduleSettingsLocked
                                    onTextEdited: workflowPage.missionIdDraft = text
                                    onEditingFinished: workflowPage.missionIdDraft = text
                                }
                            }
    
                            Label {
                                Layout.fillWidth: true
                                text: "指令内容"
                                color: workflowPage.missionDraftDirty ? "#b45309" : "#475569"
                                font.family: workflowPage.appRoot.uiFontFamily
                                font.pixelSize: 13
                                font.bold: workflowPage.missionDraftDirty
                            }
                            TextArea {
                                id: missionTextArea
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                text: workflowPage.missionSourceDraft
                                enabled: !workflowPage.moduleSettingsLocked
                                wrapMode: TextEdit.Wrap
                                selectByMouse: true
                                font.family: workflowPage.appRoot.uiFontFamily
                                font.pixelSize: workflowPage.appRoot.uiFontSize
                                color: enabled ? "#0f172a" : "#94a3b8"
                                selectedTextColor: "#ffffff"
                                selectionColor: "#2563eb"
                                onTextChanged: {
                                    if (activeFocus && !workflowPage.moduleSettingsLocked) {
                                        workflowPage.missionSourceDraft = text
                                    }
                                }
                                background: Rectangle {
                                    radius: 4
                                    color: missionTextArea.enabled ? "#ffffff" : "#eef2f7"
                                    border.color: workflowPage.missionDraftDirty ? "#f97316"
                                                  : missionTextArea.activeFocus ? workflowPage.appRoot.uiFocusColor : workflowPage.appRoot.uiBorderColor
                                    border.width: workflowPage.missionDraftDirty || missionTextArea.activeFocus ? 1.4 : 1
                                }
                            }
    
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
    
                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "保存"
                                    enabled: !workflowPage.moduleSettingsLocked && workflowPage.missionDraftDirty
                                    onClicked: workflowPage.saveMissionDraft()
                                }
                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "清除"
                                    enabled: !workflowPage.moduleSettingsLocked &&
                                             (workflowPage.missionSourceDraft.length > 0 ||
                                              workflowPage.missionNormalizedDraftId !== backend.defaultMissionId)
                                    onClicked: workflowPage.clearMissionDraft()
                                }
                            }
                        }

                        Rectangle { visible: workflowPage.selectedIsDatabaseModule(); Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }

                        ColumnLayout {
                            visible: workflowPage.selectedIsDatabaseModule()
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true
                                    text: "数据库参数"
                                    color: "#20242a"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 15
                                    font.bold: true
                                }
                                Label {
                                    visible: workflowPage.databaseDraftDirty
                                    text: "未保存"
                                    color: "#b45309"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                            }

                            WorkflowLabeledEdit {
                                Layout.fillWidth: true
                                label: "超时"
                                textValue: workflowPage.databasePlatTimeoutDraft
                                suffix: "ms"
                                warn: workflowPage.databaseDraftDirty
                                enabled: !workflowPage.moduleSettingsLocked
                                onTextEdited: function(text) { workflowPage.databasePlatTimeoutDraft = text }
                                onEditFinished: function(text) { workflowPage.databasePlatTimeoutDraft = text }
                            }

                            WorkflowLabeledEdit {
                                Layout.fillWidth: true
                                label: "算法步长"
                                textValue: workflowPage.databaseAlgoStepDraft
                                suffix: "ms"
                                warn: workflowPage.databaseDraftDirty
                                enabled: !workflowPage.moduleSettingsLocked
                                onTextEdited: function(text) { workflowPage.databaseAlgoStepDraft = text }
                                onEditFinished: function(text) { workflowPage.databaseAlgoStepDraft = text }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "保存"
                                    enabled: !workflowPage.moduleSettingsLocked && workflowPage.databaseDraftDirty
                                    onClicked: workflowPage.saveDatabaseDraft()
                                }
                                WorkflowUiButton {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32
                                    text: "重置"
                                    enabled: !workflowPage.moduleSettingsLocked && workflowPage.databaseDraftDirty
                                    onClicked: workflowPage.resetDatabaseDraft()
                                }
                            }
                        }
    
                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }
    
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 10
                            Label { text: "输入 / 输出"; color: "#20242a"; font.family: workflowPage.appRoot.uiFontFamily; font.pixelSize: 15; font.bold: true }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
    
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
    
                                    Label {
                                        Layout.fillWidth: true
                                        text: "输入"
                                        color: "#475569"
                                        font.family: workflowPage.appRoot.uiFontFamily
                                        font.pixelSize: 13
                                        font.bold: true
                                    }
                                    Repeater {
                                        model: workflowPage.selectedInputs()
                                        delegate: WorkflowPill {
                                            required property string modelData
                                            Layout.fillWidth: true
                                            text: modelData
                                            accent: true
                                        }
                                    }
                                }
    
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
    
                                    Label {
                                        Layout.fillWidth: true
                                        text: "输出"
                                        color: "#475569"
                                        font.family: workflowPage.appRoot.uiFontFamily
                                        font.pixelSize: 13
                                        font.bold: true
                                    }
                                    Repeater {
                                        model: workflowPage.selectedOutputs()
                                        delegate: WorkflowPill {
                                            required property string modelData
                                            Layout.fillWidth: true
                                            text: modelData
                                        }
                                    }
                                }
                            }
                        }
    
                        Rectangle { visible: !workflowPage.selectedIsAfsClientModule() && !workflowPage.selectedIsConceptGraphModule() && !workflowPage.selectedIsMissionModule() && !workflowPage.selectedIsDatabaseModule(); Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }

                        ColumnLayout {
                            visible: !workflowPage.selectedIsAfsClientModule() && !workflowPage.selectedIsConceptGraphModule() && !workflowPage.selectedIsMissionModule() && !workflowPage.selectedIsDatabaseModule()
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 8
                            Label { text: "约束"; color: "#20242a"; font.family: workflowPage.appRoot.uiFontFamily; font.pixelSize: 15; font.bold: true }
                            WorkflowDetailRow { visible: workflowPage.selectedIsServerModule(); Layout.fillWidth: true; name: "服务状态"; value: workflowPage.selectedServerStateText() }
                            WorkflowDetailRow { Layout.fillWidth: true; name: "所在分区"; value: workflowPage.selectedLaneTitle() }
                            WorkflowDetailRow { Layout.fillWidth: true; name: "可拖动"; value: workflowPage.selectedDraggableText() }
                            WorkflowDetailRow { Layout.fillWidth: true; name: "调度策略"; value: workflowPage.selectedScheduleMutable() ? workflowPage.scheduleOptionsForModule(workflowPage.selectedModuleId).join(" / ") : "系统固定" }
                            WorkflowDetailRow { Layout.fillWidth: true; name: "失败处理"; value: "保持上一次可用结果" }
                        }
    
                        Rectangle { visible: !workflowPage.selectedIsAfsClientModule() && !workflowPage.selectedIsConceptGraphModule() && !workflowPage.selectedIsMissionModule() && !workflowPage.selectedIsDatabaseModule(); Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }

                        ColumnLayout {
                            visible: !workflowPage.selectedIsAfsClientModule() && !workflowPage.selectedIsConceptGraphModule() && !workflowPage.selectedIsMissionModule() && !workflowPage.selectedIsDatabaseModule()
                            Layout.fillWidth: true
                            Layout.margins: 16
                            spacing: 8
                            Label { text: "关键参数"; color: "#20242a"; font.family: workflowPage.appRoot.uiFontFamily; font.pixelSize: 15; font.bold: true }
                            WorkflowDetailRow { Layout.fillWidth: true; name: "刷新周期"; value: backend.uiSyncIterMs + " ms" }
                            WorkflowDetailRow { Layout.fillWidth: true; name: "威胁阈值"; value: "0.75" }
                            WorkflowDetailRow { Layout.fillWidth: true; name: "排序数量"; value: "Top 20" }
                        }
    
                    }
                }
            }
        }
    }
    
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 206
        radius: 6
        color: "#ffffff"
        border.color: "#d4d8de"
        border.width: 1
    
        RowLayout {
            anchors.fill: parent
            spacing: 0
    
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
    
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8
    
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6
                        color: "#fbfcfe"
                        border.color: "#d8dde4"
                        border.width: 1
    
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
    
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    Layout.fillWidth: true
                                    text: "主日志"
                                    color: "#20242a"
                                    font.family: workflowPage.appRoot.uiFontFamily
                                    font.pixelSize: 16
                                    font.bold: true
                                }
                            }
    
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }
    
                            WorkflowLogList {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                rows: workflowPage.workflowMainLogRows(backend.workflowLogRows)
                            }
                        }
                    }
    
                    Rectangle {
                        visible: false
                        Layout.preferredWidth: 0
                        Layout.minimumWidth: 0
                        Layout.maximumWidth: 0
                        Layout.fillHeight: true
                        radius: 6
                        color: "#fbfcfe"
                        border.color: "#d8dde4"
                        border.width: 1
    
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
    
                            Label {
                                Layout.fillWidth: true
                                text: "时间轴"
                                color: "#20242a"
                                font.family: workflowPage.appRoot.uiFontFamily
                                font.pixelSize: 16
                                font.bold: true
                            }
    
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#e1e5ea" }
    
                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 6
                                model: workflowPage.workflowTimelineRows(workflowPage.moduleStatusRefreshTick)
                                delegate: RowLayout {
                                    required property var modelData
                                    width: ListView.view.width
                                    height: 30
                                    spacing: 8
    
                                    Rectangle {
                                        Layout.preferredWidth: 8
                                        Layout.preferredHeight: 8
                                        radius: 4
                                        color: modelData.status === "run" ? "#45a657" :
                                               modelData.status === "wait" ? "#f2b600" : "#2f80ed"
                                        Layout.alignment: Qt.AlignTop
                                        Layout.topMargin: 7
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 8
                                            Label {
                                                Layout.preferredWidth: 54
                                                text: modelData.time
                                                color: "#687380"
                                                font.family: workflowPage.appRoot.uiFontFamily
                                                font.pixelSize: 12
                                                elide: Text.ElideRight
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.title
                                                color: "#20242a"
                                                font.family: workflowPage.appRoot.uiFontFamily
                                                font.pixelSize: 13
                                                font.bold: true
                                                elide: Text.ElideRight
                                            }
                                        }
                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.detail
                                            color: "#687380"
                                            font.family: workflowPage.appRoot.uiFontFamily
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
    
            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#d4d8de" }
    
            Item {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 0
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6
                        color: "#f6faff"
                        border.color: "#58afff"
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8
                            WorkflowMiniMap { Layout.fillWidth: true; Layout.fillHeight: true }
                            WorkflowUiButton {
                                id: openSituationMonitorButton
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                text: "打开态势监控"
                                onClicked: workflowPage.appRoot.requestMainTabChange(3)
                                background: Rectangle {
                                    radius: 4
                                    color: !openSituationMonitorButton.enabled ? "#eef2f7"
                                           : openSituationMonitorButton.down ? "#d7ecfb"
                                           : openSituationMonitorButton.hovered ? "#eef7ff" : "#f8fafc"
                                    border.color: "#58afff"
                                    border.width: 1
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
    
    component WorkflowUiButton: Button {
id: buttonControl
property bool visualChecked: checked
    
hoverEnabled: true
implicitHeight: 32
font.family: workflowPage.uiFontFamily
font.pixelSize: workflowPage.uiFontSize
font.bold: visualChecked
    
contentItem: Text {
    text: buttonControl.text
    font: buttonControl.font
    color: buttonControl.enabled ? "#0f172a" : "#94a3b8"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight
}
    
background: Rectangle {
    radius: 4
    color: !buttonControl.enabled ? "#eef2f7"
           : buttonControl.visualChecked ? "#bfe0f5"
           : buttonControl.down ? "#d7ecfb"
           : buttonControl.hovered ? "#eef7ff" : "#f8fafc"
    border.color: !buttonControl.enabled ? "#d5dde6"
                  : buttonControl.visualChecked || buttonControl.activeFocus ? workflowPage.uiFocusColor
                  : workflowPage.uiBorderColor
    border.width: buttonControl.visualChecked || buttonControl.activeFocus ? 1.4 : 1
}
    }
    component WorkflowUiTextField: TextField {
id: textFieldControl
property color normalColor: "#ffffff"
property color readOnlyColor: "#edf3f8"
property bool warn: false
    
implicitHeight: 32
leftPadding: 8
rightPadding: 8
selectByMouse: true
verticalAlignment: Text.AlignVCenter
horizontalAlignment: Text.AlignLeft
font.family: workflowPage.uiFontFamily
font.pixelSize: workflowPage.uiFontSize
color: !enabled ? "#94a3b8" : readOnly ? "#334155" : "#0f172a"
selectedTextColor: "#ffffff"
selectionColor: "#2563eb"
    
background: Rectangle {
    radius: 4
    color: !textFieldControl.enabled ? "#eef2f7"
           : textFieldControl.readOnly ? textFieldControl.readOnlyColor : textFieldControl.normalColor
    border.color: textFieldControl.warn ? "#f97316"
                  : textFieldControl.activeFocus ? workflowPage.uiFocusColor : workflowPage.uiBorderColor
    border.width: textFieldControl.warn || textFieldControl.activeFocus ? 1.4 : 1
}
    }
    component WorkflowUiComboBox: ComboBox {
id: comboControl
property color normalColor: "#ffffff"
property bool showArrow: true
    
implicitHeight: 32
leftPadding: 8
rightPadding: comboControl.showArrow ? 30 : 8
font.family: workflowPage.uiFontFamily
font.pixelSize: workflowPage.uiFontSize
    
contentItem: Text {
    leftPadding: comboControl.leftPadding
    rightPadding: comboControl.rightPadding
    text: comboControl.displayText
    font: comboControl.font
    color: comboControl.enabled ? "#0f172a" : "#94a3b8"
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight
}
    
indicator: Canvas {
    visible: comboControl.showArrow
    x: comboControl.width - width - 10
    y: (comboControl.height - height) / 2
    width: 10
    height: 6
    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        ctx.fillStyle = comboControl.enabled ? "#475569" : "#94a3b8"
        ctx.beginPath()
        ctx.moveTo(0, 0)
        ctx.lineTo(width, 0)
        ctx.lineTo(width / 2, height)
        ctx.closePath()
        ctx.fill()
    }
}
    
background: Rectangle {
    radius: 4
    color: comboControl.enabled ? comboControl.normalColor : "#eef2f7"
    border.color: comboControl.activeFocus || comboControl.popup.visible ? workflowPage.uiFocusColor : workflowPage.uiBorderColor
    border.width: comboControl.activeFocus || comboControl.popup.visible ? 1.4 : 1
}
    
delegate: ItemDelegate {
    id: comboDelegate
    required property var modelData
    required property int index
    
    width: comboControl.width
    text: String(modelData)
    font.family: workflowPage.uiFontFamily
    font.pixelSize: workflowPage.uiFontSize
    highlighted: comboControl.highlightedIndex === index
    contentItem: Text {
        text: comboDelegate.text
        font: comboDelegate.font
        color: "#0f172a"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        color: comboDelegate.highlighted ? "#dbeafe" : "#ffffff"
    }
}
    
popup: Popup {
    y: comboControl.height + 2
    width: comboControl.width
    implicitHeight: Math.min(contentItem.implicitHeight, 240)
    padding: 1
    
    contentItem: ListView {
        clip: true
        implicitHeight: contentHeight
        model: comboControl.popup.visible ? comboControl.delegateModel : null
        currentIndex: comboControl.highlightedIndex
        ScrollIndicator.vertical: ScrollIndicator { }
    }
    
    background: Rectangle {
        color: "#ffffff"
        radius: 4
        border.color: workflowPage.uiBorderColor
        border.width: 1
    }
}
    }
    component WorkflowPanel: Rectangle {
id: workflowPanel
property string title: ""
property string actionText: ""
property string actionToolTip: ""
property bool actionClickable: false
property bool compactHeader: false
property bool statusLegendVisible: false
property var statusLegendRows: workflowPage.workflowStatusLegendRows
default property alias contentData: panelBody.data
signal actionTriggered()
    
radius: 6
color: "#faffffff"
border.color: "#d4d9e0"
border.width: 1
clip: true
    
ColumnLayout {
    anchors.fill: parent
    spacing: 0
    
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 48
        color: "#ffffff"
        border.color: "#e1e5ea"
        border.width: 1
    
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: workflowPanel.compactHeader ? 9 : 16
            anchors.rightMargin: workflowPanel.compactHeader ? 9 : 12
            spacing: workflowPanel.statusLegendVisible ? 10 : (workflowPanel.title.length > 0 ? 8 : 0)
    
            Label {
                visible: workflowPanel.title.length > 0
                Layout.fillWidth: true
                text: workflowPanel.title
                color: "#20242a"
                font.family: workflowPage.uiFontFamily
                font.pixelSize: 16
                font.bold: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
    
            ColumnLayout {
                visible: workflowPanel.statusLegendVisible
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                spacing: 3
    
                Repeater {
                    model: workflowPanel.statusLegendRows
                    delegate: RowLayout {
                        id: statusLegendRow
                        required property var modelData
                        Layout.alignment: Qt.AlignRight
                        spacing: 8
    
                        Repeater {
                            model: statusLegendRow.modelData
                            delegate: RowLayout {
                                id: statusLegendItem
                                required property var modelData
                                spacing: 3
    
                                Rectangle {
                                    Layout.preferredWidth: 8
                                    Layout.preferredHeight: 8
                                    radius: 4
                                    color: statusLegendItem.modelData.color
                                }
    
                                Label {
                                    text: statusLegendItem.modelData.text
                                    color: "#59616b"
                                    font.family: workflowPage.uiFontFamily
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }
    
            Label {
                visible: workflowPanel.actionText.length > 0 && !workflowPanel.actionClickable
                text: workflowPanel.actionText
                color: "#687380"
                font.family: workflowPage.uiFontFamily
                font.pixelSize: 16
                verticalAlignment: Text.AlignVCenter
            }
    
            WorkflowUiButton {
                visible: workflowPanel.actionText.length > 0 && workflowPanel.actionClickable
                Layout.preferredWidth: 30
                Layout.preferredHeight: 28
                text: workflowPanel.actionText
                onClicked: workflowPanel.actionTriggered()
                ToolTip.visible: hovered && workflowPanel.actionToolTip.length > 0
                ToolTip.text: workflowPanel.actionToolTip
            }
        }
    }
    
    Item {
        id: panelBody
        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}
    }
    component WorkflowNode: Rectangle {
id: workflowNode
property string moduleId: title
property string title: ""
property string meta: ""
property string lane: ""
property string schedule: ""
property string scheduleShortText: schedule === "停用模块" ? "停用" :
                                   schedule === "实时刷新" ? "实时" :
                                   schedule === "自动触发" ? "自动" :
                                   schedule === "手动运行" ? "手动" : schedule
property string statusKind: "ready"
property color statusColor: workflowPage.workflowStatusColor(statusKind)
property bool source: false
property bool fixed: false
property bool disabledNode: false
property bool server: false
property bool selected: false
property bool warningNode: false
property var topPorts: []
property var rightPorts: []
property var bottomPorts: []
property var leftPorts: []
property bool removable: !fixed && !source && !server
property real baseX: 0
property real baseY: 0
property real dragOffsetX: 0
property real dragOffsetY: 0
property bool dragging: false
property real pressMouseX: 0
property real pressMouseY: 0
property real pressParentX: 0
property real pressParentY: 0
signal clicked()
signal moved(real nodeX, real nodeY)
signal dragPreview(real nodeX, real nodeY)
signal dragCleared()
    
width: 176
height: 64
x: baseX + dragOffsetX
y: baseY + dragOffsetY
radius: 6
color: disabledNode ? "#f5f7fa" : source ? "#f2fbfd" : server ? "#fff8f1" : fixed ? "#f8fff9" : "#ffffff"
border.color: warningNode ? "#f97316" :
              selected ? "#2f80ed" :
              disabledNode ? "#b9c1cc" : source ? "#45b8c5" : server ? "#d8893c" : fixed ? "#45a657" : "#2c2f33"
border.width: selected || warningNode ? 2 : 1
opacity: disabledNode ? 0.84 : 1
z: workflowNode.dragging ? 30 : (workflowNode.selected ? 20 : 5)
Drag.active: workflowNode.dragging
Drag.keys: workflowNode.removable ? ["workflow-canvas-node"] : []
Drag.source: workflowNode
Drag.hotSpot.x: width / 2
Drag.hotSpot.y: height / 2
    
MouseArea {
    id: nodeDragArea
    anchors.fill: parent
    cursorShape: Qt.OpenHandCursor
    drag.threshold: 8
    onClicked: workflowNode.clicked()
    onPressed: function(mouse) {
        cursorShape = Qt.ClosedHandCursor
        var pressPoint = workflowNode.mapToItem(workflowNode.parent, mouse.x, mouse.y)
        workflowNode.dragging = true
        workflowNode.pressMouseX = mouse.x
        workflowNode.pressMouseY = mouse.y
        workflowNode.pressParentX = pressPoint.x
        workflowNode.pressParentY = pressPoint.y
        workflowNode.dragOffsetX = 0
        workflowNode.dragOffsetY = 0
        workflowNode.dragPreview(workflowNode.baseX, workflowNode.baseY)
    }
    onPositionChanged: function(mouse) {
        if (!pressed) {
            return
        }
        var movePoint = workflowNode.mapToItem(workflowNode.parent, mouse.x, mouse.y)
        workflowNode.dragOffsetX = movePoint.x - workflowNode.pressParentX
        workflowNode.dragOffsetY = movePoint.y - workflowNode.pressParentY
        workflowNode.dragPreview(workflowNode.baseX + workflowNode.dragOffsetX,
                                 workflowNode.baseY + workflowNode.dragOffsetY)
    }
    onReleased: {
        cursorShape = Qt.OpenHandCursor
        workflowNode.moved(workflowNode.baseX + workflowNode.dragOffsetX,
                           workflowNode.baseY + workflowNode.dragOffsetY)
        workflowNode.dragCleared()
        workflowNode.dragOffsetX = 0
        workflowNode.dragOffsetY = 0
        workflowNode.dragging = false
    }
    onCanceled: {
        cursorShape = Qt.OpenHandCursor
        workflowNode.dragCleared()
        workflowNode.dragOffsetX = 0
        workflowNode.dragOffsetY = 0
        workflowNode.dragging = false
    }
}
    
Repeater {
    model: workflowNode.topPorts
    delegate: Rectangle {
        required property var modelData
        readonly property real portOffset: Number(modelData)
        x: workflowNode.width / 2 + portOffset - 5
        y: -5
        width: 10
        height: 10
        radius: 5
        color: "#ffffff"
        border.color: "#60656b"
        border.width: 1.4
    }
}
    
Repeater {
    model: workflowNode.rightPorts
    delegate: Rectangle {
        required property var modelData
        readonly property real portOffset: Number(modelData)
        x: workflowNode.width - 5
        y: workflowNode.height / 2 + portOffset - 5
        width: 10
        height: 10
        radius: 5
        color: "#ffffff"
        border.color: "#60656b"
        border.width: 1.4
    }
}
    
Repeater {
    model: workflowNode.bottomPorts
    delegate: Rectangle {
        required property var modelData
        readonly property real portOffset: Number(modelData)
        x: workflowNode.width / 2 + portOffset - 5
        y: workflowNode.height - 5
        width: 10
        height: 10
        radius: 5
        color: "#ffffff"
        border.color: "#60656b"
        border.width: 1.4
    }
}
    
Repeater {
    model: workflowNode.leftPorts
    delegate: Rectangle {
        required property var modelData
        readonly property real portOffset: Number(modelData)
        x: -5
        y: workflowNode.height / 2 + portOffset - 5
        width: 10
        height: 10
        radius: 5
        color: "#ffffff"
        border.color: "#60656b"
        border.width: 1.4
    }
}
    
Column {
    anchors.fill: parent
    anchors.leftMargin: 10
    anchors.rightMargin: 10
    anchors.topMargin: 8
    anchors.bottomMargin: 8
    spacing: 4
    
    RowLayout {
        width: parent.width
        height: 17
        spacing: 6
    
        Rectangle {
            Layout.preferredWidth: 9
            Layout.preferredHeight: 9
            radius: 4.5
            Layout.alignment: Qt.AlignVCenter
            color: workflowNode.statusColor
        }
    
        Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: workflowNode.title
            color: workflowNode.disabledNode ? "#89929e" : "#20242a"
            font.family: workflowPage.uiFontFamily
            font.pixelSize: 13
            font.bold: true
            elide: Text.ElideRight
        }
    
        Rectangle {
            visible: workflowNode.scheduleShortText.length > 0
            Layout.preferredWidth: 34
            Layout.preferredHeight: 18
            Layout.alignment: Qt.AlignVCenter
            radius: 4
            color: workflowNode.schedule === "停用模块" ? "#f1f3f6" :
                   workflowNode.schedule === "实时刷新" ? "#eaf7ee" :
                   workflowNode.schedule === "自动触发" ? "#fff7dd" : "#edf6ff"
            border.color: workflowNode.schedule === "停用模块" ? "#c6ccd4" :
                          workflowNode.schedule === "实时刷新" ? "#9ed6aa" :
                          workflowNode.schedule === "自动触发" ? "#f0c76a" : "#9ed0ff"
            border.width: 1
    
            Label {
                anchors.fill: parent
                text: workflowNode.scheduleShortText
                color: workflowNode.schedule === "停用模块" ? "#59616b" :
                       workflowNode.schedule === "实时刷新" ? "#2d7a3a" :
                       workflowNode.schedule === "自动触发" ? "#8a6500" : "#0b67d9"
                font.family: workflowPage.uiFontFamily
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }
    
    Label {
        width: parent.width
        text: workflowNode.meta
        color: workflowNode.disabledNode ? "#89929e" : "#687380"
        font.family: workflowPage.uiFontFamily
        font.pixelSize: 12
        lineHeight: 0.92
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        elide: Text.ElideRight
    }
}
    }
    component WorkflowLaneLabel: Rectangle {
property string text: ""
    
width: 92
height: 28
radius: 4
color: "#eef3f8"
z: 4
    
Label {
    anchors.centerIn: parent
    text: parent.text
    color: "#59616b"
    font.family: workflowPage.uiFontFamily
    font.pixelSize: 13
    font.bold: true
}
    }
    component WorkflowPill: Rectangle {
property string text: ""
property bool accent: false
    
implicitWidth: pillText.implicitWidth + 20
implicitHeight: 28
radius: 4
color: accent ? "#eff7ff" : "#ffffff"
border.color: accent ? "#9ed0ff" : "#d4dce6"
border.width: 1
    
Label {
    id: pillText
    anchors.fill: parent
    anchors.leftMargin: 10
    anchors.rightMargin: 10
    text: parent.text
    color: parent.accent ? "#2f80ed" : "#20242a"
    font.family: workflowPage.uiFontFamily
    font.pixelSize: 13
    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight
}
    }
    component WorkflowStatusCard: Rectangle {
id: workflowStatusCard
property string title: ""
property string text: ""
property string statusKind: "ready"
property bool clickable: false
property bool selected: false
property bool compact: false
signal clicked()
    
implicitHeight: workflowStatusCard.compact ? 54 : 48
height: implicitHeight
radius: 5
color: workflowStatusCard.selected ? "#edf6ff" : "#ffffff"
border.color: workflowStatusCard.selected ? "#2f80ed" : "#d7dbe1"
border.width: workflowStatusCard.selected ? 1.4 : 1
    
RowLayout {
    anchors.fill: parent
    anchors.leftMargin: workflowStatusCard.compact ? 9 : 12
    anchors.rightMargin: workflowStatusCard.compact ? 9 : 12
    spacing: workflowStatusCard.compact ? 7 : 10
    
    Rectangle {
        Layout.preferredWidth: workflowStatusCard.compact ? 8 : 10
        Layout.preferredHeight: workflowStatusCard.compact ? 8 : 10
        radius: width / 2
        color: workflowPage.workflowStatusColor(workflowStatusCard.statusKind)
    }
    
    ColumnLayout {
        Layout.fillWidth: true
        spacing: workflowStatusCard.compact ? 1 : 0
    
        Label {
            Layout.fillWidth: true
            text: workflowStatusCard.title
            color: "#20242a"
            font.family: workflowPage.uiFontFamily
            font.pixelSize: workflowStatusCard.compact ? 12 : 13
            font.bold: true
            elide: Text.ElideRight
        }
    
        Label {
            visible: workflowStatusCard.compact
            Layout.fillWidth: true
            text: workflowStatusCard.text
            color: "#687380"
            font.family: workflowPage.uiFontFamily
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }
    
    Label {
        visible: !workflowStatusCard.compact
        text: workflowStatusCard.text
        color: "#687380"
        font.family: workflowPage.uiFontFamily
        font.pixelSize: 13
        elide: Text.ElideRight
    }
}
    
MouseArea {
    anchors.fill: parent
    enabled: workflowStatusCard.clickable
    cursorShape: workflowStatusCard.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
    onClicked: workflowStatusCard.clicked()
}
    }
    component WorkflowMiniMap: Rectangle {
id: miniMap
radius: 4
color: "#ffffff"
border.color: "#b8d8fa"
border.width: 1
clip: true
    
Image {
    anchors.fill: parent
    anchors.margins: 4
    source: "qrc:/UiICGAS/workflow_situation_monitor.png"
    fillMode: Image.PreserveAspectCrop
    smooth: true
    mipmap: true
}
    }
    component WorkflowDetailRow: RowLayout {
property string name: ""
property string value: ""
    
spacing: 10
Label {
    Layout.preferredWidth: 90
    text: parent.name
    color: "#687380"
    font.family: workflowPage.uiFontFamily
    font.pixelSize: 13
    elide: Text.ElideRight
}
Label {
    Layout.fillWidth: true
    text: parent.value
    color: "#20242a"
    font.family: workflowPage.uiFontFamily
    font.pixelSize: 13
    wrapMode: Text.Wrap
}
    }
    component WorkflowScheduleButton: Button {
id: scheduleButton
property bool visualChecked: checked
    
hoverEnabled: true
implicitHeight: 32
padding: 0
font.family: workflowPage.uiFontFamily
font.pixelSize: workflowPage.uiFontSize
font.bold: false
    
contentItem: Text {
    text: scheduleButton.text
    font: scheduleButton.font
    color: !scheduleButton.enabled ? "#94a3b8"
           : scheduleButton.visualChecked ? "#0b67d9" : "#334155"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight
}
    
background: Rectangle {
    radius: 4
    color: !scheduleButton.enabled ? "#eef2f7"
           : scheduleButton.visualChecked ? "#d7ecfb"
           : scheduleButton.down ? "#d7ecfb"
           : scheduleButton.hovered ? "#eef7ff" : "#f8fafc"
    border.color: !scheduleButton.enabled ? "#d5dde6"
                  : scheduleButton.visualChecked ? "#8fc8eb"
                  : workflowPage.uiBorderColor
    border.width: 1
}
    }
    component WorkflowLabeledEdit: RowLayout {
id: labeledEdit
property string label: ""
property string textValue: ""
property string suffix: ""
property bool warn: false
signal editFinished(string text)
signal textEdited(string text)
    
spacing: 4
Layout.minimumWidth: 0
    
Label {
    text: labeledEdit.label
    color: "#475569"
    font.family: workflowPage.uiFontFamily
    font.pixelSize: workflowPage.uiFontSize
    Layout.preferredWidth: 34
    horizontalAlignment: Text.AlignRight
}
    
WorkflowUiTextField {
    id: edit
    Layout.fillWidth: true
    text: labeledEdit.textValue
    enabled: labeledEdit.enabled
    warn: labeledEdit.warn
    onTextEdited: labeledEdit.textEdited(text)
    onEditingFinished: labeledEdit.editFinished(text)
}
    
Label {
    visible: labeledEdit.suffix.length > 0
    text: labeledEdit.suffix
    color: "#475569"
    font.family: workflowPage.uiFontFamily
    font.pixelSize: workflowPage.uiFontSize
    Layout.preferredWidth: visible ? 20 : 0
    horizontalAlignment: Text.AlignLeft
}
    }
    component WorkflowLogList: ListView {
id: logList
property var rows
model: logList.rows
clip: true
spacing: 2
delegate: Label {
    required property string modelData
    width: ListView.view.width
    text: modelData
    color: modelData.indexOf("[ERROR]") >= 0 ? "#b91c1c" :
           modelData.indexOf("[WARN]") >= 0 ? "#b45309" : "#111827"
    font.family: workflowPage.uiFontFamily
    font.pixelSize: workflowPage.uiSmallFontSize
    elide: Text.ElideRight
}
onCountChanged: positionViewAtEnd()
    }
    }
