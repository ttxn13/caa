import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
pragma ComponentBehavior: Bound

Item {
    id: root
    property var backend: uiAFSimTest
    readonly property string uiFontFamily: "新宋体"
    readonly property int uiFontSize: 14
    readonly property int uiSmallFontSize: 12
    readonly property int uiTitleFontSize: 16
    readonly property color pageColor: "#eef3f8"
    readonly property color panelColor: "#ffffff"
    readonly property color textMain: "#0f172a"
    readonly property color textMuted: "#64748b"
    readonly property color borderColor: "#cbd5e1"
    readonly property color softBorderColor: "#e2e8f0"
    readonly property color blue: "#2563eb"
    readonly property color blueSoft: "#dbeafe"
    readonly property color green: "#16a34a"
    readonly property color greenSoft: "#dcfce7"
    readonly property color red: "#dc2626"
    readonly property color redSoft: "#fee2e2"
    readonly property color fieldColor: "#f8fafc"

    Component.onDestruction: {
        if (root.backend) {
            root.backend.release()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.pageColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        TestPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Layout.maximumHeight: 52

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: "AFSim测试"
                    color: root.textMain
                    font.family: root.uiFontFamily
                    font.pixelSize: 20
                    font.bold: true
                    Layout.preferredWidth: 160
                }

                StatusPill {
                    label: root.backend.clientRunning ? "客户端监听中" : "客户端已停止"
                    active: root.backend.clientRunning
                }

                StatusPill {
                    label: root.backend.serverRunning ? "服务端发送就绪" : "服务端已停止"
                    active: root.backend.serverRunning
                }

                Item { Layout.fillWidth: true }

                TestButton {
                    text: "清空数据包"
                    onClicked: root.backend.clearPackets()
                }

                TestButton {
                    text: "清空日志"
                    onClicked: root.backend.clearLogs()
                }
            }
        }

        TestPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 86
            Layout.maximumHeight: 86

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                ColumnLayout {
                    Layout.preferredWidth: 220
                    spacing: 5
                    SmallLabel { text: "AFSim接收地址" }
                    InputBox {
                        Layout.fillWidth: true
                        text: root.backend.bindAddr
                        enabled: !root.backend.clientRunning
                        onEditingFinished: root.backend.bindAddr = text
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 105
                    spacing: 5
                    SmallLabel { text: "接收端口" }
                    InputBox {
                        Layout.fillWidth: true
                        text: root.backend.bindPort
                        enabled: !root.backend.clientRunning
                        inputMethodHints: Qt.ImhDigitsOnly
                        onEditingFinished: root.backend.bindPort = text
                    }
                }

                PrimaryButton {
                    Layout.alignment: Qt.AlignBottom
                    Layout.preferredWidth: 124
                    text: root.backend.clientRunning ? "停止接收" : "开始接收"
                    onClicked: root.backend.toggleClient()
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: root.borderColor
                }

                ColumnLayout {
                    Layout.preferredWidth: 220
                    spacing: 5
                    SmallLabel { text: "AFSim发送地址" }
                    InputBox {
                        Layout.fillWidth: true
                        text: root.backend.targetAddr
                        enabled: !root.backend.serverRunning
                        onEditingFinished: root.backend.targetAddr = text
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 105
                    spacing: 5
                    SmallLabel { text: "发送端口" }
                    InputBox {
                        Layout.fillWidth: true
                        text: root.backend.targetPort
                        enabled: !root.backend.serverRunning
                        inputMethodHints: Qt.ImhDigitsOnly
                        onEditingFinished: root.backend.targetPort = text
                    }
                }

                PrimaryButton {
                    Layout.alignment: Qt.AlignBottom
                    Layout.preferredWidth: 124
                    text: root.backend.serverRunning ? "停止发送端" : "启动发送端"
                    onClicked: root.backend.toggleServer()
                }

                Item { Layout.fillWidth: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            ColumnLayout {
                Layout.preferredWidth: 390
                Layout.minimumWidth: 330
                Layout.fillHeight: true
                spacing: 10

                TestPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 7

                        SectionHead {
                            Layout.fillWidth: true
                            title: "数据包流"
                            detail: root.backend.packetRows.length + " 包"
                        }

                        TableHeader {
                            Layout.fillWidth: true
                            SmallLabel { text: "#"; Layout.preferredWidth: 34 }
                            SmallLabel { text: "时间"; Layout.preferredWidth: 78 }
                            SmallLabel { text: "方向"; Layout.preferredWidth: 46 }
                            SmallLabel { text: "PDU"; Layout.preferredWidth: 58 }
                            SmallLabel { text: "摘要"; Layout.fillWidth: true }
                        }

                        ListView {
                            id: packetList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 4
                            model: root.backend.packetRows

                            delegate: Rectangle {
                                id: packetDelegate
                                required property int index
                                required property var modelData

                                width: packetList.width
                                height: 58
                                radius: 5
                                color: index === root.backend.selectedPacketIndex ? root.blueSoft : (packetMouse.containsMouse ? "#f8fafc" : "#ffffff")
                                border.color: index === root.backend.selectedPacketIndex ? root.blue : root.borderColor

                                MouseArea {
                                    id: packetMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.LeftButton
                                    onClicked: {
                                        packetList.currentIndex = packetDelegate.index
                                        root.backend.selectPacket(packetDelegate.index)
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    spacing: 6

                                    ValueLabel { text: packetDelegate.modelData.seq; Layout.preferredWidth: 34 }
                                    ValueLabel { text: packetDelegate.modelData.time; Layout.preferredWidth: 78 }
                                    ValueLabel {
                                        text: packetDelegate.modelData.direction === "RECV" ? "接收"
                                              : packetDelegate.modelData.direction === "SEND" ? "发送"
                                              : packetDelegate.modelData.direction
                                        color: packetDelegate.modelData.direction === "RECV" ? root.green : root.blue
                                        font.bold: true
                                        Layout.preferredWidth: 46
                                    }
                                    ValueLabel { text: packetDelegate.modelData.pduType; Layout.preferredWidth: 58 }
                                    Text {
                                        text: packetDelegate.modelData.summary
                                        color: root.textMain
                                        font.family: root.uiFontFamily
                                        font.pixelSize: 11
                                        wrapMode: Text.Wrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }

                TestPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 210

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 7

                        SectionHead {
                            Layout.fillWidth: true
                            title: "运行日志"
                            detail: "PDU 收发统计"
                        }

                        GridLayout {
                            id: logGrid
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            columns: 2
                            columnSpacing: 6
                            rowSpacing: 6

                            Repeater {
                                model: root.backend.logRows
                                delegate: Rectangle {
                                    id: logDelegate
                                    required property string modelData

                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: (logGrid.width - logGrid.columnSpacing) / 2
                                    Layout.preferredHeight: 36
                                    color: modelData.indexOf("[ERROR]") >= 0 || modelData.indexOf("[错误]") >= 0 ? root.redSoft
                                          : modelData.indexOf("[SEND]") >= 0 || modelData.indexOf("[发送]") >= 0 ? "#f1f6ff"
                                          : modelData.indexOf("[RECV]") >= 0 || modelData.indexOf("[接收]") >= 0 ? root.greenSoft
                                          : "#ffffff"
                                    border.color: root.borderColor
                                    radius: 4

                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        text: logDelegate.modelData
                                        color: root.textMain
                                        font.family: "Consolas"
                                        font.pixelSize: 11
                                        wrapMode: Text.Wrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }
            }

            TestPanel {
                Layout.fillWidth: true
                Layout.minimumWidth: 390
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    SectionHead {
                        Layout.fillWidth: true
                        title: "数据包详情"
                        detail: root.backend.selectedStructureName
                    }

                    SmallLabel {
                        text: root.backend.selectedSummary
                        Layout.fillWidth: true
                    }

                    TableHeader {
                        Layout.fillWidth: true
                        SmallLabel { text: "字段"; Layout.preferredWidth: 230 }
                        SmallLabel { text: "值"; Layout.fillWidth: true }
                    }

                    FieldList {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        rowsModel: root.backend.selectedPacketRows
                    }

                    SmallLabel {
                        text: "原始十六进制"
                        Layout.fillWidth: true
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 230
                        TextArea {
                            text: root.backend.selectedHex
                            readOnly: true
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            color: root.textMain
                            font.family: "Consolas"
                            font.pixelSize: 11
                            background: Rectangle {
                                color: root.fieldColor
                                border.color: root.borderColor
                                radius: 5
                            }
                        }
                    }
                }
            }

            TestPanel {
                Layout.preferredWidth: 420
                Layout.minimumWidth: 360
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    SectionHead {
                        Layout.fillWidth: true
                        title: "AFSim服务端发送"
                        detail: root.backend.sendStatus
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        model: root.backend.sendPduTypes
                        currentIndex: root.backend.sendPduIndex
                        onActivated: function(index) { root.backend.sendPduIndex = index }
                    }

                    TableHeader {
                        Layout.fillWidth: true
                        SmallLabel { text: "字段"; Layout.preferredWidth: 96 }
                        SmallLabel { text: "值"; Layout.fillWidth: true }
                    }

                    ListView {
                        id: sendFieldList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 4
                        model: root.backend.sendFieldRows

                        delegate: Rectangle {
                            id: sendDelegate
                            required property int index
                            required property var modelData
                            property bool grouped: modelData.parts && modelData.parts.length > 0
                            property int groupColumns: modelData.columns || (grouped ? modelData.parts.length : 1)

                            width: sendFieldList.width
                            height: modelData.rowHeight || 38
                            color: "#ffffff"
                            border.color: root.borderColor
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 8

                                SmallLabel {
                                    text: sendDelegate.modelData.label
                                    Layout.preferredWidth: 96
                                }

                                GridLayout {
                                    visible: sendDelegate.grouped
                                    Layout.fillWidth: true
                                    columns: sendDelegate.groupColumns
                                    columnSpacing: 4
                                    rowSpacing: 4

                                    Repeater {
                                        model: sendDelegate.grouped ? sendDelegate.modelData.parts : []
                                        delegate: InputBox {
                                            required property var modelData

                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 30
                                            text: modelData.value
                                            placeholderText: modelData.label
                                            font.pixelSize: 12
                                            leftPadding: 5
                                            rightPadding: 5
                                            onEditingFinished: root.backend.setSendFieldValueByKey(modelData.key, text)
                                        }
                                    }
                                }

                                InputBox {
                                    visible: !sendDelegate.grouped
                                    Layout.fillWidth: true
                                    text: sendDelegate.modelData.value
                                    onEditingFinished: root.backend.setSendFieldValueByKey(sendDelegate.modelData.key, text)
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TestButton {
                            text: "构建"
                            Layout.fillWidth: true
                            onClicked: root.backend.buildSendPacket()
                        }

                        PrimaryButton {
                            text: "发送"
                            Layout.fillWidth: true
                            onClicked: root.backend.sendStructuredPacket()
                        }
                    }

                    SmallLabel {
                        text: "十六进制预览 / 手动数据包"
                        Layout.fillWidth: true
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 118
                        TextArea {
                            text: root.backend.sendHex
                            onTextChanged: {
                                if (text !== root.backend.sendHex)
                                    root.backend.sendHex = text
                            }
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            color: root.textMain
                            font.family: "Consolas"
                            font.pixelSize: 11
                            background: Rectangle {
                                color: root.fieldColor
                                border.color: root.borderColor
                                radius: 5
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TestButton {
                            text: "解析十六进制"
                            Layout.fillWidth: true
                            onClicked: root.backend.decodeSendHex()
                        }

                        DangerButton {
                            text: "发送十六进制"
                            Layout.fillWidth: true
                            onClicked: root.backend.sendHexPacket()
                        }
                    }
                }
            }
        }
    }

    component TestPanel: Rectangle {
        radius: 6
        color: root.panelColor
        border.color: root.borderColor
        border.width: 1
        clip: true
    }

    component SmallLabel: Text {
        color: root.textMuted
        font.family: root.uiFontFamily
        font.pixelSize: root.uiSmallFontSize
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    component ValueLabel: Text {
        color: root.textMain
        font.family: "Consolas"
        font.pixelSize: 11
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    component StatusPill: Rectangle {
        id: statusPill
        property string label: ""
        property bool active: false
        implicitWidth: statusText.implicitWidth + 22
        implicitHeight: 28
        radius: 14
        color: statusPill.active ? root.greenSoft : "#f1f5f9"
        border.color: statusPill.active ? "#86efac" : root.borderColor

        Text {
            id: statusText
            anchors.centerIn: parent
            text: statusPill.label
            color: statusPill.active ? "#166534" : root.textMuted
            font.family: root.uiFontFamily
            font.pixelSize: root.uiSmallFontSize
            font.bold: statusPill.active
        }
    }

    component SectionHead: RowLayout {
        id: sectionHead
        property string title: ""
        property string detail: ""
        spacing: 8

        Text {
            text: sectionHead.title
            color: root.textMain
            font.family: root.uiFontFamily
            font.pixelSize: root.uiTitleFontSize
            font.bold: true
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        Text {
            text: sectionHead.detail
            color: root.textMuted
            font.family: root.uiFontFamily
            font.pixelSize: root.uiSmallFontSize
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component TableHeader: Rectangle {
        default property alias contentData: headerRow.data
        implicitHeight: 28
        radius: 4
        color: "#f1f5f9"
        border.color: root.softBorderColor

        RowLayout {
            id: headerRow
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 6
        }
    }

    component InputBox: TextField {
        id: inputBox
        selectByMouse: true
        implicitHeight: 32
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize
        color: enabled ? root.textMain : "#94a3b8"
        selectedTextColor: "#ffffff"
        selectionColor: root.blue
        verticalAlignment: Text.AlignVCenter
        leftPadding: 8
        rightPadding: 8
        background: Rectangle {
            radius: 5
            color: enabled ? root.fieldColor : "#eef2f7"
            border.color: inputBox.activeFocus ? root.blue : root.borderColor
            border.width: inputBox.activeFocus ? 1.4 : 1
        }
    }

    component TestButton: Button {
        id: button
        implicitHeight: 32
        font.family: root.uiFontFamily
        font.pixelSize: root.uiFontSize
        contentItem: Text {
            text: button.text
            color: button.enabled ? root.textMain : "#94a3b8"
            font: button.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 5
            color: !button.enabled ? "#eef2f7" : button.down ? "#e2e8f0" : button.hovered ? "#f8fafc" : "#ffffff"
            border.color: button.activeFocus ? root.blue : root.borderColor
            border.width: button.activeFocus ? 1.4 : 1
        }
    }

    component PrimaryButton: TestButton {
        id: primaryButton
        background: Rectangle {
            radius: 5
            color: !primaryButton.enabled ? "#eef2f7" : primaryButton.down ? "#1d4ed8" : primaryButton.hovered ? "#3b82f6" : root.blue
            border.color: color
        }
        contentItem: Text {
            text: primaryButton.text
            color: primaryButton.enabled ? "#ffffff" : "#94a3b8"
            font: primaryButton.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component DangerButton: TestButton {
        id: dangerButton
        background: Rectangle {
            radius: 5
            color: !dangerButton.enabled ? "#eef2f7" : dangerButton.down ? "#b91c1c" : dangerButton.hovered ? "#ef4444" : root.red
            border.color: color
        }
        contentItem: Text {
            text: dangerButton.text
            color: dangerButton.enabled ? "#ffffff" : "#94a3b8"
            font: dangerButton.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component FieldList: ListView {
        id: fieldList
        property var rowsModel: []
        model: rowsModel
        clip: true
        spacing: 4

        delegate: Rectangle {
            id: fieldDelegate
            required property var modelData
            width: fieldList.width
            height: Math.max(34, fieldValue.implicitHeight + 14)
            radius: 4
            color: "#ffffff"
            border.color: root.softBorderColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8

                Text {
                    text: String(fieldDelegate.modelData.type_name || fieldDelegate.modelData.name || "")
                    color: root.textMuted
                    font.family: root.uiFontFamily
                    font.pixelSize: root.uiSmallFontSize
                    Layout.preferredWidth: 230
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    id: fieldValue
                    text: String(fieldDelegate.modelData.value || "")
                    color: root.textMain
                    font.family: root.uiFontFamily
                    font.pixelSize: root.uiSmallFontSize
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
