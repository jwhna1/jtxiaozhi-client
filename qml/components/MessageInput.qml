import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    color: Theme.backgroundColor
    border.width: 0
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        
        // 文本输入框
        TextField {
            id: messageTextField
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            placeholderText: "输入文字发送......"

            background: Rectangle {
                color: Theme.backgroundColor
                radius: 6
                border.width: 1
                border.color: parent.activeFocus ? Theme.primaryColor : (parent.hovered ? Theme.borderColor : "#e0e0e0")
            }

            Keys.onReturnPressed: {
                sendMessage()
            }
        }
        
        // 发送按钮
        Button {
            text: "📤"
            width: 50
            height: 40
            font.pixelSize: 20
            ToolTip.visible: hovered
            ToolTip.text: "发送消息"

            background: Rectangle {
                color: parent.hovered ? Theme.primaryColor : Theme.buttonColor
                radius: 6
                border.width: 1
                border.color: parent.hovered ? Theme.primaryColor : "#d1d5db"
            }

            contentItem: Text {
                text: parent.text
                color: parent.hovered ? "#FFFFFF" : Theme.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: parent.font.pixelSize
            }

            onClicked: sendMessage()
        }
        
        // 右侧功能按钮
        Row {
            spacing: 8  // 增加按钮间距，让界面更清爽

            // 麦克风按钮（录音）
            Button {
                width: 50
                height: 40
                text: appModel.conversationManager && appModel.conversationManager.isRecording ? "⏹️" : "🎤"
                font.pixelSize: 20
                ToolTip.visible: hovered
                ToolTip.text: appModel.conversationManager && appModel.conversationManager.isRecording ? "停止录音" : "开始对话"
                enabled: appModel.connected  // MQTT连接后即可用

                background: Rectangle {
                    color: parent.enabled ? (parent.hovered ? Theme.highlightColor : Theme.buttonColor) : "transparent"
                    radius: 6
                    border.width: 1
                    border.color: parent.enabled ? (parent.hovered ? Theme.highlightColor : "#d1d5db") : "transparent"
                }

                onClicked: {
                    if (appModel.conversationManager) {
                        if (appModel.conversationManager.isRecording) {
                            appModel.conversationManager.stopRecording()
                        } else {
                            // 点击开始对话，自动建立UDP（如果未建立）
                            appModel.conversationManager.startConversation()
                        }
                    }
                }
            }

            // 中止按钮（说话状态时显示）
            Button {
                width: 50
                height: 40
                text: "⏸️"
                font.pixelSize: 20
                ToolTip.visible: hovered
                ToolTip.text: "中止说话"
                visible: appModel.conversationManager && appModel.conversationManager.state === 2  // Speaking = 2
                enabled: appModel.conversationManager !== null

                background: Rectangle {
                    color: parent.enabled ? (parent.hovered ? Theme.warningColor : Theme.buttonColor) : "transparent"
                    radius: 6
                    border.width: 1
                    border.color: parent.enabled ? (parent.hovered ? Theme.warningColor : "#d1d5db") : "transparent"
                }

                onClicked: {
                    if (appModel.conversationManager) {
                        appModel.conversationManager.abortSpeaking()
                    }
                }
            }
        }
    }
    
    function sendMessage() {
        var text = messageTextField.text.trim()
        if (text !== "") {
            appModel.sendTextMessage(text)
            messageTextField.text = ""
        }
    }
    
}

