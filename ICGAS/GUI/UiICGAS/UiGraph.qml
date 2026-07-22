import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Window 6.8
import ICGAS.Ui 1.0
pragma ComponentBehavior: Bound

Item {
id: conceptPage
property var appRoot: null
property var controller: uiGraph
readonly property var backend: controller && controller.backend ? controller.backend : uiICGAS
readonly property string uiFontFamily: appRoot ? appRoot.uiFontFamily : "宋体"
readonly property int uiFontSize: appRoot ? appRoot.uiFontSize : 14
readonly property int uiTitleFontSize: appRoot ? appRoot.uiTitleFontSize : 17
readonly property color uiBorderColor: appRoot ? appRoot.uiBorderColor : "#b8c7d6"
readonly property color uiSoftBorderColor: appRoot ? appRoot.uiSoftBorderColor : "#d6e0ea"
readonly property color uiFocusColor: appRoot ? appRoot.uiFocusColor : "#2f80c4"
readonly property real graphLaneTitleWidth: appRoot ? appRoot.graphLaneTitleWidth : 108
readonly property real graphLaneContentPadding: appRoot ? appRoot.graphLaneContentPadding : 16
readonly property real graphLaneVerticalPadding: appRoot ? appRoot.graphLaneVerticalPadding : 8
property var conceptGraphNodeItems: ({})
property int conceptGraphNodeTokenSeed: 0
property string selectedGraphNodeId: ""
property var selectedGraphHighlightIds: ({})
property var pendingGraphNodePositions: ({})
property string activeGraphLaneType: ""
property real activeGraphLaneOffset: 0
property bool conceptGraphCanvasReady: false
    
    
function requestConceptDetailChange(action) { appRoot.requestConceptDetailChange(action) }
function requestShowConceptOverview(typeText) { appRoot.requestShowConceptOverview(typeText) }
function requestShowConceptNode(nodeId) { appRoot.requestShowConceptNode(nodeId) }
function requestConceptScenarioChange(scenario) { appRoot.requestConceptScenarioChange(scenario) }
    
function registerConceptGraphNode(nodeId, token, item) {
    updateConceptGraphNodeGeometry(nodeId, token, item)
    requestGraphPaint()
}
    
function updateConceptGraphNodeGeometry(nodeId, token, item) {
    var targetId = String(nodeId || "")
    if (targetId === "" || !item) {
        return
    }
    
    conceptGraphNodeItems[targetId] = ({
        token: token,
        x: Number(item.x),
        y: Number(item.y),
        width: Number(item.width),
        height: Number(item.height)
    })
}
    
function unregisterConceptGraphNode(nodeId, token) {
    var targetId = String(nodeId || "")
    var item = conceptGraphNodeItems[targetId]
    if (item && Number(item.token) === Number(token)) {
        delete conceptGraphNodeItems[targetId]
        requestGraphPaint()
    }
}
    
function clearConceptGraphRuntimeState() {
    conceptGraphNodeItems = ({})
    pendingGraphNodePositions = ({})
    selectedGraphNodeId = ""
    selectedGraphHighlightIds = ({})
    requestGraphPaint()
}

function setPendingGraphNodePosition(kind, nodeId, x, y) {
    var targetId = String(nodeId || "")
    if (targetId === "")
        return
    pendingGraphNodePositions[targetId] = ({
        "kind": String(kind || ""),
        "nodeId": targetId,
        "x": Number(x),
        "y": Number(y)
    })
}

function flushPendingGraphNodePositions() {
    var rows = []
    for (var nodeId in pendingGraphNodePositions) {
        rows.push(pendingGraphNodePositions[nodeId])
    }
    pendingGraphNodePositions = ({})
    if (rows.length > 0)
        backend.setConceptGraphNodePositions(rows)
}
    
function graphNodeExists(nodeId) {
    var targetId = String(nodeId || "")
    if (targetId === "") {
        return false
    }
    
    for (var i = 0; i < backend.conceptGraphNodes.length; ++i) {
        var node = backend.conceptGraphNodes[i]
        if (String(graphValue(node, "nodeId", "")) === targetId) {
            return true
        }
    }
    return false
}
    
function rebuildGraphSelection(nodeId) {
    var targetId = String(nodeId || "")
    var highlightIds = ({})
    
    if (targetId !== "") {
        highlightIds[targetId] = true
        for (var i = 0; i < backend.conceptGraphEdges.length; ++i) {
            var edge = backend.conceptGraphEdges[i]
            var sourceId = String(graphValue(edge, "sourceId", ""))
            var targetNodeId = String(graphValue(edge, "targetId", ""))
            if (sourceId === targetId || targetNodeId === targetId) {
                highlightIds[sourceId] = true
                highlightIds[targetNodeId] = true
            }
        }
    }
    
    selectedGraphNodeId = targetId
    selectedGraphHighlightIds = highlightIds
}
    
function selectGraphNode(nodeId) {
    rebuildGraphSelection(nodeId)
    requestGraphPaint()
}
    
function clearGraphSelection() {
    rebuildGraphSelection("")
    requestGraphPaint()
}
    
function syncGraphSelection() {
    if (selectedGraphNodeId === "") {
        if (selectedGraphHighlightIds && Object.keys(selectedGraphHighlightIds).length > 0) {
            selectedGraphHighlightIds = ({})
        }
        return
    }
    
    if (!graphNodeExists(selectedGraphNodeId)) {
        rebuildGraphSelection("")
        return
    }
    
    rebuildGraphSelection(selectedGraphNodeId)
}
    
function graphNodeIsSelected(nodeId) {
    return selectedGraphNodeId !== "" && String(nodeId || "") === selectedGraphNodeId
}
    
function graphNodeIsConnected(nodeId) {
    var targetId = String(nodeId || "")
    return selectedGraphNodeId !== "" &&
            targetId !== "" &&
            targetId !== selectedGraphNodeId &&
            selectedGraphHighlightIds[targetId] === true
}
    
function graphEdgeIsSelected(edge) {
    if (selectedGraphNodeId === "") {
        return false
    }
    
    var sourceId = String(graphValue(edge, "sourceId", ""))
    var targetId = String(graphValue(edge, "targetId", ""))
    return sourceId === selectedGraphNodeId || targetId === selectedGraphNodeId
}
    
function graphNodeCenter(nodeId) {
    var item = conceptGraphNodeItems[String(nodeId)]
    if (!item || item.x === undefined) {
        return Qt.point(-1, -1)
    }
    return Qt.point(item.x + item.width / 2, item.y + item.height / 2)
}
    
function graphNodeAnchor(nodeId, toward) {
    var item = conceptGraphNodeItems[String(nodeId)]
    if (!item || item.x === undefined || !toward || toward.x < 0) {
        return Qt.point(-1, -1)
    }
    
    var center = Qt.point(item.x + item.width / 2, item.y + item.height / 2)
    var dx = toward.x - center.x
    var dy = toward.y - center.y
    if (Math.abs(dx) < 0.01 && Math.abs(dy) < 0.01) {
        return center
    }
    
    var scaleX = Math.abs(dx) < 0.01 ? 999999 : item.width / (2 * Math.abs(dx))
    var scaleY = Math.abs(dy) < 0.01 ? 999999 : item.height / (2 * Math.abs(dy))
    var scale = Math.min(scaleX, scaleY)
    return Qt.point(center.x + dx * scale, center.y + dy * scale)
}

function graphSelfLoopPoints(nodeId) {
    var item = conceptGraphNodeItems[String(nodeId)]
    if (!item || item.x === undefined) {
        return null
    }

    var radius = Math.max(24, Math.min(42, item.height * 0.48))
    var centerX = item.x + item.width / 2
    var centerY = item.y + item.height + radius * 0.55
    return {
        "center": Qt.point(centerX, centerY),
        "radius": radius,
        "startAngle": -Math.PI * 0.72,
        "endAngle": Math.PI * 1.12,
        "end": Qt.point(centerX + Math.cos(Math.PI * 1.12) * radius,
                        centerY + Math.sin(Math.PI * 1.12) * radius),
        "label": Qt.point(centerX + radius + 8, centerY + 4)
    }
}
    
function graphLaneByType(typeText) {
    for (var i = 0; i < backend.conceptGraphLanes.length; ++i) {
        var lane = backend.conceptGraphLanes[i]
        if (String(graphValue(lane, "laneType", "")) === String(typeText)) {
            return lane
        }
    }
    return null
}
    
function requestGraphPaint() {
    if (conceptGraphCanvasReady && edgeCanvas)
        Qt.callLater(edgeCanvas.requestPaint)
}
    
function graphLaneOffset(typeText) {
    return activeGraphLaneType === String(typeText) ? activeGraphLaneOffset : 0
}
    
function graphLaneNodeMinX(lane) {
    return Number(graphValue(lane, "laneX", 0))
            + Number(graphValue(lane, "laneTitleW", graphLaneTitleWidth))
            + Number(graphValue(lane, "laneContentPadding", graphLaneContentPadding))
}
    
function graphLaneNodeMaxX(lane, nodeWidth) {
    var laneX = Number(graphValue(lane, "laneX", 0))
    var laneW = Number(graphValue(lane, "laneW", graphContent ? graphContent.width : 0))
    var padding = Number(graphValue(lane, "laneContentPadding", graphLaneContentPadding))
    return Math.max(graphLaneNodeMinX(lane), laneX + laneW - padding - nodeWidth)
}
    
function graphLaneNodeMinY(lane) {
    return Number(graphValue(lane, "laneY", 0))
            + Number(graphValue(lane, "laneVerticalPadding", graphLaneVerticalPadding))
}
    
function graphLaneNodeMaxY(lane, nodeHeight) {
    var laneY = Number(graphValue(lane, "laneY", 0))
    var laneH = Number(graphValue(lane, "laneH", graphContent ? graphContent.height : 0))
    var padding = Number(graphValue(lane, "laneVerticalPadding", graphLaneVerticalPadding))
    return Math.max(graphLaneNodeMinY(lane), laneY + laneH - padding - nodeHeight)
}

function clampGraphValue(value, minValue, maxValue) {
    return Math.max(minValue, Math.min(maxValue, Number(value)))
}

function overviewNodeX(nodeX, nodeW) {
    var maxX = Math.max(0, (graphContent ? graphContent.width : 0) - Number(nodeW))
    return clampGraphValue(nodeX, 0, maxX)
}

function overviewNodeY(nodeY, nodeH) {
    var maxY = Math.max(0, (graphContent ? graphContent.height : 0) - Number(nodeH))
    return clampGraphValue(nodeY, 0, maxY)
}
    
function conceptLaneDropIndex(centerY) {
    var target = 0
    for (var i = 0; i < backend.conceptGraphLanes.length; ++i) {
        var lane = backend.conceptGraphLanes[i]
        var laneType = String(graphValue(lane, "laneType", ""))
        if (laneType === activeGraphLaneType) {
            continue
        }
        if (centerY > Number(graphValue(lane, "laneY", 0)) + Number(graphValue(lane, "laneH", 0)) / 2) {
            target += 1
        }
    }
    return target
}
    
function graphValue(row, name, fallbackValue) {
    if (row && row[name] !== undefined && row[name] !== null) {
        return row[name]
    }
    return fallbackValue
}
ColumnLayout {
    anchors.fill: parent
    spacing: 8
    
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 72
        radius: 6
        color: "#bfffffff"
        border.color: conceptPage.uiBorderColor
        border.width: 1
    
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8
    
            Label {
                text: "概念图谱"
                font.bold: true
                font.family: conceptPage.uiFontFamily
                font.pixelSize: 18
                color: "#0f172a"
            }
    
            Label {
                text: "当前场景"
                color: "#475569"
                font.family: conceptPage.uiFontFamily
                font.pixelSize: conceptPage.uiFontSize
            }
    
            ConceptUiComboBox {
                id: comboConcept
                Layout.preferredWidth: 150
                model: backend.conceptScenarioNames
                currentIndex: Math.max(0, backend.conceptScenarioNames.indexOf(backend.selectedConceptScenario))
                onActivated: conceptPage.requestConceptScenarioChange(currentText)
            }
    
            Repeater {
                model: backend.conceptScenarioRows
                delegate: RowLayout {
                    required property string name
                    required property string value
                    spacing: 4
                    Label {
                        text: name
                        color: "#475569"
                        font.family: conceptPage.uiFontFamily
                        font.pixelSize: conceptPage.uiFontSize
                    }
                    ConceptUiTextField {
                        text: value
                        readOnly: true
                        Layout.preferredWidth: name === "场景名称" ? 200 : 82
                    }
                }
            }
    
            ConceptUiTextField {
                text: backend.conceptStatus
                readOnly: true
                Layout.preferredWidth: 128
                color: text.indexOf("失败") >= 0 || text.indexOf("错误") >= 0 ? "#b91c1c" : "#166534"
            }
    
            ConceptUiButton {
                text: "重置配置"
                enabled: backend.conceptDirty || backend.conceptPendingDetailChanges
                Layout.preferredWidth: 92
                onClicked: backend.resetConceptConfig()
            }
    
            ConceptUiButton {
                text: "确认配置"
                Layout.preferredWidth: 92
                onClicked: backend.confirmConceptConfig()
            }
        }
    }
    
    SplitView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        orientation: Qt.Horizontal
        handle: Item {
            implicitWidth: 0
            implicitHeight: 0
        }
    
        Rectangle {
            SplitView.preferredWidth: 250
            SplitView.minimumWidth: 250
            SplitView.maximumWidth: 250
            radius: 6
            color: "#a6ffffff"
            border.color: conceptPage.uiBorderColor
            border.width: 1
    
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
    
                ConceptFilterGroup {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 158
                    title: "导航"
    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.topMargin: 34
                        spacing: 6
    
                        ConceptUiButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            text: "总览导航"
                            visualChecked: backend.conceptOverviewMode
                            onClicked: conceptPage.requestConceptDetailChange(function() { backend.setConceptNavMode("overview") })
                        }
                        ConceptUiButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            text: "概念图谱"
                            visualChecked: !backend.conceptOverviewMode
                            onClicked: conceptPage.requestConceptDetailChange(function() { backend.setConceptNavMode("graph") })
                        }
                        ConceptUiButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            text: backend.conceptFocusMode ? "退出聚焦模式" : "聚焦模式"
                            checkable: true
                            checked: backend.conceptFocusMode
                            enabled: !backend.conceptOverviewMode && backend.conceptFocusMode
                            opacity: enabled ? 1.0 : 0.55
                            onClicked: function() {
                                if (!backend.conceptFocusMode)
                                    return
                                conceptPage.clearConceptGraphRuntimeState()
                                backend.setConceptFocusMode(false)
                                conceptPage.clearGraphSelection()
                            }
                        }
                    }
                }
    
                ConceptFilterGroup {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 472
                    title: "类型筛选"
    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.topMargin: 34
                        spacing: 8
    
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.minimumHeight: 34
                            Layout.preferredHeight: 34
                            Layout.maximumHeight: 34
                            Layout.fillHeight: false
                            spacing: 8
                            ConceptUiButton {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.minimumHeight: 34
                                Layout.preferredHeight: 34
                                Layout.maximumHeight: 34
                                Layout.fillHeight: false
                                text: "节点类型"
                                checkable: true
                                checked: backend.conceptTypeFilterPage === "node"
                                onClicked: backend.setConceptTypeFilterPage("node")
                            }
                            ConceptUiButton {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                text: "边类型"
                                checkable: true
                                checked: backend.conceptTypeFilterPage === "edge"
                                Layout.minimumHeight: 34
                                Layout.preferredHeight: 34
                                Layout.maximumHeight: 34
                                Layout.fillHeight: false
                                onClicked: backend.setConceptTypeFilterPage("edge")
                            }
                        }
    
                        ConceptTypeTable {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight
                            Layout.alignment: Qt.AlignTop
                            rows: backend.conceptTypeFilterPage === "node"
                                  ? backend.conceptNodeTypeRows
                                  : backend.conceptEdgeTypeRows
                            nodeTable: backend.conceptTypeFilterPage === "node"
                        }
    
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }
                }
    
                ConceptFilterGroup {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 176
                    enabled: !backend.conceptOverviewMode
                    opacity: enabled ? 1.0 : 0.55
                    title: "字段筛选"
    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.topMargin: 34
                        spacing: 0
    
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            Label { Layout.preferredWidth: 70; text: "字段"; color: "#475569"; font.family: conceptPage.uiFontFamily; font.pixelSize: conceptPage.uiFontSize; horizontalAlignment: Text.AlignHCenter }
                            Label { Layout.fillWidth: true; text: "节点"; color: "#475569"; font.family: conceptPage.uiFontFamily; font.pixelSize: conceptPage.uiFontSize; horizontalAlignment: Text.AlignHCenter }
                            Label { Layout.fillWidth: true; text: "边"; color: "#475569"; font.family: conceptPage.uiFontFamily; font.pixelSize: conceptPage.uiFontSize; horizontalAlignment: Text.AlignHCenter }
                        }
    
                        Repeater {
                            model: backend.conceptFieldRows
                            delegate: RowLayout {
                                required property string label
                                required property string field
                                required property bool nodeChecked
                                required property bool nodeEnabled
                                required property bool edgeChecked
                                required property bool edgeEnabled
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                spacing: 4
                                Label {
                                    Layout.preferredWidth: 70
                                    text: label
                                    color: "#0f172a"
                                    font.family: conceptPage.uiFontFamily
                                    font.pixelSize: conceptPage.uiFontSize
                                    horizontalAlignment: Text.AlignHCenter
                                }
                                ConceptUiCheckBox {
                                    Layout.fillWidth: true
                                    checked: nodeChecked
                                    enabled: nodeEnabled
                                    onToggled: backend.setConceptFieldChecked("node", field, checked)
                                }
                                ConceptUiCheckBox {
                                    Layout.fillWidth: true
                                    checked: edgeChecked
                                    enabled: edgeEnabled
                                    onToggled: backend.setConceptFieldChecked("edge", field, checked)
                                }
                            }
                        }
                    }
                }
            }
        }
    
        Rectangle {
            SplitView.preferredWidth: 1180
            SplitView.minimumWidth: 1180
            SplitView.maximumWidth: 1180
            radius: 6
            color: "#8cffffff"
            border.color: conceptPage.uiBorderColor
            border.width: 1
    
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
    
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
    
                    Label {
                        text: backend.conceptGraphTitle
                        font.bold: true
                        font.family: conceptPage.uiFontFamily
                        font.pixelSize: 17
                        color: "#0f172a"
                    }
    
                    Label {
                        Layout.fillWidth: true
                        text: backend.conceptMissionText
                        color: "#475569"
                        font.family: conceptPage.uiFontFamily
                        font.pixelSize: conceptPage.uiFontSize
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
    
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 4
                        color: "#c8f8fafc"
                        border.color: conceptPage.uiBorderColor
                        border.width: 1
    
                        Flickable {
                            id: conceptGraphFlick
                            anchors.fill: parent
                            anchors.margins: 8
                            boundsBehavior: Flickable.StopAtBounds
                            interactive: false
                            flickableDirection: Flickable.VerticalFlick
                            contentWidth: width
                            contentHeight: backend.conceptOverviewMode
                                           ? height
                                           : Math.max(height, graphContent.height + 20)
                            clip: true
    
                            WheelHandler {
                                enabled: !backend.conceptOverviewMode
                                onWheel: function(event) {
                                    var maxY = Math.max(0, conceptGraphFlick.contentHeight - conceptGraphFlick.height)
                                    conceptGraphFlick.contentY = Math.max(0, Math.min(maxY,
                                        conceptGraphFlick.contentY - event.angleDelta.y))
                                    event.accepted = true
                                }
                            }
    
                            Item {
                                id: graphContent
                                width: conceptGraphFlick.width
                                height: backend.conceptOverviewMode
                                        ? conceptGraphFlick.height
                                        : Math.max(conceptGraphFlick.height, maxGraphBottom() + 96)
    
                                function maxGraphBottom() {
                                    var maxBottom = 0
                                    for (var i = 0; i < backend.conceptGraphLanes.length; ++i) {
                                        var lane = backend.conceptGraphLanes[i]
                                        maxBottom = Math.max(maxBottom, lane.laneY + lane.laneH)
                                    }
                                    for (var n = 0; n < backend.conceptGraphNodes.length; ++n) {
                                        var node = backend.conceptGraphNodes[n]
                                        maxBottom = Math.max(maxBottom,
                                            Number(conceptPage.graphValue(node, "nodeY", 0)) + Number(conceptPage.graphValue(node, "nodeH", 0)))
                                    }
                                    return maxBottom
                                }
    
                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.LeftButton
                                    cursorShape: Qt.ArrowCursor
                                    onClicked: conceptPage.clearGraphSelection()
                                }
    
                                Repeater {
                                    model: backend.conceptGraphLanes
                                    delegate: Rectangle {
                                        required property var modelData
                                        property string laneType: String(conceptPage.graphValue(modelData, "laneType", ""))
                                        property string laneName: String(conceptPage.graphValue(modelData, "laneName", ""))
                                        property real laneX: Number(conceptPage.graphValue(modelData, "laneX", 0))
                                        property real laneY: Number(conceptPage.graphValue(modelData, "laneY", 0))
                                        property real laneW: Number(conceptPage.graphValue(modelData, "laneW", 0))
                                        property real laneH: Number(conceptPage.graphValue(modelData, "laneH", 0))
                                        property real laneTitleW: Number(conceptPage.graphValue(modelData, "laneTitleW", conceptPage.graphLaneTitleWidth))
                                        property string fillColor: String(conceptPage.graphValue(modelData, "fillColor", "#EEF2F6"))
                                        property string borderColor: String(conceptPage.graphValue(modelData, "borderColor", "#5C728A"))
                                        visible: !backend.conceptFocusActive
                                        x: laneX
                                        y: laneY + conceptPage.graphLaneOffset(laneType)
                                        width: laneW
                                        height: laneH
                                        radius: 6
                                        color: "#44ffffff"
                                        border.color: borderColor
                                        border.width: 1.2
    
                                        Rectangle {
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            width: laneTitleW
                                            radius: 6
                                            color: fillColor
                                            border.color: borderColor
                                            border.width: 1.2
                                            Label {
                                                anchors.centerIn: parent
                                                text: laneType + "\n" + laneName
                                                color: "#0f172a"
                                                font.bold: true
                                                horizontalAlignment: Text.AlignHCenter
                                            }
                                        }
    
                                        MouseArea {
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            width: laneTitleW
                                            enabled: !backend.conceptOverviewMode && !backend.conceptFocusMode
                                            acceptedButtons: Qt.LeftButton
                                            cursorShape: Qt.SizeAllCursor
                                            preventStealing: true
                                            property real pressSceneY: 0
                                            property real originalLaneY: 0
                                            property bool laneDragMoved: false
                                            onPressed: function(mouse) {
                                                pressSceneY = mapToItem(graphContent, mouse.x, mouse.y).y
                                                originalLaneY = laneY
                                                laneDragMoved = false
                                                conceptPage.activeGraphLaneType = ""
                                                conceptPage.activeGraphLaneOffset = 0
                                            }
                                            onPositionChanged: function(mouse) {
                                                if (!pressed) {
                                                    return
                                                }
                                                var sceneY = mapToItem(graphContent, mouse.x, mouse.y).y
                                                var deltaY = sceneY - pressSceneY
                                                if (Math.abs(deltaY) > 4) {
                                                    laneDragMoved = true
                                                    conceptPage.activeGraphLaneType = laneType
                                                    conceptPage.activeGraphLaneOffset = deltaY
                                                    conceptPage.requestGraphPaint()
                                                }
                                            }
                                            onReleased: function(mouse) {
                                                var sceneY = mapToItem(graphContent, mouse.x, mouse.y).y
                                                var deltaY = sceneY - pressSceneY
                                                var shouldMoveLane = laneDragMoved || Math.abs(deltaY) > 4
                                                if (shouldMoveLane) {
                                                    conceptPage.activeGraphLaneType = laneType
                                                    conceptPage.activeGraphLaneOffset = deltaY
                                                }
                                                var targetIndex = shouldMoveLane
                                                        ? conceptPage.conceptLaneDropIndex(originalLaneY + deltaY + laneH / 2)
                                                        : -1
                                                conceptPage.activeGraphLaneType = ""
                                                conceptPage.activeGraphLaneOffset = 0
                                                if (shouldMoveLane) {
                                                    backend.moveConceptGraphLane(laneType, targetIndex)
                                                } else {
                                                    conceptPage.requestShowConceptOverview(laneType)
                                                }
                                                laneDragMoved = false
                                                conceptPage.requestGraphPaint()
                                            }
                                            onCanceled: {
                                                conceptPage.activeGraphLaneType = ""
                                                conceptPage.activeGraphLaneOffset = 0
                                                laneDragMoved = false
                                                conceptPage.requestGraphPaint()
                                            }
                                        }
                                    }
                                }
    
                            Canvas {
                                id: edgeCanvas
                                anchors.fill: parent
                                z: 2
                                Component.onCompleted: conceptPage.conceptGraphCanvasReady = true
                                Component.onDestruction: conceptPage.conceptGraphCanvasReady = false
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.clearRect(0, 0, width, height)
                                    ctx.lineWidth = 1.6
                                    ctx.strokeStyle = "#64748b"
                                    ctx.fillStyle = "#334155"
                                    for (var i = 0; i < backend.conceptGraphEdges.length; ++i) {
                                        var edge = backend.conceptGraphEdges[i]
                                        var sourceId = conceptPage.graphValue(edge, "sourceId", "")
                                        var targetId = conceptPage.graphValue(edge, "targetId", "")
                                        var lineColor = conceptPage.graphValue(edge, "lineColor", "#64748b")
                                        var highlighted = conceptPage.graphEdgeIsSelected(edge)
                                        ctx.lineWidth = highlighted ? 3.8 : 1.6
                                        ctx.strokeStyle = lineColor
                                        ctx.fillStyle = lineColor
                                        if (String(sourceId) === String(targetId)) {
                                            var loop = conceptPage.graphSelfLoopPoints(sourceId)
                                            if (!loop) {
                                                continue
                                            }
                                            ctx.beginPath()
                                            ctx.arc(loop.center.x, loop.center.y, loop.radius,
                                                    loop.startAngle, loop.endAngle, false)
                                            ctx.stroke()

                                            var loopAngle = loop.endAngle + Math.PI / 2
                                            var loopArrow = highlighted ? 10 : 9
                                            ctx.beginPath()
                                            ctx.moveTo(loop.end.x, loop.end.y)
                                            ctx.lineTo(loop.end.x - Math.cos(loopAngle - Math.PI / 6) * loopArrow,
                                                       loop.end.y - Math.sin(loopAngle - Math.PI / 6) * loopArrow)
                                            ctx.lineTo(loop.end.x - Math.cos(loopAngle + Math.PI / 6) * loopArrow,
                                                       loop.end.y - Math.sin(loopAngle + Math.PI / 6) * loopArrow)
                                            ctx.closePath()
                                            ctx.fill()

                                            var loopLabel = String(conceptPage.graphValue(edge, "label", ""))
                                            if (loopLabel !== "") {
                                                ctx.font = (highlighted ? "bold " : "") + "11px '" + conceptPage.uiFontFamily + "'"
                                                ctx.fillStyle = "#334155"
                                                var loopParts = loopLabel.split("\n")
                                                for (var lp = 0; lp < loopParts.length; ++lp) {
                                                    ctx.fillText(loopParts[lp], loop.label.x, loop.label.y + lp * 14)
                                                }
                                            }
                                            continue
                                        }
                                        var sourceCenter = conceptPage.graphNodeCenter(sourceId)
                                        var targetCenter = conceptPage.graphNodeCenter(targetId)
                                        var start = conceptPage.graphNodeAnchor(sourceId, targetCenter)
                                        var end = conceptPage.graphNodeAnchor(targetId, sourceCenter)
                                        if (start.x < 0 || end.x < 0) {
                                            continue
                                        }
                                        ctx.beginPath()
                                        ctx.moveTo(start.x, start.y)
                                        ctx.lineTo(end.x, end.y)
                                        ctx.stroke()
    
                                            var angle = Math.atan2(end.y - start.y, end.x - start.x)
                                            var arrowBack = highlighted ? 16 : 14
                                            var arrowSize = highlighted ? 10 : 9
                                            var arrowX = end.x - Math.cos(angle) * arrowBack
                                            var arrowY = end.y - Math.sin(angle) * arrowBack
                                            ctx.beginPath()
                                            ctx.moveTo(end.x, end.y)
                                            ctx.lineTo(arrowX - Math.cos(angle - Math.PI / 6) * arrowSize, arrowY - Math.sin(angle - Math.PI / 6) * arrowSize)
                                            ctx.lineTo(arrowX - Math.cos(angle + Math.PI / 6) * arrowSize, arrowY - Math.sin(angle + Math.PI / 6) * arrowSize)
                                            ctx.closePath()
                                            ctx.fill()
    
                                        var edgeLabel = String(conceptPage.graphValue(edge, "label", ""))
                                            if (edgeLabel !== "") {
                                                var labelX = (start.x + end.x) / 2 + 6
                                                var labelY = (start.y + end.y) / 2 - 4
                                                ctx.font = (highlighted ? "bold " : "") + "11px '" + conceptPage.uiFontFamily + "'"
                                                ctx.fillStyle = "#334155"
                                                var parts = edgeLabel.split("\n")
                                                for (var p = 0; p < parts.length; ++p) {
                                                    ctx.fillText(parts[p], labelX, labelY + p * 14)
                                                }
                                            }
                                    }
                                }
    
                                Connections {
                                    target: backend
                                    function onConceptChanged() {
                                        conceptPage.syncGraphSelection()
                                        conceptPage.requestGraphPaint()
                                    }
                                }
                            }
    
                            Repeater {
                                model: backend.conceptGraphNodes
                                delegate: Item {
                                    id: graphNodeItem
                                    required property var modelData
                                    property string nodeId: String(conceptPage.graphValue(modelData, "nodeId", ""))
                                    property string nodeType: String(conceptPage.graphValue(modelData, "nodeType", ""))
                                    property string titleText: String(conceptPage.graphValue(modelData, "titleText", ""))
                                    property string bodyText: String(conceptPage.graphValue(modelData, "bodyText", ""))
                                    property real nodeX: Number(conceptPage.graphValue(modelData, "nodeX", 0))
                                    property real nodeY: Number(conceptPage.graphValue(modelData, "nodeY", 0))
                                    property real nodeW: Number(conceptPage.graphValue(modelData, "nodeW", 150))
                                    property real nodeH: Number(conceptPage.graphValue(modelData, "nodeH", 56))
                                    property string fillColor: String(conceptPage.graphValue(modelData, "fillColor", "#CAD5E2"))
                                    property string borderColor: String(conceptPage.graphValue(modelData, "borderColor", "#5C728A"))
                                    property string textColor: String(conceptPage.graphValue(modelData, "textColor", "#0f172a"))
                                    property string kind: String(conceptPage.graphValue(modelData, "kind", "node"))
                                    property bool draggable: Boolean(conceptPage.graphValue(modelData, "draggable", false))
                                    property bool editable: Boolean(conceptPage.graphValue(modelData, "editable", false))
                                    property bool focusActive: Boolean(conceptPage.graphValue(modelData, "focusActive", false))
                                    property bool graphSelected: conceptPage.graphNodeIsSelected(nodeId)
                                    property bool graphConnected: conceptPage.graphNodeIsConnected(nodeId)
                                    property bool nodeReady: false
                                    property bool nodeDragging: false
                                    property bool hovered: false
                                    property int nodeToken: 0
                                    x: kind === "overview" ? conceptPage.overviewNodeX(nodeX, nodeW) : nodeX
                                    y: kind === "overview"
                                       ? conceptPage.overviewNodeY(nodeY, nodeH)
                                       : nodeY + (backend.conceptFocusActive ? 0 : conceptPage.graphLaneOffset(nodeType))
                                    width: nodeW
                                    height: nodeH
                                    z: graphSelected ? 10 : (focusActive ? 9 : (hovered ? 8 : (graphConnected ? 7 : 4)))
    
                                    Component.onCompleted: {
                                        nodeToken = ++conceptPage.conceptGraphNodeTokenSeed
                                        nodeReady = true
                                        conceptPage.registerConceptGraphNode(nodeId, nodeToken, graphNodeItem)
                                    }
    
                                    Component.onDestruction: conceptPage.unregisterConceptGraphNode(nodeId, nodeToken)
    
                                    onXChanged: {
                                        if (nodeReady)
                                            conceptPage.updateConceptGraphNodeGeometry(nodeId, nodeToken, graphNodeItem)
                                        if (nodeReady && draggable && nodeDragging) {
                                            conceptPage.setPendingGraphNodePosition(kind, nodeId, x, y)
                                            conceptPage.requestGraphPaint()
                                        }
                                    }
                                    onYChanged: {
                                        if (nodeReady)
                                            conceptPage.updateConceptGraphNodeGeometry(nodeId, nodeToken, graphNodeItem)
                                        if (nodeReady && draggable && nodeDragging) {
                                            conceptPage.setPendingGraphNodePosition(kind, nodeId, x, y)
                                            conceptPage.requestGraphPaint()
                                        }
                                    }
                                    onWidthChanged: if (nodeReady) conceptPage.updateConceptGraphNodeGeometry(nodeId, nodeToken, graphNodeItem)
                                    onHeightChanged: if (nodeReady) conceptPage.updateConceptGraphNodeGeometry(nodeId, nodeToken, graphNodeItem)
    
                                    Rectangle {
                                        id: graphNodeBase
                                        anchors.fill: parent
                                        radius: kind === "overview" ? 14 : 6
                                        color: fillColor
                                        border.color: graphSelected ? conceptPage.uiFocusColor
                                                    : focusActive ? borderColor
                                                    : graphConnected ? "#60a5fa"
                                                    : borderColor
                                        border.width: graphSelected ? 3.0
                                                     : focusActive ? 2.8
                                                     : graphConnected ? 2.1
                                                     : hovered ? 2.4
                                                     : 1.5
                                        opacity: editable ? 1.0 : (graphSelected || graphConnected ? 0.95 : 0.78)
                                    }
    
                                    Rectangle {
                                        anchors.fill: graphNodeBase
                                        radius: graphNodeBase.radius
                                        color: graphSelected ? "#163b82f6"
                                              : graphConnected ? "#0f60a5fa"
                                              : "#24ffffff"
                                        border.color: graphSelected ? "#93c5fd"
                                                     : focusActive ? borderColor
                                                     : graphConnected ? "#bfdbfe"
                                                     : "#66ffffff"
                                        border.width: graphSelected ? 1.6
                                                     : focusActive ? 1.4
                                                     : graphConnected ? 1.2
                                                     : 1
                                        visible: graphNodeItem.hovered || graphSelected || graphConnected || focusActive
                                    }
    
                                    Column {
                                        width: parent.width - (kind === "overview" ? 20 : 14)
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: kind === "overview" ? 6 : 3
                                        Label {
                                            width: parent.width
                                            text: titleText
                                            color: editable ? textColor : "#64748b"
                                            font.pixelSize: kind === "overview" ? 15 : 12
                                            font.bold: true
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                        }
                                        Label {
                                            width: parent.width
                                            text: bodyText
                                            color: editable ? textColor : "#64748b"
                                            font.pixelSize: kind === "overview" ? 13 : 11
                                            font.bold: focusActive
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            wrapMode: Text.Wrap
                                        }
                                    }
    
                                    MouseArea {
                                        id: graphNodeMouse
                                        anchors.fill: parent
                                        acceptedButtons: Qt.LeftButton
                                        hoverEnabled: true
                                        preventStealing: true
                                        cursorShape: draggable ? Qt.SizeAllCursor : Qt.PointingHandCursor
                                        drag.target: draggable ? graphNodeItem : null
                                        drag.axis: Drag.XAndYAxis
                                        onPressed: graphNodeItem.nodeDragging = graphNodeItem.draggable
                                        onEntered: graphNodeItem.hovered = true
                                        onExited: graphNodeItem.hovered = false
                                        drag.minimumX: {
                                            if (backend.conceptFocusActive && graphNodeItem.kind === "node")
                                                return 0
                                            var lane = conceptPage.graphLaneByType(graphNodeItem.nodeType)
                                            return graphNodeItem.kind === "overview" || !lane
                                                    ? 0
                                                    : conceptPage.graphLaneNodeMinX(lane)
                                        }
                                        drag.minimumY: {
                                            if (backend.conceptFocusActive && graphNodeItem.kind === "node")
                                                return 0
                                            var lane = conceptPage.graphLaneByType(graphNodeItem.nodeType)
                                            return graphNodeItem.kind === "overview" || !lane
                                                    ? 0
                                                    : conceptPage.graphLaneNodeMinY(lane)
                                        }
                                        drag.maximumX: {
                                            if (backend.conceptFocusActive && graphNodeItem.kind === "node")
                                                return Math.max(0, graphContent.width - graphNodeItem.width)
                                            var lane = conceptPage.graphLaneByType(graphNodeItem.nodeType)
                                            return graphNodeItem.kind === "overview" || !lane
                                                    ? Math.max(0, graphContent.width - graphNodeItem.width)
                                                    : conceptPage.graphLaneNodeMaxX(lane, graphNodeItem.width)
                                        }
                                        drag.maximumY: {
                                            if (backend.conceptFocusActive && graphNodeItem.kind === "node")
                                                return Math.max(0, graphContent.height - graphNodeItem.height)
                                            var lane = conceptPage.graphLaneByType(graphNodeItem.nodeType)
                                            return graphNodeItem.kind === "overview" || !lane
                                                    ? Math.max(0, graphContent.height - graphNodeItem.height)
                                                    : conceptPage.graphLaneNodeMaxY(lane, graphNodeItem.height)
                                        }
                                        onClicked: {
                                            conceptPage.selectGraphNode(nodeId)
                                            if (kind === "overview")
                                                conceptPage.requestShowConceptOverview(nodeType)
                                            else
                                                conceptPage.requestShowConceptNode(nodeId)
                                        }
                                        onDoubleClicked: function(mouse) {
                                            if (kind !== "node" || backend.conceptOverviewMode)
                                                return
                                            mouse.accepted = true
                                            conceptPage.selectGraphNode(nodeId)
                                            Qt.callLater(function() {
                                                backend.toggleConceptFocusNode(nodeId)
                                                conceptPage.requestGraphPaint()
                                            })
                                        }
                                        onReleased: {
                                            if (graphNodeItem.nodeDragging) {
                                                conceptPage.setPendingGraphNodePosition(kind, nodeId, graphNodeItem.x, graphNodeItem.y)
                                                conceptPage.requestGraphPaint()
                                            }
                                            graphNodeItem.nodeDragging = false
                                            conceptPage.flushPendingGraphNodePositions()
                                        }
                                        onCanceled: {
                                            if (graphNodeItem.nodeDragging)
                                                conceptPage.flushPendingGraphNodePositions()
                                            graphNodeItem.nodeDragging = false
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
            SplitView.preferredWidth: 470
            SplitView.minimumWidth: 470
            SplitView.maximumWidth: 470
            radius: 6
            color: "#a6ffffff"
            border.color: conceptPage.uiBorderColor
            border.width: 1
    
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
    
                Label {
                    text: backend.conceptDetailTitle
                    font.bold: true
                    font.family: conceptPage.uiFontFamily
                    font.pixelSize: 17
                    color: "#0f172a"
                }
    
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: backend.conceptDetailMode === "overview" ? 1 :
                                  backend.conceptDetailMode === "node" ? 2 : 0
    
                    Rectangle {
                        color: "#33ffffff"
                        radius: 4
                        border.color: conceptPage.uiSoftBorderColor
                        Label {
                            anchors.centerIn: parent
                            text: "请选择图谱中的类型或节点"
                            color: "#64748b"
                            font.family: conceptPage.uiFontFamily
                            font.pixelSize: conceptPage.uiFontSize
                        }
                    }
    
                    ConceptOverviewDetailPanel {
                        enabled: !backend.conceptConfigLocked
                    }
    
                    ConceptNodeDetailPanel {
                        enabled: !backend.conceptConfigLocked
                    }
                }
            }
        }
    }
}
    
    component ConceptUiButton: Button {
id: buttonControl
property bool visualChecked: checked
    
hoverEnabled: true
implicitHeight: 32
font.family: conceptPage.uiFontFamily
font.pixelSize: conceptPage.uiFontSize
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
                  : buttonControl.visualChecked || buttonControl.activeFocus ? conceptPage.uiFocusColor
                  : conceptPage.uiBorderColor
    border.width: buttonControl.visualChecked || buttonControl.activeFocus ? 1.4 : 1
}
    }
    component ConceptUiTextField: TextField {
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
font.family: conceptPage.uiFontFamily
font.pixelSize: conceptPage.uiFontSize
color: !enabled ? "#94a3b8" : readOnly ? "#334155" : "#0f172a"
selectedTextColor: "#ffffff"
selectionColor: "#2563eb"
    
background: Rectangle {
    radius: 4
    color: !textFieldControl.enabled ? "#eef2f7"
           : textFieldControl.readOnly ? textFieldControl.readOnlyColor : textFieldControl.normalColor
    border.color: textFieldControl.warn ? "#f97316"
                  : textFieldControl.activeFocus ? conceptPage.uiFocusColor : conceptPage.uiBorderColor
    border.width: textFieldControl.warn || textFieldControl.activeFocus ? 1.4 : 1
}
    }
    component ConceptUiComboBox: ComboBox {
id: comboControl
property color normalColor: "#ffffff"
property bool showArrow: true
    
implicitHeight: 32
leftPadding: 8
rightPadding: comboControl.showArrow ? 30 : 8
font.family: conceptPage.uiFontFamily
font.pixelSize: conceptPage.uiFontSize
    
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
    border.color: comboControl.activeFocus || comboControl.popup.visible ? conceptPage.uiFocusColor : conceptPage.uiBorderColor
    border.width: comboControl.activeFocus || comboControl.popup.visible ? 1.4 : 1
}
    
