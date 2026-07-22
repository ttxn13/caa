import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Window 6.8
import ICGAS.Ui 1.0
pragma ComponentBehavior: Bound

Window {
    id: root
    width: 1920
    height: 1080
    minimumWidth: 960
    minimumHeight: 540
    visible: false
    title: "作战行动方案智能生成与评估软件"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinMaxButtonsHint
    color: "#f8fafc"

    onClosing: function(close) {
        if (!closingAfterConceptPrompt && conceptHasConfigChanges()) {
            close.accepted = false
            openConceptConfigPrompt(function() {
                closingAfterConceptPrompt = true
                root.close()
            }, "退出软件前，需要决定是否将知识图谱修改写入 JSON 配置。")
            return
        }
        Qt.quit()
    }
    Component.onCompleted: {
        width = 1920
        height = 1080
        minimumWidth = 960
        minimumHeight = 540
    }

    readonly property real designWidth: 1920
    readonly property real designHeight: 1080
    readonly property real designAspect: designWidth / designHeight
    readonly property real viewScale: Math.max(0.1, Math.min(width / designWidth, height / designHeight))
    readonly property int resizeMargin: 8
    readonly property int resizeCornerSize: resizeMargin * 2
    property point dragPressPos: Qt.point(0, 0)
    property bool fallbackMove: false
    property bool titleDragArmed: false
    property bool aspectResizeActive: false
    property bool closingAfterConceptPrompt: false
    property bool uiStartupComplete: false
    property int aspectResizeEdges: 0
    property point aspectResizePressGlobal: Qt.point(0, 0)
    property rect aspectResizeStartRect: Qt.rect(0, 0, 1920, 1080)
    property int activeMainTabIndex: 0
    property bool conceptPageLoaded: false
    property bool workflowPageLoaded: false
    property bool sitMonitorPageLoaded: false
    property string workflowInputMode: uiICGAS.workflowInputMode
    property bool workflowRealtimeMode: workflowInputMode !== "replay"
    readonly property bool afsimTestPageActive: workflowInputMode === "afsim" && activeMainTabIndex === 0
    readonly property bool afsimTestRunning: uiAFSimTest.clientRunning || uiAFSimTest.serverRunning
    property string selectedWorkflowReplayVenue: uiICGAS.replayLogNames.length > 0 ? uiICGAS.replayLogNames[0] : ""
    property var pendingConceptAction: null
    property string conceptConfigPromptMessage: ""
    property string conceptDetailPromptMessage: ""
    property string workflowUnsavedPromptMessage: ""
    readonly property string uiFontFamily: "新宋体"
    readonly property int uiFontSize: 14
    readonly property int uiMainTabFontSize: 18
    readonly property int uiMainTabHeight: 36
    readonly property int uiSmallFontSize: 13
    readonly property int uiTitleFontSize: 17
    readonly property color uiBorderColor: "#b8c7d6"
    readonly property color uiSoftBorderColor: "#d6e0ea"
    readonly property color uiFocusColor: "#2f80c4"
    readonly property real graphLaneTitleWidth: 108
    readonly property real graphLaneContentPadding: 16
    readonly property real graphLaneVerticalPadding: 8

    function conceptHasConfigChanges() {
        return uiICGAS.conceptPendingDetailChanges || uiICGAS.conceptDirty
    }

    function runPendingConceptAction(action) {
        if (action) {
            action()
        }
    }

    function requestConceptDetailChange(action) {
        if (uiICGAS.conceptPendingDetailChanges) {
            openConceptDetailPrompt(action)
        } else {
            runPendingConceptAction(action)
        }
    }

    function requestShowConceptOverview(typeText) {
        requestConceptDetailChange(function() { uiICGAS.showConceptOverviewDetail(typeText) })
    }

    function requestShowConceptNode(nodeId) {
        requestConceptDetailChange(function() { uiICGAS.showConceptNodeDetail(nodeId) })
    }

    function requestConceptScenarioChange(scenario) {
        if (scenario === uiICGAS.selectedConceptScenario) {
            return
        }
        var action = function() { uiICGAS.selectedConceptScenario = scenario }
        if (uiICGAS.conceptPendingDetailChanges) {
            openConceptDetailPrompt(action, "切换场景前，当前详情还有未应用的修改。")
        } else {
            action()
        }
    }

    function requestMainTabChange(index) {
        if (index === activeMainTabIndex) {
            return
        }
        var action = function() {
            activeMainTabIndex = index
            markPageLoaded(index)
        }
        if (activeMainTabIndex === 1 && conceptHasConfigChanges()) {
            openConceptConfigPrompt(action, "离开知识图谱前，需要决定是否写入当前配置。")
        } else {
            action()
        }
    }

    function markPageLoaded(index) {
        if (index === 1)
            conceptPageLoaded = true
        else if (index === 2)
            workflowPageLoaded = true
        else if (index === 3)
            sitMonitorPageLoaded = true
    }

    function workflowReplayIndex(name) {
        var index = uiICGAS.replayLogNames.indexOf(name)
        if (index >= 0) {
            return index
        }
        return uiICGAS.replayLogNames.length > 0 ? 0 : -1
    }

    function workflowSceneModel() {
        if (workflowInputMode === "afsim") {
            return uiICGAS.scenarioNames
        }
        if (workflowInputMode === "replay") {
            return uiICGAS.replayLogNames
        }
        return uiICGAS.simScenarioNames
    }

    function workflowSceneIndex() {
        if (workflowInputMode === "replay") {
            return workflowReplayIndex(selectedWorkflowReplayVenue)
        }
        return Math.max(0, workflowSceneModel().indexOf(uiICGAS.selectedScenario))
    }

    function workflowSceneLabel() {
        if (workflowInputMode === "afsim") {
            return "加载AFSim场景"
        }
        if (workflowInputMode === "replay") {
            return "加载回放场景"
        }
        return "加载仿真场景"
    }

    function setWorkflowInputMode(mode) {
        workflowInputMode = mode
        workflowRealtimeMode = mode !== "replay"
        uiICGAS.setWorkflowInputMode(mode)
        if (mode === "replay" && selectedWorkflowReplayVenue === "" && uiICGAS.replayLogNames.length > 0) {
            selectedWorkflowReplayVenue = uiICGAS.replayLogNames[0]
        } else if (mode === "icgas" && uiICGAS.selectedScenario === "" && uiICGAS.simScenarioNames.length > 0) {
            uiICGAS.selectedScenario = uiICGAS.simScenarioNames[0]
        } else if (mode === "afsim" && uiICGAS.selectedScenario === "" && uiICGAS.scenarioNames.length > 0) {
            uiICGAS.selectedScenario = uiICGAS.scenarioNames[0]
        }
    }

    function workflowStatusColor(statusKind) {
        if (statusKind === "fail" || statusKind === "failed") {
            return "#d64545"
        }
        if (statusKind === "not_ready" || statusKind === "unknown") {
            return "#f97316"
        }
        if (statusKind === "run") {
            return "#45a657"
        }
        if (statusKind === "done" || statusKind === "complete" || statusKind === "completed") {
            return "#7c3aed"
        }
        if (statusKind === "wait") {
            return "#f2b600"
        }
        if (statusKind === "disabled") {
            return "#c6ccd4"
        }
        return "#3c98f2"
    }

    readonly property var workflowStatusLegendRows: [
        [
            { "text": "已就绪", "color": workflowStatusColor("ready") },
            { "text": "未就绪", "color": workflowStatusColor("not_ready") },
            { "text": "完成", "color": workflowStatusColor("done") }
        ],
        [
            { "text": "运行中", "color": workflowStatusColor("run") },
            { "text": "等待中", "color": workflowStatusColor("wait") },
            { "text": "失败", "color": workflowStatusColor("fail") }
        ]
    ]

    Shortcut {
        sequence: "F1"
        context: Qt.WindowShortcut
        enabled: !conceptDetailUnsavedDialog.visible && !conceptConfigUnsavedDialog.visible && !workflowUnsavedDialog.visible
        onActivated: root.requestMainTabChange(0)
    }

    Shortcut {
        sequence: "F2"
        context: Qt.WindowShortcut
        enabled: !conceptDetailUnsavedDialog.visible && !conceptConfigUnsavedDialog.visible && !workflowUnsavedDialog.visible
        onActivated: root.requestMainTabChange(1)
    }

    Shortcut {
        sequence: "F3"
        context: Qt.WindowShortcut
        enabled: !conceptDetailUnsavedDialog.visible && !conceptConfigUnsavedDialog.visible && !workflowUnsavedDialog.visible
        onActivated: root.requestMainTabChange(2)
    }

    Shortcut {
        sequence: "F4"
        context: Qt.WindowShortcut
        enabled: !conceptDetailUnsavedDialog.visible && !conceptConfigUnsavedDialog.visible && !workflowUnsavedDialog.visible
        onActivated: root.requestMainTabChange(3)
    }

    Shortcut {
        sequence: "F5"
        context: Qt.WindowShortcut
        enabled: !conceptDetailUnsavedDialog.visible && !conceptConfigUnsavedDialog.visible && !workflowUnsavedDialog.visible
        onActivated: root.requestMainTabChange(4)
    }

    Shortcut {
        sequence: "F6"
        context: Qt.WindowShortcut
        enabled: !conceptDetailUnsavedDialog.visible && !conceptConfigUnsavedDialog.visible && !workflowUnsavedDialog.visible
        onActivated: root.requestMainTabChange(5)
    }

    Shortcut {
        sequence: "Ctrl+W"
        context: Qt.WindowShortcut
        enabled: !conceptDetailUnsavedDialog.visible && !conceptConfigUnsavedDialog.visible && !workflowUnsavedDialog.visible
        onActivated: root.close()
    }

    function openConceptDetailPrompt(action, message) {
        conceptDetailPromptMessage = message || "当前详情中有尚未应用的节点或边修改。"
        pendingConceptAction = action
        conceptDetailUnsavedDialog.open()
    }

    function openConceptConfigPrompt(action, message) {
        conceptConfigPromptMessage = message || "知识图谱配置存在尚未写入 JSON 的修改。"
        pendingConceptAction = action
        conceptConfigUnsavedDialog.open()
    }

    function requestRunWorkflow() {
        if (uiICGAS.hasUnsavedWorkflowChanges()) {
            workflowUnsavedPromptMessage = uiICGAS.unsavedWorkflowMessage()
            workflowUnsavedDialog.open()
            return
        }
        uiICGAS.runWorkflow()
    }

    function resolveConceptDetailPrompt(choice) {
        var action = pendingConceptAction
        pendingConceptAction = null
        if (choice === "cancel") {
            conceptDetailUnsavedDialog.close()
            return
        }

        var ok = true
        if (choice === "save") {
            ok = uiICGAS.applyPendingConceptDetail()
        } else if (choice === "discard") {
            uiICGAS.discardPendingConceptDetail()
        }

        conceptDetailUnsavedDialog.close()
        if (ok) {
            runPendingConceptAction(action)
        }
    }

    function resolveConceptConfigPrompt(choice) {
        var action = pendingConceptAction
        pendingConceptAction = null
        if (choice === "cancel") {
            conceptConfigUnsavedDialog.close()
            return
        }

        var ok = true
        if (choice === "save") {
            if (uiICGAS.conceptPendingDetailChanges) {
                ok = uiICGAS.applyPendingConceptDetail()
            }
            if (ok && uiICGAS.conceptDirty) {
                ok = uiICGAS.confirmConceptConfig()
            }
        } else if (choice === "discard") {
            ok = uiICGAS.resetAllConceptConfigChanges()
        }

        conceptConfigUnsavedDialog.close()
        if (ok) {
            runPendingConceptAction(action)
        }
    }

    function graphValue(row, name, fallbackValue) {
        if (row && row[name] !== undefined && row[name] !== null) {
            return row[name]
        }
        return fallbackValue
    }

    function beginAspectResize(edgeFlags, globalPos) {
        if (root.visibility === Window.FullScreen) {
            return
        }
        aspectResizeActive = true
        aspectResizeEdges = edgeFlags
        aspectResizePressGlobal = globalPos
        aspectResizeStartRect = Qt.rect(root.x, root.y, root.width, root.height)
    }

    function updateAspectResize(globalPos) {
        if (!aspectResizeActive) {
            return
        }

        var dx = globalPos.x - aspectResizePressGlobal.x
        var dy = globalPos.y - aspectResizePressGlobal.y
        var hasLeft = (aspectResizeEdges & Qt.LeftEdge) !== 0
        var hasRight = (aspectResizeEdges & Qt.RightEdge) !== 0
        var hasTop = (aspectResizeEdges & Qt.TopEdge) !== 0
        var hasBottom = (aspectResizeEdges & Qt.BottomEdge) !== 0
        var hasHorizontal = hasLeft || hasRight
        var hasVertical = hasTop || hasBottom

        var widthCandidate = aspectResizeStartRect.width
        var heightCandidate = aspectResizeStartRect.height
        if (hasRight) widthCandidate = aspectResizeStartRect.width + dx
        if (hasLeft) widthCandidate = aspectResizeStartRect.width - dx
        if (hasBottom) heightCandidate = aspectResizeStartRect.height + dy
        if (hasTop) heightCandidate = aspectResizeStartRect.height - dy

        var newWidth = aspectResizeStartRect.width
        var newHeight = aspectResizeStartRect.height
        if (hasHorizontal && hasVertical) {
            var widthRatioDelta = Math.abs(widthCandidate - aspectResizeStartRect.width) / Math.max(1, aspectResizeStartRect.width)
            var heightRatioDelta = Math.abs(heightCandidate - aspectResizeStartRect.height) / Math.max(1, aspectResizeStartRect.height)
            if (widthRatioDelta >= heightRatioDelta) {
                newWidth = widthCandidate
                newHeight = newWidth / designAspect
            } else {
                newHeight = heightCandidate
                newWidth = newHeight * designAspect
            }
        } else if (hasHorizontal) {
            newWidth = widthCandidate
            newHeight = newWidth / designAspect
        } else if (hasVertical) {
            newHeight = heightCandidate
            newWidth = newHeight * designAspect
        }

        newWidth = Math.max(root.minimumWidth, newWidth)
        newHeight = Math.max(root.minimumHeight, newHeight)
        if (newWidth / newHeight > designAspect) {
            newWidth = newHeight * designAspect
        } else {
            newHeight = newWidth / designAspect
        }

        var newX = aspectResizeStartRect.x
        var newY = aspectResizeStartRect.y
        if (hasLeft) {
            newX = aspectResizeStartRect.x + aspectResizeStartRect.width - newWidth
        } else if (!hasRight) {
            newX = aspectResizeStartRect.x + (aspectResizeStartRect.width - newWidth) / 2
        }
        if (hasTop) {
            newY = aspectResizeStartRect.y + aspectResizeStartRect.height - newHeight
        } else if (!hasBottom) {
            newY = aspectResizeStartRect.y + (aspectResizeStartRect.height - newHeight) / 2
        }

        root.x = Math.round(newX)
        root.y = Math.round(newY)
        root.width = Math.round(newWidth)
        root.height = Math.round(newHeight)
    }

    function endAspectResize() {
        aspectResizeActive = false
        aspectResizeEdges = 0
    }

    Rectangle {
        anchors.fill: parent
        color: "#0f172a"
    }

    Item {
        id: scaledRoot
        width: root.designWidth
        height: root.designHeight
        x: (root.width - width * root.viewScale) / 2
        y: (root.height - height * root.viewScale) / 2
        scale: root.viewScale
        transformOrigin: Item.TopLeft
        clip: true

        Image {
            anchors.fill: parent
            source: "qrc:/UiICGAS/pic_main.png"
            fillMode: Image.Stretch
        }

        Rectangle {
            anchors.fill: parent
            color: "#99f8fafc"
            border.color: "#7aa8b2c1"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

        Rectangle {
            id: titleBar
            Layout.fillWidth: true
            Layout.preferredHeight: 101
            radius: 6
            color: "#d9ffffff"
            border.color: "#8aa8b2c1"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 44

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: function(mouse) {
                    root.dragPressPos = Qt.point(mouse.x, mouse.y)
                    root.titleDragArmed = root.visibility === Window.FullScreen
                    root.fallbackMove = root.titleDragArmed ? false : !root.startSystemMove()
                }
                onPositionChanged: function(mouse) {
                    if (pressed && root.titleDragArmed) {
                        var moveDistance = Math.max(Math.abs(mouse.x - root.dragPressPos.x),
                                                    Math.abs(mouse.y - root.dragPressPos.y))
                        if (moveDistance > 4) {
                            var restoreWidth = root.minimumWidth * 1.5
                            var restoreHeight = restoreWidth / root.designAspect
                            var titleRatioX = Math.max(0.0, Math.min(1.0, root.dragPressPos.x / root.designWidth))
                            var globalPoint = mapToGlobal(mouse.x, mouse.y)
                            root.showNormal()
                            root.width = Math.round(restoreWidth)
                            root.height = Math.round(restoreHeight)
                            root.x = Math.round(globalPoint.x - restoreWidth * titleRatioX)
                            root.y = Math.round(globalPoint.y - root.dragPressPos.y * root.viewScale)
                            root.titleDragArmed = false
                            root.fallbackMove = !root.startSystemMove()
                        }
                    }
                    if (pressed && root.fallbackMove) {
                        root.x += (mouse.x - root.dragPressPos.x) * root.viewScale
                        root.y += (mouse.y - root.dragPressPos.y) * root.viewScale
                    }
                }
                onDoubleClicked: {
                    if (root.visibility === Window.FullScreen)
                        root.showNormal()
                    else
                        root.showFullScreen()
                }
                onReleased: {
                    root.fallbackMove = false
                    root.titleDragArmed = false
                }
                onCanceled: {
                    root.fallbackMove = false
                    root.titleDragArmed = false
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 8
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    text: "作战行动方案智能生成与评估软件"
                    color: "#0f172a"
                    font.family: root.uiFontFamily
                    font.pixelSize: 20
                    font.bold: true
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                WindowControlButton {
                    iconType: "minimize"
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 30
                    onClicked: root.showMinimized()
                }

                WindowControlButton {
                    iconType: root.visibility === Window.FullScreen ? "restore" : "maximize"
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 30
                    onClicked: {
                        if (root.visibility === Window.FullScreen)
                            root.showNormal()
                        else
                            root.showFullScreen()
                    }
                }

                WindowControlButton {
                    iconType: "close"
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 30
                    onClicked: root.close()
                }
            }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: "#8aa8b2c1"
            }

            Item {
                id: workflowToolbar
                Layout.fillWidth: true
                Layout.preferredHeight: 56

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 10

                RowLayout {
                    Layout.preferredWidth: 270
                    Layout.preferredHeight: 36
                    spacing: 0

                    ToolbarSegmentButton {
                        Layout.preferredWidth: 90
                        Layout.minimumWidth: 90
                        Layout.maximumWidth: 90
                        text: "AFSim"
                        visualChecked: root.workflowInputMode === "afsim"
                        leftSegment: true
                        onClicked: {
                            root.setWorkflowInputMode("afsim")
                        }
                    }

                    ToolbarSegmentButton {
                        Layout.preferredWidth: 90
                        Layout.minimumWidth: 90
                        Layout.maximumWidth: 90
                        text: "ICGAS"
                        visualChecked: root.workflowInputMode === "icgas"
                        onClicked: {
                            root.setWorkflowInputMode("icgas")
                        }
                    }

                    ToolbarSegmentButton {
                        Layout.preferredWidth: 90
                        Layout.minimumWidth: 90
                        Layout.maximumWidth: 90
                        text: "回放"
                        visualChecked: root.workflowInputMode === "replay"
                        rightSegment: true
                        onClicked: {
                            root.setWorkflowInputMode("replay")
                        }
                    }
                }

                RowLayout {
                    Layout.preferredWidth: 296
                    spacing: 8
                    Label {
                        text: root.workflowSceneLabel()
                        color: "#475569"
                        font.family: root.uiFontFamily
                        font.pixelSize: root.uiFontSize
                    }
                    UiComboBox {
                        Layout.preferredWidth: 196
                        Layout.preferredHeight: 36
                        model: root.workflowSceneModel()
                        currentIndex: root.workflowSceneIndex()
                        onActivated: {
                            if (root.workflowInputMode === "replay") {
                                root.selectedWorkflowReplayVenue = currentText
                            } else {
                                uiICGAS.selectedScenario = currentText
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: 4
                    color: "#eef2f6"
                    border.color: "#cbd5e1"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0

                        Repeater {
                            model: [root.workflowInputMode === "afsim" ? "AFSim测试" : "初始态势编辑", "概念图谱", "流程编排", "态势监控", "COA比较", "效能评估"]
                            delegate: ToolbarNavButton {
                                required property int index
                                required property string modelData
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: modelData
                                visualChecked: root.activeMainTabIndex === index
                                onClicked: root.requestMainTabChange(index)
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.preferredWidth: 344
                    spacing: 8
                    ToolbarActionButton {
                        Layout.preferredWidth: 104
                        text: uiICGAS.workflowRunning ? "正在运行" : "运行流程"
                        iconType: "run"
                        visualChecked: uiICGAS.workflowRunning
                        enabled: !uiICGAS.workflowRunning && !root.afsimTestPageActive && !root.afsimTestRunning
                        onClicked: root.requestRunWorkflow()
                    }
                    ToolbarActionButton { Layout.preferredWidth: 72; text: "暂停"; iconType: "pause"; visualChecked: uiICGAS.workflowPaused; onClicked: uiICGAS.pauseWorkflow() }
                    ToolbarActionButton { Layout.preferredWidth: 72; text: "停止"; iconType: "stop"; onClicked: uiICGAS.stopWorkflow() }
                    ToolbarActionButton { Layout.preferredWidth: 72; text: "保存"; iconType: "save"; enabled: !uiICGAS.workflowRunning; onClicked: uiICGAS.saveWorkflowConfig() }
                }
            }
        }
        }
        }

        TabBar {
            id: tabMain
            Layout.fillWidth: true
            Layout.preferredHeight: 0
            visible: false
            spacing: 4
            currentIndex: root.activeMainTabIndex

            background: Rectangle {
                color: "#e8eef5"
            }

            TabButton {
                id: tabConceptButton
                hoverEnabled: true
                height: root.uiMainTabHeight
                implicitHeight: root.uiMainTabHeight
                text: "概念图谱"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiMainTabFontSize
                font.bold: root.activeMainTabIndex === 0
                onClicked: root.requestMainTabChange(0)
                contentItem: Text {
                    text: tabConceptButton.text
                    font: tabConceptButton.font
                    color: root.activeMainTabIndex === 0 ? "#0f172a" : "#475569"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 4
                    color: root.activeMainTabIndex === 0 ? "#dbeafe"
                           : tabConceptButton.hovered ? "#eef7ff" : "#ffffff"
                    border.color: root.activeMainTabIndex === 0 ? root.uiFocusColor : "#9fb4c9"
                    border.width: root.activeMainTabIndex === 0 ? 1.6 : 1
                }
            }
            TabButton {
                id: tabWorkflowButton
                hoverEnabled: true
                height: root.uiMainTabHeight
                implicitHeight: root.uiMainTabHeight
                text: "流程编排"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiMainTabFontSize
                font.bold: root.activeMainTabIndex === 1
                onClicked: root.requestMainTabChange(1)
                contentItem: Text {
                    text: tabWorkflowButton.text
                    font: tabWorkflowButton.font
                    color: root.activeMainTabIndex === 1 ? "#0f172a" : "#475569"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 4
                    color: root.activeMainTabIndex === 1 ? "#dbeafe"
                           : tabWorkflowButton.hovered ? "#eef7ff" : "#ffffff"
                    border.color: root.activeMainTabIndex === 1 ? root.uiFocusColor : "#9fb4c9"
                    border.width: root.activeMainTabIndex === 1 ? 1.6 : 1
                }
            }
            TabButton {
                id: tabCommButton
                hoverEnabled: true
                height: root.uiMainTabHeight
                implicitHeight: root.uiMainTabHeight
                text: "通信接入"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiMainTabFontSize
                font.bold: root.activeMainTabIndex === 2
                onClicked: root.requestMainTabChange(2)
                contentItem: Text {
                    text: tabCommButton.text
                    font: tabCommButton.font
                    color: root.activeMainTabIndex === 2 ? "#0f172a" : "#475569"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 4
                    color: root.activeMainTabIndex === 2 ? "#dbeafe"
                           : tabCommButton.hovered ? "#eef7ff" : "#ffffff"
                    border.color: root.activeMainTabIndex === 2 ? root.uiFocusColor : "#9fb4c9"
                    border.width: root.activeMainTabIndex === 2 ? 1.6 : 1
                }
            }
            TabButton {
                id: tabPlanButton
                hoverEnabled: true
                height: root.uiMainTabHeight
                implicitHeight: root.uiMainTabHeight
                text: "方案生成"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiMainTabFontSize
                font.bold: root.activeMainTabIndex === 3
                onClicked: root.requestMainTabChange(3)
                contentItem: Text {
                    text: tabPlanButton.text
                    font: tabPlanButton.font
                    color: root.activeMainTabIndex === 3 ? "#0f172a" : "#475569"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 4
                    color: root.activeMainTabIndex === 3 ? "#dbeafe"
                           : tabPlanButton.hovered ? "#eef7ff" : "#ffffff"
                    border.color: root.activeMainTabIndex === 3 ? root.uiFocusColor : "#9fb4c9"
                    border.width: root.activeMainTabIndex === 3 ? 1.6 : 1
                }
            }
            TabButton {
                id: tabAssessmentButton
                hoverEnabled: true
                height: root.uiMainTabHeight
                implicitHeight: root.uiMainTabHeight
                text: "效能评估"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiMainTabFontSize
                font.bold: root.activeMainTabIndex === 4
                onClicked: root.requestMainTabChange(4)
                contentItem: Text {
                    text: tabAssessmentButton.text
                    font: tabAssessmentButton.font
                    color: root.activeMainTabIndex === 4 ? "#0f172a" : "#475569"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 4
                    color: root.activeMainTabIndex === 4 ? "#dbeafe"
                           : tabAssessmentButton.hovered ? "#eef7ff" : "#ffffff"
                    border.color: root.activeMainTabIndex === 4 ? root.uiFocusColor : "#9fb4c9"
                    border.width: root.activeMainTabIndex === 4 ? 1.6 : 1
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.activeMainTabIndex

            Loader {
                id: tabInitialSituation
                active: true
                sourceComponent: root.workflowInputMode === "afsim" ? afsimTestPageComponent : initialSituationPageComponent
            }

            Loader {
                id: tabConcept
                active: root.activeMainTabIndex === 1 || root.conceptPageLoaded
                sourceComponent: conceptGraphPageComponent
            }

            Loader {
                id: tabWorkflow
                active: root.activeMainTabIndex === 2 || root.workflowPageLoaded
                sourceComponent: workflowOrchestrationPageComponent
            }

            Loader {
                id: tabSitMonitor
                active: root.activeMainTabIndex === 3 || root.sitMonitorPageLoaded
                sourceComponent: sitMonitorPageComponent
            }

            PlaceholderPage { text: "COA比较" }
            PlaceholderPage { text: "效能评估" }
        }
    }
    }

    Dialog {
        id: workflowUnsavedDialog
        modal: true
        dim: true
        closePolicy: Popup.NoAutoClose
        width: 500
        height: 210
        padding: 18
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        title: ""
        header: null

        background: Item {
            property real dragPressX: 0
            property real dragPressY: 0

            Item {
                anchors.fill: parent
                Image {
                    anchors.fill: parent
                    source: "qrc:/UiICGAS/msg_box.png"
                    fillMode: Image.Stretch
                    cache: false
                }
            }
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: "transparent"
                border.color: root.uiBorderColor
                border.width: 1
            }
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    parent.dragPressX = mouse.x
                    parent.dragPressY = mouse.y
                }
                onPositionChanged: function(mouse) {
                    if (!pressed)
                        return
                    workflowUnsavedDialog.x += mouse.x - parent.dragPressX
                    workflowUnsavedDialog.y += mouse.y - parent.dragPressY
                }
            }
        }

        contentItem: ColumnLayout {
            width: 444
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: "存在未保存内容"
                color: "#0f172a"
                font.family: root.uiFontFamily
                font.pixelSize: 16
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: root.workflowUnsavedPromptMessage
                color: "#475569"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiFontSize
                wrapMode: Text.Wrap
            }
        }

        footer: Item {
            implicitHeight: 58
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 8
                anchors.bottomMargin: 18
                spacing: 8
                Item { Layout.fillWidth: true }
                UiButton {
                    Layout.preferredWidth: 120
                    text: "返回"
                    onClicked: workflowUnsavedDialog.close()
                }
            }
        }
    }

    Dialog {
        id: conceptDetailUnsavedDialog
        modal: true
        dim: true
        closePolicy: Popup.NoAutoClose
        width: 450
        height: 150
        padding: 18
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        title: ""
        header: null

        background: Item {
            property real dragPressX: 0
            property real dragPressY: 0

            Item {
                anchors.fill: parent
                Image {
                    anchors.fill: parent
                    source: "qrc:/UiICGAS/msg_box.png"
                    fillMode: Image.Stretch
                    cache: false
                }
            }
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: "transparent"
                border.color: root.uiBorderColor
                border.width: 1
            }
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    parent.dragPressX = mouse.x
                    parent.dragPressY = mouse.y
                }
                onPositionChanged: function(mouse) {
                    if (!pressed)
                        return
                    conceptDetailUnsavedDialog.x += mouse.x - parent.dragPressX
                    conceptDetailUnsavedDialog.y += mouse.y - parent.dragPressY
                }
            }
        }

        contentItem: ColumnLayout {
            width: 374
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: "当前详情尚未应用"
                color: "#0f172a"
                font.family: root.uiFontFamily
                font.pixelSize: 16
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: root.conceptDetailPromptMessage
                color: "#475569"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiFontSize
                wrapMode: Text.Wrap
            }
        }

        footer: Item {
            implicitHeight: 58
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 8
                anchors.bottomMargin: 18
                spacing: 8
                UiButton {
                    Layout.fillWidth: true
                    text: "应用"
                    onClicked: root.resolveConceptDetailPrompt("save")
                }
                UiButton {
                    Layout.fillWidth: true
                    text: "放弃"
                    onClicked: root.resolveConceptDetailPrompt("discard")
                }
                UiButton {
                    Layout.fillWidth: true
                    text: "返回"
                    onClicked: root.resolveConceptDetailPrompt("cancel")
                }
            }
        }
    }

    Dialog {
        id: conceptConfigUnsavedDialog
        modal: true
        dim: true
        closePolicy: Popup.NoAutoClose
        width: 450
        height: 150
        padding: 18
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        title: ""
        header: null

        background: Item {
            property real dragPressX: 0
            property real dragPressY: 0

            Item {
                anchors.fill: parent
                Image {
                    anchors.fill: parent
                    source: "qrc:/UiICGAS/msg_box.png"
                    fillMode: Image.Stretch
                    cache: false
                }
            }
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: "transparent"
                border.color: root.uiBorderColor
                border.width: 1
            }
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                onPressed: function(mouse) {
                    parent.dragPressX = mouse.x
                    parent.dragPressY = mouse.y
                }
                onPositionChanged: function(mouse) {
                    if (!pressed)
                        return
                    conceptConfigUnsavedDialog.x += mouse.x - parent.dragPressX
                    conceptConfigUnsavedDialog.y += mouse.y - parent.dragPressY
                }
            }
        }

        contentItem: ColumnLayout {
            width: 394
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: "配置修改尚未写入"
                color: "#0f172a"
                font.family: root.uiFontFamily
                font.pixelSize: 16
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: root.conceptConfigPromptMessage
                color: "#475569"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiFontSize
                wrapMode: Text.Wrap
            }
        }

        footer: Item {
            implicitHeight: 58
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 8
                anchors.bottomMargin: 18
                spacing: 8
                UiButton {
                    Layout.fillWidth: true
                    text: "应用"
                    onClicked: root.resolveConceptConfigPrompt("save")
                }
                UiButton {
                    Layout.fillWidth: true
                    text: "放弃"
                    onClicked: root.resolveConceptConfigPrompt("discard")
                }
                UiButton {
                    Layout.fillWidth: true
                    text: "返回"
                    onClicked: root.resolveConceptConfigPrompt("cancel")
                }
            }
        }
    }

    ResizeHandle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.resizeMargin
        edgeFlags: Qt.TopEdge
        cursorShape: Qt.SizeVerCursor
    }

    ResizeHandle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.resizeMargin
        edgeFlags: Qt.BottomEdge
        cursorShape: Qt.SizeVerCursor
    }

    ResizeHandle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.resizeMargin
        edgeFlags: Qt.LeftEdge
        cursorShape: Qt.SizeHorCursor
    }

    ResizeHandle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.resizeMargin
        edgeFlags: Qt.RightEdge
        cursorShape: Qt.SizeHorCursor
    }

    ResizeHandle {
        anchors.left: parent.left
        anchors.top: parent.top
        width: root.resizeCornerSize
        height: root.resizeCornerSize
        edgeFlags: Qt.LeftEdge | Qt.TopEdge
        cursorShape: Qt.SizeFDiagCursor
    }

    ResizeHandle {
        anchors.right: parent.right
        anchors.top: parent.top
        width: root.resizeCornerSize
        height: root.resizeCornerSize
        edgeFlags: Qt.RightEdge | Qt.TopEdge
        cursorShape: Qt.SizeBDiagCursor
    }

    ResizeHandle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: root.resizeCornerSize
        height: root.resizeCornerSize
        edgeFlags: Qt.LeftEdge | Qt.BottomEdge
        cursorShape: Qt.SizeBDiagCursor
    }

    ResizeHandle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: root.resizeCornerSize
        height: root.resizeCornerSize
        edgeFlags: Qt.RightEdge | Qt.BottomEdge
        cursorShape: Qt.SizeFDiagCursor
    }

    Component {
        id: afsimTestPageComponent

        UiAFSimTest {
            anchors.fill: parent
            backend: uiAFSimTest
        }
    }

    Component {
        id: initialSituationPageComponent

        UiScenario {
            anchors.fill: parent
            appRoot: root
            controller: uiScenario
        }
    }

    Component {
        id: conceptGraphPageComponent

        UiGraph {
            anchors.fill: parent
            appRoot: root
            controller: uiGraph
        }
    }
    
    Component {
        id: workflowOrchestrationPageComponent

        UiWorkflow {
            anchors.fill: parent
            appRoot: root
            controller: uiWorkflow
        }
    }

    Component {
        id: sitMonitorPageComponent

        UiSitMonitor {
            anchors.fill: parent
            appRoot: root
            controller: uiSitMonitor
        }
    }

    component ResizeHandle: MouseArea {
        property int edgeFlags: Qt.RightEdge
        property var targetWindow: root

        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        preventStealing: true
        z: 100
        onPressed: function(mouse) {
            targetWindow.beginAspectResize(edgeFlags, mapToGlobal(mouse.x, mouse.y))
            mouse.accepted = true
        }
        onPositionChanged: function(mouse) {
            targetWindow.updateAspectResize(mapToGlobal(mouse.x, mouse.y))
            mouse.accepted = true
        }
        onReleased: targetWindow.endAspectResize()
        onCanceled: targetWindow.endAspectResize()
    }

    component UiButton: Button {
        id: buttonControl
        property bool visualChecked: checked

        hoverEnabled: true
        implicitHeight: 32
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize
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
                          : buttonControl.visualChecked || buttonControl.activeFocus ? root.uiFocusColor
                          : root.uiBorderColor
            border.width: buttonControl.visualChecked || buttonControl.activeFocus ? 1.4 : 1
        }
    }

    component ToolbarSegmentButton: Button {
        id: segmentButton
        property bool visualChecked: checked
        property bool leftSegment: false
        property bool rightSegment: false

        hoverEnabled: true
        implicitHeight: 36
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize
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

    component ToolbarNavButton: Button {
        id: navButton
        property bool visualChecked: checked

        hoverEnabled: true
        implicitHeight: 34
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize
        font.bold: visualChecked
        padding: 0

        contentItem: Text {
            text: navButton.text
            font: navButton.font
            color: navButton.visualChecked ? "#0b67d9" : "#475569"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 3
            color: navButton.visualChecked ? "#ffffff"
                   : navButton.down ? "#dde5ee"
                   : navButton.hovered ? "#f5f8fc" : "transparent"
            border.color: navButton.visualChecked ? "#2f80ed" : "transparent"
            border.width: navButton.visualChecked ? 1 : 0
        }
    }

    component ToolbarActionButton: Button {
        id: actionButton
        property bool visualChecked: checked
        property string iconType: ""

        hoverEnabled: true
        implicitHeight: 36
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize
        font.bold: visualChecked
        padding: 0

        contentItem: Row {
            spacing: 5
            leftPadding: 10
            rightPadding: 10

            Canvas {
                id: toolbarActionIcon
                width: 14
                height: 14
                anchors.verticalCenter: parent.verticalCenter

                onPaint: {
                    var ctx = getContext("2d")
                    var color = !actionButton.enabled && !actionButton.visualChecked ? "#94a3b8"
                              : actionButton.visualChecked ? "#0b67d9" : "#0f172a"
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = color
                    ctx.fillStyle = color
                    ctx.lineWidth = 1.6
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"

                    if (actionButton.iconType === "run") {
                        ctx.beginPath()
                        ctx.moveTo(4, 2.5)
                        ctx.lineTo(11, 7)
                        ctx.lineTo(4, 11.5)
                        ctx.closePath()
                        ctx.fill()
                    } else if (actionButton.iconType === "pause") {
                        ctx.fillRect(4, 3, 2, 8)
                        ctx.fillRect(8, 3, 2, 8)
                    } else if (actionButton.iconType === "step") {
                        ctx.beginPath()
                        ctx.moveTo(3, 2.5)
                        ctx.lineTo(9, 7)
                        ctx.lineTo(3, 11.5)
                        ctx.closePath()
                        ctx.fill()
                        ctx.beginPath()
                        ctx.moveTo(11, 3)
                        ctx.lineTo(11, 11)
                        ctx.stroke()
                    } else if (actionButton.iconType === "stop") {
                        ctx.fillRect(3.5, 3.5, 7, 7)
                    } else if (actionButton.iconType === "save") {
                        ctx.beginPath()
                        ctx.rect(2.5, 2.5, 9, 9)
                        ctx.stroke()
                        ctx.fillRect(4, 2.5, 5, 3)
                        ctx.clearRect(5, 8, 4, 3.5)
                        ctx.strokeRect(5, 8, 4, 3.5)
                    }
                }

                Connections {
                    target: actionButton
                    function onVisualCheckedChanged() { toolbarActionIcon.requestPaint() }
                    function onEnabledChanged() { toolbarActionIcon.requestPaint() }
                    function onHoveredChanged() { toolbarActionIcon.requestPaint() }
                    function onDownChanged() { toolbarActionIcon.requestPaint() }
                }
            }

            Text {
                text: actionButton.text
                color: !actionButton.enabled && !actionButton.visualChecked ? "#94a3b8"
                     : actionButton.visualChecked ? "#0b67d9" : "#0f172a"
                font: actionButton.font
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
            }
        }

        background: Rectangle {
            radius: 4
            color: !actionButton.enabled && !actionButton.visualChecked ? "#f1f5f9"
                   : actionButton.visualChecked ? "#e8f2ff"
                   : actionButton.down ? "#e2e8f0"
                   : actionButton.hovered ? "#f8fbff" : "#ffffff"
            border.color: !actionButton.enabled && !actionButton.visualChecked ? "#d7dee8"
                        : actionButton.visualChecked ? "#2f80ed" : "#cbd5e1"
            border.width: 1
        }
    }

    component WindowControlButton: Button {
        id: windowButton
        property string iconType: "minimize"

        hoverEnabled: true
        implicitWidth: 38
        implicitHeight: 30
        padding: 0
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        contentItem: Item {
            implicitWidth: 38
            implicitHeight: 30

            Canvas {
                id: windowButtonIcon
                anchors.centerIn: parent
                width: 18
                height: 18
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = windowButton.down ? "#0b4f86" : "#0f172a"
                    ctx.lineWidth = 1.8
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"

                    if (windowButton.iconType === "minimize") {
                        ctx.beginPath()
                        ctx.moveTo(4, 9)
                        ctx.lineTo(14, 9)
                        ctx.stroke()
                    } else if (windowButton.iconType === "maximize") {
                        ctx.strokeRect(5, 5, 8, 8)
                    } else if (windowButton.iconType === "restore") {
                        ctx.strokeRect(4, 7, 7, 7)
                        ctx.beginPath()
                        ctx.moveTo(7, 4)
                        ctx.lineTo(14, 4)
                        ctx.lineTo(14, 11)
                        ctx.stroke()
                    } else {
                        ctx.beginPath()
                        ctx.moveTo(5, 5)
                        ctx.lineTo(13, 13)
                        ctx.moveTo(13, 5)
                        ctx.lineTo(5, 13)
                        ctx.stroke()
                    }
                }

                Connections {
                    target: windowButton
                    function onIconTypeChanged() { windowButtonIcon.requestPaint() }
                    function onDownChanged() { windowButtonIcon.requestPaint() }
                    function onHoveredChanged() { windowButtonIcon.requestPaint() }
                }
            }
        }

        background: Rectangle {
            radius: 4
            color: windowButton.down ? "#d7ecfb"
                   : windowButton.hovered ? "#eef7ff" : "#f8fafc"
            border.color: windowButton.activeFocus ? root.uiFocusColor : root.uiBorderColor
            border.width: windowButton.activeFocus ? 1.4 : 1
        }
    }

    component UiTextField: TextField {
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
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize
        color: !enabled ? "#94a3b8" : readOnly ? "#334155" : "#0f172a"
        selectedTextColor: "#ffffff"
        selectionColor: "#2563eb"

        background: Rectangle {
            radius: 4
            color: !textFieldControl.enabled ? "#eef2f7"
                   : textFieldControl.readOnly ? textFieldControl.readOnlyColor : textFieldControl.normalColor
            border.color: textFieldControl.warn ? "#f97316"
                          : textFieldControl.activeFocus ? root.uiFocusColor : root.uiBorderColor
            border.width: textFieldControl.warn || textFieldControl.activeFocus ? 1.4 : 1
        }
    }

    component UiComboBox: ComboBox {
        id: comboControl
        property color normalColor: "#ffffff"
        property bool showArrow: true

        implicitHeight: 32
        leftPadding: 8
        rightPadding: comboControl.showArrow ? 30 : 8
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize

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
            border.color: comboControl.activeFocus || comboControl.popup.visible ? root.uiFocusColor : root.uiBorderColor
            border.width: comboControl.activeFocus || comboControl.popup.visible ? 1.4 : 1
        }

        delegate: ItemDelegate {
            id: comboDelegate
            required property var modelData
            required property int index

            width: comboControl.width
            text: String(modelData)
            font.family: root.uiFontFamily
            font.pixelSize: root.uiFontSize
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
                border.color: root.uiBorderColor
                border.width: 1
            }
        }
    }

    component UiCheckBox: CheckBox {
        id: checkControl

        hoverEnabled: true
        implicitWidth: String(text).length === 0 ? 22 : indicator.width + spacing + contentItem.implicitWidth
        implicitHeight: 24
        spacing: 6
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize

        indicator: Rectangle {
            x: String(checkControl.text).length === 0 ? (checkControl.width - width) / 2 : 0
            y: (checkControl.height - height) / 2
            implicitWidth: 16
            implicitHeight: 16
            radius: 3
            color: !checkControl.enabled ? "#eef2f7"
                   : checkControl.checked ? "#1976bd" : "#ffffff"
            border.color: !checkControl.enabled ? "#d5dde6"
                          : checkControl.activeFocus ? root.uiFocusColor : root.uiBorderColor
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

    component PduLogGrid: GridLayout {
        id: pduLogGrid
        property var rows: []
        columns: 2
        columnSpacing: 12
        rowSpacing: 8
        clip: true

        Repeater {
            model: pduLogGrid.rows
            delegate: Label {
                required property string modelData
                Layout.fillWidth: true
                Layout.preferredWidth: Math.max(0, (pduLogGrid.width - pduLogGrid.columnSpacing) / 2)
                Layout.preferredHeight: 24
                text: modelData
                color: "#111827"
                font.family: root.uiFontFamily
                font.pixelSize: root.uiSmallFontSize
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    component LogList: ListView {
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
            font.family: root.uiFontFamily
            font.pixelSize: root.uiSmallFontSize
            elide: Text.ElideRight
        }
        onCountChanged: positionViewAtEnd()
    }

    component PlaceholderPage: Rectangle {
        property string text: ""
        color: "#80ffffff"
        radius: 6
        border.color: root.uiBorderColor
        border.width: 1
        Label {
            anchors.centerIn: parent
            text: parent.text
            color: "#475569"
            font.family: root.uiFontFamily
            font.pixelSize: 20
        }
    }

}
