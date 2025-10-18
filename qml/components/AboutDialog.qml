/*
Project: jtxiaozhi-client
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-13T15:35:00Z
File: AboutDialog.qml
Desc: 关于对话框组件 - 显示程序信息和版本详情
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../theme"

Popup {
    id: aboutDialog
    width: 800
    height: 600
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    // 使用窗口 Overlay 进行全窗口居中
    anchors.centerIn: Overlay.overlay
    
    // 对话框背景
    background: Rectangle {
        color: Theme.backgroundColor
        border.width: 0
        radius: 12
        
        // 添加拖拽功能
        MouseArea {
            anchors.fill: parent
            drag.target: root
            drag.axis: Drag.XAndYAxis
            drag.minimumX: 0
            drag.minimumY: 0
            drag.maximumX: (Overlay.overlay ? Overlay.overlay.width - root.width : 0)
            drag.maximumY: (Overlay.overlay ? Overlay.overlay.height - root.height : 0)
            cursorShape: Qt.SizeAllCursor
            acceptedButtons: Qt.LeftButton
        }
    }
    
    // 标题栏
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 50
        color: "transparent"
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12
            
            // 应用图标
            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                radius: 16
                color: Theme.primaryColor
                
                Text {
                    anchors.centerIn: parent
                    text: "智"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#FFFFFF"
                }
            }
            
            Text {
                text: "关于小智跨平台客户端"
                font.pixelSize: 18
                font.bold: true
                color: Theme.textColor
                Layout.fillWidth: true
            }
            
            // 关闭按钮
            Button {
                text: "✕"
                width: 28
                height: 28
                font.pixelSize: 14
                background: Rectangle {
                    color: parent.hovered ? "#ff6b6b" : "transparent"
                    radius: 14
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#FFFFFF" : Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: aboutDialog.close()
            }
        }
    }
    
    // 主内容区域
    ScrollView {
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footerBar.top
        clip: true
        
        ColumnLayout {
            width: aboutDialog.width - 32
            spacing: 16
            anchors.margins: 16
            
            // 应用信息卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    TextEdit {
                        text: appModel.getAppTitle()
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textColor
                        Layout.alignment: Qt.AlignHCenter
                        readOnly: true
                        selectByMouse: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    TextEdit {
                        text: "基于虾哥xiaozhi-esp32固件源码灵感编写 | 免费使用 | 交流无套路"
                        font.pixelSize: 12
                        color: Theme.textColor
                        Layout.alignment: Qt.AlignHCenter
                        readOnly: true
                        selectByMouse: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
            
            // 服务器兼容性说明
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    TextEdit {
                        text: "服务器兼容性说明"
                        font.pixelSize: 16
                        font.bold: true
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                    }

                    TextEdit {
                        text: "• 虾哥官方服务器：https://xiaozhi.me/\n• xinnan-tech开源服务器：https://github.com/xinnan-tech/xiaozhi-esp32-server\n• jtxiaozhi-server商用服务版：支持高级功能和订制优化。可联系我们获取商业服务端+app购买\n• 其他版本：暂未测试，如有不兼容可反馈后续兼容"
                        font.pixelSize: 11
                        color: Theme.textColor
                        wrapMode: TextEdit.WordWrap
                        Layout.fillWidth: true
                        readOnly: true
                        selectByMouse: true
                    }
                }
            }
            
            // 重要链接信息
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 140
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    TextEdit {
                        text: "相关链接"
                        font.pixelSize: 16
                        font.bold: true
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: 4
                        columnSpacing: 12
                        Layout.fillWidth: true

                        TextEdit { text: "ESP32固件源码:"; color: Theme.textColor; font.pixelSize: 11; font.bold: true; readOnly: true; selectByMouse: true }
                        TextEdit {
                            Layout.fillWidth: true
                            text: "https://github.com/78/xiaozhi-esp32"
                            font.pixelSize: 11
                            color: Theme.primaryColor
                            readOnly: true
                            selectByMouse: true
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally("https://github.com/78/xiaozhi-esp32")
                            }
                        }

                        TextEdit { text: "开源服务器:"; color: Theme.textColor; font.pixelSize: 11; font.bold: true; readOnly: true; selectByMouse: true }
                        TextEdit {
                            Layout.fillWidth: true
                            text: "https://github.com/xinnan-tech/xiaozhi-esp32-server"
                            font.pixelSize: 11
                            color: Theme.primaryColor
                            readOnly: true
                            selectByMouse: true
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally("https://github.com/xinnan-tech/xiaozhi-esp32-server")
                            }
                        }

                        TextEdit { text: "Git仓库地址:"; color: Theme.textColor; font.pixelSize: 11; font.bold: true; readOnly: true; selectByMouse: true }
                        TextEdit {
                            Layout.fillWidth: true
                            text: "https://github.com/jwhna1"
                            font.pixelSize: 11
                            color: Theme.primaryColor
                            readOnly: true
                            selectByMouse: true
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally("https://github.com/jwhna1")
                            }
                        }

                        TextEdit { text: "B站地址:"; color: Theme.textColor; font.pixelSize: 11; font.bold: true; readOnly: true; selectByMouse: true }
                        TextEdit {
                            Layout.fillWidth: true
                            text: "https://space.bilibili.com/298384872"
                            font.pixelSize: 11
                            color: Theme.primaryColor
                            readOnly: true
                            selectByMouse: true
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally("https://space.bilibili.com/298384872")
                            }
                        }
                    }
                }
            }
            
            // 联系方式信息
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    TextEdit {
                        text: "联系我们"
                        font.pixelSize: 16
                        font.bold: true
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: 2
                        columnSpacing: 12
                        Layout.fillWidth: true

                        TextEdit { text: "开发团队:"; color: Theme.textColor; font.pixelSize: 11; font.bold: true; readOnly: true; selectByMouse: true }
                        TextEdit { text: "jtserver团队(曾能混，tang先森)"; color: Theme.primaryColor; font.pixelSize: 11; readOnly: true; selectByMouse: true; Layout.fillWidth: true }

                        TextEdit { text: "商务邮箱:"; color: Theme.textColor; font.pixelSize: 11; font.bold: true; readOnly: true; selectByMouse: true }
                        TextEdit { text: "jwhna1@gmail.com"; color: Theme.primaryColor; font.pixelSize: 11; readOnly: true; selectByMouse: true; Layout.fillWidth: true }

                        TextEdit { text: "联系方式:"; color: Theme.textColor; font.pixelSize: 11; font.bold: true; readOnly: true; selectByMouse: true }
                        TextEdit { text: "QQ: 7280051 | 微信: cxshow066（添加好友时请注明来意）"; color: Theme.primaryColor; font.pixelSize: 11; readOnly: true; selectByMouse: true; Layout.fillWidth: true; wrapMode: TextEdit.Wrap }
                    }
                }
            }
            
            // 免费声明
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    TextEdit {
                        text: "免费使用声明"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#e74c3c"
                        readOnly: true
                        selectByMouse: true
                    }

                    TextEdit {
                        text: "本程序为免费使用，交流无套路，任何向您收取费用的行为与本团队无关。我们承诺软件永久免费，不会以任何形式收取使用费用。如遇到有人收费，请警惕并向我们举报。"
                        font.pixelSize: 11
                        color: Theme.textColor
                        wrapMode: TextEdit.WordWrap
                        Layout.fillWidth: true
                        readOnly: true
                        selectByMouse: true
                    }

                    TextEdit {
                        text: "© 2025 jtserver团队. ."
                        font.pixelSize: 10
                        color: Theme.timestampColor
                        Layout.alignment: Qt.AlignHCenter
                        readOnly: true
                        selectByMouse: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }
    
    // 底部按钮
    Rectangle {
        id: footerBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: "transparent"
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12
            
            Item { Layout.fillWidth: true }

            Button {
                text: "检查更新"
                Layout.preferredWidth: 100
                Layout.preferredHeight: 32
                visible: appModel && appModel.updateManager

                background: Rectangle {
                    color: parent.hovered ? "#4CAF50" : Theme.buttonColor
                    radius: 6
                    border.width: 1
                    border.color: parent.hovered ? "#45a049" : "#d1d5db"
                }

                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#FFFFFF" : Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSizeMedium
                }

                onClicked: {
                    if (appModel && appModel.updateManager) {
                        var updateDialog = Qt.createQmlObject(
                            'import QtQuick 6.5; import "../components"; UpdateDialog { updateManager: appModel.updateManager }',
                            parent,
                            'dynamicUpdateDialog'
                        )
                        updateDialog.checkForUpdates()
                    }
                }
            }

            Button {
                text: "确定"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 32
                background: Rectangle {
                    color: parent.hovered ? Theme.primaryColor : Theme.buttonColor
                    radius: 6
                    border.width: 0
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#FFFFFF" : Theme.buttonTextColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                }
                onClicked: aboutDialog.close()
            }
        }
    }
}