delegate: ItemDelegate {
    id: comboDelegate
    required property var modelData
    required property int index
    
    width: comboControl.width
    text: String(modelData)
    font.family: conceptPage.uiFontFamily
    font.pixelSize: conceptPage.uiFontSize
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
        border.color: conceptPage.uiBorderColor
        border.width: 1
    }
}
    }
    component ConceptUiCheckBox: CheckBox {
id: checkControl
    
hoverEnabled: true
implicitWidth: String(text).length === 0 ? 22 : indicator.width + spacing + contentItem.implicitWidth
implicitHeight: 24
spacing: 6
font.family: conceptPage.uiFontFamily
font.pixelSize: conceptPage.uiFontSize
    
indicator: Rectangle {
    x: String(checkControl.text).length === 0 ? (checkControl.width - width) / 2 : 0
    y: (checkControl.height - height) / 2
    implicitWidth: 16
    implicitHeight: 16
    radius: 3
    color: !checkControl.enabled ? "#eef2f7"
           : checkControl.checked ? "#1976bd" : "#ffffff"
    border.color: !checkControl.enabled ? "#d5dde6"
                  : checkControl.activeFocus ? conceptPage.uiFocusColor : conceptPage.uiBorderColor
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
    component ConceptFilterGroup: Rectangle {
id: filterGroup
property string title: ""
    
radius: 6
color: "#7affffff"
border.color: conceptPage.uiBorderColor
border.width: 1
    
Label {
    x: 10
    y: 8
    text: filterGroup.title
    font.bold: true
    font.family: conceptPage.uiFontFamily
    font.pixelSize: conceptPage.uiTitleFontSize
    color: "#0f172a"
}
    }
    component ConceptTypeTable: ColumnLayout {
id: typeTable
property var rows: []
property bool nodeTable: true
readonly property int rowHeight: 28
readonly property int visibleRows: rows.length
readonly property real endpointColumnWidth: (width - 58) / 2
spacing: 0
    
RowLayout {
    Layout.fillWidth: true
    Layout.preferredHeight: typeTable.rowHeight
    spacing: 0
    ConceptHeaderCell { Layout.preferredWidth: typeTable.nodeTable ? 54 : typeTable.endpointColumnWidth; text: typeTable.nodeTable ? "显示" : "入节点" }
    ConceptHeaderCell { Layout.preferredWidth: typeTable.nodeTable ? Math.max(1, typeTable.width - 54 - 58) : typeTable.endpointColumnWidth; text: typeTable.nodeTable ? "类型" : "出节点" }
    ConceptHeaderCell { Layout.preferredWidth: 58; text: "数量" }
}
    
Flickable {
    Layout.fillWidth: true
    Layout.preferredHeight: typeTable.rowHeight * typeTable.visibleRows
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    contentWidth: width
    contentHeight: typeTableBody.implicitHeight
    interactive: false
    
    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AlwaysOff
    }
    
    ColumnLayout {
        id: typeTableBody
        width: parent.width
        spacing: 0
    
        Repeater {
            model: typeTable.rows
            delegate: RowLayout {
                required property int index
                required property string name
                required property string type
                required property string sourceType
                required property string targetType
                required property int count
                required property bool showChecked
                required property bool showEnabled
                Layout.fillWidth: true
                Layout.preferredHeight: typeTable.rowHeight
                spacing: 0
                Rectangle {
                    Layout.preferredWidth: typeTable.nodeTable ? 54 : typeTable.endpointColumnWidth
                    Layout.fillHeight: true
                    color: "#ffffff"
                    border.color: conceptPage.uiSoftBorderColor
                    ConceptUiCheckBox {
                        visible: typeTable.nodeTable
                        anchors.centerIn: parent
                        checked: showChecked
                        enabled: showEnabled
                        onClicked: {
                            if (typeTable.nodeTable)
                                backend.setConceptNodeTypeChecked(type, checked)
                        }
                    }
                    Label {
                        visible: !typeTable.nodeTable
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        text: sourceType
                        color: "#0f172a"
                        font.family: conceptPage.uiFontFamily
                        font.pixelSize: conceptPage.uiFontSize
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
                ConceptTableCell {
                    Layout.preferredWidth: typeTable.nodeTable ? Math.max(1, typeTable.width - 54 - 58) : typeTable.endpointColumnWidth
                    text: typeTable.nodeTable ? name : targetType
                    fontSize: conceptPage.uiFontSize
                }
                ConceptTableCell { Layout.preferredWidth: 58; text: count }
            }
        }
    }
}
    }
    component ConceptHeaderCell: Rectangle {
property string text: ""
implicitHeight: 34
color: "#e7edf4"
border.color: conceptPage.uiBorderColor
border.width: 1
Label {
    anchors.centerIn: parent
    text: parent.text
    color: "#475569"
    font.family: conceptPage.uiFontFamily
    font.pixelSize: conceptPage.uiFontSize
    font.bold: true
}
    }
    component ConceptTableCell: Rectangle {
property string text: ""
property bool selected: false
property bool clickable: false
property int fontSize: conceptPage.uiFontSize
signal clicked()
implicitHeight: 28
color: selected ? "#dbeafe" : "#ffffff"
border.color: conceptPage.uiSoftBorderColor
border.width: 1
Label {
    anchors.fill: parent
    anchors.leftMargin: 6
    anchors.rightMargin: 6
    text: parent.text
    color: "#0f172a"
    font.family: conceptPage.uiFontFamily
    font.pixelSize: parent.fontSize
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight
}
    
MouseArea {
    anchors.fill: parent
    enabled: parent.clickable
    acceptedButtons: Qt.LeftButton
    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    onClicked: parent.clicked()
}
    }
    component ConceptOverviewDetailPanel: ColumnLayout {
id: overviewDetail
property int selectedRow: -1
spacing: 8
    
ColumnLayout {
    Layout.fillWidth: true
    spacing: 6
    ConceptLabeledReadonly { Layout.fillWidth: true; label: "类型"; textValue: backend.conceptOverviewType }
    ConceptLabeledReadonly { Layout.fillWidth: true; label: "类型名称"; textValue: backend.conceptOverviewTypeName }
}
    
Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: 4
    color: "#33ffffff"
    border.color: conceptPage.uiSoftBorderColor
    border.width: 1
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 0
    
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            spacing: 0
            ConceptHeaderCell { Layout.preferredWidth: 112; text: "节点ID" }
            ConceptHeaderCell { Layout.fillWidth: true; text: "节点名称" }
        }
    
        ListView {
            id: overviewList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: backend.conceptOverviewDetailRows
            delegate: Item {
                id: overviewNodeRow
                required property int index
                required property string nodeId
                required property string name
                property bool rowSelected: overviewDetail.selectedRow === overviewNodeRow.index
                width: overviewList.width
                height: 36
    
                function selectRow() {
                    overviewDetail.selectedRow = overviewNodeRow.index
                }
    
                TapHandler {
                    onTapped: overviewNodeRow.selectRow()
                }
    
                Rectangle {
                    anchors.fill: parent
                    radius: 4
                    color: rowSelected ? "#dbeafe" : "#ffffff"
                    border.color: conceptPage.uiSoftBorderColor
                    border.width: 1
                }
    
                RowLayout {
                    anchors.fill: parent
                    spacing: 0
                    Rectangle {
                        Layout.preferredWidth: 112
                        Layout.preferredHeight: 36
                        color: "transparent"
                        border.color: conceptPage.uiSoftBorderColor
                        Label {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            text: overviewNodeRow.nodeId
                            color: "#0f172a"
                            font.family: conceptPage.uiFontFamily
                            font.pixelSize: conceptPage.uiFontSize
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            cursorShape: Qt.PointingHandCursor
                            onClicked: overviewNodeRow.selectRow()
                        }
                    }
                    ConceptUiTextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        text: overviewNodeRow.name
                        enabled: overviewDetail.enabled
                        normalColor: rowSelected ? "#eaf3ff" : "#ffffff"
                        horizontalAlignment: Text.AlignLeft
                        onEditingFinished: backend.setConceptOverviewNodeName(overviewNodeRow.index, text)
                        onActiveFocusChanged: if (activeFocus) overviewNodeRow.selectRow()
                    }
                }
            }
        }
    }
}
    
