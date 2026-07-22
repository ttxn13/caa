import QtQuick 6.8
import QtQuick 6.7 as Quick67
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Window 6.8
import ICGAS.Ui 1.0
pragma ComponentBehavior: Bound

Item {
id: initialEditor
property var appRoot: null
property var controller: uiScenario
readonly property var backend: controller && controller.backend ? controller.backend : uiICGAS
readonly property string uiFontFamily: appRoot ? appRoot.uiFontFamily : "Microsoft YaHei UI"
readonly property int uiFontSize: appRoot ? appRoot.uiFontSize : 14
readonly property int uiSmallFontSize: appRoot ? appRoot.uiSmallFontSize : 13
readonly property int uiTitleFontSize: appRoot ? appRoot.uiTitleFontSize : 17
readonly property color uiBorderColor: appRoot ? appRoot.uiBorderColor : "#b8c7d6"
readonly property color uiSoftBorderColor: appRoot ? appRoot.uiSoftBorderColor : "#d6e0ea"
readonly property color uiFocusColor: appRoot ? appRoot.uiFocusColor : "#2f80c4"
readonly property int detailFieldSpacing: 8
readonly property int detailSingleFieldWidth: 112
readonly property int detailGridWidth: detailSingleFieldWidth * 3 + detailFieldSpacing * 2
    
function graphValue(row, name, fallbackValue) {
    return appRoot ? appRoot.graphValue(row, name, fallbackValue)
                   : (row && row[name] !== undefined && row[name] !== null ? row[name] : fallbackValue)
}
    
    property string catalogMode: "platform"
    property string selectedCatalogId: ""
    property bool showPlatformId: false
    property bool showCommandLink: true
    property bool showSensorRange: false
    property bool showWeaponRange: false
    property bool showRoute: true
    property bool draggingCatalog: false
    property string dragKind: ""
    property string dragTypeId: ""
    property string dragLabel: ""
    property real dragX: 0
    property real dragY: 0
    readonly property bool initialPageVisible: appRoot && appRoot.uiStartupComplete && appRoot.activeMainTabIndex === 0
    property var initialRowsCache: []
    property bool mapContainerReady: false
    property int selectedWaypointIndex: 0
    property bool capabilitySectionExpanded: true
    property bool signatureSectionExpanded: true

    Timer {
        id: deferredMapContainerTimer
        interval: 120
        repeat: false
        onTriggered: initialEditor.mapContainerReady = true
    }
    
    Component.onCompleted: {
        ensureCatalogSelection()
        rebuildInitialGeometry()
        if (initialPageVisible)
            deferredMapContainerTimer.restart()
    }
    onInitialPageVisibleChanged: {
        if (initialPageVisible && initialRowsCache.length === 0) {
            rebuildInitialGeometry()
        }
        if (initialPageVisible && !mapContainerReady)
            deferredMapContainerTimer.restart()
    }
    
    Connections {
        target: backend
        function onInitialCatalogChanged() {
            initialEditor.ensureCatalogSelection()
        }
        function onInitialScenarioChanged() {
            initialEditor.rebuildInitialGeometry()
        }
    }
    
    function currentCatalogRows() {
        if (catalogMode === "weapon")
            return backend.initialWeaponTypeRows
        if (catalogMode === "sensor")
            return backend.initialSensorTypeRows
        return backend.initialPlatformTypeRows
    }
    
    function catalogTitle() {
        if (catalogMode === "weapon")
            return "武器类型"
        if (catalogMode === "sensor")
            return "传感器类型"
        return "平台类型"
    }
    
    function rowTypeId(row) {
        return String(initialEditor.graphValue(row, "type_id", ""))
    }
    
    function selectedCatalogObject() {
        var rows = currentCatalogRows()
        for (var i = 0; i < rows.length; ++i) {
            if (rowTypeId(rows[i]) === selectedCatalogId)
                return rows[i]
        }
        return rows.length > 0 ? rows[0] : ({})
    }
    
    function ensureCatalogSelection() {
        var rows = currentCatalogRows()
        if (rows.length === 0) {
            selectedCatalogId = ""
            return
        }
        for (var i = 0; i < rows.length; ++i) {
            if (rowTypeId(rows[i]) === selectedCatalogId)
                return
        }
        selectedCatalogId = rowTypeId(rows[0])
    }
    
    function nestedValue(row, path, fallbackValue) {
        var value = row
        var parts = String(path).split(".")
        for (var i = 0; i < parts.length; ++i) {
            if (!value || value[parts[i]] === undefined || value[parts[i]] === null)
                return fallbackValue
            value = value[parts[i]]
        }
        return value
    }
    
    function asArray(value) {
        return value && value.length !== undefined ? value : []
    }

    function detailFieldSpan(field) {
        var span = Number(graphValue(field, "span", 1))
        return span >= 2 ? 2 : 1
    }

    function detailFieldWidth(field) {
        var span = detailFieldSpan(field)
        return detailSingleFieldWidth * span + detailFieldSpacing * (span - 1)
    }

    function rangeText(value) {
        var number = Number(value || 0)
        if (number <= 0)
            return "--"
        if (number >= 1000)
            return formatNumber(number / 1000) + " km"
        return formatNumber(number) + " m"
    }

    function formatNumber(value) {
        if (value === undefined || value === null || value === "")
            return ""
        var number = Number(value)
        if (!isFinite(number))
            return String(value)
        var text = number.toFixed(4)
        text = text.replace(/\.?0+$/, "")
        return text === "-0" ? "0" : text
    }

    function localizedEnumText(value) {
        var text = String(value === undefined || value === null ? "" : value)
        var labels = {
            "DETECT": "探测",
            "TRACK": "跟踪",
            "GUIDE": "制导",
            "RADAR": "雷达",
            "OPTICAL": "光电",
            "INFRARED": "红外",
            "AN_APG_63_V3": "AN/APG-63(V)3",
            "AN_APQ_166": "AN/APQ-166",
            "AN_APY_2": "AN/APY-2",
            "AN_ASQ_228_ATFLIR": "AN/ASQ-228 ATFLIR",
            "AN_SPS_48G": "AN/SPS-48G",
            "AN_SPY_6_V1": "AN/SPY-6(V)1",
            "WESCAM_MX_15D": "WESCAM MX-15D"
        }
        return labels[text.toUpperCase()] || text
    }

    function catalogFieldOptions(field) {
        var path = String(initialEditor.graphValue(field, "path", ""))
        if (path === "icon_path")
            return backend.initialPlatformIconOptions
        if (path === "mover")
            return backend.initialMoverTypeOptions
        if (path === "domain")
            return backend.initialDomainOptions
        if (path === "type")
            return catalogMode === "weapon" ? initialEditor.weaponClassOptions()
                 : (catalogMode === "sensor" ? backend.initialSensorClassOptions : backend.initialPlatformClassOptions)
        return []
    }

    function weaponClassOptions() {
        return backend.initialWeaponClassOptions
    }

    function platformTypeOptions() {
        var rows = backend.initialPlatformTypeRows || []
        var options = []
        for (var i = 0; i < rows.length; ++i)
            options.push(rowTypeId(rows[i]))
        return options
    }

    function sideOptions() {
        return [ "RED", "BLUE", "WHITE", "UKN" ]
    }

    function normalizedSideValue(value) {
        var text = String(value || "").trim().toUpperCase()
        if (text === "RED" || text === "红方")
            return "RED"
        if (text === "BLUE" || text === "蓝方")
            return "BLUE"
        if (text === "WHITE" || text === "白方")
            return "WHITE"
        if (text === "UNKNOWN" || text === "不明" || text === "")
            return "UKN"
        return text
    }

    function commanderOptions() {
        var options = [ "SELF" ]
        var rows = platformRows()
        var selectedId = String(initialEditor.graphValue(selectedPlatform(), "platform_id", ""))
        for (var i = 0; i < rows.length; ++i) {
            var platformId = String(initialEditor.graphValue(rows[i], "platform_id", ""))
            if (platformId !== "" && platformId !== selectedId && options.indexOf(platformId) < 0)
                options.push(platformId)
        }

        var current = String(initialEditor.graphValue(selectedPlatform(), "commander", ""))
        if (current !== "" && options.indexOf(current) < 0)
            options.push(current)
        return options
    }

    function detailComboOptions(field) {
        var path = String(initialEditor.graphValue(field, "path", ""))
        if (path === "type_id")
            return platformTypeOptions()
        if (path === "side")
            return sideOptions()
        if (path === "commander")
            return commanderOptions()
        return []
    }

    function detailComboValue(field) {
        var path = String(initialEditor.graphValue(field, "path", ""))
        var value = initialEditor.graphValue(field, "value", "")
        if (path === "side")
            return normalizedSideValue(value)
        return String(value)
    }

    function detailComboIndex(field) {
        var options = detailComboOptions(field)
        if (options.length <= 0)
            return -1
        var index = options.indexOf(detailComboValue(field))
        return index >= 0 ? index : 0
    }

    function catalogComboIndex(field) {
        var options = catalogFieldOptions(field)
        var value = String(initialEditor.graphValue(field, "value", ""))
        var index = options.indexOf(value)
        return index >= 0 ? index : 0
    }

    function toggleCatalogSection(sectionKey) {
        if (sectionKey === "capability")
            capabilitySectionExpanded = !capabilitySectionExpanded
        else if (sectionKey === "signature")
            signatureSectionExpanded = !signatureSectionExpanded
    }
    
    function catalogFields() {
        var item = selectedCatalogObject()
        if (catalogMode === "platform") {
            var rows = [
                { "label": "平台类型名称", "path": "type_name", "value": nestedValue(item, "type_name", ""), "editable": true },
                { "label": "图标文件", "path": "icon_path", "value": nestedValue(item, "icon_path", ""), "editor": "combo", "editable": true },
                { "group": true, "fields": [
                    { "label": "作战域", "path": "domain", "value": nestedValue(item, "domain", ""), "editor": "combo", "editable": true },
                    { "label": "平台类型", "path": "type", "value": nestedValue(item, "type", ""), "editor": "combo", "editable": true },
                    { "label": "运动模型", "path": "mover", "value": nestedValue(item, "mover", ""), "editor": "combo", "editable": true }
                ] },
                { "label": "能力边界", "section": true, "sectionKey": "capability", "expanded": capabilitySectionExpanded }
            ]
            var mover = nestedValue(item, "mover", "")
            if (capabilitySectionExpanded) {
                if (mover === "AIR_MOVER") {
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最小速度", "unit": "m/s", "path": "cap.air_mover.min_speed_ms", "value": nestedValue(item, "cap.air_mover.min_speed_ms", 0), "editable": true },
                        { "label": "最大速度", "unit": "m/s", "path": "cap.air_mover.max_speed_ms", "value": nestedValue(item, "cap.air_mover.max_speed_ms", 0), "editable": true }
                    ] })
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最低高度", "unit": "m", "path": "cap.air_mover.min_alt_m", "value": nestedValue(item, "cap.air_mover.min_alt_m", 0), "editable": true },
                        { "label": "最高高度", "unit": "m", "path": "cap.air_mover.max_alt_m", "value": nestedValue(item, "cap.air_mover.max_alt_m", 0), "editable": true }
                    ] })
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最大线性过载", "unit": "g", "path": "cap.air_mover.max_linear_load_g", "value": nestedValue(item, "cap.air_mover.max_linear_load_g", 0), "editable": true },
                        { "label": "最大转向过载", "unit": "g", "path": "cap.air_mover.max_radial_load_g", "value": nestedValue(item, "cap.air_mover.max_radial_load_g", 0), "editable": true }
                    ] })
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最大爬升速度", "unit": "m/s", "path": "cap.air_mover.max_climb_vel_ms", "value": nestedValue(item, "cap.air_mover.max_climb_vel_ms", 0), "editable": true },
                        { "label": "最大俯冲速度", "unit": "m/s", "path": "cap.air_mover.max_drop_vel_ms", "value": nestedValue(item, "cap.air_mover.max_drop_vel_ms", 0), "editable": true }
                    ] })
                } else if (mover === "SEA_MOVER") {
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最小航速", "unit": "m/s", "path": "cap.sea_mover.min_speed_ms", "value": nestedValue(item, "cap.sea_mover.min_speed_ms", 0), "editable": true },
                        { "label": "最大航速", "unit": "m/s", "path": "cap.sea_mover.max_speed_ms", "value": nestedValue(item, "cap.sea_mover.max_speed_ms", 0), "editable": true }
                    ] })
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最大线性加速度", "unit": "m/s2", "path": "cap.sea_mover.max_linear_acc_ms2", "value": nestedValue(item, "cap.sea_mover.max_linear_acc_ms2", 0), "editable": true },
                        { "label": "最大转向加速度", "unit": "m/s2", "path": "cap.sea_mover.max_radial_acc_ms2", "value": nestedValue(item, "cap.sea_mover.max_radial_acc_ms2", 0), "editable": true }
                    ] })
                } else if (mover === "MIS_MOVER") {
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最小速度", "unit": "m/s", "path": "cap.mis_mover.min_speed_ms", "value": nestedValue(item, "cap.mis_mover.min_speed_ms", 0), "editable": true },
                        { "label": "最大速度", "unit": "m/s", "path": "cap.mis_mover.max_speed_ms", "value": nestedValue(item, "cap.mis_mover.max_speed_ms", 0), "editable": true }
                    ] })
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最低高度", "unit": "m", "path": "cap.mis_mover.min_alt_m", "value": nestedValue(item, "cap.mis_mover.min_alt_m", 0), "editable": true },
                        { "label": "最高高度", "unit": "m", "path": "cap.mis_mover.max_alt_m", "value": nestedValue(item, "cap.mis_mover.max_alt_m", 0), "editable": true }
                    ] })
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最大线性过载", "unit": "g", "path": "cap.mis_mover.max_linear_load_g", "value": nestedValue(item, "cap.mis_mover.max_linear_load_g", 0), "editable": true },
                        { "label": "最大转向过载", "unit": "g", "path": "cap.mis_mover.max_radial_load_g", "value": nestedValue(item, "cap.mis_mover.max_radial_load_g", 0), "editable": true }
                    ] })
                    rows.push({ "group": true, "paired": true, "fields": [
                        { "label": "最大爬升速度", "unit": "m/s", "path": "cap.mis_mover.max_climb_vel_ms", "value": nestedValue(item, "cap.mis_mover.max_climb_vel_ms", 0), "editable": true },
                        { "label": "最大俯冲速度", "unit": "m/s", "path": "cap.mis_mover.max_drop_vel_ms", "value": nestedValue(item, "cap.mis_mover.max_drop_vel_ms", 0), "editable": true }
                    ] })
                    rows.push({ "label": "寿命", "unit": "s", "path": "cap.mis_mover.life_time_s", "value": nestedValue(item, "cap.mis_mover.life_time_s", 0), "editable": true })
                }
            }
            rows.push({ "label": "探测特征", "section": true, "sectionKey": "signature", "expanded": signatureSectionExpanded })
            if (signatureSectionExpanded) {
                rows.push({ "group": true, "paired": true, "fields": [
                    { "label": "雷达截面", "unit": "m2", "path": "sign.radar_rcs_m2", "value": nestedValue(item, "sign.radar_rcs_m2", 0), "editable": true },
                    { "label": "光学可见面积", "unit": "m2", "path": "sign.opt_area_m2", "value": nestedValue(item, "sign.opt_area_m2", 0), "editable": true },
                    { "label": "红外强度", "unit": "W/sr", "path": "sign.infrared_wsr", "value": nestedValue(item, "sign.infrared_wsr", 0), "editable": true }
                ] })
            }
            return rows
        }
        if (catalogMode === "weapon") {
            return [
                { "label": "武器类型名称", "path": "type_name", "value": nestedValue(item, "type_name", ""), "editable": true },
                { "group": true, "fields": [
                    { "label": "作战域", "path": "domain", "value": nestedValue(item, "domain", ""), "editor": "combo", "editable": true },
                    { "label": "武器类型", "path": "type", "value": nestedValue(item, "type", ""), "editor": "combo", "editable": true },
                    { "label": "运动模型", "path": "mover", "value": nestedValue(item, "mover", ""), "editor": "combo", "editable": true }
                ] },
                { "group": true, "paired": true, "fields": [
                    { "label": "最小射程", "unit": "m", "path": "launch_cap.min_range_m", "value": nestedValue(item, "launch_cap.min_range_m", 0), "editable": true },
                    { "label": "最大射程", "unit": "m", "path": "launch_cap.max_range_m", "value": nestedValue(item, "launch_cap.max_range_m", 0), "editable": true }
                ] },
                { "label": "发射延迟", "unit": "s", "path": "launch_cap.firing_delay_s", "value": nestedValue(item, "launch_cap.firing_delay_s", 0), "editable": true },
                { "group": true, "paired": true, "fields": [
                    { "label": "最小速度", "unit": "m/s", "path": "mover_cap.mis_mover.min_speed_ms", "value": nestedValue(item, "mover_cap.mis_mover.min_speed_ms", 0), "editable": true },
                    { "label": "最大速度", "unit": "m/s", "path": "mover_cap.mis_mover.max_speed_ms", "value": nestedValue(item, "mover_cap.mis_mover.max_speed_ms", 0), "editable": true }
                ] },
                { "group": true, "paired": true, "fields": [
                    { "label": "最低高度", "unit": "m", "path": "mover_cap.mis_mover.min_alt_m", "value": nestedValue(item, "mover_cap.mis_mover.min_alt_m", 0), "editable": true },
                    { "label": "最高高度", "unit": "m", "path": "mover_cap.mis_mover.max_alt_m", "value": nestedValue(item, "mover_cap.mis_mover.max_alt_m", 0), "editable": true }
                ] },
                { "group": true, "paired": true, "fields": [
                    { "label": "最大线性过载", "unit": "g", "path": "mover_cap.mis_mover.max_linear_load_g", "value": nestedValue(item, "mover_cap.mis_mover.max_linear_load_g", 0), "editable": true },
                    { "label": "最大转向过载", "unit": "g", "path": "mover_cap.mis_mover.max_radial_load_g", "value": nestedValue(item, "mover_cap.mis_mover.max_radial_load_g", 0), "editable": true }
                ] },
                { "group": true, "paired": true, "fields": [
                    { "label": "最大爬升速度", "unit": "m/s", "path": "mover_cap.mis_mover.max_climb_vel_ms", "value": nestedValue(item, "mover_cap.mis_mover.max_climb_vel_ms", 0), "editable": true },
                    { "label": "最大俯冲速度", "unit": "m/s", "path": "mover_cap.mis_mover.max_drop_vel_ms", "value": nestedValue(item, "mover_cap.mis_mover.max_drop_vel_ms", 0), "editable": true }
                ] },
                { "group": true, "paired": true, "fields": [
                    { "label": "距离寿命", "unit": "m", "path": "mover_cap.mis_mover.life_range_m", "value": nestedValue(item, "mover_cap.mis_mover.life_range_m", 0), "editable": true },
                    { "label": "寿命", "unit": "s", "path": "mover_cap.mis_mover.life_time_s", "value": nestedValue(item, "mover_cap.mis_mover.life_time_s", 0), "editable": true }
                ] },
                { "group": true, "paired": true, "fields": [
                    { "label": "杀伤半径", "unit": "m", "path": "kill_cap.kill_range_m", "value": nestedValue(item, "kill_cap.kill_range_m", 0), "editable": true },
                    { "label": "杀伤概率", "path": "kill_cap.kill_prob", "value": nestedValue(item, "kill_cap.kill_prob", 0), "editable": true }
                ] },
                { "group": true, "paired": true, "fields": [
                    { "label": "雷达截面", "unit": "m2", "path": "sign.radar_rcs_m2", "value": nestedValue(item, "sign.radar_rcs_m2", 0), "editable": true },
                    { "label": "光学可见面积", "unit": "m2", "path": "sign.opt_area_m2", "value": nestedValue(item, "sign.opt_area_m2", 0), "editable": true },
                    { "label": "红外强度", "unit": "W/sr", "path": "sign.infrared_wsr", "value": nestedValue(item, "sign.infrared_wsr", 0), "editable": true }
                ] }
            ]
        }
        if (catalogMode === "sensor") {
            var sensorRows = [
                { "label": "传感器类型名称", "path": "type_name", "value": nestedValue(item, "type_name", ""), "editable": true },
                { "group": true, "fields": [
                    { "label": "传感器类型", "path": "type", "value": nestedValue(item, "type", ""), "editor": "combo", "editable": true },
                    { "label": "可靠度", "path": "reliability", "value": nestedValue(item, "reliability", 1), "editable": true },
                    { "label": "可探测域", "path": "act_domain_list", "value": nestedValue(item, "act_domain_list", ""), "editable": true }
                ] }
            ]
            var tasks = [ "DETECT", "TRACK", "GUIDE" ]
            for (var ti = 0; ti < tasks.length; ++ti) {
                var task = tasks[ti]
                sensorRows.push({ "label": localizedEnumText(task), "section": true, "sectionKey": "capability", "expanded": true })
                sensorRows.push({ "group": true, "paired": true, "fields": [
                    { "label": "目标容量", "path": "task_mode_list." + task + ".base.max_task_num", "value": nestedValue(item, "task_mode_list." + task + ".base.max_task_num", 0), "editable": true },
                    { "label": "最大距离", "unit": "m", "path": "task_mode_list." + task + ".cap_env.max_range_m", "value": nestedValue(item, "task_mode_list." + task + ".cap_env.max_range_m", 0), "editable": true }
                ] })
                sensorRows.push({ "group": true, "paired": true, "fields": [
                    { "label": "最小距离", "unit": "m", "path": "task_mode_list." + task + ".cap_env.min_range_m", "value": nestedValue(item, "task_mode_list." + task + ".cap_env.min_range_m", 0), "editable": true },
                    { "label": "方位视场", "unit": "deg", "path": "task_mode_list." + task + ".cap_env.az_fov_deg", "value": nestedValue(item, "task_mode_list." + task + ".cap_env.az_fov_deg", 0), "editable": true }
                ] })
                sensorRows.push({ "group": true, "paired": true, "fields": [
                    { "label": "最小俯仰角", "unit": "deg", "path": "task_mode_list." + task + ".cap_env.min_el_deg", "value": nestedValue(item, "task_mode_list." + task + ".cap_env.min_el_deg", 0), "editable": true },
                    { "label": "最大俯仰角", "unit": "deg", "path": "task_mode_list." + task + ".cap_env.max_el_deg", "value": nestedValue(item, "task_mode_list." + task + ".cap_env.max_el_deg", 0), "editable": true }
                ] })
                sensorRows.push({ "group": true, "paired": true, "fields": [
                    { "label": "基础探测概率", "path": "task_mode_list." + task + ".cap_prob.detection_prob", "value": nestedValue(item, "task_mode_list." + task + ".cap_prob.detection_prob", 0), "editable": true },
                    { "label": "虚警概率", "path": "task_mode_list." + task + ".cap_prob.false_alarm_prob", "value": nestedValue(item, "task_mode_list." + task + ".cap_prob.false_alarm_prob", 0), "editable": true }
                ] })
                sensorRows.push({ "group": true, "paired": true, "fields": [
                    { "label": "距离误差", "unit": "m", "path": "task_mode_list." + task + ".cap_acc.range_sigma_m", "value": nestedValue(item, "task_mode_list." + task + ".cap_acc.range_sigma_m", 0), "editable": true },
                    { "label": "方位误差", "unit": "deg", "path": "task_mode_list." + task + ".cap_acc.azimuth_sigma_deg", "value": nestedValue(item, "task_mode_list." + task + ".cap_acc.azimuth_sigma_deg", 0), "editable": true }
                ] })
                sensorRows.push({ "group": true, "paired": true, "fields": [
                    { "label": "扫描周期", "unit": "s", "path": "task_mode_list." + task + ".cap_time.update_period_s", "value": nestedValue(item, "task_mode_list." + task + ".cap_time.update_period_s", 0), "editable": true },
                    { "label": "延迟", "unit": "ms", "path": "task_mode_list." + task + ".cap_time.delay_ms", "value": nestedValue(item, "task_mode_list." + task + ".cap_time.delay_ms", 0), "editable": true }
                ] })
            }
            return sensorRows
        }
        return [
            { "label": "名称", "path": "name", "value": nestedValue(item, "name", ""), "editable": true },
            { "label": "阵营", "path": "side", "value": nestedValue(item, "side", ""), "editable": true },
            { "label": "类型", "path": "class", "value": nestedValue(item, "class", ""), "editable": true },
            { "label": "扫描模式", "path": "scan_mode", "value": nestedValue(item, "scan_mode", ""), "editable": true },
            { "label": "探测边界(m)", "path": "capability_boundary.max_range_m", "value": nestedValue(item, "capability_boundary.max_range_m", 0), "editable": true }
        ]
    }
    function updateCatalogField(fieldPath, value) {
        if (selectedCatalogId !== "") {
            var oldId = selectedCatalogId
            var ok = backend.updateInitialCatalogField(catalogMode, oldId, fieldPath, value)
            if (ok && (catalogMode === "platform" || catalogMode === "weapon" || catalogMode === "sensor") && String(fieldPath) === "type_name")
                selectedCatalogId = String(value).trim()
        }
    }
    
    function platformRows() {
        return initialRowsCache
    }
    
    function selectedPlatform() {
        return backend.initialSelectedScenarioPlatform || ({})
    }

    function rebuildInitialGeometry() {
        initialRowsCache = backend.initialScenarioPlatformRows || []
    }
    
    function xToLon(x) {
        return situationMap.lonAt(x)
    }
    
    function yToLat(y) {
        return situationMap.latAt(y)
    }
    
    function platformAtPoint(x, y) {
        return situationMap.platformAt(x, y)
    }

    function geoAtPoint(x, y) {
        return situationMap.geoAt(x, y)
    }
    
    function beginCatalogDrag(kind, typeId, label, sourceItem, mouse) {
        dragKind = kind
        dragTypeId = typeId
        dragLabel = label
        draggingCatalog = true
        updateCatalogDrag(sourceItem, mouse)
    }
    
    function updateCatalogDrag(sourceItem, mouse) {
        var p = sourceItem.mapToItem(initialEditor, mouse.x, mouse.y)
        dragX = p.x
        dragY = p.y
    }
    
    function finishCatalogDrag(sourceItem, mouse) {
        updateCatalogDrag(sourceItem, mouse)
        var plotPos = sourceItem.mapToItem(plotArea, mouse.x, mouse.y)
        if (plotPos.x >= 0 && plotPos.x <= plotArea.width && plotPos.y >= 0 && plotPos.y <= plotArea.height) {
            if (dragKind === "platform") {
                var geo = geoAtPoint(plotPos.x, plotPos.y)
                backend.addInitialScenarioPlatform(dragTypeId, Number(geo.lon_deg), Number(geo.lat_deg))
            } else {
                var targetId = platformAtPoint(plotPos.x, plotPos.y)
                if (targetId !== "")
                    backend.mountInitialEquipment(targetId, dragKind, dragTypeId)
            }
        }
        draggingCatalog = false
        dragKind = ""
        dragTypeId = ""
        dragLabel = ""
    }

    function openCreateScenarioDialog() {
        createScenarioNameField.text = initialEditor.defaultScenarioName()
        createScenarioMode.currentIndex = 0
        createScenarioDialog.visible = true
        createScenarioDialog.raise()
        createScenarioDialog.requestActivate()
        Qt.callLater(function() {
            createScenarioNameField.forceActiveFocus()
            createScenarioNameField.selectAll()
        })
    }

    function defaultScenarioName() {
        for (var i = 1; i < 1000; ++i) {
            var padded = String(i)
            while (padded.length < 3)
                padded = "0" + padded
            var candidate = "Scenario_" + padded
            if (backend.simScenarioNames.indexOf(candidate) < 0)
                return candidate
        }
        return ""
    }

    function confirmCreateScenario() {
        var scenarioId = createScenarioNameField.text.trim()
        if (scenarioId === "")
            return
        var createdId = backend.createInitialScenario(scenarioId, createScenarioMode.currentIndex === 1)
        if (createdId !== "")
            createScenarioDialog.close()
    }

    function openDeleteScenarioDialog() {
        if (backend.selectedScenario === "")
            return
        deleteScenarioDialog.visible = true
        deleteScenarioDialog.raise()
        deleteScenarioDialog.requestActivate()
    }
    
    function selectedPlatformFields() {
        var item = selectedPlatform()
        return [
            { "label": "平台类型", "path": "type_id", "value": nestedValue(item, "type_id", ""), "editable": true },
            { "label": "指挥关系", "path": "commander", "value": nestedValue(item, "commander", ""), "editable": true },
            { "label": "经度(deg)", "path": "initial_position.lon_deg", "value": formatNumber(nestedValue(item, "initial_position.lon_deg", 0)), "editable": true },
            { "label": "纬度(deg)", "path": "initial_position.lat_deg", "value": formatNumber(nestedValue(item, "initial_position.lat_deg", 0)), "editable": true },
            { "label": "高度(m)", "path": "initial_position.altitude_m", "value": formatNumber(nestedValue(item, "initial_position.altitude_m", 0)), "editable": true },
            { "label": "速度(m/s)", "path": "initial_position.speed_mps", "value": formatNumber(nestedValue(item, "initial_position.speed_mps", 0)), "editable": true },
            { "label": "航向角(deg)", "path": "initial_position.heading_deg", "value": formatNumber(nestedValue(item, "initial_position.heading_deg", 0)), "editable": true },
            { "label": "航迹俯仰角(deg)", "path": "initial_position.path_angle_deg", "value": formatNumber(nestedValue(item, "initial_position.path_angle_deg", 0)), "editable": true }
        ]
    }

    function selectedPlatformDetailGroups() {
        var item = selectedPlatform()
        return [
            [
                { "label": "平台ID", "value": nestedValue(item, "platform_id", ""), "readOnly": true },
                { "label": "平台类型", "path": "type_id", "value": nestedValue(item, "type_id", ""), "editor": "combo", "editable": true, "span": 2 }
            ],
            [
                { "label": "阵营", "path": "side", "value": normalizedSideValue(nestedValue(item, "side", "")), "editor": "combo", "editable": true },
                { "label": "指挥关系", "path": "commander", "value": nestedValue(item, "commander", "SELF"), "editor": "combo", "editable": true, "span": 2 }
            ],
            [
                { "label": "经度(deg)", "path": "initial_position.lon_deg", "value": formatNumber(nestedValue(item, "initial_position.lon_deg", 0)), "editable": true },
                { "label": "纬度(deg)", "path": "initial_position.lat_deg", "value": formatNumber(nestedValue(item, "initial_position.lat_deg", 0)), "editable": true },
                { "label": "高度(m)", "path": "initial_position.altitude_m", "value": formatNumber(nestedValue(item, "initial_position.altitude_m", 0)), "editable": true }
            ],
            [
                { "label": "速度(m/s)", "path": "initial_position.speed_mps", "value": formatNumber(nestedValue(item, "initial_position.speed_mps", 0)), "editable": true },
                { "label": "航向角(deg)", "path": "initial_position.heading_deg", "value": formatNumber(nestedValue(item, "initial_position.heading_deg", 0)), "editable": true },
                { "label": "航迹俯仰角(deg)", "path": "initial_position.path_angle_deg", "value": formatNumber(nestedValue(item, "initial_position.path_angle_deg", 0)), "editable": true }
            ]
        ]
    }
    
    function updateSelectedPlatformField(fieldPath, value) {
        var platformId = String(initialEditor.graphValue(selectedPlatform(), "platform_id", ""))
        if (platformId !== "")
            backend.updateInitialScenarioPlatformField(platformId, fieldPath, value)
    }
    
    function mountRows(kind) {
        return asArray(initialEditor.graphValue(selectedPlatform(), kind === "sensor" ? "mounted_sensors" : "mounted_weapons", []))
    }

    function weaponMountTypeOptions(currentType) {
        var options = []
        var rows = backend.initialWeaponTypeRows || []
        for (var i = 0; i < rows.length; ++i) {
            var typeId = rowTypeId(rows[i])
            if (typeId !== "" && options.indexOf(typeId) < 0)
                options.push(typeId)
        }
        currentType = String(currentType || "")
        if (currentType !== "" && options.indexOf(currentType) < 0)
            options.unshift(currentType)
        return options
    }

    function sensorMountTypeOptions(currentType) {
        var options = []
        var rows = backend.initialSensorTypeRows || []
        for (var i = 0; i < rows.length; ++i) {
            var typeId = rowTypeId(rows[i])
            if (typeId !== "" && options.indexOf(typeId) < 0)
                options.push(typeId)
        }
        currentType = String(currentType || "")
        if (currentType !== "" && options.indexOf(currentType) < 0)
            options.unshift(currentType)
        return options
    }

    function weaponMountTypeIndex(options, currentType) {
        var index = options.indexOf(String(currentType || ""))
        return index >= 0 ? index : 0
    }

    function updateWeaponMount(oldTypeId, newTypeId, quantityText) {
        var platformId = backend.initialSelectedScenarioPlatformId
        if (platformId === "")
            return
        var quantity = parseInt(String(quantityText).trim())
        if (!isFinite(quantity) || quantity < 0)
            quantity = 0
        backend.updateInitialWeaponMount(platformId, oldTypeId, newTypeId, quantity)
    }

    function updateSensorMount(oldTypeId, newTypeId, quantityText) {
        var platformId = backend.initialSelectedScenarioPlatformId
        if (platformId === "")
            return
        var quantity = parseInt(String(quantityText).trim())
        if (!isFinite(quantity) || quantity < 0)
            quantity = 0
        backend.updateInitialSensorMount(platformId, oldTypeId, newTypeId, quantity)
    }
    
    function routeIsCirculate() {
        return Boolean(initialEditor.graphValue(selectedPlatform(), "route_is_circulate", false))
    }

    function routeRows() {
        return asArray(initialEditor.graphValue(selectedPlatform(), "route_points", []))
    }

    function normalizedSelectedWaypointIndex() {
        var rows = routeRows()
        if (rows.length <= 0)
            return -1
        return Math.max(0, Math.min(selectedWaypointIndex, rows.length - 1))
    }

    function canEditWaypoint(index) {
        return index > 0
    }
    
    RowLayout {
        anchors.fill: parent
        spacing: 8
    
        Rectangle {
            Layout.preferredWidth: 500
            Layout.minimumWidth: 500
            Layout.maximumWidth: 500
            Layout.fillHeight: true
            radius: 6
            color: "#f8fbff"
            border.color: initialEditor.uiBorderColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 520
                        Layout.minimumHeight: 520
                        Layout.maximumHeight: 520
                        radius: 5
                        color: "#ffffff"
                        border.color: initialEditor.uiSoftBorderColor
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                InitialToolbarSegmentButton {
                                    Layout.preferredWidth: 92
                                    Layout.minimumWidth: 92
                                    Layout.maximumWidth: 92
                                    text: "平台类型"
                                    visualChecked: initialEditor.catalogMode === "platform"
                                    leftSegment: true
                                    onClicked: {
                                        initialEditor.catalogMode = "platform"
                                        initialEditor.ensureCatalogSelection()
                                    }
                                }
                                InitialToolbarSegmentButton {
                                    Layout.preferredWidth: 92
                                    Layout.minimumWidth: 92
                                    Layout.maximumWidth: 92
                                    text: "武器类型"
                                    visualChecked: initialEditor.catalogMode === "weapon"
                                    onClicked: {
                                        initialEditor.catalogMode = "weapon"
                                        initialEditor.ensureCatalogSelection()
                                    }
                                }
                                InitialToolbarSegmentButton {
                                    Layout.preferredWidth: 92
                                    Layout.minimumWidth: 92
                                    Layout.maximumWidth: 92
                                    text: "传感器类型"
                                    visualChecked: initialEditor.catalogMode === "sensor"
                                    rightSegment: true
                                    onClicked: {
                                        initialEditor.catalogMode = "sensor"
                                        initialEditor.ensureCatalogSelection()
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 10
                                    text: initialEditor.catalogMode === "platform" ? "拖入态势区" : "拖至平台挂载"
                                    color: "#64748b"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiSmallFontSize
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                            }

                            StackLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                currentIndex: initialEditor.catalogMode === "sensor" ? 2 : (initialEditor.catalogMode === "weapon" ? 1 : 0)

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "transparent"

                                    GridView {
                                        id: platformCatalogGrid
                                        anchors.fill: parent
                                        clip: true
                                        cellWidth: Math.max(1, width / 2)
                                        cellHeight: 96
                                        model: backend.initialPlatformTypeRows
                                        boundsBehavior: Flickable.StopAtBounds
                                        ScrollBar.vertical: ScrollBar { }

                                        delegate: Item {
                                            id: platformCatalogCell
                                            required property var modelData
                                            required property int index
                                            width: platformCatalogGrid.cellWidth
                                            height: platformCatalogGrid.cellHeight

                                            PlatformCatalogRow {
                                                anchors.fill: parent
                                                anchors.margins: 4
                                                rowData: platformCatalogCell.modelData
                                                rowIndex: platformCatalogCell.index
                                                catalogKind: "platform"
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "transparent"

                                    GridView {
                                        id: weaponCatalogGrid
                                        anchors.fill: parent
                                        clip: true
                                        cellWidth: Math.max(1, width / 2)
                                        cellHeight: 96
                                        model: backend.initialWeaponTypeRows
                                        boundsBehavior: Flickable.StopAtBounds
                                        ScrollBar.vertical: ScrollBar { }

                                        delegate: Item {
                                            id: weaponCatalogCell
                                            required property var modelData
                                            required property int index
                                            width: weaponCatalogGrid.cellWidth
                                            height: weaponCatalogGrid.cellHeight

                                            WeaponCatalogRow {
                                                anchors.fill: parent
                                                anchors.margins: 4
                                                rowData: weaponCatalogCell.modelData
                                                rowIndex: weaponCatalogCell.index
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "transparent"

                                    GridView {
                                        id: sensorCatalogGrid
                                        anchors.fill: parent
                                        clip: true
                                        cellWidth: Math.max(1, width / 2)
                                        cellHeight: 96
                                        model: backend.initialSensorTypeRows
                                        boundsBehavior: Flickable.StopAtBounds
                                        ScrollBar.vertical: ScrollBar { }

                                        delegate: Item {
                                            id: sensorCatalogCell
                                            required property var modelData
                                            required property int index
                                            width: sensorCatalogGrid.cellWidth
                                            height: sensorCatalogGrid.cellHeight

                                            SensorCatalogRow {
                                                anchors.fill: parent
                                                anchors.margins: 4
                                                rowData: sensorCatalogCell.modelData
                                                rowIndex: sensorCatalogCell.index
                                            }
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                InitialUiButton {
                                    Layout.fillWidth: true
                                    text: "新建"
                                    onClicked: {
                                        if (initialEditor.catalogMode === "weapon") {
                                            var weaponId = backend.addInitialWeaponType()
                                            if (weaponId !== "")
                                                initialEditor.selectedCatalogId = weaponId
                                        } else if (initialEditor.catalogMode === "sensor") {
                                            var sensorId = backend.addInitialSensorType()
                                            if (sensorId !== "")
                                                initialEditor.selectedCatalogId = sensorId
                                        } else {
                                            var platformId = backend.addInitialPlatformType()
                                            if (platformId !== "")
                                                initialEditor.selectedCatalogId = platformId
                                        }
                                    }
                                }
                                InitialUiButton {
                                    Layout.fillWidth: true
                                    text: "删除"
                                    enabled: initialEditor.selectedCatalogId !== ""
                                    onClicked: {
                                        if (initialEditor.catalogMode === "weapon") {
                                            if (backend.deleteInitialWeaponType(initialEditor.selectedCatalogId)) {
                                                initialEditor.selectedCatalogId = ""
                                                initialEditor.ensureCatalogSelection()
                                            }
                                        } else if (initialEditor.catalogMode === "sensor") {
                                            if (backend.deleteInitialSensorType(initialEditor.selectedCatalogId)) {
                                                initialEditor.selectedCatalogId = ""
                                                initialEditor.ensureCatalogSelection()
                                            }
                                        } else if (backend.deleteInitialPlatformType(initialEditor.selectedCatalogId)) {
                                            initialEditor.selectedCatalogId = ""
                                            initialEditor.ensureCatalogSelection()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 360
                        radius: 5
                        color: "#ffffff"
                        border.color: initialEditor.uiSoftBorderColor
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    Layout.fillWidth: true
                                    text: initialEditor.catalogTitle() + "编辑"
                                    color: "#0f172a"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiTitleFontSize
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.preferredWidth: 112
                                    Layout.minimumWidth: 112
                                    Layout.maximumWidth: 112
                                    text: initialEditor.catalogMode === "weapon" ? backend.initialWeaponTypeSaveStatus
                                         : (initialEditor.catalogMode === "sensor" ? backend.initialSensorTypeSaveStatus : backend.initialPlatformTypeSaveStatus)
                                    color: text.indexOf("失败") >= 0 ? "#d64545" : "#256f3f"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiSmallFontSize
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                                InitialUiButton {
                                    Layout.preferredWidth: 82
                                    Layout.minimumWidth: 82
                                    Layout.maximumWidth: 82
                                    text: "保存"
                                    enabled: initialEditor.selectedCatalogId !== ""
                                    onClicked: {
                                        if (initialEditor.catalogMode === "weapon")
                                            backend.saveInitialWeaponType(initialEditor.selectedCatalogId)
                                        else if (initialEditor.catalogMode === "sensor")
                                            backend.saveInitialSensorType(initialEditor.selectedCatalogId)
                                        else
                                            backend.saveInitialPlatformType(initialEditor.selectedCatalogId)
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "transparent"
                                clip: true

                                Flickable {
                                    anchors.fill: parent
                                    contentWidth: width
                                    contentHeight: catalogDetailColumn.implicitHeight
                                    clip: true

                                    ColumnLayout {
                                        id: catalogDetailColumn
                                        width: parent.width
                                        spacing: initialEditor.catalogMode === "sensor" ? 4 : 8

                                        Repeater {
                                            model: initialEditor.catalogFields()
                                            delegate: Loader {
                                                required property var modelData
                                                Layout.fillWidth: true
                                                sourceComponent: Boolean(initialEditor.graphValue(modelData, "section", false))
                                                                 ? sectionFieldComponent
                                                                 : (Boolean(initialEditor.graphValue(modelData, "group", false))
                                                                    ? catalogEditGroupComponent
                                                                    : catalogEditFieldComponent)
                                                onLoaded: item.modelData = modelData
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        Rectangle {
            id: plotPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 700
            radius: 6
            color: "#eef5fb"
            border.color: initialEditor.uiBorderColor
            border.width: 1
    
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
    
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Label {
                        Layout.fillWidth: true
                        text: "初始态势显示区"
                        color: "#0f172a"
                        font.family: initialEditor.uiFontFamily
                        font.pixelSize: initialEditor.uiTitleFontSize
                        font.bold: true
                    }
                    InitialUiCheckBox { text: "ID"; checked: initialEditor.showPlatformId; onClicked: initialEditor.showPlatformId = checked }
                    InitialUiCheckBox { text: "指挥关系"; checked: initialEditor.showCommandLink; onClicked: initialEditor.showCommandLink = checked }
                    InitialUiCheckBox { text: "传感器范围"; checked: initialEditor.showSensorRange; onClicked: initialEditor.showSensorRange = checked }
                    InitialUiCheckBox { text: "武器范围"; checked: initialEditor.showWeaponRange; onClicked: initialEditor.showWeaponRange = checked }
                    InitialUiCheckBox { text: "预设航路"; checked: initialEditor.showRoute; onClicked: initialEditor.showRoute = checked }
                }
    
                Rectangle {
                    id: plotArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 5
                    color: "#f8fafc"
                    border.color: "#b8c7d6"
                    border.width: 1
                    clip: true
    
                    UiOsgInitMap {
                        id: situationMap
                        anchors.fill: parent
                        platformRows: initialEditor.initialRowsCache
                        selectedPlatformId: backend.initialSelectedScenarioPlatformId
                        showPlatformId: initialEditor.showPlatformId
                        showCommandLink: initialEditor.showCommandLink
                        showSensorRange: initialEditor.showSensorRange
                        showWeaponRange: initialEditor.showWeaponRange
                        showRoute: initialEditor.showRoute
                        labelFontFamily: initialEditor.uiFontFamily
                        labelPixelSize: initialEditor.uiSmallFontSize
                        onPlatformClicked: function(platformId) {
                            if (platformId !== "")
                                backend.selectInitialScenarioPlatform(platformId)
                        }
                        Loader {
                            anchors.fill: parent
                            active: initialEditor.initialPageVisible && initialEditor.mapContainerReady
                            sourceComponent: Component {
                                Quick67.WindowContainer {
                                    anchors.fill: parent
                                    window: situationMap.mapWindow
                                }
                            }
                        }
                    }
                }
    
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        Layout.fillWidth: true
                        text: backend.initialSituationStatus
                        color: "#475569"
                        font.family: initialEditor.uiFontFamily
                        font.pixelSize: initialEditor.uiFontSize
                        elide: Text.ElideRight
                    }
                    InitialUiButton {
                        Layout.preferredWidth: 86
                        Layout.minimumWidth: 86
                        Layout.maximumWidth: 86
                        text: "新建"
                        onClicked: initialEditor.openCreateScenarioDialog()
                    }
                    InitialUiButton {
                        Layout.preferredWidth: 86
                        Layout.minimumWidth: 86
                        Layout.maximumWidth: 86
                        text: "删除"
                        enabled: backend.selectedScenario !== ""
                        onClicked: initialEditor.openDeleteScenarioDialog()
                    }
                    InitialUiButton {
                        Layout.preferredWidth: 86
                        Layout.minimumWidth: 86
                        Layout.maximumWidth: 86
                        text: "重置"
                        enabled: backend.selectedScenario !== ""
                        onClicked: backend.resetInitialScenarioConfig()
                    }
                    Label {
                        Layout.preferredWidth: 112
                        Layout.minimumWidth: 112
                        Layout.maximumWidth: 112
                        text: backend.initialScenarioSaveStatus
                        color: text.indexOf("失败") >= 0 ? "#d64545" : "#256f3f"
                        font.family: initialEditor.uiFontFamily
                        font.pixelSize: initialEditor.uiSmallFontSize
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    InitialUiButton {
                        Layout.preferredWidth: 86
                        Layout.minimumWidth: 86
                        Layout.maximumWidth: 86
                        text: "保存"
                        enabled: backend.selectedScenario !== ""
                        onClicked: backend.saveInitialScenarioConfig()
                    }
                }
            }
        }
    
        Rectangle {
            Layout.preferredWidth: 390
            Layout.minimumWidth: 390
            Layout.maximumWidth: 390
            Layout.fillHeight: true
            radius: 6
            color: "#f8fbff"
            border.color: initialEditor.uiBorderColor
            border.width: 1
    
            Flickable {
                anchors.fill: parent
                anchors.margins: 10
                contentWidth: width
                contentHeight: selectedDetailColumn.implicitHeight
                clip: true
    
                ColumnLayout {
                    id: selectedDetailColumn
                    width: parent.width
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label {
                            Layout.fillWidth: true
                            text: "仿真控制"
                            color: "#0f172a"
                            font.family: initialEditor.uiFontFamily
                            font.pixelSize: initialEditor.uiTitleFontSize
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.preferredWidth: 104
                            Layout.minimumWidth: 104
                            Layout.maximumWidth: 104
                            text: backend.initialSimCtrlSaveStatus
                            color: text.indexOf("失败") >= 0 || text.indexOf("必须") >= 0 ? "#d64545" : "#256f3f"
                            font.family: initialEditor.uiFontFamily
                            font.pixelSize: initialEditor.uiSmallFontSize
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        InitialUiButton {
                            Layout.preferredWidth: 72
                            Layout.minimumWidth: 72
                            Layout.maximumWidth: 72
                            text: "保存"
                            enabled: !backend.workflowRunning
                            onClicked: backend.saveInitialSimCtrlConfig()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 5
                        color: "#ffffff"
                        border.color: initialEditor.uiSoftBorderColor
                        border.width: 1
                        implicitHeight: simCtrlFieldsColumn.implicitHeight + 16

                        ColumnLayout {
                            id: simCtrlFieldsColumn
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.margins: 8
                            width: initialEditor.detailGridWidth
                            spacing: 5

                            RowLayout {
                                Layout.preferredWidth: initialEditor.detailGridWidth
                                Layout.minimumWidth: initialEditor.detailGridWidth
                                Layout.maximumWidth: initialEditor.detailGridWidth
                                spacing: initialEditor.detailFieldSpacing

                                ColumnLayout {
                                    Layout.preferredWidth: initialEditor.detailSingleFieldWidth
                                    Layout.minimumWidth: initialEditor.detailSingleFieldWidth
                                    Layout.maximumWidth: initialEditor.detailSingleFieldWidth
                                    spacing: 3
                                    Label {
                                        Layout.fillWidth: true
                                        text: "仿真步长(ms)"
                                        color: "#475569"
                                        font.family: initialEditor.uiFontFamily
                                        font.pixelSize: initialEditor.uiSmallFontSize
                                        elide: Text.ElideRight
                                    }
                                    InitialUiTextField {
                                        Layout.fillWidth: true
                                        text: backend.initialSimIterMs
                                        enabled: !backend.workflowRunning
                                        validator: IntValidator { bottom: 1 }
                                        onEditingFinished: backend.updateInitialSimCtrlField("sim_iter_ms", text)
                                    }
                                }

                                ColumnLayout {
                                    Layout.preferredWidth: initialEditor.detailSingleFieldWidth
                                    Layout.minimumWidth: initialEditor.detailSingleFieldWidth
                                    Layout.maximumWidth: initialEditor.detailSingleFieldWidth
                                    spacing: 3
                                    Label {
                                        Layout.fillWidth: true
                                        text: "时间倍速"
                                        color: "#475569"
                                        font.family: initialEditor.uiFontFamily
                                        font.pixelSize: initialEditor.uiSmallFontSize
                                        elide: Text.ElideRight
                                    }
                                    InitialUiTextField {
                                        Layout.fillWidth: true
                                        text: backend.initialTimeAccRatio
                                        enabled: !backend.workflowRunning
                                        validator: DoubleValidator { bottom: 0.000001; notation: DoubleValidator.StandardNotation }
                                        onEditingFinished: backend.updateInitialSimCtrlField("time_acc_ratio", text)
                                    }
                                }
                            }
                        }
                    }
    
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label {
                            Layout.fillWidth: true
                            text: "平台详情"
                            color: "#0f172a"
                            font.family: initialEditor.uiFontFamily
                            font.pixelSize: initialEditor.uiTitleFontSize
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.preferredWidth: 104
                            Layout.minimumWidth: 104
                            Layout.maximumWidth: 104
                            text: backend.initialPlatformDetailSaveStatus
                            color: text.indexOf("失败") >= 0 ? "#d64545" : "#256f3f"
                            font.family: initialEditor.uiFontFamily
                            font.pixelSize: initialEditor.uiSmallFontSize
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        InitialUiButton {
                            Layout.preferredWidth: 72
                            Layout.minimumWidth: 72
                            Layout.maximumWidth: 72
                            text: "保存"
                            enabled: backend.initialSelectedScenarioPlatformId !== ""
                            onClicked: backend.saveInitialSelectedPlatformConfig()
                        }
                    }
    
                    Rectangle {
                        Layout.fillWidth: true
                        radius: 5
                        color: "#ffffff"
                        border.color: initialEditor.uiSoftBorderColor
                        border.width: 1
                        implicitHeight: detailFieldsColumn.implicitHeight + 16
    
                        ColumnLayout {
                            id: detailFieldsColumn
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.margins: 8
                            width: initialEditor.detailGridWidth
                            spacing: 5
    
                            Repeater {
                                model: initialEditor.selectedPlatformDetailGroups()
                                delegate: RowLayout {
                                    id: detailFieldRow
                                    required property var modelData
                                    Layout.preferredWidth: initialEditor.detailGridWidth
                                    Layout.minimumWidth: initialEditor.detailGridWidth
                                    Layout.maximumWidth: initialEditor.detailGridWidth
                                    spacing: initialEditor.detailFieldSpacing

                                    Repeater {
                                        model: initialEditor.asArray(detailFieldRow.modelData)
                                        delegate: ColumnLayout {
                                            id: detailFieldColumn
                                            required property var modelData
                                            readonly property int fixedFieldWidth: initialEditor.detailFieldWidth(detailFieldColumn.modelData)
                                            Layout.preferredWidth: fixedFieldWidth
                                            Layout.minimumWidth: fixedFieldWidth
                                            Layout.maximumWidth: fixedFieldWidth
                                            spacing: 2

                                            Label {
                                                Layout.preferredWidth: detailFieldColumn.fixedFieldWidth
                                                Layout.minimumWidth: detailFieldColumn.fixedFieldWidth
                                                Layout.maximumWidth: detailFieldColumn.fixedFieldWidth
                                                text: String(initialEditor.graphValue(detailFieldColumn.modelData, "label", ""))
                                                color: "#475569"
                                                font.family: initialEditor.uiFontFamily
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                elide: Text.ElideRight
                                            }
                                            InitialUiComboBox {
                                                visible: String(initialEditor.graphValue(detailFieldColumn.modelData, "editor", "")) === "combo"
                                                Layout.preferredWidth: detailFieldColumn.fixedFieldWidth
                                                Layout.minimumWidth: detailFieldColumn.fixedFieldWidth
                                                Layout.maximumWidth: detailFieldColumn.fixedFieldWidth
                                                Layout.preferredHeight: 28
                                                Layout.minimumHeight: 28
                                                Layout.maximumHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                model: visible ? initialEditor.detailComboOptions(detailFieldColumn.modelData) : []
                                                currentIndex: visible ? initialEditor.detailComboIndex(detailFieldColumn.modelData) : -1
                                                enabled: backend.initialSelectedScenarioPlatformId !== "" &&
                                                         model.length > 0 &&
                                                         Boolean(initialEditor.graphValue(detailFieldColumn.modelData, "editable", true))
                                                onActivated: function(index) {
                                                    if (index >= 0)
                                                        initialEditor.updateSelectedPlatformField(String(initialEditor.graphValue(detailFieldColumn.modelData, "path", "")), String(model[index]))
                                                }
                                            }
                                            InitialUiTextField {
                                                visible: String(initialEditor.graphValue(detailFieldColumn.modelData, "editor", "")) !== "combo"
                                                Layout.preferredWidth: detailFieldColumn.fixedFieldWidth
                                                Layout.minimumWidth: detailFieldColumn.fixedFieldWidth
                                                Layout.maximumWidth: detailFieldColumn.fixedFieldWidth
                                                Layout.preferredHeight: 28
                                                Layout.minimumHeight: 28
                                                Layout.maximumHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                text: String(initialEditor.graphValue(detailFieldColumn.modelData, "value", ""))
                                                readOnly: Boolean(initialEditor.graphValue(detailFieldColumn.modelData, "readOnly", false))
                                                onEditingFinished: {
                                                    if (!readOnly)
                                                        initialEditor.updateSelectedPlatformField(String(initialEditor.graphValue(detailFieldColumn.modelData, "path", "")), text)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
    
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label {
                            Layout.fillWidth: true
                            text: "传感器挂载"
                            color: "#0f172a"
                            font.family: initialEditor.uiFontFamily
                            font.pixelSize: initialEditor.uiTitleFontSize
                            font.bold: true
                        }
                        InitialUiButton {
                            Layout.preferredWidth: 58
                            Layout.minimumWidth: 58
                            Layout.maximumWidth: 58
                            Layout.preferredHeight: 28
                            text: "新增"
                            enabled: backend.initialSelectedScenarioPlatformId !== ""
                            onClicked: backend.addInitialSensorMount(backend.initialSelectedScenarioPlatformId)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 5
                        color: "#ffffff"
                        border.color: initialEditor.uiSoftBorderColor
                        border.width: 1
                        implicitHeight: sensorLoadoutColumn.implicitHeight + 12

                        ColumnLayout {
                            id: sensorLoadoutColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 6
                            spacing: 5

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 3
                                columnSpacing: 6
                                rowSpacing: 4

                                Label {
                                    Layout.fillWidth: true
                                    text: "类型"
                                    color: "#64748b"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiSmallFontSize
                                    font.bold: true
                                }
                                Label {
                                    Layout.preferredWidth: 56
                                    text: "数量"
                                    color: "#64748b"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiSmallFontSize
                                    font.bold: true
                                    horizontalAlignment: Text.AlignLeft
                                }
                                Label {
                                    Layout.preferredWidth: 58
                                    text: ""
                                }

                                Label {
                                    Layout.columnSpan: 3
                                    Layout.fillWidth: true
                                    visible: initialEditor.mountRows("sensor").length === 0
                                    text: "暂无传感器"
                                    color: "#94a3b8"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiSmallFontSize
                                }

                                Repeater {
                                    model: initialEditor.mountRows("sensor")
                                    delegate: Item {
                                        id: sensorMountRow
                                        required property var modelData
                                        required property int index
                                        readonly property string currentTypeId: String(initialEditor.graphValue(modelData, "type_id", ""))
                                        readonly property var typeOptions: initialEditor.sensorMountTypeOptions(currentTypeId)
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                        implicitHeight: 30

                                        GridLayout {
                                            anchors.fill: parent
                                            columns: 3
                                            columnSpacing: 6
                                            rowSpacing: 0

                                            InitialUiComboBox {
                                                id: sensorMountTypeCombo
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                model: sensorMountRow.typeOptions
                                                currentIndex: initialEditor.weaponMountTypeIndex(sensorMountRow.typeOptions, sensorMountRow.currentTypeId)
                                                enabled: backend.initialSelectedScenarioPlatformId !== "" && model.length > 0
                                                onActivated: function(index) {
                                                    if (index >= 0)
                                                        initialEditor.updateSensorMount(
                                                                    sensorMountRow.currentTypeId,
                                                                    String(model[index]),
                                                                    sensorMountQuantityField.text)
                                                }
                                            }
                                            InitialUiTextField {
                                                id: sensorMountQuantityField
                                                Layout.preferredWidth: 56
                                                Layout.minimumWidth: 56
                                                Layout.maximumWidth: 56
                                                Layout.preferredHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                text: String(initialEditor.graphValue(modelData, "quantity", 0))
                                                onEditingFinished: initialEditor.updateSensorMount(
                                                                       sensorMountRow.currentTypeId,
                                                                       sensorMountTypeCombo.displayText,
                                                                       text)
                                            }
                                            InitialUiButton {
                                                Layout.preferredWidth: 58
                                                Layout.minimumWidth: 58
                                                Layout.maximumWidth: 58
                                                Layout.preferredHeight: 28
                                                text: "删除"
                                                enabled: backend.initialSelectedScenarioPlatformId !== ""
                                                onClicked: backend.deleteInitialSensorMount(
                                                               backend.initialSelectedScenarioPlatformId,
                                                               sensorMountRow.currentTypeId)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Label {
                            Layout.fillWidth: true
                            text: "武器挂载"
                            color: "#0f172a"
                            font.family: initialEditor.uiFontFamily
                            font.pixelSize: initialEditor.uiTitleFontSize
                            font.bold: true
                        }
                        InitialUiButton {
                            Layout.preferredWidth: 58
                            Layout.minimumWidth: 58
                            Layout.maximumWidth: 58
                            Layout.preferredHeight: 28
                            text: "新增"
                            enabled: backend.initialSelectedScenarioPlatformId !== ""
                            onClicked: backend.addInitialWeaponMount(backend.initialSelectedScenarioPlatformId)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 5
                        color: "#ffffff"
                        border.color: initialEditor.uiSoftBorderColor
                        border.width: 1
                        implicitHeight: weaponLoadoutColumn.implicitHeight + 12

                        ColumnLayout {
                            id: weaponLoadoutColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 6
                            spacing: 5

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 3
                                columnSpacing: 6
                                rowSpacing: 4

                                Label {
                                    Layout.fillWidth: true
                                    text: "类型"
                                    color: "#64748b"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiSmallFontSize
                                    font.bold: true
                                }
                                Label {
                                    Layout.preferredWidth: 56
                                    text: "数量"
                                    color: "#64748b"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiSmallFontSize
                                    font.bold: true
                                    horizontalAlignment: Text.AlignLeft
                                }
                                Label {
                                    Layout.preferredWidth: 58
                                    text: ""
                                }

                                Repeater {
                                    model: initialEditor.mountRows("weapon")
                                    delegate: Item {
                                        id: weaponMountRow
                                        required property var modelData
                                        required property int index
                                        readonly property string currentTypeId: String(initialEditor.graphValue(modelData, "type_id", ""))
                                        readonly property var typeOptions: initialEditor.weaponMountTypeOptions(currentTypeId)
                                        Layout.columnSpan: 3
                                        Layout.fillWidth: true
                                        implicitHeight: 30

                                        GridLayout {
                                            anchors.fill: parent
                                            columns: 3
                                            columnSpacing: 6
                                            rowSpacing: 0

                                            InitialUiComboBox {
                                                id: weaponMountTypeCombo
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                model: weaponMountRow.typeOptions
                                                currentIndex: initialEditor.weaponMountTypeIndex(weaponMountRow.typeOptions, weaponMountRow.currentTypeId)
                                                enabled: backend.initialSelectedScenarioPlatformId !== "" && model.length > 0
                                                onActivated: function(index) {
                                                    if (index >= 0)
                                                        initialEditor.updateWeaponMount(
                                                                    weaponMountRow.currentTypeId,
                                                                    String(model[index]),
                                                                    weaponMountQuantityField.text)
                                                }
                                            }
                                            InitialUiTextField {
                                                id: weaponMountQuantityField
                                                Layout.preferredWidth: 56
                                                Layout.minimumWidth: 56
                                                Layout.maximumWidth: 56
                                                Layout.preferredHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                text: String(initialEditor.graphValue(modelData, "quantity", 0))
                                                onEditingFinished: initialEditor.updateWeaponMount(
                                                                       weaponMountRow.currentTypeId,
                                                                       weaponMountTypeCombo.displayText,
                                                                       text)
                                            }
                                            InitialUiButton {
                                                Layout.preferredWidth: 58
                                                Layout.minimumWidth: 58
                                                Layout.maximumWidth: 58
                                                Layout.preferredHeight: 28
                                                text: "删除"
                                                enabled: backend.initialSelectedScenarioPlatformId !== ""
                                                onClicked: backend.deleteInitialWeaponMount(
                                                               backend.initialSelectedScenarioPlatformId,
                                                               weaponMountRow.currentTypeId)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
    
							Label {
								Layout.fillWidth: true
								text: "预设航点"
								color: "#0f172a"
								font.family: initialEditor.uiFontFamily
								font.pixelSize: initialEditor.uiTitleFontSize
                        font.bold: true
                    }
    
                    Rectangle {
                        Layout.fillWidth: true
                        radius: 5
                        color: "#ffffff"
                        border.color: initialEditor.uiSoftBorderColor
                        border.width: 1
                        implicitHeight: waypointColumn.implicitHeight + 12

                        ColumnLayout {
                            id: waypointColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 6
                            spacing: 5

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Label {
                                    Layout.fillWidth: true
                                    text: "当前 " + (initialEditor.normalizedSelectedWaypointIndex() + 1)
                                    color: "#64748b"
                                    font.family: initialEditor.uiFontFamily
                                    font.pixelSize: initialEditor.uiSmallFontSize
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                                InitialUiCheckBox {
                                    Layout.preferredWidth: 92
                                    Layout.minimumWidth: 92
                                    Layout.maximumWidth: 92
                                    text: "循环航线"
                                    checked: initialEditor.routeIsCirculate()
                                    enabled: backend.initialSelectedScenarioPlatformId !== ""
                                    onClicked: backend.setInitialRouteCirculate(
                                                   backend.initialSelectedScenarioPlatformId,
                                                   checked)
                                }
                                InitialUiButton {
                                    Layout.preferredWidth: 58
                                    Layout.preferredHeight: 28
                                    text: "删除"
                                    enabled: backend.initialSelectedScenarioPlatformId !== "" &&
                                             initialEditor.canEditWaypoint(initialEditor.normalizedSelectedWaypointIndex())
                                    onClicked: backend.deleteInitialRoutePoint(
                                                   backend.initialSelectedScenarioPlatformId,
                                                   initialEditor.normalizedSelectedWaypointIndex())
                                }
                                InitialUiButton {
                                    Layout.preferredWidth: 58
                                    Layout.preferredHeight: 28
                                    text: "增加"
                                    enabled: backend.initialSelectedScenarioPlatformId !== ""
                                    onClicked: {
                                        var nextIndex = initialEditor.routeRows().length
                                        if (backend.addInitialRoutePoint(backend.initialSelectedScenarioPlatformId))
                                            initialEditor.selectedWaypointIndex = nextIndex
                                    }
                                }
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 6
                                rowSpacing: 4
                                columnSpacing: 5

										Repeater {
											model: [
												{ "label": "ID", "width": 30, "align": Text.AlignHCenter },
												{ "label": "经度", "width": 70, "align": Text.AlignLeft },
												{ "label": "纬度", "width": 70, "align": Text.AlignLeft },
												{ "label": "高度", "width": 52, "align": Text.AlignLeft },
												{ "label": "速度", "width": 52, "align": Text.AlignLeft },
												{ "label": "", "width": 1, "align": Text.AlignHCenter }
											]
                                    delegate: Label {
                                        required property var modelData
                                        required property int index
                                        Layout.preferredWidth: Number(initialEditor.graphValue(modelData, "width", 60))
                                        Layout.fillWidth: index > 0 && index < 5
                                        text: String(initialEditor.graphValue(modelData, "label", ""))
                                        color: "#64748b"
                                        font.family: initialEditor.uiFontFamily
                                        font.pixelSize: initialEditor.uiSmallFontSize
                                        font.bold: true
                                        horizontalAlignment: initialEditor.graphValue(modelData, "align", Text.AlignLeft)
                                        elide: Text.ElideRight
                                    }
                                }

                                Repeater {
                                    model: initialEditor.routeRows()
                                    delegate: Item {
                                        id: waypointRow
                                        required property var modelData
                                        required property int index
                                        Layout.columnSpan: 6
                                        Layout.fillWidth: true
                                        implicitHeight: 30
                                        readonly property bool initialPoint: index === 0
                                        Rectangle {
                                            anchors.fill: parent
                                            radius: 4
                                            color: waypointRow.initialPoint ? "#f1f5f9"
                                                   : initialEditor.normalizedSelectedWaypointIndex() === index ? "#e8f2ff" : "transparent"
                                            border.color: initialEditor.normalizedSelectedWaypointIndex() === index ? "#93c5fd" : "transparent"
                                            border.width: initialEditor.normalizedSelectedWaypointIndex() === index ? 1 : 0
                                        }

                                        GridLayout {
                                            anchors.fill: parent
                                            columns: 6
                                            rowSpacing: 0
                                            columnSpacing: 5

													Button {
                                                id: waypointIndexButton
														Layout.preferredWidth: 30
                                                Layout.preferredHeight: 28
                                                text: String(index + 1)
                                                font.family: initialEditor.uiFontFamily
                                                font.pixelSize: initialEditor.uiFontSize
                                                padding: 0
                                                onClicked: initialEditor.selectedWaypointIndex = index
                                                contentItem: Text {
                                                    text: waypointIndexButton.text
                                                    color: waypointRow.initialPoint ? "#64748b"
                                                           : initialEditor.normalizedSelectedWaypointIndex() === index ? "#0b67d9" : "#334155"
                                                    font: waypointIndexButton.font
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }
                                                background: Rectangle {
                                                    radius: 4
                                                    color: waypointIndexButton.down ? "#bfdbfe"
                                                           : waypointIndexButton.hovered ? "#dbeafe" : "transparent"
                                                }
                                            }
													InitialUiTextField {
														Layout.preferredWidth: 70
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                readOnly: waypointRow.initialPoint
                                                readOnlyColor: "#f1f5f9"
                                                text: initialEditor.formatNumber(initialEditor.graphValue(modelData, "lon_deg", ""))
                                                onEditingFinished: {
                                                    if (!readOnly)
                                                        backend.updateInitialRoutePoint(backend.initialSelectedScenarioPlatformId, index, "lon_deg", text)
                                                }
                                            }
													InitialUiTextField {
														Layout.preferredWidth: 70
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                readOnly: waypointRow.initialPoint
                                                readOnlyColor: "#f1f5f9"
                                                text: initialEditor.formatNumber(initialEditor.graphValue(modelData, "lat_deg", ""))
                                                onEditingFinished: {
                                                    if (!readOnly)
                                                        backend.updateInitialRoutePoint(backend.initialSelectedScenarioPlatformId, index, "lat_deg", text)
                                                }
                                            }
													InitialUiTextField {
														Layout.preferredWidth: 52
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                readOnly: waypointRow.initialPoint
                                                readOnlyColor: "#f1f5f9"
                                                text: initialEditor.formatNumber(initialEditor.graphValue(modelData, "altitude_m", ""))
                                                onEditingFinished: {
                                                    if (!readOnly)
                                                        backend.updateInitialRoutePoint(backend.initialSelectedScenarioPlatformId, index, "altitude_m", text)
                                                }
                                            }
													InitialUiTextField {
														Layout.preferredWidth: 52
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                font.pixelSize: initialEditor.uiSmallFontSize
                                                readOnly: waypointRow.initialPoint
                                                readOnlyColor: "#f1f5f9"
                                                text: initialEditor.formatNumber(initialEditor.graphValue(modelData, "speed_mps", ""))
                                                onEditingFinished: {
                                                    if (!readOnly)
                                                        backend.updateInitialRoutePoint(backend.initialSelectedScenarioPlatformId, index, "speed_mps", text)
                                                }
                                            }
                                            Item { Layout.preferredWidth: 1; Layout.preferredHeight: 28 }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    Rectangle {
        visible: initialEditor.draggingCatalog
        x: initialEditor.dragX + 12
        y: initialEditor.dragY + 12
        z: 99
        width: Math.min(210, Math.max(120, dragLabelText.implicitWidth + 22))
        height: 34
        radius: 5
        color: "#111827"
        opacity: 0.86
        Label {
            id: dragLabelText
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            text: initialEditor.dragLabel
            color: "#ffffff"
            font.family: initialEditor.uiFontFamily
            font.pixelSize: initialEditor.uiFontSize
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    Window {
        id: createScenarioDialog
        width: 420
        height: 244
        visible: false
        title: "新建场景"
        transientParent: initialEditor.Window.window
        modality: Qt.ApplicationModal
        flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
        color: "transparent"
        x: initialEditor.Window.window ? initialEditor.Window.window.x + Math.round((initialEditor.Window.window.width - width) / 2)
                                       : Math.round((Screen.width - width) / 2)
        y: initialEditor.Window.window ? initialEditor.Window.window.y + Math.round((initialEditor.Window.window.height - height) / 2)
                                       : Math.round((Screen.height - height) / 2)

        Rectangle {
            anchors.fill: parent
            radius: 8
            color: "#ffffff"
            border.color: initialEditor.uiBorderColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    text: "新建场景"
                    color: "#0f172a"
                    font.family: initialEditor.uiFontFamily
                    font.pixelSize: initialEditor.uiTitleFontSize
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    text: "场景名称"
                    color: "#475569"
                    font.family: initialEditor.uiFontFamily
                    font.pixelSize: initialEditor.uiSmallFontSize
                }

                InitialUiTextField {
                    id: createScenarioNameField
                    Layout.fillWidth: true
                    text: ""
                    onAccepted: initialEditor.confirmCreateScenario()
                }

                InitialUiComboBox {
                    id: createScenarioMode
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    model: [ "从空场景创建", "复制当前场景" ]
                    currentIndex: 0
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    InitialUiButton {
                        Layout.fillWidth: true
                        text: "创建"
                        enabled: createScenarioNameField.text.trim() !== ""
                        onClicked: initialEditor.confirmCreateScenario()
                    }
                    InitialUiButton {
                        Layout.fillWidth: true
                        text: "取消"
                        onClicked: createScenarioDialog.close()
                    }
                }
            }
        }

        Shortcut {
            sequence: StandardKey.Cancel
            onActivated: createScenarioDialog.close()
        }
    }

    Window {
        id: deleteScenarioDialog
        width: 420
        height: 190
        visible: false
        title: "删除场景"
        transientParent: initialEditor.Window.window
        modality: Qt.ApplicationModal
        flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
        color: "transparent"
        x: initialEditor.Window.window ? initialEditor.Window.window.x + Math.round((initialEditor.Window.window.width - width) / 2)
                                       : Math.round((Screen.width - width) / 2)
        y: initialEditor.Window.window ? initialEditor.Window.window.y + Math.round((initialEditor.Window.window.height - height) / 2)
                                       : Math.round((Screen.height - height) / 2)

        Rectangle {
            anchors.fill: parent
            radius: 8
            color: "#ffffff"
            border.color: initialEditor.uiBorderColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    text: "删除场景"
                    color: "#0f172a"
                    font.family: initialEditor.uiFontFamily
                    font.pixelSize: initialEditor.uiTitleFontSize
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    text: "确认删除场景 " + backend.selectedScenario + "？"
                    color: "#475569"
                    font.family: initialEditor.uiFontFamily
                    font.pixelSize: initialEditor.uiFontSize
                    wrapMode: Text.Wrap
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    InitialUiButton {
                        Layout.fillWidth: true
                        text: "删除"
                        enabled: backend.selectedScenario !== ""
                        onClicked: {
                            if (backend.deleteInitialScenario(backend.selectedScenario))
                                deleteScenarioDialog.close()
                        }
                    }
                    InitialUiButton {
                        Layout.fillWidth: true
                        text: "取消"
                        onClicked: deleteScenarioDialog.close()
                    }
                }
            }
        }

        Shortcut {
            sequence: StandardKey.Cancel
            onActivated: deleteScenarioDialog.close()
        }
    }

    Component {
id: sectionFieldComponent
Rectangle {
    id: sectionField
    property var modelData
    property bool expanded: Boolean(initialEditor.graphValue(modelData, "expanded", true))

    implicitHeight: initialEditor.catalogMode === "sensor" ? 26 : 32
    radius: 4
    color: sectionMouse.containsMouse ? "#eef7ff" : "#f8fafc"
    border.color: initialEditor.uiSoftBorderColor
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: initialEditor.catalogMode === "sensor" ? 6 : 8
        anchors.rightMargin: initialEditor.catalogMode === "sensor" ? 6 : 8
        spacing: initialEditor.catalogMode === "sensor" ? 4 : 6

        Canvas {
            id: sectionArrowIcon
            Layout.preferredWidth: 14
            Layout.preferredHeight: 14

            Connections {
                target: sectionField
                function onExpandedChanged() { sectionArrowIcon.requestPaint() }
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#475569"
                ctx.lineWidth = 1.7
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                if (sectionField.expanded) {
                    ctx.moveTo(3, 5)
                    ctx.lineTo(width / 2, 9)
                    ctx.lineTo(width - 3, 5)
                } else {
                    ctx.moveTo(5, 3)
                    ctx.lineTo(9, height / 2)
                    ctx.lineTo(5, height - 3)
                }
                ctx.stroke()
            }
        }

        Label {
            Layout.fillWidth: true
            text: String(initialEditor.graphValue(modelData, "label", ""))
            color: "#0f172a"
            font.family: initialEditor.uiFontFamily
            font.pixelSize: initialEditor.uiFontSize
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    MouseArea {
        id: sectionMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: initialEditor.toggleCatalogSection(String(initialEditor.graphValue(sectionField.modelData, "sectionKey", "")))
    }
}
    }

    Component {
id: catalogEditFieldComponent
Item {
    id: catalogEditField
    property var modelData
    property bool compact: false
    property bool paired: false
    readonly property bool dense: initialEditor.catalogMode === "sensor" && paired
    property string unitText: String(initialEditor.graphValue(modelData, "unit", ""))
    property bool comboEditor: String(initialEditor.graphValue(modelData, "editor", "")) === "combo"
    implicitHeight: compact ? compactFieldLayout.implicitHeight : fullFieldLayout.implicitHeight

    RowLayout {
        id: fullFieldLayout
        visible: !compact
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 8

        Label {
            Layout.preferredWidth: 128
            text: String(initialEditor.graphValue(modelData, "label", ""))
            color: "#475569"
            font.family: initialEditor.uiFontFamily
            font.pixelSize: initialEditor.uiSmallFontSize
            elide: Text.ElideRight
        }

        InitialUiComboBox {
            visible: comboEditor
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.minimumHeight: 30
            Layout.maximumHeight: 30
            model: visible ? initialEditor.catalogFieldOptions(modelData) : []
            currentIndex: visible ? initialEditor.catalogComboIndex(modelData) : -1
            onActivated: function(index) {
                if (index >= 0)
                    initialEditor.updateCatalogField(String(initialEditor.graphValue(modelData, "path", "")), String(model[index]))
            }
        }

            InitialUiTextField {
                visible: !comboEditor
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                Layout.minimumHeight: 30
                Layout.maximumHeight: 30
            text: initialEditor.formatNumber(initialEditor.graphValue(modelData, "value", ""))
            readOnly: !Boolean(initialEditor.graphValue(modelData, "editable", true))
            onEditingFinished: {
                if (!readOnly)
                    initialEditor.updateCatalogField(String(initialEditor.graphValue(modelData, "path", "")), text)
            }
        }

        Label {
            visible: unitText !== ""
            Layout.preferredWidth: visible ? 38 : 0
            text: unitText
            color: "#64748b"
            font.family: initialEditor.uiFontFamily
            font.pixelSize: initialEditor.uiSmallFontSize
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    ColumnLayout {
        id: compactFieldLayout
        visible: compact
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: catalogEditField.paired ? 0 : 4

        Label {
            visible: !catalogEditField.paired
            Layout.fillWidth: true
            text: String(initialEditor.graphValue(modelData, "label", ""))
            color: "#475569"
            font.family: initialEditor.uiFontFamily
            font.pixelSize: initialEditor.uiSmallFontSize
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: catalogEditField.paired ? 4 : 5

            Label {
                visible: catalogEditField.paired
                Layout.preferredWidth: 96
                Layout.minimumWidth: 96
                text: String(initialEditor.graphValue(modelData, "label", ""))
                color: "#475569"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: initialEditor.uiSmallFontSize
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            InitialUiComboBox {
                visible: comboEditor
                Layout.fillWidth: true
                Layout.preferredHeight: catalogEditField.dense ? 24 : (catalogEditField.paired ? 28 : 30)
                Layout.minimumHeight: catalogEditField.dense ? 24 : (catalogEditField.paired ? 28 : 30)
                Layout.maximumHeight: catalogEditField.dense ? 24 : (catalogEditField.paired ? 28 : 30)
                model: visible ? initialEditor.catalogFieldOptions(modelData) : []
                currentIndex: visible ? initialEditor.catalogComboIndex(modelData) : -1
                onActivated: function(index) {
                    if (index >= 0)
                        initialEditor.updateCatalogField(String(initialEditor.graphValue(modelData, "path", "")), String(model[index]))
                }
            }

            InitialUiTextField {
                visible: !comboEditor
                Layout.fillWidth: true
                Layout.preferredHeight: catalogEditField.dense ? 24 : (catalogEditField.paired ? 28 : 30)
                Layout.minimumHeight: catalogEditField.dense ? 24 : (catalogEditField.paired ? 28 : 30)
                Layout.maximumHeight: catalogEditField.dense ? 24 : (catalogEditField.paired ? 28 : 30)
                text: initialEditor.formatNumber(initialEditor.graphValue(modelData, "value", ""))
                readOnly: !Boolean(initialEditor.graphValue(modelData, "editable", true))
                onEditingFinished: {
                    if (!readOnly)
                        initialEditor.updateCatalogField(String(initialEditor.graphValue(modelData, "path", "")), text)
                }
            }

            Label {
                visible: unitText !== ""
                Layout.preferredWidth: visible ? (catalogEditField.paired ? 28 : 34) : 0
                text: unitText
                color: "#64748b"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: initialEditor.uiSmallFontSize
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }
}
    }

    Component {
id: catalogEditGroupComponent
GridLayout {
    property var modelData
    property var fields: initialEditor.asArray(initialEditor.graphValue(modelData, "fields", []))
    property bool paired: Boolean(initialEditor.graphValue(modelData, "paired", false))
    columns: paired ? Math.min(2, Math.max(1, fields.length)) : fields.length
    readonly property bool dense: initialEditor.catalogMode === "sensor"
    columnSpacing: paired ? (dense ? 6 : 8) : 10
    rowSpacing: paired ? (dense ? 2 : 4) : 0
    implicitHeight: paired
                    ? (Math.ceil(fields.length / Math.max(1, columns)) * (dense ? 24 : 28) +
                       Math.max(0, Math.ceil(fields.length / Math.max(1, columns)) - 1) * rowSpacing)
                    : 54

    Repeater {
        model: fields
        delegate: Loader {
            required property var modelData
            Layout.preferredWidth: parent.paired ? Math.max(0, (parent.width - parent.columnSpacing) / 2) : 0
            Layout.fillWidth: true
            sourceComponent: catalogEditFieldComponent
            onLoaded: {
                item.modelData = modelData
                item.compact = true
                item.paired = parent.paired
            }
        }
    }
}
    }

    component PlatformCatalogRow: Rectangle {
id: platformCatalogRow
property var rowData: ({})
property string catalogKind: "platform"
property int rowIndex: 0
property string typeId: initialEditor.rowTypeId(rowData)
property string rowName: String(initialEditor.graphValue(rowData, "name", typeId))
property string domainText: String(initialEditor.graphValue(rowData, "domain", ""))
property string classText: String(initialEditor.graphValue(rowData, "type", ""))
property string moverText: String(initialEditor.graphValue(rowData, "mover", ""))
property bool selected: initialEditor.catalogMode === catalogKind && initialEditor.selectedCatalogId === typeId
property real minSpeedMps: Number(initialEditor.nestedValue(rowData, "capability_boundary.min_speed_ms", 0))
property real maxSpeedMps: Number(initialEditor.nestedValue(rowData, "capability_boundary.max_speed_ms", 0))
property real minAltM: Number(initialEditor.nestedValue(rowData, "capability_boundary.min_alt_m", 0))
property real maxAltM: Number(initialEditor.nestedValue(rowData, "capability_boundary.max_alt_m", 0))
property real maxLinearAccMps2: Number(initialEditor.nestedValue(rowData, "capability_boundary.max_linear_acc_ms2", 0))
property real maxRadialAccMps2: Number(initialEditor.nestedValue(rowData, "capability_boundary.max_radial_acc_ms2", 0))
property real maxLinearLoadG: Number(initialEditor.nestedValue(rowData, "capability_boundary.max_linear_load_g", 0))
property real maxRadialLoadG: Number(initialEditor.nestedValue(rowData, "capability_boundary.max_radial_load_g", 0))

function domainTone(slot) {
    var domain = domainText.toUpperCase()
    if (domain === "AIR")
        return slot === "fill" ? "#eff6ff" : slot === "hover" ? "#f0f7ff" : slot === "selected" ? "#e0f2fe" : slot === "text" ? "#1d4ed8" : "#3b82f6"
    if (domain === "SEA")
        return slot === "fill" ? "#ecfdf5" : slot === "hover" ? "#f0fdf4" : slot === "selected" ? "#d1fae5" : slot === "text" ? "#047857" : "#10b981"
    if (domain === "MIS")
        return slot === "fill" ? "#fff7ed" : slot === "hover" ? "#fff7ed" : slot === "selected" ? "#ffedd5" : slot === "text" ? "#c2410c" : "#f97316"
    return slot === "fill" ? "#f8fafc" : slot === "hover" ? "#f8fafc" : slot === "selected" ? "#e2e8f0" : slot === "text" ? "#475569" : "#94a3b8"
}

function boundaryRangeText(minValue, maxValue) {
    var minNumber = Number(minValue || 0)
    var maxNumber = Number(maxValue || 0)
    if (minNumber <= 0 && maxNumber <= 0)
        return "--"
    return initialEditor.formatNumber(minNumber) + "-" + initialEditor.formatNumber(maxNumber)
}

function boundaryPrimaryText() {
    var speedText = boundaryRangeText(minSpeedMps, maxSpeedMps)
    return speedText === "--" ? "速:--" : "速:" + speedText + "m/s"
}

function boundarySecondaryText() {
    if (domainText.toUpperCase() === "SEA") {
        if (maxLinearAccMps2 <= 0 && maxRadialAccMps2 <= 0)
            return "加:--"
        return "加:" + initialEditor.formatNumber(maxLinearAccMps2) + "/" + initialEditor.formatNumber(maxRadialAccMps2)
    }
    var altitudeText = boundaryRangeText(minAltM, maxAltM)
    if (altitudeText !== "--")
        return "高:" + altitudeText + "m"
    if (maxLinearLoadG > 0 || maxRadialLoadG > 0)
        return "载:" + initialEditor.formatNumber(maxLinearLoadG) + "/" + initialEditor.formatNumber(maxRadialLoadG) + "g"
    return "边界 --"
}

function drawTypeIcon(ctx, w, h) {
    ctx.clearRect(0, 0, w, h)
    ctx.strokeStyle = domainTone("line")
    ctx.fillStyle = domainTone("line")
    ctx.lineWidth = 1.5
    ctx.lineCap = "round"
    ctx.lineJoin = "round"

    if (classText === "FLT") {
        ctx.beginPath()
        ctx.moveTo(w * 0.50, h * 0.12)
        ctx.lineTo(w * 0.72, h * 0.86)
        ctx.lineTo(w * 0.50, h * 0.72)
        ctx.lineTo(w * 0.28, h * 0.86)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.50, h * 0.28)
        ctx.lineTo(w * 0.20, h * 0.54)
        ctx.moveTo(w * 0.50, h * 0.28)
        ctx.lineTo(w * 0.80, h * 0.54)
        ctx.stroke()
    } else if (classText === "BOM") {
        ctx.beginPath()
        ctx.moveTo(w * 0.50, h * 0.10)
        ctx.lineTo(w * 0.60, h * 0.82)
        ctx.lineTo(w * 0.50, h * 0.90)
        ctx.lineTo(w * 0.40, h * 0.82)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.22, h * 0.50)
        ctx.lineTo(w * 0.78, h * 0.50)
        ctx.moveTo(w * 0.32, h * 0.72)
        ctx.lineTo(w * 0.68, h * 0.72)
        ctx.stroke()
    } else if (classText === "MIS") {
        ctx.beginPath()
        ctx.moveTo(w * 0.18, h * 0.72)
        ctx.lineTo(w * 0.78, h * 0.20)
        ctx.lineTo(w * 0.86, h * 0.28)
        ctx.lineTo(w * 0.28, h * 0.80)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.22, h * 0.75)
        ctx.lineTo(w * 0.14, h * 0.90)
        ctx.moveTo(w * 0.32, h * 0.82)
        ctx.lineTo(w * 0.38, h * 0.96)
        ctx.stroke()
    } else if (classText === "ELC") {
        ctx.beginPath()
        ctx.moveTo(w * 0.46, h * 0.10)
        ctx.lineTo(w * 0.28, h * 0.54)
        ctx.lineTo(w * 0.50, h * 0.50)
        ctx.lineTo(w * 0.38, h * 0.90)
        ctx.lineTo(w * 0.76, h * 0.40)
        ctx.lineTo(w * 0.54, h * 0.44)
        ctx.closePath()
        ctx.stroke()
    } else if (classText === "CAR") {
        ctx.beginPath()
        ctx.moveTo(w * 0.18, h * 0.34)
        ctx.lineTo(w * 0.78, h * 0.24)
        ctx.lineTo(w * 0.88, h * 0.66)
        ctx.lineTo(w * 0.26, h * 0.78)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.36, h * 0.34)
        ctx.lineTo(w * 0.62, h * 0.70)
        ctx.stroke()
    } else if (classText === "WAN") {
        ctx.beginPath()
        ctx.moveTo(w * 0.50, h * 0.16)
        ctx.lineTo(w * 0.70, h * 0.76)
        ctx.lineTo(w * 0.50, h * 0.64)
        ctx.lineTo(w * 0.30, h * 0.76)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.arc(w * 0.50, h * 0.32, Math.min(w, h) * 0.16, 0, Math.PI * 2)
        ctx.stroke()
    } else if (classText === "DST") {
        ctx.beginPath()
        ctx.moveTo(w * 0.12, h * 0.56)
        ctx.lineTo(w * 0.84, h * 0.44)
        ctx.lineTo(w * 0.72, h * 0.70)
        ctx.lineTo(w * 0.26, h * 0.76)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.40, h * 0.42)
        ctx.lineTo(w * 0.48, h * 0.28)
        ctx.lineTo(w * 0.58, h * 0.42)
        ctx.stroke()
    } else if (classText === "FRI") {
        ctx.beginPath()
        ctx.moveTo(w * 0.16, h * 0.58)
        ctx.lineTo(w * 0.78, h * 0.48)
        ctx.lineTo(w * 0.68, h * 0.70)
        ctx.lineTo(w * 0.30, h * 0.76)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.44, h * 0.48)
        ctx.lineTo(w * 0.44, h * 0.22)
        ctx.moveTo(w * 0.44, h * 0.30)
        ctx.lineTo(w * 0.62, h * 0.40)
        ctx.stroke()
    } else {
        ctx.beginPath()
        ctx.arc(w * 0.48, h * 0.50, Math.min(w, h) * 0.26, 0, Math.PI * 2)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.48, h * 0.18)
        ctx.lineTo(w * 0.48, h * 0.04)
        ctx.moveTo(w * 0.48, h * 0.82)
        ctx.lineTo(w * 0.48, h * 0.96)
        ctx.moveTo(w * 0.16, h * 0.50)
        ctx.lineTo(w * 0.02, h * 0.50)
        ctx.moveTo(w * 0.80, h * 0.50)
        ctx.lineTo(w * 0.96, h * 0.50)
        ctx.stroke()
    }
}

implicitHeight: 88
radius: 6
color: selected ? platformCatalogRow.domainTone("selected")
       : platformCatalogMouse.containsMouse ? platformCatalogRow.domainTone("hover")
       : "#ffffff"
border.color: selected ? platformCatalogRow.domainTone("text")
             : platformCatalogMouse.containsMouse ? platformCatalogRow.domainTone("line")
             : "#d8e0ea"
border.width: selected ? 1.5 : 1

ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    spacing: 5

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 34
            Layout.minimumWidth: 34
            Layout.maximumWidth: 34
            Layout.preferredHeight: 30
            radius: 5
            color: platformCatalogRow.domainTone("fill")
            border.color: platformCatalogRow.domainTone("line")
            border.width: 1

            Canvas {
                id: typeIcon
                anchors.fill: parent
                anchors.margins: 4
                onPaint: platformCatalogRow.drawTypeIcon(getContext("2d"), width, height)

                Connections {
                    target: platformCatalogRow
                    function onClassTextChanged() { typeIcon.requestPaint() }
                    function onDomainTextChanged() { typeIcon.requestPaint() }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: platformCatalogRow.rowName
                color: "#0f172a"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: initialEditor.uiSmallFontSize
                font.bold: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }

        Rectangle {
            Layout.preferredWidth: 78
            Layout.minimumWidth: 78
            Layout.maximumWidth: 78
            Layout.preferredHeight: 34
            radius: 4
            color: platformCatalogRow.domainTone("fill")
            border.color: platformCatalogRow.domainTone("line")
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 0

                Label {
                    Layout.fillWidth: true
                    text: platformCatalogRow.boundaryPrimaryText()
                    color: "#334155"
                    font.family: initialEditor.uiFontFamily
                    font.pixelSize: 9
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                Label {
                    Layout.fillWidth: true
                    text: platformCatalogRow.boundarySecondaryText()
                    color: "#475569"
                    font.family: initialEditor.uiFontFamily
                    font.pixelSize: 9
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 5

        Rectangle {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 20
            radius: 4
            color: platformCatalogRow.domainTone("fill")
            border.color: platformCatalogRow.domainTone("line")
            border.width: 1

            Label {
                anchors.fill: parent
                anchors.leftMargin: 5
                anchors.rightMargin: 5
                text: platformCatalogRow.domainText
                color: platformCatalogRow.domainTone("text")
                font.family: initialEditor.uiFontFamily
                font.pixelSize: 10
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 20
            radius: 4
            color: "#f8fafc"
            border.color: "#dbe3ee"
            border.width: 1

            Label {
                anchors.fill: parent
                anchors.leftMargin: 5
                anchors.rightMargin: 5
                text: platformCatalogRow.classText
                color: "#334155"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: 10
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 20
            radius: 4
            color: "#f8fafc"
            border.color: "#dbe3ee"
            border.width: 1

            Label {
                anchors.fill: parent
                anchors.leftMargin: 5
                anchors.rightMargin: 5
                text: platformCatalogRow.moverText
                color: "#334155"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: 9
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }
}

MouseArea {
    id: platformCatalogMouse
    anchors.fill: parent
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton
    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    onPressed: function(mouse) {
        initialEditor.catalogMode = platformCatalogRow.catalogKind
        initialEditor.selectedCatalogId = platformCatalogRow.typeId
        initialEditor.beginCatalogDrag(platformCatalogRow.catalogKind, platformCatalogRow.typeId, platformCatalogRow.rowName, platformCatalogRow, mouse)
    }
    onPositionChanged: function(mouse) {
        if (pressed)
            initialEditor.updateCatalogDrag(platformCatalogRow, mouse)
    }
    onReleased: function(mouse) {
        initialEditor.finishCatalogDrag(platformCatalogRow, mouse)
    }
}
    }

component WeaponCatalogRow: Rectangle {
id: weaponCatalogRow
property var rowData: ({})
property int rowIndex: 0
property string typeId: initialEditor.rowTypeId(rowData)
property string rowName: String(initialEditor.graphValue(rowData, "name", typeId))
property string classText: String(initialEditor.graphValue(rowData, "type", ""))
property real maxRangeM: Number(initialEditor.nestedValue(rowData, "launch_cap.max_range_m", 0))
property bool selected: initialEditor.catalogMode === "weapon" && initialEditor.selectedCatalogId === typeId
property color accentColor: weaponTone("text")
property color accentFill: weaponTone("fill")
property color accentBorder: weaponTone("line")

function weaponKind() {
    var kind = classText.toUpperCase()
    return kind === "ASCM" ? "ASM" : kind
}

function weaponTone(slot) {
    var kind = weaponKind()
    if (kind === "AAM")
        return slot === "fill" ? "#eff6ff" : slot === "hover" ? "#dbeafe" : slot === "selected" ? "#e0f2fe" : slot === "text" ? "#1d4ed8" : "#60a5fa"
    if (kind === "ASM")
        return slot === "fill" ? "#ecfdf5" : slot === "hover" ? "#d1fae5" : slot === "selected" ? "#ccfbf1" : slot === "text" ? "#047857" : "#34d399"
    if (kind === "SAM")
        return slot === "fill" ? "#fff7ed" : slot === "hover" ? "#ffedd5" : slot === "selected" ? "#fed7aa" : slot === "text" ? "#c2410c" : "#fb923c"
    return slot === "fill" ? "#f8fafc" : slot === "hover" ? "#e2e8f0" : slot === "selected" ? "#e2e8f0" : slot === "text" ? "#475569" : "#94a3b8"
}

function drawWeaponIcon(ctx, w, h) {
    var kind = weaponKind()
    ctx.clearRect(0, 0, w, h)
    ctx.strokeStyle = weaponCatalogRow.accentColor
    ctx.fillStyle = weaponCatalogRow.accentColor
    ctx.lineWidth = 1.55
    ctx.lineCap = "round"
    ctx.lineJoin = "round"

    if (kind === "AAM") {
        ctx.beginPath()
        ctx.moveTo(w * 0.14, h * 0.74)
        ctx.lineTo(w * 0.76, h * 0.20)
        ctx.lineTo(w * 0.88, h * 0.30)
        ctx.lineTo(w * 0.28, h * 0.82)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.30, h * 0.76)
        ctx.lineTo(w * 0.18, h * 0.92)
        ctx.moveTo(w * 0.42, h * 0.84)
        ctx.lineTo(w * 0.54, h * 0.96)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.12, h * 0.26)
        ctx.lineTo(w * 0.26, h * 0.18)
        ctx.lineTo(w * 0.40, h * 0.26)
        ctx.stroke()
    } else if (kind === "ASM") {
        ctx.beginPath()
        ctx.moveTo(w * 0.20, h * 0.22)
        ctx.lineTo(w * 0.76, h * 0.58)
        ctx.lineTo(w * 0.66, h * 0.72)
        ctx.lineTo(w * 0.12, h * 0.34)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.24, h * 0.38)
        ctx.lineTo(w * 0.12, h * 0.56)
        ctx.moveTo(w * 0.40, h * 0.46)
        ctx.lineTo(w * 0.42, h * 0.66)
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.16, h * 0.90)
        ctx.quadraticCurveTo(w * 0.32, h * 0.80, w * 0.48, h * 0.90)
        ctx.quadraticCurveTo(w * 0.64, h * 1.00, w * 0.82, h * 0.88)
        ctx.stroke()
    } else if (kind === "SAM") {
        ctx.beginPath()
        ctx.moveTo(w * 0.52, h * 0.10)
        ctx.lineTo(w * 0.66, h * 0.72)
        ctx.lineTo(w * 0.52, h * 0.86)
        ctx.lineTo(w * 0.38, h * 0.72)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.42, h * 0.70)
        ctx.lineTo(w * 0.24, h * 0.84)
        ctx.moveTo(w * 0.62, h * 0.70)
        ctx.lineTo(w * 0.82, h * 0.84)
        ctx.stroke()
        ctx.beginPath()
        ctx.arc(w * 0.52, h * 0.90, w * 0.26, Math.PI * 1.08, Math.PI * 1.92)
        ctx.stroke()
    } else {
        ctx.beginPath()
        ctx.moveTo(w * 0.14, h * 0.72)
        ctx.lineTo(w * 0.74, h * 0.18)
        ctx.lineTo(w * 0.88, h * 0.32)
        ctx.lineTo(w * 0.28, h * 0.84)
        ctx.closePath()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(w * 0.22, h * 0.76)
        ctx.lineTo(w * 0.10, h * 0.94)
        ctx.moveTo(w * 0.36, h * 0.84)
        ctx.lineTo(w * 0.45, h * 0.98)
        ctx.stroke()
    }
}

implicitHeight: 88
radius: 6
color: selected ? weaponCatalogRow.weaponTone("selected")
       : weaponCatalogMouse.containsMouse ? weaponCatalogRow.weaponTone("hover")
       : "#ffffff"
border.color: selected ? weaponCatalogRow.accentColor : "#d8e0ea"
border.width: selected ? 1.5 : 1

ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    spacing: 5

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 34
            Layout.minimumWidth: 34
            Layout.maximumWidth: 34
            Layout.preferredHeight: 30
            radius: 5
            color: weaponCatalogRow.accentFill
            border.color: weaponCatalogRow.accentBorder
            border.width: 1

            Canvas {
                id: weaponTypeIcon
                anchors.fill: parent
                anchors.margins: 4
                onPaint: weaponCatalogRow.drawWeaponIcon(getContext("2d"), width, height)

                Connections {
                    target: weaponCatalogRow
                    function onClassTextChanged() { weaponTypeIcon.requestPaint() }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: weaponCatalogRow.rowName
                color: "#0f172a"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: initialEditor.uiSmallFontSize
                font.bold: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            Label {
                Layout.fillWidth: true
                text: weaponCatalogRow.typeId
                color: "#64748b"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: 10
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Rectangle {
            Layout.preferredWidth: 64
            Layout.minimumWidth: 64
            Layout.maximumWidth: 64
            Layout.preferredHeight: 20
            radius: 4
            color: weaponCatalogRow.accentFill
            border.color: weaponCatalogRow.accentBorder
            border.width: 1

            Label {
                anchors.fill: parent
                anchors.leftMargin: 7
                anchors.rightMargin: 7
                text: weaponCatalogRow.classText
                color: weaponCatalogRow.accentColor
                font.family: initialEditor.uiFontFamily
                font.pixelSize: 10
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        Label {
            Layout.fillWidth: true
            text: initialEditor.rangeText(weaponCatalogRow.maxRangeM)
            color: "#475569"
            font.family: initialEditor.uiFontFamily
            font.pixelSize: 10
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    Label {
        Layout.fillWidth: true
        text: "拖至平台挂载"
        color: "#64748b"
        font.family: initialEditor.uiFontFamily
        font.pixelSize: 10
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}

MouseArea {
    id: weaponCatalogMouse
    anchors.fill: parent
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton
    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    onPressed: function(mouse) {
        initialEditor.catalogMode = "weapon"
        initialEditor.selectedCatalogId = weaponCatalogRow.typeId
        initialEditor.beginCatalogDrag("weapon", weaponCatalogRow.typeId, weaponCatalogRow.rowName, weaponCatalogRow, mouse)
    }
    onPositionChanged: function(mouse) {
        if (pressed)
            initialEditor.updateCatalogDrag(weaponCatalogRow, mouse)
    }
    onReleased: function(mouse) {
        initialEditor.finishCatalogDrag(weaponCatalogRow, mouse)
    }
}
    }

    component SensorCatalogRow: Rectangle {
id: sensorCatalogRow
property var rowData: ({})
property int rowIndex: 0
property string typeId: initialEditor.rowTypeId(rowData)
property string rowName: initialEditor.localizedEnumText(initialEditor.graphValue(rowData, "name", typeId))
property string classText: String(initialEditor.graphValue(rowData, "type", ""))
property string classLabel: initialEditor.localizedEnumText(classText)
property real maxRangeM: Number(initialEditor.graphValue(rowData, "max_range_m", 0))
property bool selected: initialEditor.catalogMode === "sensor" && initialEditor.selectedCatalogId === typeId

function sensorTone(slot) {
    var kind = classText.toUpperCase()
    if (kind === "RADAR")
        return slot === "fill" ? "#eef6ff" : slot === "hover" ? "#dbeafe" : slot === "selected" ? "#dbeafe" : slot === "text" ? "#1d4ed8" : "#60a5fa"
    if (kind === "OPTICAL")
        return slot === "fill" ? "#f0fdf4" : slot === "hover" ? "#dcfce7" : slot === "selected" ? "#dcfce7" : slot === "text" ? "#047857" : "#34d399"
    if (kind === "INFRARED")
        return slot === "fill" ? "#fff7ed" : slot === "hover" ? "#ffedd5" : slot === "selected" ? "#ffedd5" : slot === "text" ? "#c2410c" : "#fb923c"
    return slot === "fill" ? "#f8fafc" : slot === "hover" ? "#e2e8f0" : slot === "selected" ? "#e2e8f0" : slot === "text" ? "#475569" : "#94a3b8"
}

implicitHeight: 88
radius: 6
color: selected ? sensorCatalogRow.sensorTone("selected")
       : sensorCatalogMouse.containsMouse ? sensorCatalogRow.sensorTone("hover")
       : "#ffffff"
border.color: selected ? sensorCatalogRow.sensorTone("text") : "#d8e0ea"
border.width: selected ? 1.5 : 1

ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    spacing: 5

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 34
            Layout.minimumWidth: 34
            Layout.maximumWidth: 34
            Layout.preferredHeight: 30
            radius: 5
            color: sensorCatalogRow.sensorTone("fill")
            border.color: sensorCatalogRow.sensorTone("line")
            border.width: 1

            Label {
                anchors.fill: parent
                text: sensorCatalogRow.classLabel.length > 0 ? sensorCatalogRow.classLabel[0] : "传"
                color: sensorCatalogRow.sensorTone("text")
                font.family: initialEditor.uiFontFamily
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: sensorCatalogRow.rowName
                color: "#0f172a"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: initialEditor.uiSmallFontSize
                font.bold: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            Label {
                Layout.fillWidth: true
                text: sensorCatalogRow.typeId
                color: "#64748b"
                font.family: initialEditor.uiFontFamily
                font.pixelSize: 10
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Rectangle {
            Layout.preferredWidth: 74
            Layout.minimumWidth: 74
            Layout.maximumWidth: 74
            Layout.preferredHeight: 20
            radius: 4
            color: sensorCatalogRow.sensorTone("fill")
            border.color: sensorCatalogRow.sensorTone("line")
            border.width: 1

            Label {
                anchors.fill: parent
                anchors.leftMargin: 7
                anchors.rightMargin: 7
                text: sensorCatalogRow.classLabel
                color: sensorCatalogRow.sensorTone("text")
                font.family: initialEditor.uiFontFamily
                font.pixelSize: 10
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        Label {
            Layout.fillWidth: true
            text: initialEditor.rangeText(sensorCatalogRow.maxRangeM)
            color: "#475569"
            font.family: initialEditor.uiFontFamily
            font.pixelSize: 10
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    Label {
        Layout.fillWidth: true
        text: "拖至平台挂载"
        color: "#64748b"
        font.family: initialEditor.uiFontFamily
        font.pixelSize: 10
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}

MouseArea {
    id: sensorCatalogMouse
    anchors.fill: parent
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton
    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    onPressed: function(mouse) {
        initialEditor.catalogMode = "sensor"
        initialEditor.selectedCatalogId = sensorCatalogRow.typeId
        initialEditor.beginCatalogDrag("sensor", sensorCatalogRow.typeId, sensorCatalogRow.rowName, sensorCatalogRow, mouse)
    }
    onPositionChanged: function(mouse) {
        if (pressed)
            initialEditor.updateCatalogDrag(sensorCatalogRow, mouse)
    }
    onReleased: function(mouse) {
        initialEditor.finishCatalogDrag(sensorCatalogRow, mouse)
    }
}
    }
    
    component InitialUiButton: Button {
    id: buttonControl
    property bool visualChecked: checked
    
    hoverEnabled: true
    implicitHeight: 32
    font.family: initialEditor.uiFontFamily
    font.pixelSize: initialEditor.uiFontSize
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
                      : buttonControl.visualChecked || buttonControl.activeFocus ? initialEditor.uiFocusColor
                      : initialEditor.uiBorderColor
        border.width: buttonControl.visualChecked || buttonControl.activeFocus ? 1.4 : 1
    }
}
    
    component InitialToolbarSegmentButton: Button {
    id: segmentButton
    property bool visualChecked: checked
    property bool leftSegment: false
    property bool rightSegment: false
    
    hoverEnabled: true
    implicitHeight: 36
    font.family: initialEditor.uiFontFamily
    font.pixelSize: initialEditor.uiFontSize
    font.bold: visualChecked
    padding: 0
    
    contentItem: Text {
        text: segmentButton.text
        font: segmentButton.font
        color: segmentButton.visualChecked ? "#0b67d9" : "#334155"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    
    background: Rectangle {
        radius: 4
        color: segmentButton.visualChecked ? "#e8f2ff"
               : segmentButton.down ? "#e2e8f0"
               : segmentButton.hovered ? "#f8fbff" : "#f8fafc"
        border.color: segmentButton.visualChecked ? "#2f80ed" : "#cbd5e1"
        border.width: 1
    
        Rectangle {
            visible: segmentButton.leftSegment
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: "#cbd5e1"
        }
    }
}
    
    component InitialUiTextField: TextField {
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
    font.family: initialEditor.uiFontFamily
    font.pixelSize: initialEditor.uiFontSize
    color: !enabled ? "#94a3b8" : readOnly ? "#334155" : "#0f172a"
    selectedTextColor: "#ffffff"
    selectionColor: "#2563eb"
    
    background: Rectangle {
        radius: 4
        color: !textFieldControl.enabled ? "#eef2f7"
               : textFieldControl.readOnly ? textFieldControl.readOnlyColor : textFieldControl.normalColor
        border.color: textFieldControl.warn ? "#f97316"
                      : textFieldControl.activeFocus ? initialEditor.uiFocusColor : initialEditor.uiBorderColor
        border.width: textFieldControl.warn || textFieldControl.activeFocus ? 1.4 : 1
    }
}

    component InitialUiComboBox: ComboBox {
    id: comboControl
    property color normalColor: "#ffffff"

    implicitHeight: 32
    leftPadding: 8
    rightPadding: 30
    font.family: initialEditor.uiFontFamily
    font.pixelSize: initialEditor.uiFontSize

    contentItem: Text {
        leftPadding: comboControl.leftPadding
        rightPadding: comboControl.rightPadding
        text: initialEditor.localizedEnumText(comboControl.displayText)
        font: comboControl.font
        color: comboControl.enabled ? "#0f172a" : "#94a3b8"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Canvas {
        id: comboArrowIcon
        x: comboControl.width - width - 10
        y: (comboControl.height - height) / 2
        width: 12
        height: 12

        Connections {
            target: comboControl
            function onEnabledChanged() { comboArrowIcon.requestPaint() }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = comboControl.enabled ? "#475569" : "#94a3b8"
            ctx.lineWidth = 1.8
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            ctx.beginPath()
            ctx.moveTo(3, 5)
            ctx.lineTo(width / 2, 8)
            ctx.lineTo(width - 3, 5)
            ctx.stroke()
        }
    }

    background: Rectangle {
        radius: 4
        color: comboControl.enabled ? comboControl.normalColor : "#eef2f7"
        border.color: comboControl.activeFocus || comboControl.popup.visible ? initialEditor.uiFocusColor : initialEditor.uiBorderColor
        border.width: comboControl.activeFocus || comboControl.popup.visible ? 1.4 : 1
    }

    delegate: ItemDelegate {
        id: comboDelegate
        required property var modelData
        required property int index
        width: comboControl.width
        text: String(modelData)
        font.family: initialEditor.uiFontFamily
        font.pixelSize: initialEditor.uiFontSize
        highlighted: comboControl.highlightedIndex === index
        contentItem: Text {
            text: initialEditor.localizedEnumText(comboDelegate.text)
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
        implicitHeight: Math.min(contentItem.implicitHeight, 260)
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
            border.color: initialEditor.uiBorderColor
            border.width: 1
        }
    }
}
    
    component InitialUiCheckBox: CheckBox {
    id: checkControl
    
    hoverEnabled: true
    implicitWidth: String(text).length === 0 ? 22 : indicator.width + spacing + contentItem.implicitWidth
    implicitHeight: 24
    spacing: 6
    font.family: initialEditor.uiFontFamily
    font.pixelSize: initialEditor.uiFontSize
    
    indicator: Rectangle {
        x: String(checkControl.text).length === 0 ? (checkControl.width - width) / 2 : 0
        y: (checkControl.height - height) / 2
        implicitWidth: 16
        implicitHeight: 16
        radius: 3
        color: !checkControl.enabled ? "#eef2f7"
               : checkControl.checked ? "#1976bd" : "#ffffff"
        border.color: !checkControl.enabled ? "#d5dde6"
                      : checkControl.activeFocus ? initialEditor.uiFocusColor : initialEditor.uiBorderColor
        border.width: 1
    
        Canvas {
            id: checkMark
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (!checkControl.checked) {
                    return
                }
                ctx.strokeStyle = "#ffffff"
                ctx.lineWidth = 2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                ctx.moveTo(4, 8)
                ctx.lineTo(7, 11)
                ctx.lineTo(12, 5)
                ctx.stroke()
            }
    
            Connections {
                target: checkControl
                function onCheckedChanged() { checkMark.requestPaint() }
                function onEnabledChanged() { checkMark.requestPaint() }
            }
        }
    }
    
    contentItem: Text {
        visible: String(checkControl.text).length > 0
        leftPadding: checkControl.indicator.width + checkControl.spacing
        text: checkControl.text
        font: checkControl.font
        color: checkControl.enabled ? "#0f172a" : "#94a3b8"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
    
    component InitialFilterGroup: Rectangle {
    id: filterGroup
    property string title: ""
    
    radius: 6
    color: "#7affffff"
    border.color: initialEditor.uiBorderColor
    border.width: 1
    
    Label {
        x: 10
        y: 8
        text: filterGroup.title
        font.bold: true
        font.family: initialEditor.uiFontFamily
        font.pixelSize: initialEditor.uiTitleFontSize
        color: "#0f172a"
    }
}
    }
