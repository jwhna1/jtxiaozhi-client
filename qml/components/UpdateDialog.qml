/*
Project: jtxiaozhi-client
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-18T11:15:00Z
File: UpdateDialog.qml
Desc: 版本升级对话框
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import XiaozhiClient
import "../theme"

Dialog {
    id: root
    title: "🚀 版本升级"
    modal: true
    width: 600
    height: 500
    anchors.centerIn: parent
    closePolicy: Dialog.CloseOnEscape | Popup.CloseOnPressOutside

    // 自定义属性
    property var updateManager: null
    property alias downloadProgress: progressBar.value

    background: Rectangle {
        color: Theme.backgroundColor
        radius: 12
        border.width: 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // 头部信息
        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            // 图标
            Rectangle {
                width: 48
                height: 48
                radius: 24
                color: Theme.primaryColor

                Text {
                    anchors.centerIn: parent
                    text: "🔄"
                    font.pixelSize: 24
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    text: "发现新版本"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textColor
                }

                Text {
                    text: updateManager ?
                           ("当前版本: " + updateManager.currentVersion + " → 最新版本: " + updateManager.latestVersion) :
                           "正在检查版本信息..."
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.primaryColor
                }
            }
        }

        // 版本信息
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: Theme.highlightColor
            radius: 8
            border.width: 0

            ScrollView {
                anchors.fill: parent
                anchors.margins: 15

                TextArea {
                    id: releaseNotesText
                    text: (updateManager && updateManager.releaseInfo && updateManager.releaseInfo.isValid) ?
                         (updateManager.releaseInfo.body + "\n\n📂 下载地址: https://github.com/jwhna1/jtxiaozhi-client/releases") :
                         "正在获取更新内容..."
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.textColor
                    wrapMode: TextArea.Wrap
                    readOnly: true
                    selectByMouse: true
                    background: Item {}

                    onLinkActivated: (link) => {
                        Qt.openUrlExternally(link)
                    }
                }
            }
        }

        // 下载进度
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            visible: updateManager && updateManager.status !== undefined && updateManager.status === UpdateManager.Downloading

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10

                Text {
                    text: updateManager ? (updateManager.updateStatusText || "准备下载...") : "准备下载..."
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.textColor
                }

                ProgressBar {
                    id: progressBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: 8
                    value: updateManager ? (updateManager.downloadProgress || 0) : 0

                    background: Rectangle {
                        color: Theme.borderColor
                        radius: 4
                        border.width: 0
                    }

                    contentItem: Rectangle {
                        color: Theme.primaryColor
                        radius: 4
                        width: progressBar.visualPosition * progressBar.width
                    }
                }
            }
        }

        // 状态信息
        Text {
            Layout.fillWidth: true
            text: updateManager ? (updateManager.updateStatusText || "") : ""
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.textColor
            visible: updateManager && updateManager.status !== undefined && updateManager.status !== UpdateManager.Downloading
            horizontalAlignment: Text.AlignHCenter
        }

        // 按钮区域
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight

            // GitHub链接按钮
            Button {
                text: "📂 GitHub"
                Layout.preferredWidth: 100
                Layout.preferredHeight: 36
                visible: updateManager && updateManager.status !== undefined && updateManager.status === UpdateManager.UpdateAvailable

                background: Rectangle {
                    color: parent.hovered ? "#24292e" : "#2f363d"
                    radius: 6
                    border.width: 1
                    border.color: "#d1d5db"
                }

                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSizeMedium
                }

                onClicked: {
                    Qt.openUrlExternally("https://github.com/jwhna1/jtxiaozhi-client/releases")
                }
            }

            // 取消按钮
            Button {
                text: "取消"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 36
                visible: updateManager && updateManager.status !== undefined && updateManager.status === UpdateManager.UpdateAvailable

                background: Rectangle {
                    color: parent.hovered ? Theme.buttonPressedColor : Theme.buttonColor
                    radius: 6
                    border.width: 1
                    border.color: "#d1d5db"
                }

                contentItem: Text {
                    text: parent.text
                    color: Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSizeMedium
                }

                onClicked: root.close()
            }

            // 稍后提醒按钮
            Button {
                text: "稍后提醒"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 36
                visible: updateManager && updateManager.status !== undefined && updateManager.status === UpdateManager.UpdateAvailable

                background: Rectangle {
                    color: parent.hovered ? Theme.highlightColor : Theme.buttonColor
                    radius: 6
                    border.width: 1
                    border.color: "#d1d5db"
                }

                contentItem: Text {
                    text: parent.text
                    color: Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSizeMedium
                }

                onClicked: root.close()
            }

            // 下载按钮
            Button {
                text: "立即更新"
                Layout.preferredWidth: 100
                Layout.preferredHeight: 36
                visible: updateManager && updateManager.status !== undefined && updateManager.status === UpdateManager.UpdateAvailable
                enabled: updateManager && updateManager.releaseInfo && updateManager.releaseInfo.isValid

                background: Rectangle {
                    color: parent.enabled ? (parent.hovered ? Theme.primaryColor : "#4CAF50") : Theme.buttonColor
                    radius: 6
                    border.width: 1
                    border.color: parent.enabled ? (parent.hovered ? Theme.primaryColor : "#45a049") : "#d1d5db"
                }

                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "#FFFFFF" : Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSizeMedium
                    font.bold: true
                }

                onClicked: {
                    if (updateManager) {
                        updateManager.downloadUpdate()
                    }
                }
            }

            // 安装按钮
            Button {
                text: "安装更新"
                Layout.preferredWidth: 100
                Layout.preferredHeight: 36
                visible: updateManager && updateManager.status !== undefined && updateManager.status === UpdateManager.InstallReady

                background: Rectangle {
                    color: parent.hovered ? Theme.primaryColor : "#4CAF50"
                    radius: 6
                    border.width: 1
                    border.color: parent.hovered ? Theme.primaryColor : "#45a049"
                }

                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSizeMedium
                    font.bold: true
                }

                onClicked: {
                    if (updateManager) {
                        updateManager.installUpdate()
                        root.close()
                    }
                }
            }

            // 重试按钮
            Button {
                text: "重试"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 36
                visible: updateManager && updateManager.status !== undefined && (updateManager.status === UpdateManager.DownloadFailed ||
                                        updateManager.status === UpdateManager.Checking)

                background: Rectangle {
                    color: parent.hovered ? Theme.warningColor : Theme.buttonColor
                    radius: 6
                    border.width: 1
                    border.color: parent.hovered ? Theme.warningColor : "#d1d5db"
                }

                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#FFFFFF" : Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontSizeMedium
                }

                onClicked: {
                    if (updateManager) {
                        updateManager.checkForUpdates(false)
                    }
                }
            }
        }
    }

    // 监听更新管理器状态变化
    Connections {
        target: updateManager

        function onStatusChanged() {
            if (!updateManager || updateManager.status === undefined) return

            switch(updateManager.status) {
                case UpdateManager.UpdateAvailable:
                    root.open()
                    break
                case UpdateManager.NoUpdateAvailable:
                    if (root.visible) {
                        root.close()
                    }
                    break
                case UpdateManager.InstallReady:
                    // 安装就绪时保持对话框打开
                    break
                case UpdateManager.DownloadFailed:
                    // 下载失败时保持对话框打开，显示重试按钮
                    break
            }
        }
    }

    // 手动检查更新
    function checkForUpdates() {
        if (updateManager) {
            root.open()
            updateManager.checkForUpdates(false)
        }
    }

    // 显示更新可用
    function showUpdateAvailable() {
        root.open()
    }
}