Label {
    Layout.fillWidth: true
    text: backend.conceptOverviewApplyStatus
    color: text.indexOf("等待") >= 0 ? "#b45309" : "#166534"
    font.family: conceptPage.uiFontFamily
    font.pixelSize: conceptPage.uiFontSize
    horizontalAlignment: Text.AlignRight
}
    
RowLayout {
    Layout.fillWidth: true
    spacing: 8
    ConceptUiButton { Layout.fillWidth: true; text: "新增节点"; enabled: overviewDetail.enabled; onClicked: backend.addConceptOverviewNode() }
    ConceptUiButton {
        Layout.fillWidth: true
        text: "删除节点"
        enabled: overviewDetail.enabled && overviewDetail.selectedRow >= 0
        onClicked: {
            backend.deleteConceptOverviewNode(overviewDetail.selectedRow)
            overviewDetail.selectedRow = -1
        }
    }
    ConceptUiButton { Layout.fillWidth: true; text: "应用修改"; enabled: overviewDetail.enabled; onClicked: backend.applyConceptOverviewDetail() }
}
    }
    component ConceptNodeDetailPanel: ColumnLayout {
id: nodeDetail
property int selectedRow: -1
spacing: 8
    
ColumnLayout {
    Layout.fillWidth: true
    spacing: 6
    ConceptLabeledReadonly { Layout.fillWidth: true; label: "节点类型"; textValue: backend.conceptNodeTypeLine }
    ConceptLabeledReadonly { Layout.fillWidth: true; label: "节点ID"; textValue: backend.conceptNodeIdLine }
    RowLayout {
        Layout.fillWidth: true
        Label { Layout.preferredWidth: 68; text: "节点名称"; color: "#475569"; font.family: conceptPage.uiFontFamily; font.pixelSize: conceptPage.uiFontSize }
        ConceptUiTextField {
            Layout.fillWidth: true
            text: backend.conceptNodeName
            enabled: nodeDetail.enabled
            horizontalAlignment: Text.AlignLeft
            onEditingFinished: backend.setConceptNodeName(text)
        }
    }
}
    
Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: 4
    color: "#33ffffff"
    border.color: conceptPage.uiSoftBorderColor
    border.width: 1
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 0
    
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            spacing: 0
            ConceptHeaderCell { Layout.preferredWidth: 52; text: "方向" }
            ConceptHeaderCell { Layout.preferredWidth: 56; text: "类型" }
            ConceptHeaderCell { Layout.preferredWidth: 76; text: "节点ID" }
            ConceptHeaderCell { Layout.fillWidth: true; text: "名称" }
            ConceptHeaderCell { Layout.preferredWidth: 68; text: "权重" }
        }
    
        ListView {
            id: edgeList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: backend.conceptNodeEdgeRows
            delegate: Item {
                id: edgeRow
                required property int index
                required property string direction
                required property string edgeId
                required property string edgeType
                required property string otherType
                required property string otherNodeId
                required property string edgeName
                required property string weight
                property bool rowSelected: nodeDetail.selectedRow === edgeRow.index
                width: edgeList.width
                height: 36
    
                function selectRow() {
                    nodeDetail.selectedRow = edgeRow.index
                }
    
                TapHandler {
                    onTapped: edgeRow.selectRow()
                }
    
                Rectangle {
                    anchors.fill: parent
                    radius: 4
                    color: rowSelected ? "#dbeafe" : "#ffffff"
                    border.color: conceptPage.uiSoftBorderColor
                    border.width: 1
                }
    
                RowLayout {
                    anchors.fill: parent
                    spacing: 0
    
                    ConceptTableCell {
                        Layout.preferredWidth: 52
                        Layout.preferredHeight: 36
                        text: edgeRow.direction
                        selected: edgeRow.rowSelected
                        clickable: true
                        onClicked: edgeRow.selectRow()
                    }
                    ConceptUiComboBox {
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 36
                        model: backend.conceptNodeEdgeTypeOptions(edgeRow.direction, edgeRow.otherType)
                        currentIndex: backend.conceptNodeEdgeTypeOptions(edgeRow.direction, edgeRow.otherType).indexOf(edgeRow.otherType)
                        enabled: nodeDetail.enabled
                        showArrow: false
                        normalColor: rowSelected ? "#eaf3ff" : "#ffffff"
                        onPressedChanged: if (pressed) edgeRow.selectRow()
                        onActiveFocusChanged: if (activeFocus) edgeRow.selectRow()
                        onActivated: backend.setConceptNodeEdgeOtherType(edgeRow.index, currentText)
                    }
                    ConceptUiComboBox {
                        Layout.preferredWidth: 76
                        Layout.preferredHeight: 36
                        model: backend.conceptNodeIdsByType(edgeRow.otherType)
                        currentIndex: backend.conceptNodeIdsByType(edgeRow.otherType).indexOf(edgeRow.otherNodeId)
                        enabled: nodeDetail.enabled
                        showArrow: false
                        normalColor: rowSelected ? "#eaf3ff" : "#ffffff"
                        onPressedChanged: if (pressed) edgeRow.selectRow()
                        onActiveFocusChanged: if (activeFocus) edgeRow.selectRow()
                        onActivated: backend.setConceptNodeEdgeOtherId(edgeRow.index, currentText)
                    }
                    ConceptUiTextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        text: edgeRow.edgeName
                        enabled: nodeDetail.enabled
                        normalColor: rowSelected ? "#eaf3ff" : "#ffffff"
                        horizontalAlignment: Text.AlignLeft
                        onEditingFinished: backend.setConceptNodeEdgeName(edgeRow.index, text)
                        onActiveFocusChanged: if (activeFocus) edgeRow.selectRow()
                    }
                    ConceptUiTextField {
                        Layout.preferredWidth: 68
                        Layout.preferredHeight: 36
                        text: edgeRow.weight
                        enabled: nodeDetail.enabled
                        horizontalAlignment: Text.AlignHCenter
                        normalColor: rowSelected ? "#eaf3ff" : "#ffffff"
                        onEditingFinished: backend.setConceptNodeEdgeWeight(edgeRow.index, text)
                        onActiveFocusChanged: if (activeFocus) edgeRow.selectRow()
                    }
                }
            }
        }
    }
}
    
