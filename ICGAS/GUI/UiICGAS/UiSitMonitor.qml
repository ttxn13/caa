import QtQuick 6.8
import QtQuick 6.7 as Quick67
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import ICGAS.Ui 1.0
pragma ComponentBehavior: Bound

Item {
    id: sitMonitor

    property var appRoot: null
    property var controller: null
    readonly property var monitorController: controller
    readonly property var backend: controller && controller.backend ? controller.backend : uiICGAS
    readonly property string uiFontFamily: appRoot ? appRoot.uiFontFamily : "Microsoft YaHei UI"
    readonly property int uiFontSize: appRoot ? appRoot.uiFontSize : 14
    readonly property int uiSmallFontSize: appRoot ? appRoot.uiSmallFontSize : 12
    readonly property int uiTitleFontSize: appRoot ? appRoot.uiTitleFontSize : 16
    readonly property color uiBorderColor: appRoot ? appRoot.uiBorderColor : "#cbd5e1"
    readonly property color uiSoftBorderColor: appRoot ? appRoot.uiSoftBorderColor : "#e2e8f0"
    readonly property color uiFocusColor: appRoot ? appRoot.uiFocusColor : "#2f80ed"

    property bool showMonitorIds: false
    property bool showMonitorLinks: false
    property bool showMonitorRanges: false
    property bool showMonitorFormationRange: true
    property bool showMonitorPresetRoutes: false
    property bool showMonitorActualRoutes: true
    property bool mapContainerReady: false
    property string blueGroupSortMetric: "cog"
    property bool blueGroupSortReversed: false
    property string redGroupSortMetric: "displayId"
    property bool redGroupSortReversed: false
    property int algorithmTableIndex: 0
    readonly property bool monitorPageVisible: !appRoot || appRoot.activeMainTabIndex === 3

    function sortNumber(row, key) {
        var value = Number(row && row[key] !== undefined ? row[key] : 0)
        return isNaN(value) ? 0 : value
    }

    function groupSortMetricForSide(sideKey) {
        return sideKey === "red" ? sitMonitor.redGroupSortMetric : sitMonitor.blueGroupSortMetric
    }

    function groupSortReversedForSide(sideKey) {
        return sideKey === "red" ? sitMonitor.redGroupSortReversed : sitMonitor.blueGroupSortReversed
    }

    function sortedGroupRows(sourceRows, sideText, sideAccent, sideKey) {
        var rows = []
        var i = 0
        var row
        if (sourceRows) {
            for (i = 0; i < sourceRows.length; ++i) {
                row = Object.assign({}, sourceRows[i])
                row.sideText = sideText
                row.sideAccent = sideAccent
                rows.push(row)
            }
        }
        var metric = sitMonitor.groupSortMetricForSide(sideKey)
        var reversed = sitMonitor.groupSortReversedForSide(sideKey)
        rows.sort(function(a, b) {
            var result = sitMonitor.compareGroupRows(a, b, metric)
            return reversed ? -result : result
        })
        return rows
    }

    function compareGroupRows(a, b, metric) {
        if (metric === "cog") {
            var cogSortA = Number(a.cogSort || 0)
            var cogSortB = Number(b.cogSort || 0)
            if (cogSortA > 0 && cogSortB > 0 && cogSortA !== cogSortB)
                return cogSortA - cogSortB
            if (cogSortA > 0 && cogSortB <= 0)
                return -1
            if (cogSortB > 0 && cogSortA <= 0)
                return 1
            var cogValueDiff = sortNumber(b, "cogValue") - sortNumber(a, "cogValue")
            if (Math.abs(cogValueDiff) > 0.000001)
                return cogValueDiff
        } else if (metric === "threat") {
            var threatSortA = Number(a.threatSort || 0)
            var threatSortB = Number(b.threatSort || 0)
            if (threatSortA > 0 && threatSortB > 0 && threatSortA !== threatSortB)
                return threatSortA - threatSortB
            if (threatSortA > 0 && threatSortB <= 0)
                return -1
            if (threatSortB > 0 && threatSortA <= 0)
                return 1
            var threatValueDiff = sortNumber(b, "threatValue") - sortNumber(a, "threatValue")
            if (Math.abs(threatValueDiff) > 0.000001)
                return threatValueDiff
        }
        return String(a.displayId || "").localeCompare(String(b.displayId || ""))
    }

    function setGroupSortMetric(sideKey, metric) {
        if (sideKey === "red") {
            if (sitMonitor.redGroupSortMetric === metric)
                sitMonitor.redGroupSortReversed = !sitMonitor.redGroupSortReversed
            else {
                sitMonitor.redGroupSortMetric = metric
                sitMonitor.redGroupSortReversed = false
            }
        } else {
            if (sitMonitor.blueGroupSortMetric === metric)
                sitMonitor.blueGroupSortReversed = !sitMonitor.blueGroupSortReversed
            else {
                sitMonitor.blueGroupSortMetric = metric
                sitMonitor.blueGroupSortReversed = false
            }
        }
    }

    function sortMarker(sideKey, metric) {
        if (sitMonitor.groupSortMetricForSide(sideKey) !== metric)
            return ""
        return sitMonitor.groupSortReversedForSide(sideKey) ? " ▼" : " ▲"
    }

    function allocationTableRows(stageRows) {
        var rows = []
        if (!stageRows)
            return rows
        for (var i = 0; i < stageRows.length; ++i) {
            var stage = stageRows[i]
            var assignments = stage.assignments || []
            for (var j = 0; j < assignments.length; ++j) {
                var row = Object.assign({}, assignments[j])
                row.stageKey = stage.stageKey
                row.stageText = stage.stageText
                rows.push(row)
            }
        }
        return rows
    }

    Timer {
        id: deferredMapContainerTimer
        interval: 120
        repeat: false
        onTriggered: sitMonitor.mapContainerReady = true
    }

    Component.onCompleted: {
        if (monitorPageVisible)
            deferredMapContainerTimer.restart()
    }

    onMonitorPageVisibleChanged: {
        if (monitorPageVisible && !mapContainerReady)
            deferredMapContainerTimer.restart()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Rectangle {
            id: monitorArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 760
            radius: 6
            color: "#eef5fb"
            border.color: sitMonitor.uiBorderColor
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        Layout.fillWidth: true
                        text: "态势监控显示区"
                        color: "#0f172a"
                        font.family: sitMonitor.uiFontFamily
                        font.pixelSize: sitMonitor.uiTitleFontSize
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    MonitorCheckBox { text: "ID"; checked: sitMonitor.showMonitorIds; onClicked: sitMonitor.showMonitorIds = checked }
                    MonitorCheckBox { text: "分群"; checked: sitMonitor.showMonitorFormationRange; onClicked: sitMonitor.showMonitorFormationRange = checked }
                    MonitorCheckBox { text: "指挥关系"; checked: sitMonitor.showMonitorLinks; onClicked: sitMonitor.showMonitorLinks = checked }
                    MonitorCheckBox { text: "范围"; checked: sitMonitor.showMonitorRanges; onClicked: sitMonitor.showMonitorRanges = checked }
                    MonitorCheckBox { text: "预设航路"; checked: sitMonitor.showMonitorPresetRoutes; onClicked: sitMonitor.showMonitorPresetRoutes = checked }
                    MonitorCheckBox { text: "实际航路"; checked: sitMonitor.showMonitorActualRoutes; onClicked: sitMonitor.showMonitorActualRoutes = checked }

                    Label {
                        Layout.preferredHeight: 24
                        text: "拖尾"
                        color: "#475569"
                        font.family: sitMonitor.uiFontFamily
                        font.pixelSize: sitMonitor.uiSmallFontSize
                        verticalAlignment: Text.AlignVCenter
                    }

                    Slider {
                        id: trackLengthSlider
                        Layout.preferredWidth: 118
                        Layout.minimumWidth: 118
                        Layout.maximumWidth: 118
                        Layout.preferredHeight: 24
                        from: 0
                        to: 120
                        stepSize: 1
                        snapMode: Slider.SnapAlways
                        value: sitMonitor.monitorController.trackLength
                        onMoved: sitMonitor.monitorController.trackLength = Math.round(value)
                    }

                    Label {
                        Layout.preferredWidth: 44
                        Layout.minimumWidth: 44
                        Layout.maximumWidth: 44
                        Layout.preferredHeight: 24
                        text: sitMonitor.monitorController.trackLength + "点"
                        color: "#475569"
                        font.family: sitMonitor.uiFontFamily
                        font.pixelSize: sitMonitor.uiSmallFontSize
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Rectangle {
                    id: monitorMap
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 5
                    color: "#f8fafc"
                    border.color: "#b8c7d6"
                    border.width: 1
                    clip: true

                    UiOsgInitMap {
                        id: monitorGlobe
                        anchors.fill: parent
                        platformRows: sitMonitor.monitorController.platformRows
                        selectedPlatformId: sitMonitor.backend.primaryPlatId
                        showPlatformId: sitMonitor.showMonitorIds
                        showCommandLink: sitMonitor.showMonitorLinks
                        showSensorRange: sitMonitor.showMonitorRanges
                        showWeaponRange: sitMonitor.showMonitorRanges
                        showFormationRange: sitMonitor.showMonitorFormationRange
                        showRoute: sitMonitor.showMonitorPresetRoutes || sitMonitor.showMonitorActualRoutes
                        showPresetRoute: sitMonitor.showMonitorPresetRoutes
                        showActualRoute: sitMonitor.showMonitorActualRoutes
                        labelFontFamily: sitMonitor.uiFontFamily
                        labelPixelSize: sitMonitor.uiSmallFontSize
                        iconScale: 0.72
                        useSourceModels: true
                        onPlatformClicked: function(platformId) {
                            if (platformId !== "")
                                sitMonitor.backend.selectPrimaryPlat(platformId)
                        }
                        onPlatformRightClicked: function(platformId) {
                            if (platformId !== "")
                                sitMonitor.backend.selectSecondaryPlat(platformId)
                        }

                        Loader {
                            anchors.fill: parent
                            active: sitMonitor.monitorPageVisible && sitMonitor.mapContainerReady
                            sourceComponent: Component {
                                Quick67.WindowContainer {
                                    anchors.fill: parent
                                    window: monitorGlobe.mapWindow
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: algorithmArea
            Layout.preferredWidth: 620
            Layout.minimumWidth: 560
            Layout.maximumWidth: 720
            Layout.fillHeight: true
            radius: 6
            color: "#a6ffffff"
            border.color: sitMonitor.uiBorderColor
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                GroupResultTable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 260
                    blueRows: sitMonitor.sortedGroupRows(sitMonitor.monitorController.blueGroupRows,
                                                         "蓝方", "#2563eb", "blue")
                    redRows: sitMonitor.sortedGroupRows(sitMonitor.monitorController.redGroupRows,
                                                        "红方", "#dc2626", "red")
                }

                AlgorithmTablesPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.minimumHeight: 230
                    Layout.preferredHeight: Math.max(230, Math.min(320, algorithmArea.height * 0.28))
                    Layout.maximumHeight: 320
                    allocationRows: sitMonitor.allocationTableRows(sitMonitor.monitorController.coaStageRows)
                }
            }
        }
    }

    component MonitorCheckBox: CheckBox {
        id: checkControl

        hoverEnabled: true
        implicitWidth: String(text).length === 0 ? 22 : indicator.width + spacing + contentItem.implicitWidth
        implicitHeight: 24
        spacing: 6
        font.family: sitMonitor.uiFontFamily
        font.pixelSize: sitMonitor.uiFontSize

        indicator: Rectangle {
            x: String(checkControl.text).length === 0 ? (checkControl.width - width) / 2 : 0
            y: (checkControl.height - height) / 2
            implicitWidth: 16
            implicitHeight: 16
            radius: 3
            color: !checkControl.enabled ? "#eef2f7"
                   : checkControl.checked ? "#1976bd" : "#ffffff"
            border.color: !checkControl.enabled ? "#d5dde6"
                          : checkControl.activeFocus ? sitMonitor.uiFocusColor : sitMonitor.uiBorderColor
            border.width: 1

            Canvas {
                id: checkMark
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (!checkControl.checked)
                        return
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

    component GroupResultTable: Rectangle {
        id: groupTable
        property var blueRows: []
        property var redRows: []

        radius: 5
        color: "#f8fafc"
        border.color: sitMonitor.uiSoftBorderColor
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 6

            Label {
                Layout.fillWidth: true
                text: "编队结果"
                color: "#0f172a"
                font.family: sitMonitor.uiFontFamily
                font.pixelSize: sitMonitor.uiTitleFontSize
                font.bold: true
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 6

                SideGroupResultTable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sideKey: "blue"
                    title: "蓝方"
                    accentColor: "#2563eb"
                    rows: groupTable.blueRows
                }

                SideGroupResultTable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sideKey: "red"
                    title: "红方"
                    accentColor: "#dc2626"
                    rows: groupTable.redRows
                }
            }
        }
    }

    component SideGroupResultTable: Rectangle {
        id: sideGroupTable
        property string sideKey: ""
        property string title: ""
        property color accentColor: "#475569"
        property var rows: []
        property real rememberedContentY: 0
        property bool restoringContentY: false
        property int contentYRestoreGeneration: 0

        function restoreContentY() {
            var generation = ++contentYRestoreGeneration
            restoringContentY = true
            Qt.callLater(function() {
                if (generation !== sideGroupTable.contentYRestoreGeneration)
                    return
                var minimumY = groupResultList.originY
                var maximumY = minimumY + Math.max(0, groupResultList.contentHeight - groupResultList.height)
                groupResultList.contentY = Math.max(minimumY,
                                                    Math.min(sideGroupTable.rememberedContentY, maximumY))
                Qt.callLater(function() {
                    if (generation === sideGroupTable.contentYRestoreGeneration)
                        sideGroupTable.restoringContentY = false
                })
            })
        }

        onRowsChanged: restoreContentY()

        radius: 4
        color: "#ffffff"
        border.color: "#dbe4ee"
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                radius: 4
                color: "#f8fafc"
                border.color: "#e2e8f0"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: sideGroupTable.accentColor
                    }

                    Label {
                        Layout.fillWidth: true
                        text: sideGroupTable.title
                        color: "#0f172a"
                        font.family: sitMonitor.uiFontFamily
                        font.pixelSize: sitMonitor.uiFontSize
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        text: String(sideGroupTable.rows ? sideGroupTable.rows.length : 0) + " 群"
                        color: "#64748b"
                        font.family: sitMonitor.uiFontFamily
                        font.pixelSize: sitMonitor.uiSmallFontSize
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                radius: 3
                color: "#edf3f8"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 6

                    TableHeaderLabel {
                        Layout.preferredWidth: 34
                        text: "序号"
                        horizontalAlignment: Text.AlignHCenter
                    }
                    SortableTableHeaderLabel {
                        Layout.fillWidth: true
                        text: "编队" + sitMonitor.sortMarker(sideGroupTable.sideKey, "displayId")
                        sideKey: sideGroupTable.sideKey
                        sortMetric: "displayId"
                    }
                    SortableTableHeaderLabel {
                        Layout.preferredWidth: 58
                        text: "威胁" + sitMonitor.sortMarker(sideGroupTable.sideKey, "threat")
                        sideKey: sideGroupTable.sideKey
                        sortMetric: "threat"
                    }
                    SortableTableHeaderLabel {
                        Layout.preferredWidth: 58
                        text: "COG" + sitMonitor.sortMarker(sideGroupTable.sideKey, "cog")
                        sideKey: sideGroupTable.sideKey
                        sortMetric: "cog"
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !sideGroupTable.rows || sideGroupTable.rows.length === 0
                text: "暂无编队结果"
                color: "#94a3b8"
                font.family: sitMonitor.uiFontFamily
                font.pixelSize: sitMonitor.uiSmallFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            ListView {
                id: groupResultList
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: sideGroupTable.rows && sideGroupTable.rows.length > 0
                clip: true
                spacing: 3
                reuseItems: true
                cacheBuffer: 120
                model: sideGroupTable.rows ? sideGroupTable.rows : []
                onContentYChanged: {
                    if (!sideGroupTable.restoringContentY)
                        sideGroupTable.rememberedContentY = contentY
                }

                delegate: Rectangle {
                    id: groupResultRow
                    required property int index
                    required property string groupId
                    required property string displayId
                    required property string threatValueText
                    required property string cogValueText

                    readonly property bool selected: groupId !== "" && groupId === sitMonitor.backend.primaryPlatId
                    readonly property bool secondarySelected: groupId !== "" && groupId === sitMonitor.backend.secondaryPlatId

                    width: ListView.view.width
                    height: 34
                    radius: 3
                    color: selected ? "#e0efff"
                           : secondarySelected ? "#eef2ff"
                           : "#ffffff"
                    border.color: selected || secondarySelected ? sitMonitor.uiFocusColor : "#e2e8f0"
                    border.width: selected || secondarySelected ? 1.3 : 1

                    MouseArea {
                        anchors.fill: parent
                        enabled: groupResultRow.groupId !== ""
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton)
                                sitMonitor.backend.selectSecondaryPlat(groupResultRow.groupId)
                            else
                                sitMonitor.backend.selectPrimaryPlat(groupResultRow.groupId)
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        spacing: 6

                        TableBodyLabel {
                            Layout.preferredWidth: 34
                            text: String(groupResultRow.index + 1)
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Label {
                            Layout.fillWidth: true
                            text: groupResultRow.displayId
                            color: "#0f172a"
                            font.family: sitMonitor.uiFontFamily
                            font.pixelSize: sitMonitor.uiSmallFontSize
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        MetricBodyLabel {
                            Layout.preferredWidth: 58
                            valueText: groupResultRow.threatValueText
                        }

                        MetricBodyLabel {
                            Layout.preferredWidth: 58
                            valueText: groupResultRow.cogValueText
                        }
                    }
                }
            }
        }
    }

    component AlgorithmTablesPanel: Rectangle {
        id: tablePanel
        property var allocationRows: []
        property string allocationStageFilter: "ALL"
        property int allocationViewMode: 0
        readonly property var visibleAllocationRows: filteredAllocationRows()
        readonly property var graphRows: buildGraphRows()
        readonly property var stageDefinitions: [
            { "key": "WAN", "label": "预警区", "width": 132 },
            { "key": "FLT", "label": "战斗机交战区", "width": 132 },
            { "key": "COP", "label": "协同交战区", "width": 132 },
            { "key": "VSL", "label": "舰艇自防区", "width": 132 }
        ]
        readonly property int ownColumnWidth: 104
        readonly property int targetColumnWidth: 104
        readonly property int flowBoardWidth: ownColumnWidth + targetColumnWidth + 4 * 132

        function stageAccent(stageKey) {
            if (stageKey === "WAN") return "#0f766e"
            if (stageKey === "FLT") return "#c2410c"
            if (stageKey === "COP") return "#4d7c0f"
            if (stageKey === "VSL") return "#6d28d9"
            return "#64748b"
        }

        function stageSoftColor(stageKey) {
            if (stageKey === "WAN") return "#e0f2f1"
            if (stageKey === "FLT") return "#fff7ed"
            if (stageKey === "COP") return "#f1f8e9"
            if (stageKey === "VSL") return "#f5f3ff"
            return "#f1f5f9"
        }

        function filteredAllocationRows() {
            var rows = []
            var sourceRows = tablePanel.allocationRows || []
            for (var i = 0; i < sourceRows.length; ++i) {
                if (tablePanel.allocationStageFilter === "ALL" ||
                        sourceRows[i].stageKey === tablePanel.allocationStageFilter)
                    rows.push(sourceRows[i])
            }
            return rows
        }

        function stageCount(stageKey) {
            var count = 0
            var sourceRows = tablePanel.allocationRows || []
            for (var i = 0; i < sourceRows.length; ++i) {
                if (stageKey === "ALL" || sourceRows[i].stageKey === stageKey)
                    ++count
            }
            return count
        }

        function uniqueCount(rows, fieldName) {
            var values = {}
            var count = 0
            for (var i = 0; rows && i < rows.length; ++i) {
                var value = String(rows[i][fieldName] || "")
                if (value === "" || value === "--" || values[value])
                    continue
                values[value] = true
                ++count
            }
            return count
        }

        function buildGraphRows() {
            var lanes = []
            var laneLookup = {}
            var sourceRows = tablePanel.visibleAllocationRows || []
            for (var i = 0; i < sourceRows.length; ++i) {
                var source = sourceRows[i]
                var ownId = String(source.ownFmtId || "--")
                var targetId = String(source.targetFmtId || "--")
                var laneKey = ownId + "\u001f" + targetId
                var lane = laneLookup[laneKey]
                if (!lane) {
                    lane = {
                        "ownFmtId": ownId,
                        "targetFmtId": targetId,
                        "WAN": [],
                        "FLT": [],
                        "COP": [],
                        "VSL": []
                    }
                    laneLookup[laneKey] = lane
                    lanes.push(lane)
                }
                if (lane[source.stageKey] !== undefined) {
                    lane[source.stageKey].push({
                        "actionText": String(source.actionText || "--"),
                        "serialText": String(source.serialText || "")
                    })
                }
            }
            lanes.sort(function(left, right) {
                var targetCompare = left.targetFmtId.localeCompare(right.targetFmtId)
                return targetCompare !== 0 ? targetCompare :
                       left.ownFmtId.localeCompare(right.ownFmtId)
            })
            return lanes
        }

        function graphLaneHeight(lane) {
            var maximumCount = 1
            for (var i = 0; i < tablePanel.stageDefinitions.length; ++i) {
                var key = tablePanel.stageDefinitions[i].key
                maximumCount = Math.max(maximumCount, lane && lane[key] ? lane[key].length : 0)
            }
            return Math.max(58, 10 + maximumCount * 34)
        }

        radius: 5
        color: "#f8fafc"
        border.color: sitMonitor.uiSoftBorderColor
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 5

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                radius: 4
                color: "#eef2f6"
                border.color: "#cbd5e1"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                        Repeater {
                            model: ["群目标分配", "COA", "WTA"]

                        delegate: AlgorithmTabButton {
                            required property int index
                            required property string modelData
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: modelData
                            visualChecked: sitMonitor.algorithmTableIndex === index
                            onClicked: sitMonitor.algorithmTableIndex = index
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: sitMonitor.algorithmTableIndex === 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 5

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.minimumHeight: 26
                        Layout.preferredHeight: 26
                        Layout.maximumHeight: 26
                        spacing: 3

                        Repeater {
                            model: [
                                { "key": "ALL", "label": "全部" },
                                { "key": "WAN", "label": "WAN" },
                                { "key": "FLT", "label": "FLT" },
                                { "key": "COP", "label": "COP" },
                                { "key": "VSL", "label": "VSL" }
                            ]

                            delegate: Rectangle {
                                id: stageFilterButton
                                required property var modelData
                                readonly property bool selected: tablePanel.allocationStageFilter === modelData.key
                                Layout.preferredWidth: modelData.key === "ALL" ? 58 : 50
                                Layout.minimumHeight: 26
                                Layout.preferredHeight: 26
                                Layout.maximumHeight: 26
                                Layout.fillHeight: true
                                radius: 4
                                color: selected ? tablePanel.stageSoftColor(modelData.key) : "#ffffff"
                                border.color: selected ? tablePanel.stageAccent(modelData.key) : "#dbe4ee"
                                border.width: selected ? 1.4 : 1

                                Label {
                                    anchors.centerIn: parent
                                    text: stageFilterButton.modelData.label + " " +
                                          tablePanel.stageCount(stageFilterButton.modelData.key)
                                    color: stageFilterButton.selected ?
                                               tablePanel.stageAccent(stageFilterButton.modelData.key) : "#475569"
                                    font.family: sitMonitor.uiFontFamily
                                    font.pixelSize: sitMonitor.uiSmallFontSize
                                    font.bold: stageFilterButton.selected
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: tablePanel.allocationStageFilter = stageFilterButton.modelData.key
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            Layout.minimumWidth: 0
                            Layout.preferredWidth: 100
                            Layout.maximumWidth: 100
                            text: tablePanel.visibleAllocationRows.length + "关系 / " +
                                  tablePanel.uniqueCount(tablePanel.visibleAllocationRows, "ownFmtId") + "我方"
                            color: "#64748b"
                            font.family: sitMonitor.uiFontFamily
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            Layout.preferredWidth: 94
                            Layout.minimumHeight: 26
                            Layout.preferredHeight: 26
                            Layout.maximumHeight: 26
                            Layout.fillHeight: true
                            radius: 4
                            color: "#e8eef5"
                            border.color: "#cbd5e1"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 2
                                spacing: 2

                                Repeater {
                                    model: ["流程图", "明细"]

                                    delegate: Rectangle {
                                        id: viewModeButton
                                        required property int index
                                        required property string modelData
                                        readonly property bool selected: tablePanel.allocationViewMode === index
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: 3
                                        color: selected ? "#ffffff" : "transparent"
                                        border.color: selected ? sitMonitor.uiFocusColor : "transparent"
                                        border.width: selected ? 1 : 0

                                        Label {
                                            anchors.centerIn: parent
                                            text: viewModeButton.modelData
                                            color: viewModeButton.selected ? "#0b67d9" : "#64748b"
                                            font.family: sitMonitor.uiFontFamily
                                            font.pixelSize: 11
                                            font.bold: viewModeButton.selected
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: tablePanel.allocationViewMode = viewModeButton.index
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: allocationFlowFrame
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 80
                        visible: tablePanel.allocationViewMode === 0 &&
                                 tablePanel.graphRows.length > 0
                        radius: 4
                        color: "#ffffff"
                        border.color: "#cbd5e1"
                        border.width: 1
                        clip: true

                        Flickable {
                            id: allocationFlowFlick
                            anchors.fill: parent
                            anchors.margins: 1
                            contentWidth: allocationFlowBoard.width
                            contentHeight: allocationFlowBoard.height
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.HorizontalAndVerticalFlick
                            clip: true
                            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            Item {
                                id: allocationFlowBoard
                                width: tablePanel.flowBoardWidth
                                height: Math.max(allocationFlowFlick.height,
                                                 allocationFlowHeader.height + allocationLaneColumn.implicitHeight)

                                Row {
                                    id: allocationFlowHeader
                                    width: parent.width
                                    height: 30
                                    spacing: 0

                                    Rectangle {
                                        width: tablePanel.ownColumnWidth
                                        height: parent.height
                                        color: "#f8edea"
                                        border.color: "#9f5f54"
                                        border.width: 1

                                        Label {
                                            anchors.centerIn: parent
                                            text: "我方编队"
                                            color: "#572c27"
                                            font.family: sitMonitor.uiFontFamily
                                            font.pixelSize: sitMonitor.uiSmallFontSize
                                            font.bold: true
                                        }
                                    }

                                    Repeater {
                                        model: tablePanel.stageDefinitions

                                        delegate: Rectangle {
                                            id: flowHeaderStage
                                            required property var modelData
                                            width: flowHeaderStage.modelData.width
                                            height: allocationFlowHeader.height
                                            color: tablePanel.stageSoftColor(flowHeaderStage.modelData.key)
                                            border.color: tablePanel.stageAccent(flowHeaderStage.modelData.key)
                                            border.width: 1

                                            Column {
                                                anchors.centerIn: parent
                                                spacing: 0

                                                Label {
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    text: flowHeaderStage.modelData.label
                                                    color: tablePanel.stageAccent(flowHeaderStage.modelData.key)
                                                    font.family: sitMonitor.uiFontFamily
                                                    font.pixelSize: 10
                                                    font.bold: true
                                                }

                                                Label {
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    text: flowHeaderStage.modelData.key
                                                    color: "#64748b"
                                                    font.family: sitMonitor.uiFontFamily
                                                    font.pixelSize: 8
                                                }
                                            }
                                        }
                                    }

                                    Rectangle {
                                        width: tablePanel.targetColumnWidth
                                        height: parent.height
                                        color: "#eaf6f8"
                                        border.color: "#31758a"
                                        border.width: 1

                                        Label {
                                            anchors.centerIn: parent
                                            text: "敌方分群"
                                            color: "#174a5a"
                                            font.family: sitMonitor.uiFontFamily
                                            font.pixelSize: sitMonitor.uiSmallFontSize
                                            font.bold: true
                                        }
                                    }
                                }

                                Column {
                                    id: allocationLaneColumn
                                    y: allocationFlowHeader.height
                                    width: parent.width
                                    spacing: 0

                                    Repeater {
                                        model: tablePanel.graphRows

                                        delegate: Rectangle {
                                            id: allocationLane
                                            required property var modelData
                                            required property int index
                                            width: allocationFlowBoard.width
                                            height: tablePanel.graphLaneHeight(allocationLane.modelData)
                                            color: allocationLane.index % 2 === 0 ? "#fffdfa" : "#f8fafc"

                                            Canvas {
                                                id: allocationLaneCanvas
                                                anchors.fill: parent
                                                z: 0

                                                onPaint: {
                                                    var ctx = getContext("2d")
                                                    ctx.clearRect(0, 0, width, height)
                                                    var centerY = Math.round(height / 2) + 0.5
                                                    var startX = tablePanel.ownColumnWidth - 12
                                                    var endX = width - tablePanel.targetColumnWidth + 12
                                                    ctx.strokeStyle = "#8b6b65"
                                                    ctx.fillStyle = "#8b6b65"
                                                    ctx.lineWidth = 1.4
                                                    ctx.beginPath()
                                                    ctx.moveTo(startX, centerY)
                                                    ctx.lineTo(endX, centerY)
                                                    ctx.stroke()

                                                    var arrowXs = [
                                                        tablePanel.ownColumnWidth,
                                                        tablePanel.ownColumnWidth + 132,
                                                        tablePanel.ownColumnWidth + 264,
                                                        tablePanel.ownColumnWidth + 396,
                                                        tablePanel.ownColumnWidth + 528,
                                                        endX
                                                    ]
                                                    for (var i = 0; i < arrowXs.length; ++i) {
                                                        var x = arrowXs[i]
                                                        ctx.beginPath()
                                                        ctx.moveTo(x, centerY)
                                                        ctx.lineTo(x - 7, centerY - 4)
                                                        ctx.lineTo(x - 7, centerY + 4)
                                                        ctx.closePath()
                                                        ctx.fill()
                                                    }

                                                    ctx.strokeStyle = "#b8b8b8"
                                                    ctx.lineWidth = 1
                                                    for (var dashX = 0; dashX < width; dashX += 18) {
                                                        ctx.beginPath()
                                                        ctx.moveTo(dashX, height - 1)
                                                        ctx.lineTo(Math.min(dashX + 10, width), height - 1)
                                                        ctx.stroke()
                                                    }
                                                }

                                                onWidthChanged: requestPaint()
                                                onHeightChanged: requestPaint()
                                            }

                                            Row {
                                                anchors.fill: parent
                                                spacing: 0
                                                z: 1

                                                Item {
                                                    width: tablePanel.ownColumnWidth
                                                    height: parent.height

                                                    Rectangle {
                                                        anchors.centerIn: parent
                                                        width: 84
                                                        height: 34
                                                        radius: 5
                                                        color: "#f8e8e4"
                                                        border.color: "#9f5f54"
                                                        border.width: 1.2

                                                        Column {
                                                            anchors.centerIn: parent
                                                            width: parent.width - 12
                                                            spacing: 1

                                                            Label {
                                                                width: parent.width
                                                                text: "执行编队"
                                                                color: "#8b5b52"
                                                                font.family: sitMonitor.uiFontFamily
                                                                font.pixelSize: 8
                                                                horizontalAlignment: Text.AlignHCenter
                                                            }

                                                            Label {
                                                                width: parent.width
                                                                text: allocationLane.modelData.ownFmtId
                                                                color: "#572c27"
                                                                font.family: sitMonitor.uiFontFamily
                                                                font.pixelSize: 10
                                                                font.bold: true
                                                                horizontalAlignment: Text.AlignHCenter
                                                                elide: Text.ElideRight
                                                            }
                                                        }
                                                    }
                                                }

                                                Repeater {
                                                    model: tablePanel.stageDefinitions

                                                    delegate: Item {
                                                        id: allocationStageCell
                                                        required property var modelData
                                                        readonly property var stageRows:
                                                            allocationLane.modelData[allocationStageCell.modelData.key] || []
                                                        width: allocationStageCell.modelData.width
                                                        height: allocationLane.height

                                                        Rectangle {
                                                            anchors.fill: parent
                                                            color: tablePanel.stageSoftColor(allocationStageCell.modelData.key)
                                                            opacity: 0.34
                                                            border.color: tablePanel.stageAccent(allocationStageCell.modelData.key)
                                                            border.width: 1
                                                        }

                                                        Column {
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            x: 5
                                                            width: parent.width - 10
                                                            spacing: 3

                                                            Repeater {
                                                                model: allocationStageCell.stageRows

                                                                delegate: Rectangle {
                                                                    id: stageActionCard
                                                                    required property var modelData
                                                                    width: parent.width
                                                                    height: 30
                                                                    radius: 3
                                                                    color: "#ffffff"
                                                                    border.color: tablePanel.stageAccent(allocationStageCell.modelData.key)
                                                                    border.width: 1.2

                                                                    Row {
                                                                        anchors.fill: parent
                                                                        anchors.margins: 3
                                                                        spacing: 3

                                                                        Rectangle {
                                                                            width: 23
                                                                            height: parent.height
                                                                            radius: 3
                                                                            color: tablePanel.stageSoftColor(allocationStageCell.modelData.key)

                                                                            Label {
                                                                                anchors.centerIn: parent
                                                                                text: "ACT"
                                                                                color: tablePanel.stageAccent(allocationStageCell.modelData.key)
                                                                                font.family: sitMonitor.uiFontFamily
                                                                                font.pixelSize: 8
                                                                                font.bold: true
                                                                            }
                                                                        }

                                                                        Label {
                                                                            width: parent.width - 52
                                                                            height: parent.height
                                                                            text: stageActionCard.modelData.actionText
                                                                            color: "#1f2937"
                                                                            font.family: sitMonitor.uiFontFamily
                                                                            font.pixelSize: 9
                                                                            font.bold: true
                                                                            verticalAlignment: Text.AlignVCenter
                                                                            wrapMode: Text.Wrap
                                                                            maximumLineCount: 2
                                                                            elide: Text.ElideRight
                                                                        }

                                                                        Rectangle {
                                                                            width: 23
                                                                            height: parent.height
                                                                            radius: 3
                                                                            color: tablePanel.stageSoftColor(allocationStageCell.modelData.key)

                                                                            Label {
                                                                                anchors.centerIn: parent
                                                                                text: "DEC"
                                                                                color: tablePanel.stageAccent(allocationStageCell.modelData.key)
                                                                                font.family: sitMonitor.uiFontFamily
                                                                                font.pixelSize: 8
                                                                                font.bold: true
                                                                            }
                                                                        }
                                                                    }

                                                                    ToolTip.visible: stageActionMouse.containsMouse
                                                                    ToolTip.text: allocationLane.modelData.ownFmtId + " → " +
                                                                                  allocationLane.modelData.targetFmtId +
                                                                                  "\n" + allocationStageCell.modelData.label +
                                                                                  "：" + stageActionCard.modelData.actionText
                                                                    ToolTip.delay: 350

                                                                    MouseArea {
                                                                        id: stageActionMouse
                                                                        anchors.fill: parent
                                                                        hoverEnabled: true
                                                                        acceptedButtons: Qt.NoButton
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }

                                                Item {
                                                    width: tablePanel.targetColumnWidth
                                                    height: parent.height

                                                    Rectangle {
                                                        anchors.centerIn: parent
                                                        width: 84
                                                        height: 34
                                                        radius: 5
                                                        color: "#e9f5f8"
                                                        border.color: "#31758a"
                                                        border.width: 1.2

                                                        Column {
                                                            anchors.centerIn: parent
                                                            width: parent.width - 12
                                                            spacing: 1

                                                            Label {
                                                                width: parent.width
                                                                text: "目标分群"
                                                                color: "#4f7580"
                                                                font.family: sitMonitor.uiFontFamily
                                                                font.pixelSize: 8
                                                                horizontalAlignment: Text.AlignHCenter
                                                            }

                                                            Label {
                                                                width: parent.width
                                                                text: allocationLane.modelData.targetFmtId
                                                                color: "#174a5a"
                                                                font.family: sitMonitor.uiFontFamily
                                                                font.pixelSize: 10
                                                                font.bold: true
                                                                horizontalAlignment: Text.AlignHCenter
                                                                elide: Text.ElideRight
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
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 80
                        visible: tablePanel.allocationViewMode === 1 &&
                                 tablePanel.visibleAllocationRows.length > 0
                        radius: 4
                        color: "#ffffff"
                        border.color: "#dbe4ee"
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                color: "#e8eef5"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 6
                                    anchors.rightMargin: 6
                                    spacing: 6

                                    TableHeaderLabel { Layout.preferredWidth: 96; text: "阶段" }
                                    TableHeaderLabel { Layout.fillWidth: true; text: "我方编队" }
                                    TableHeaderLabel { Layout.fillWidth: true; text: "目标编队" }
                                    TableHeaderLabel { Layout.preferredWidth: 100; text: "动作" }
                                }
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 2
                                model: tablePanel.visibleAllocationRows
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                delegate: Rectangle {
                                    id: allocationTableRow
                                    required property string stageKey
                                    required property string stageText
                                    required property string ownFmtId
                                    required property string targetFmtId
                                    required property string actionText

                                    width: ListView.view.width
                                    height: 34
                                    color: "#ffffff"
                                    border.color: "#edf2f7"
                                    border.width: 1

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 6
                                        anchors.rightMargin: 6
                                        spacing: 6

                                        Rectangle {
                                            Layout.preferredWidth: 96
                                            Layout.preferredHeight: 22
                                            radius: 3
                                            color: tablePanel.stageSoftColor(allocationTableRow.stageKey)

                                            Label {
                                                anchors.fill: parent
                                                anchors.leftMargin: 5
                                                anchors.rightMargin: 5
                                                text: allocationTableRow.stageKey + " · " + allocationTableRow.stageText
                                                color: tablePanel.stageAccent(allocationTableRow.stageKey)
                                                font.family: sitMonitor.uiFontFamily
                                                font.pixelSize: 10
                                                font.bold: true
                                                verticalAlignment: Text.AlignVCenter
                                                elide: Text.ElideRight
                                            }
                                        }

                                        TableBodyLabel {
                                            Layout.fillWidth: true
                                            text: allocationTableRow.ownFmtId
                                            boldText: true
                                        }
                                        TableBodyLabel {
                                            Layout.fillWidth: true
                                            text: allocationTableRow.targetFmtId
                                            boldText: true
                                        }
                                        TableBodyLabel {
                                            Layout.preferredWidth: 100
                                            text: allocationTableRow.actionText
                                        }
                                    }
                                }
                            }
                        }
                    }

                    EmptyResultTable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: tablePanel.visibleAllocationRows.length === 0
                        title: tablePanel.allocationRows && tablePanel.allocationRows.length > 0 ?
                                   "当前阶段暂无分配" : "暂无群目标分配结果"
                        detail: tablePanel.allocationRows && tablePanel.allocationRows.length > 0 ?
                                    "可切换上方阶段查看其他分配关系" :
                                    "等待 COA 群目标分配输出"
                    }
                }
            }

            EmptyResultTable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: sitMonitor.algorithmTableIndex === 1
                title: "COA 表预留"
                detail: "后续接入 COA 方案摘要、阶段动作和执行状态"
            }

            EmptyResultTable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: sitMonitor.algorithmTableIndex === 2
                title: "WTA 表预留"
                detail: "后续接入武器目标分配、资源占用和冲突状态"
            }
        }
    }

    component AlgorithmTabButton: Button {
        id: algorithmTabButton
        property bool visualChecked: checked

        hoverEnabled: true
        font.family: sitMonitor.uiFontFamily
        font.pixelSize: sitMonitor.uiFontSize
        font.bold: visualChecked
        padding: 0

        contentItem: Text {
            text: algorithmTabButton.text
            font: algorithmTabButton.font
            color: algorithmTabButton.visualChecked ? "#0b67d9" : "#475569"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 3
            color: algorithmTabButton.visualChecked ? "#ffffff"
                   : algorithmTabButton.down ? "#dde5ee"
                   : algorithmTabButton.hovered ? "#f5f8fc" : "transparent"
            border.color: algorithmTabButton.visualChecked ? "#2f80ed" : "transparent"
            border.width: algorithmTabButton.visualChecked ? 1 : 0
        }
    }

    component TableHeaderLabel: Label {
        color: "#475569"
        font.family: sitMonitor.uiFontFamily
        font.pixelSize: sitMonitor.uiSmallFontSize
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    component SortableTableHeaderLabel: Label {
        id: sortableHeader
        property string sideKey: ""
        property string sortMetric: ""

        color: sitMonitor.groupSortMetricForSide(sideKey) === sortMetric ? "#0f172a" : "#475569"
        font.family: sitMonitor.uiFontFamily
        font.pixelSize: sitMonitor.uiSmallFontSize
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: sitMonitor.setGroupSortMetric(sortableHeader.sideKey, sortableHeader.sortMetric)
        }
    }

    component TableBodyLabel: Label {
        property bool boldText: false
        color: "#0f172a"
        font.family: sitMonitor.uiFontFamily
        font.pixelSize: sitMonitor.uiSmallFontSize
        font.bold: boldText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    component MetricBodyLabel: Rectangle {
        id: metricLabel
        property string valueText: "--"

        implicitHeight: 24
        radius: 3
        color: "#eef5fb"
        border.color: "#dbe7f1"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 5
            anchors.rightMargin: 5
            spacing: 3

            Label {
                Layout.fillWidth: true
                text: metricLabel.valueText
                color: "#0f172a"
                font.family: sitMonitor.uiFontFamily
                font.pixelSize: sitMonitor.uiSmallFontSize
                font.bold: true
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }

    component EmptyResultTable: Rectangle {
        id: emptyTable
        property string title: ""
        property string detail: ""

        radius: 4
        color: "#ffffff"
        border.color: "#e2e8f0"
        border.width: 1

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 32, 360)
            spacing: 6

            Label {
                Layout.fillWidth: true
                text: emptyTable.title
                color: "#475569"
                font.family: sitMonitor.uiFontFamily
                font.pixelSize: sitMonitor.uiFontSize
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                Layout.fillWidth: true
                text: emptyTable.detail
                color: "#94a3b8"
                font.family: sitMonitor.uiFontFamily
                font.pixelSize: sitMonitor.uiSmallFontSize
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }

    component MonitorDetailGroup: Rectangle {
        id: detailGroup
        property string title: ""
        property var rows: []

        implicitHeight: Math.max(52, groupColumn.implicitHeight + 24)
        radius: 6
        color: "#78ffffff"
        border.color: sitMonitor.uiBorderColor
        border.width: 1

        ColumnLayout {
            id: groupColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 10
            spacing: 4

            Label {
                text: detailGroup.title
                font.bold: true
                font.family: sitMonitor.uiFontFamily
                font.pixelSize: sitMonitor.uiTitleFontSize
                color: "#0f172a"
            }

            Repeater {
                model: detailGroup.rows
                delegate: RowLayout {
                    id: detailRow
                    required property string name
                    required property string value
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: detailRow.name
                        Layout.preferredWidth: 108
                        color: "#475569"
                        font.family: sitMonitor.uiFontFamily
                        font.pixelSize: sitMonitor.uiSmallFontSize
                        elide: Text.ElideRight
                    }
                    Label {
                        text: detailRow.value
                        Layout.fillWidth: true
                        color: "#0f172a"
                        font.family: sitMonitor.uiFontFamily
                        font.pixelSize: sitMonitor.uiSmallFontSize
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    component MonitorAllocationPanel: Rectangle {
        id: allocationPanel
        property var stageRows: []

        function stageAccent(stageKey) {
            if (stageKey === "WAN") return "#0f766e"
            if (stageKey === "FLT") return "#2563eb"
            if (stageKey === "COP") return "#7c3aed"
            if (stageKey === "VSL") return "#dc2626"
            return "#64748b"
        }

        function stageSoftColor(stageKey) {
            if (stageKey === "WAN") return "#e0f2f1"
            if (stageKey === "FLT") return "#dbeafe"
            if (stageKey === "COP") return "#ede9fe"
            if (stageKey === "VSL") return "#fee2e2"
            return "#f1f5f9"
        }

        radius: 5
        color: "#f8fafc"
        border.color: sitMonitor.uiSoftBorderColor
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                    Layout.fillWidth: true
                    text: "群目标分配结果"
                    color: "#0f172a"
                    font.family: sitMonitor.uiFontFamily
                    font.pixelSize: sitMonitor.uiFontSize
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    text: "按 COA 阶段"
                    color: "#64748b"
                    font.family: sitMonitor.uiFontFamily
                    font.pixelSize: sitMonitor.uiSmallFontSize
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 6

                Repeater {
                    model: allocationPanel.stageRows ? allocationPanel.stageRows : []

                    delegate: Rectangle {
                        id: stageCard
                        required property string stageKey
                        required property string stageText
                        required property int assignmentCount
                        required property var assignments

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 4
                        color: "#ffffff"
                        border.color: "#dbe4ee"
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                Rectangle {
                                    Layout.preferredWidth: 7
                                    Layout.preferredHeight: 7
                                    radius: 4
                                    color: allocationPanel.stageAccent(stageCard.stageKey)
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: stageCard.stageKey + " " + stageCard.stageText
                                    color: "#0f172a"
                                    font.family: sitMonitor.uiFontFamily
                                    font.pixelSize: sitMonitor.uiSmallFontSize
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: String(stageCard.assignmentCount)
                                    color: allocationPanel.stageAccent(stageCard.stageKey)
                                    font.family: sitMonitor.uiFontFamily
                                    font.pixelSize: sitMonitor.uiSmallFontSize
                                    font.bold: true
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                visible: !stageCard.assignments || stageCard.assignments.length === 0
                                text: "暂无分配"
                                color: "#94a3b8"
                                font.family: sitMonitor.uiFontFamily
                                font.pixelSize: sitMonitor.uiSmallFontSize
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                visible: stageCard.assignments && stageCard.assignments.length > 0
                                clip: true
                                spacing: 3
                                model: stageCard.assignments ? stageCard.assignments : []

                                delegate: Rectangle {
                                    id: allocationRow
                                    required property string serialText
                                    required property string assignmentText
                                    required property string actionText

                                    width: ListView.view.width
                                    height: 34
                                    radius: 3
                                    color: "#f8fafc"
                                    border.color: "#e2e8f0"
                                    border.width: 1

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        spacing: 5

                                        Rectangle {
                                            Layout.preferredWidth: 18
                                            Layout.preferredHeight: 18
                                            radius: 3
                                            color: allocationPanel.stageSoftColor(stageCard.stageKey)
                                            border.color: allocationPanel.stageAccent(stageCard.stageKey)
                                            border.width: 1

                                            Label {
                                                anchors.fill: parent
                                                text: allocationRow.serialText
                                                color: allocationPanel.stageAccent(stageCard.stageKey)
                                                font.family: sitMonitor.uiFontFamily
                                                font.pixelSize: 10
                                                font.bold: true
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 0

                                            Label {
                                                Layout.fillWidth: true
                                                text: allocationRow.assignmentText
                                                color: "#0f172a"
                                                font.family: sitMonitor.uiFontFamily
                                                font.pixelSize: sitMonitor.uiSmallFontSize
                                                font.bold: true
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: allocationRow.actionText
                                                color: "#475569"
                                                font.family: sitMonitor.uiFontFamily
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
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
    }

    component MonitorSideList: Rectangle {
        id: sideList
        property string title: ""
        property color accentColor: "#475569"
        property var rows: []
        property int totalMemberCount: 0

        onRowsChanged: {
            var memberCount = 0
            if (rows) {
                for (var i = 0; i < rows.length; ++i)
                    memberCount += rows[i].memberCount || 0
            }
            totalMemberCount = memberCount
        }

        function groupCountText() {
            var groupCount = rows ? rows.length : 0
            return title + "  " + groupCount + "群 / " + totalMemberCount + "平台"
        }

        radius: 5
        color: "#f8fafc"
        border.color: sitMonitor.uiSoftBorderColor
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Rectangle {
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: sideList.accentColor
                }
                Label {
                    Layout.fillWidth: true
                    text: sideList.groupCountText()
                    color: "#0f172a"
                    font.family: sitMonitor.uiFontFamily
                    font.pixelSize: sitMonitor.uiFontSize
                    font.bold: true
                    elide: Text.ElideRight
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !sideList.rows || sideList.rows.length === 0
                text: "暂无分群"
                color: "#64748b"
                font.family: sitMonitor.uiFontFamily
                font.pixelSize: sitMonitor.uiSmallFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: sideList.rows && sideList.rows.length > 0
                clip: true
                spacing: 4
                reuseItems: true
                cacheBuffer: 120
                model: sideList.rows

                delegate: Rectangle {
                    id: groupRow
                    required property string groupId
                    required property string displayId
                    required property string fmtId
                    required property string memberText
                    required property string updateTime
                    required property string threatValueText
                    required property string threatSortText
                    required property string cogValueText
                    required property string cogSortText
                    required property string rangeText
                    required property int memberCount
                    required property var memberRows

                    readonly property bool primarySelected: groupId !== "" && groupId === sitMonitor.backend.primaryPlatId
                    readonly property bool secondarySelected: groupId !== "" && groupId === sitMonitor.backend.secondaryPlatId

                    width: ListView.view.width
                    height: groupColumn.implicitHeight + 8
                    radius: 4
                    color: primarySelected ? "#e0efff"
                           : secondarySelected ? "#eef2ff" : "#ffffff"
                    border.color: primarySelected || secondarySelected ? sitMonitor.uiFocusColor : sitMonitor.uiSoftBorderColor
                    border.width: primarySelected || secondarySelected ? 1.5 : 1

                    ColumnLayout {
                        id: groupColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 4
                        spacing: 4

                        Rectangle {
                            id: groupHeader
                            Layout.fillWidth: true
                            Layout.preferredHeight: 56
                            radius: 4
                            color: groupRow.primarySelected ? "#d8ecff"
                                   : "#f8fafc"
                            border.color: groupRow.primarySelected ? sitMonitor.uiFocusColor : "#dbe4ee"
                            border.width: groupRow.primarySelected ? 1.3 : 1

                            MouseArea {
                                id: groupMouse
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                enabled: groupRow.groupId !== ""
                                onClicked: function(mouse) {
                                    if (mouse.button === Qt.RightButton)
                                        sitMonitor.backend.selectSecondaryPlat(groupRow.groupId)
                                    else
                                        sitMonitor.backend.selectPrimaryPlat(groupRow.groupId)
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 5
                                spacing: 6

                                Rectangle {
                                    Layout.preferredWidth: 3
                                    Layout.fillHeight: true
                                    radius: 2
                                    color: sideList.accentColor
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        Label {
                                            Layout.fillWidth: true
                                            text: groupRow.displayId
                                            color: "#0f172a"
                                            font.family: sitMonitor.uiFontFamily
                                            font.pixelSize: sitMonitor.uiFontSize
                                            font.bold: true
                                            elide: Text.ElideRight
                                        }

                                        Label {
                                            text: "威胁#" + groupRow.threatSortText
                                            color: "#0f172a"
                                            font.family: sitMonitor.uiFontFamily
                                            font.pixelSize: sitMonitor.uiSmallFontSize
                                            font.bold: true
                                        }

                                        Label {
                                            text: "COG#" + groupRow.cogSortText
                                            color: "#0f172a"
                                            font.family: sitMonitor.uiFontFamily
                                            font.pixelSize: sitMonitor.uiSmallFontSize
                                            font.bold: true
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: "威胁 " + groupRow.threatValueText + "    COG " + groupRow.cogValueText +
                                              "    平台 " + groupRow.memberCount +
                                              "    范围 " + (groupRow.rangeText === "--" ? "--" : groupRow.rangeText + "m")
                                        color: "#475569"
                                        font.family: sitMonitor.uiFontFamily
                                        font.pixelSize: sitMonitor.uiSmallFontSize
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        visible: false
                                        Layout.fillWidth: true
                                        text: groupRow.updateTime
                                        color: "#64748b"
                                        font.family: sitMonitor.uiFontFamily
                                        font.pixelSize: sitMonitor.uiSmallFontSize
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }

                        Flow {
                            id: platformFlow
                            Layout.fillWidth: true
                            Layout.preferredHeight: childrenRect.height
                            spacing: 4
                            visible: groupRow.memberRows && groupRow.memberRows.length > 0

                            Repeater {
                                model: groupRow.memberRows ? groupRow.memberRows : []

                                delegate: Rectangle {
                                    id: platformChip
                                    required property string platId
                                    required property string displayId
                                    required property string type
                                    required property string typeName
                                    required property string updateTime
                                    required property string threatValueText
                                    required property string threatSortText

                                    readonly property bool primarySelected: platId === sitMonitor.backend.primaryPlatId
                                    readonly property bool secondarySelected: platId === sitMonitor.backend.secondaryPlatId

                                    width: Math.max(118, Math.floor((platformFlow.width - 4) / 2))
                                    height: 40
                                    radius: 3
                                    color: primarySelected ? "#e0efff"
                                           : secondarySelected ? "#eef2ff"
                                           : "#ffffff"
                                    border.color: primarySelected || secondarySelected ? sitMonitor.uiFocusColor : "#dbe4ee"
                                    border.width: primarySelected || secondarySelected ? 1.4 : 1

                                    MouseArea {
                                        id: platformMouse
                                        anchors.fill: parent
                                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                                        onClicked: function(mouse) {
                                            if (mouse.button === Qt.RightButton)
                                                sitMonitor.backend.selectSecondaryPlat(platformChip.platId)
                                            else
                                                sitMonitor.backend.selectPrimaryPlat(platformChip.platId)
                                        }
                                    }

                                    Column {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        spacing: 1

                                        Row {
                                            width: parent.width
                                            spacing: 4

                                            Label {
                                                width: parent.width - rankLabel.width - 4
                                                text: platformChip.displayId
                                                color: "#0f172a"
                                                font.family: sitMonitor.uiFontFamily
                                                font.pixelSize: sitMonitor.uiSmallFontSize
                                                font.bold: true
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                id: rankLabel
                                                text: platformChip.threatSortText === "--" ? "排序 --" : "#" + platformChip.threatSortText
                                                color: "#0f172a"
                                                font.family: sitMonitor.uiFontFamily
                                                font.pixelSize: sitMonitor.uiSmallFontSize
                                                font.bold: true
                                            }
                                        }

                                        Label {
                                            width: parent.width
                                            text: (platformChip.typeName !== "" ? platformChip.typeName : platformChip.type) +
                                                  "  威胁 " + platformChip.threatValueText
                                            color: "#475569"
                                            font.family: sitMonitor.uiFontFamily
                                            font.pixelSize: sitMonitor.uiSmallFontSize
                                            elide: Text.ElideRight
                                        }

                                        Label {
                                            visible: false
                                            width: parent.width
                                            text: "威胁 " + platformChip.threatValueText
                                            color: "#64748b"
                                            font.family: sitMonitor.uiFontFamily
                                            font.pixelSize: sitMonitor.uiSmallFontSize
                                            elide: Text.ElideRight
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
}
