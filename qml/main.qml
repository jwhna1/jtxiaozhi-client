import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "components"
import "theme"

ApplicationWindow {
    id: appWindow
    width: 1200
    height: 720
    minimumWidth: 960
    minimumHeight: 600
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowSystemMenuHint |
           Qt.WindowMinimizeButtonHint | Qt.WindowMaximizeButtonHint
    title: appModel.getAppTitle()
    color: "transparent"
    property int windowRadius: 12
    property int resizeMargin: 8

    background: Rectangle {
        radius: appWindow.windowRadius
        color: Theme.backgroundColor
        border.width: 0
    }

    Item {
        id: contentRoot
        anchors.fill: parent
        anchors.margins: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TitleBar {
                id: titleBar
                Layout.fillWidth: true
                Layout.preferredHeight: 60
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    SideBar {
                        id: sideBar
                        Layout.preferredWidth: 280
                        Layout.maximumWidth: 320
                        Layout.fillHeight: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 16

                        ChatArea {
                            id: chatArea
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        MessageInput {
                            id: messageInput
                            Layout.fillWidth: true
                            Layout.preferredHeight: 120
                        }
                    }
                }
            }
        }
    }

    // 设置对话框
    SettingsDialog {
        id: settingsDialog
        audioDeviceManager: appModel.audioDeviceManager
    }

    // 版本升级对话框
    UpdateDialog {
        id: updateDialog
        updateManager: appModel.updateManager
    }

    // 边缘拖拽区域用于调整窗口大小
    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: appWindow.resizeMargin
        hoverEnabled: true
        cursorShape: Qt.SizeHorCursor
        z: 999
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                appWindow.startSystemResize(Qt.LeftEdge, mouse)
        }
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: appWindow.resizeMargin
        hoverEnabled: true
        cursorShape: Qt.SizeHorCursor
        z: 999
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                appWindow.startSystemResize(Qt.RightEdge, mouse)
        }
    }

    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: appWindow.resizeMargin
        hoverEnabled: true
        cursorShape: Qt.SizeVerCursor
        z: 999
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                appWindow.startSystemResize(Qt.TopEdge, mouse)
        }
    }

    MouseArea {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: appWindow.resizeMargin
        hoverEnabled: true
        cursorShape: Qt.SizeVerCursor
        z: 999
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                appWindow.startSystemResize(Qt.BottomEdge, mouse)
        }
    }

    MouseArea {
        width: appWindow.resizeMargin * 2
        height: appWindow.resizeMargin * 2
        anchors.left: parent.left
        anchors.top: parent.top
        hoverEnabled: true
        cursorShape: Qt.SizeFDiagCursor
        z: 1000
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                appWindow.startSystemResize(Qt.TopEdge | Qt.LeftEdge, mouse)
        }
    }

    MouseArea {
        width: appWindow.resizeMargin * 2
        height: appWindow.resizeMargin * 2
        anchors.right: parent.right
        anchors.top: parent.top
        hoverEnabled: true
        cursorShape: Qt.SizeBDiagCursor
        z: 1000
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                appWindow.startSystemResize(Qt.TopEdge | Qt.RightEdge, mouse)
        }
    }

    MouseArea {
        width: appWindow.resizeMargin * 2
        height: appWindow.resizeMargin * 2
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        hoverEnabled: true
        cursorShape: Qt.SizeBDiagCursor
        z: 1000
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                appWindow.startSystemResize(Qt.BottomEdge | Qt.LeftEdge, mouse)
        }
    }

    MouseArea {
        width: appWindow.resizeMargin * 2
        height: appWindow.resizeMargin * 2
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        hoverEnabled: true
        cursorShape: Qt.SizeFDiagCursor
        z: 1000
        acceptedButtons: Qt.LeftButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                appWindow.startSystemResize(Qt.BottomEdge | Qt.RightEdge, mouse)
        }
    }
}