Label {
    Layout.fillWidth: true
    text: backend.conceptNodeApplyStatus
    color: text.indexOf("等待") >= 0 ? "#b45309" : "#166534"
    font.family: conceptPage.uiFontFamily
    font.pixelSize: conceptPage.uiFontSize
    horizontalAlignment: Text.AlignRight
}
    
GridLayout {
    Layout.fillWidth: true
    columns: 2
    columnSpacing: 8
    rowSpacing: 8
    uniformCellWidths: true
    ConceptUiButton { Layout.fillWidth: true; text: "新增入边"; enabled: nodeDetail.enabled; onClicked: backend.addConceptNodeEdge("in") }
    ConceptUiButton { Layout.fillWidth: true; text: "新增出边"; enabled: nodeDetail.enabled; onClicked: backend.addConceptNodeEdge("out") }
    ConceptUiButton {
        Layout.fillWidth: true
        text: "删除边"
        enabled: nodeDetail.enabled && nodeDetail.selectedRow >= 0
        onClicked: {
            backend.deleteConceptNodeEdge(nodeDetail.selectedRow)
            nodeDetail.selectedRow = -1
        }
    }
    ConceptUiButton { Layout.fillWidth: true; text: "应用修改"; enabled: nodeDetail.enabled; onClicked: backend.applyConceptNodeDetail() }
}
    }
    component ConceptLabeledReadonly: RowLayout {
id: labeledReadonly
property string label: ""
property string textValue: ""
spacing: 6
Label { Layout.preferredWidth: 68; text: labeledReadonly.label; color: "#475569"; font.family: conceptPage.uiFontFamily; font.pixelSize: conceptPage.uiFontSize }
ConceptUiTextField {
    Layout.fillWidth: true
    text: labeledReadonly.textValue
    readOnly: true
    color: "#64748b"
}
    }
    }
