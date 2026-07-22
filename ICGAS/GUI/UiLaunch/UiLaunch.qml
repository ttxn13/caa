import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Window 6.8

Window {
    id: uiLaunch
    objectName: "UiICGASClass"
    width: 720
    height: 540
    visible: false
    title: "UiICGAS"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinMaxButtonsHint

    property alias userName: edit_user.text
    property alias password: edit_pass.text
    property alias messageText: msg_pass.text
    property bool startEnabled: false
    property point dragPressPos: Qt.point(0, 0)
    property bool fallbackMove: false

    signal loginClicked(string userName, string password)
    signal startClicked()
    signal closeClicked()

    Image {
        id: centralWidget
        objectName: "centralWidget"
        anchors.fill: parent
        source: "qrc:/UiICGAS/back.png"
        fillMode: Image.Stretch
    }

    MouseArea {
        id: windowMoveArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton

        onPressed: function(mouse) {
            uiLaunch.dragPressPos = Qt.point(mouse.x, mouse.y)
            uiLaunch.fallbackMove = !uiLaunch.startSystemMove()
        }

        onPositionChanged: function(mouse) {
            if (pressed && uiLaunch.fallbackMove) {
                uiLaunch.x += mouse.x - uiLaunch.dragPressPos.x
                uiLaunch.y += mouse.y - uiLaunch.dragPressPos.y
            }
        }

        onReleased: uiLaunch.fallbackMove = false
        onCanceled: uiLaunch.fallbackMove = false
    }

    Button {
        id: push_start
        objectName: "push_start"
        x: 121
        y: 381
        width: 191
        height: 71
        enabled: uiLaunch.startEnabled
        text: "开始"
        padding: 0
        font.family: "新宋体"
        font.pointSize: 20
        font.bold: false
        font.italic: false

        contentItem: Text {
            text: push_start.text
            color: push_start.enabled ? (push_start.hovered ? "white" : "black") : "white"
            font: push_start.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: push_start.enabled
                   ? (push_start.hovered ? Qt.rgba(50 / 255, 105 / 255, 232 / 255, 200 / 255)
                                         : Qt.rgba(1, 1, 1, 200 / 255))
                   : Qt.rgba(169 / 255, 169 / 255, 169 / 255, 200 / 255)
        }

        onClicked: uiLaunch.startClicked()
    }

    Text {
        id: label
        objectName: "label"
        x: 11
        y: 31
        width: 701
        height: 91
        text: "作战行动方案智能生成与评估软件"
        color: "black"
        font.family: "新宋体"
        font.pointSize: 26
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Item {
        id: layoutWidget
        objectName: "layoutWidget"
        x: 121
        y: 146
        width: 481
        height: 181

        Text {
            id: label_user
            objectName: "label_user"
            x: 0
            y: 9
            width: 165
            height: 40
            text: "用户名："
            color: "black"
            font.family: "新宋体"
            font.pointSize: 18
            verticalAlignment: Text.AlignVCenter
        }

        TextField {
            id: edit_user
            objectName: "edit_user"
            x: 171
            y: 9
            width: 310
            height: 40
            padding: 4
            font.family: "新宋体"
            font.pointSize: 14
            selectByMouse: true

            background: Rectangle {
                color: Qt.rgba(1, 1, 1, 150 / 255)
            }
        }

        Text {
            id: label_pass
            objectName: "label_pass"
            x: 0
            y: 64
            width: 165
            height: 40
            text: "密  码："
            color: "black"
            font.family: "新宋体"
            font.pointSize: 18
            verticalAlignment: Text.AlignVCenter
        }

        TextField {
            id: edit_pass
            objectName: "edit_pass"
            x: 171
            y: 64
            width: 310
            height: 40
            padding: 4
            font.family: "新宋体"
            font.pointSize: 14
            echoMode: TextInput.Password
            selectByMouse: true

            background: Rectangle {
                color: Qt.rgba(1, 1, 1, 150 / 255)
            }
        }

        TextField {
            id: msg_pass
            objectName: "msg_pass"
            x: 0
            y: 129
            width: 335
            height: 30
            text: "请输入用户名及密码！"
            readOnly: true
            padding: 0
            font.family: "新宋体"
            font.pointSize: 16
            color: "black"
            selectionColor: "transparent"
            selectedTextColor: "black"

            background: Rectangle {
                color: "transparent"
            }
        }

        Button {
            id: push_login
            objectName: "push_login"
            x: 341
            y: 119
            width: 140
            height: 50
            text: "确认"
            padding: 0
            font.family: "新宋体"
            font.pointSize: 16

            contentItem: Text {
                text: push_login.text
                color: push_login.hovered ? "white" : "black"
                font: push_login.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: push_login.hovered ? Qt.rgba(50 / 255, 105 / 255, 232 / 255, 200 / 255)
                                          : Qt.rgba(1, 1, 1, 200 / 255)
            }

            onClicked: uiLaunch.loginClicked(edit_user.text, edit_pass.text)
        }
    }

    Button {
        id: push_close
        objectName: "push_close"
        x: 410
        y: 380
        width: 191
        height: 71
        text: "退出"
        padding: 0
        font.family: "新宋体"
        font.pointSize: 20
        font.bold: false
        font.italic: false

        contentItem: Text {
            text: push_close.text
            color: push_close.hovered ? "white" : "black"
            font: push_close.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: push_close.hovered ? Qt.rgba(50 / 255, 105 / 255, 232 / 255, 200 / 255)
                                      : Qt.rgba(1, 1, 1, 200 / 255)
        }

        onClicked: {
            uiLaunch.closeClicked()
            uiLaunch.close()
        }
    }
}
