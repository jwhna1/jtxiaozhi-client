import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    color: Theme.backgroundColor
    border.width: 0
    
    // 窗口拖动区域
    MouseArea {
        id: dragArea
        anchors.fill: parent
        anchors.rightMargin: controlButtons.width
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                Window.window.startSystemMove()
            }
        }
        
        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                if (Window.window.visibility === Window.Maximized) {
                    Window.window.showNormal()
                } else {
                    Window.window.showMaximized()
                }
            }
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15
        
        // 左侧：程序名称和版本
        Row {
            spacing: 15
            Layout.fillWidth: true
            
            // 应用图标
            Rectangle {
                width: 32
                height: 32
                radius: 16
                color: Theme.primaryColor
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    anchors.centerIn: parent
                    text: "智"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#FFFFFF"
                }
            }
            
            // 程序名称和版本
            Text {
                text: appModel.getAppTitle()
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: Theme.textColor
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // 右侧：控制按钮
        Row {
            id: controlButtons
            spacing: 5
            
            // 设置按钮
            Button {
                text: "⚙️"
                width: 35
                height: 35
                onClicked: settingsDialog.open()
                ToolTip.visible: hovered
                ToolTip.text: "设置"
                ToolTip.delay: 500
            }
            
            // 主题切换按钮
            Button {
                text: appModel.isDarkTheme ? "☀" : "🌙"
                width: 35
                height: 35
                onClicked: appModel.toggleTheme()
                ToolTip.visible: hovered
                ToolTip.text: appModel.isDarkTheme ? "切换到浅色主题" : "切换到深色主题"
                ToolTip.delay: 500
            }
            
            // 最小化按钮
            Button {
                text: "—"
                width: 35
                height: 35
                onClicked: Window.window.showMinimized()
            }
            
            // 最大化/还原按钮
            Button {
                text: Window.window.visibility === Window.Maximized ? "❐" : "□"
                width: 35
                height: 35
                onClicked: {
                    if (Window.window.visibility === Window.Maximized) {
                        Window.window.showNormal()
                    } else {
                        Window.window.showMaximized()
                    }
                }
            }
            
            // 关闭按钮
            Button {
                text: "✕"
                width: 35
                height: 35
                onClicked: Qt.quit()
                
                background: Rectangle {
                    color: parent.hovered ? Theme.errorColor : Theme.backgroundColor
                    radius: 3
                }
                
                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#FFFFFF" : Theme.errorColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
    
  }

