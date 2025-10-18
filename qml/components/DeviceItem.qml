import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    height: 70
    color: selected ? Theme.highlightColor : (mouseArea.containsMouse ? Theme.borderColor : "transparent")
    radius: 5
    
    property string deviceName: ""
    property string deviceId: ""
    property bool connected: false
    property bool selected: false
    
    signal clicked()
    signal doubleClicked()
    signal deleteClicked()
    signal editClicked()
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
        onDoubleClicked: root.doubleClicked()
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // 设备头像
        Rectangle {
            width: 50
            height: 50
            radius: 25
            color: Theme.borderColor
            
            Text {
                anchors.centerIn: parent
                text: deviceName.substring(0, 2)
                font.pixelSize: 18
                color: Theme.textColor
            }
        }
        
        // 设备信息
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            
            Text {
                text: deviceName
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: Theme.primaryColor
            }
            
            Text {
                text: connected ? "🟢 已连接" : "⚪ 未连接"
                font.pixelSize: Theme.fontSizeSmall
                color: connected ? Theme.successColor : Theme.textColor
                opacity: 0.8
            }
        }
        
        // 操作按钮
        Row {
            spacing: 5
            
            // 编辑按钮
            Button {
                text: "✏️"
                width: 30
                height: 30
                font.pixelSize: 14
                ToolTip.visible: hovered
                ToolTip.text: "编辑设备"
                onClicked: root.editClicked()
            }
            
            // 删除按钮
            Button {
                text: "🗑️"
                width: 30
                height: 30
                font.pixelSize: 14
                ToolTip.visible: hovered
                ToolTip.text: "删除设备"
                onClicked: {
                    deleteConfirmDialog.open()
                }
            }
        }
    }
    
    // 删除确认对话框
    Dialog {
        id: deleteConfirmDialog
        title: "确认删除"
        modal: true
        anchors.centerIn: parent
        width: 300
        height: 150
        
        contentItem: Text {
            text: "确定要删除设备 \"" + deviceName + "\" 吗？\n删除后无法恢复。"
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.textColor
        }
        
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        onAccepted: {
            root.deleteClicked()
        }
    }
}

