import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    color: Theme.backgroundColor
    border.width: 0
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // 状态信息栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 70  // 稍微增加高度，为按钮留出更多空间
            color: Theme.highlightColor
            radius: 8  // 增加圆角，与按钮保持一致

            RowLayout {
                anchors.fill: parent
                anchors.margins: 18  // 增加边距，给按钮更多呼吸空间
                spacing: 20  // 增加元素间距，避免视觉拥挤

                Text {
                    text: "状态："
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.textColor
                }

                Text {
                    text: appModel.statusMessage
                    font.pixelSize: Theme.fontSizeMedium
                    font.bold: true
                    color: appModel.connected ? Theme.successColor : Theme.textColor
                }

                // 使用间距代替分割线
                Item {
                    Layout.preferredWidth: 1
                    visible: appModel.conversationManager !== null
                }

                Text {
                    text: {
                        if (!appModel.conversationManager) return ""
                        switch(appModel.conversationManager.state) {
                            case 0: return "💤 空闲"     // Idle
                            case 1: return "👂 聆听中"   // Listening
                            case 2: return "🗣️ 说话中"   // Speaking
                            default: return ""
                        }
                    }
                    font.pixelSize: Theme.fontSizeMedium
                    font.bold: true
                    color: {
                        if (!appModel.conversationManager) return Theme.textColor
                        switch(appModel.conversationManager.state) {
                            case 1: return "#4CAF50"  // 绿色（聆听）
                            case 2: return "#2196F3"  // 蓝色（说话）
                            default: return Theme.textColor
                        }
                    }
                    visible: appModel.conversationManager !== null
                }
                
                Item {
                    Layout.fillWidth: true
                }
                
                // 连接按钮（触发完整OTA流程：OTA -> MQTT -> UDP自动建立）
                Button {
                    text: appModel.connected ? "🔌 断开连接" : "🔗 连接"
                    Layout.preferredHeight: 40  // 增加高度，更好地融入状态栏
                    Layout.preferredWidth: 120
                    Layout.alignment: Qt.AlignVCenter  // 垂直居中对齐
                    font.pixelSize: Theme.fontSizeMedium
                    
                    background: Rectangle {
                        // 优化背景色，实现更好的视觉融合
                        color: {
                            if (parent.hovered) {
                                return appModel.connected ? "#ff6b6b" : "#4CAF50"  // 悬停时显示状态色
                            } else {
                                return appModel.connected ? "#e8f4fd" : "#e8f4fd"  // 默认使用更浅的蓝色，与状态栏协调
                            }
                        }
                        radius: 8  // 增加圆角，更现代化
                        border.width: 1
                        border.color: {
                            if (parent.hovered) {
                                return appModel.connected ? "#e53e3e" : "#45a049"
                            } else {
                                return appModel.connected ? "#d1d5db" : "#d1d5db"
                            }
                        }
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: {
                            if (parent.parent.hovered) {
                                return "#FFFFFF"  // 悬停时白色文字
                            } else {
                                return appModel.connected ? Theme.textColor : Theme.textColor  // 默认文字色
                            }
                        }
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: parent.font.pixelSize
                        font.bold: (parent.parent && parent.parent.hovered) || false
                    }
                    
                    // 添加平滑的过渡动画
                    Behavior on scale {
                        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
                    }
                    
                    // 悬停效果
                    onHoveredChanged: {
                        if (hovered) {
                            scale = 1.05
                        } else {
                            scale = 1.0
                        }
                    }
                    
                    onClicked: {
                        if (appModel.connected) {
                            appModel.disconnectDevice(appModel.currentDeviceId)
                        } else {
                            // 连接：获取OTA配置 -> 自动连接MQTT -> 自动发送hello -> 自动建立UDP
                            appModel.connectDevice(appModel.currentDeviceId)
                        }
                    }
                }
            }
        }
        
        // 聊天消息列表（替换原日志显示）
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ListView {
                id: chatListView
                spacing: 8
                
                model: appModel.chatMessages  // 绑定到AppModel
                
                delegate: ChatBubble {
                    width: chatListView.width
                    // 使用modelData访问QVariantMap中的字段
                    messageId: (modelData && modelData.id) ? modelData.id : -1
                    isUser: modelData && (modelData.messageType === "stt" || 
                                          modelData.messageType === "text" || 
                                          modelData.messageType === "image")
                    messageText: (modelData && modelData.textContent) ? modelData.textContent : ""
                    audioPath: (modelData && modelData.audioFilePath) ? modelData.audioFilePath : ""
                    imagePath: (modelData && modelData.imagePath) ? modelData.imagePath : ""
                    isPlaying: (modelData && modelData.isPlaying) ? modelData.isPlaying : false
                    timestamp: (modelData && modelData.timestamp) ? modelData.timestamp : 0
                    messageType: (modelData && modelData.messageType) ? modelData.messageType : ""
                }
                
                // 自动滚动到底部
                onCountChanged: {
                    Qt.callLater(() => {
                        chatListView.positionViewAtEnd()
                    })
                }
            }
        }
    }
    
    // 监听设备切换，刷新聊天记录
    Connections {
        target: appModel
        function onCurrentDeviceIdChanged() {
            // AppModel内部会自动加载新设备的聊天记录
        }
    }
}

