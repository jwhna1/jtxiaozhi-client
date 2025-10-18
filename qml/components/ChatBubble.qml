/*
Project: jtxiaozhi-client
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-12T08:30:00Z
File: ChatBubble.qml
Desc: 聊天气泡组件（支持文字和音频播放）
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import "../theme"

Rectangle {
    id: root
    
    // 属性
    property bool isUser: false      // true=用户(右侧), false=智能体(左侧)
    property string messageText: ""
    property string audioPath: ""    // 音频文件路径
    property string imagePath: ""    // 图片文件路径
    property bool hasAudio: audioPath !== ""
    property bool hasImage: imagePath !== ""
    property bool isPlaying: false
    property int messageId: -1
    property int timestamp: 0        // 消息时间戳
    property string messageType: ""  // 消息类型，用于判断是否为激活码消息
    
    width: parent.width
    height: bubbleColumn.height + 20
    color: "transparent"
    
    RowLayout {
        anchors.fill: parent
        spacing: 10
        
        // 左侧占位（用户在右侧 → 需要左侧拉伸）
        Item { 
            Layout.fillWidth: root.isUser
            visible: root.isUser
        }
        
        // 气泡容器
        Rectangle {
            Layout.preferredWidth: Math.min(bubbleColumn.implicitWidth + 20, root.width * 0.7)
            Layout.preferredHeight: bubbleColumn.height + 20
            radius: 10
            color: root.isUser ? Theme.userBubbleColor : Theme.aiBubbleColor
            border.width: 0
            
            ColumnLayout {
                id: bubbleColumn
                anchors.centerIn: parent
                width: parent.width - 20
                spacing: 8
                
                // 文字内容（使用TextEdit以支持文本选择和复制）
                TextEdit {
                    id: messageTextEdit
                    Layout.fillWidth: true
                    text: root.messageText
                    wrapMode: TextEdit.Wrap
                    color: root.isUser ? Theme.userTextColor : Theme.aiTextColor
                    font.pixelSize: Theme.fontSizeMedium
                    font.family: Theme.fontFamily
                    readOnly: true
                    selectByMouse: true
                    selectByKeyboard: true
                    
                    // 右键菜单
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: {
                            contextMenu.popup()
                        }
                    }
                    
                    Menu {
                        id: contextMenu
                        MenuItem {
                            text: "复制"
                            enabled: messageTextEdit.selectedText !== ""
                            onTriggered: {
                                messageTextEdit.copy()
                            }
                        }
                        MenuItem {
                            text: "全选"
                            onTriggered: {
                                messageTextEdit.selectAll()
                            }
                        }
                    }
                }
                
                // 激活码消息的特殊处理 - 简化版本，显示完整原始文本
                Rectangle {
                    visible: root.messageType === "activation"
                    Layout.fillWidth: true
                    Layout.preferredHeight: activationText.implicitHeight + 30
                    color: Theme.highlightColor
                    radius: 8
                    border.width: 0
                    
                    TextEdit {
                        id: activationText
                        anchors.fill: parent
                        anchors.margins: 15
                        text: root.messageText
                        wrapMode: TextEdit.Wrap
                        font.pixelSize: Theme.fontSizeMedium
                        color: Theme.textColor
                        readOnly: true
                        selectByMouse: true
                        selectByKeyboard: true
                        
                        // 右键菜单
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: {
                                activationContextMenu.popup()
                            }
                        }
                        
                        Menu {
                            id: activationContextMenu
                            MenuItem {
                                text: "复制"
                                enabled: activationText.selectedText !== ""
                                onTriggered: {
                                    activationText.copy()
                                }
                            }
                            MenuItem {
                                text: "全选"
                                onTriggered: {
                                    activationText.selectAll()
                                }
                            }
                        }
                    }
                }
                
                // 图片预览（如果有图片）
                Rectangle {
                    visible: root.hasImage
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: 150
                    color: "transparent"
                    border.width: 0
                    radius: 5
                    clip: true
                    
                    Image {
                        id: previewImage
                        anchors.fill: parent
                        anchors.margins: 2
                        source: root.hasImage ? ("file:///" + root.imagePath) : ""
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        cache: false
                        
                        // 加载指示器
                        BusyIndicator {
                            anchors.centerIn: parent
                            running: previewImage.status === Image.Loading
                            visible: running
                        }
                        
                        // 加载失败提示
                        Text {
                            anchors.centerIn: parent
                            visible: previewImage.status === Image.Error
                            text: "❌ 图片加载失败"
                            color: Theme.textColor
                        }

                        // 错误处理：停止重试加载
                        onStatusChanged: {
                            if (status === Image.Error) {
                                // 延迟清空source以停止重试，避免持续报错
                                errorTimer.restart()
                            }
                        }

                        Timer {
                            id: errorTimer
                            interval: 100
                            onTriggered: {
                                if (previewImage.status === Image.Error) {
                                    previewImage.source = ""
                                }
                            }
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: imageViewWindow.show()
                    }
                }
                
                // 音频播放按钮
                Button {
                    visible: root.hasAudio
                    Layout.alignment: Qt.AlignRight
                    text: root.isPlaying ? "⏸ 停止" : "▶ 播放音频"
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    
                    background: Rectangle {
                        color: parent.pressed ? Theme.buttonPressedColor : Theme.buttonColor
                        radius: 5
                        border.width: 0
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: Theme.buttonTextColor
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        if (root.isPlaying) {
                            appModel.stopAudioPlayback()
                        } else {
                            appModel.playAudioMessage(root.messageId)
                        }
                    }
                }
                
                // 时间戳
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: {
                        if (root.timestamp > 0) {
                            var date = new Date(root.timestamp);
                            return Qt.formatDateTime(date, "hh:mm:ss");
                        }
                        return "";
                    }
                    color: Theme.timestampColor
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                }
            }
        }
        
        // 右侧占位（智能体在左侧 → 需要右侧拉伸）
        Item { 
            Layout.fillWidth: !root.isUser
            visible: !root.isUser
        }
    }
    
    // 动画效果
    Behavior on opacity {
        NumberAnimation { duration: 200 }
    }
    
    // 鼠标悬停效果（不拦截子项点击）
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton  // 仅用于hover，不接收点击，避免吞掉按钮点击
        propagateComposedEvents: true
        onEntered: {
            if (root.hasAudio) root.opacity = 0.9
        }
        onExited: root.opacity = 1.0
    }
    
    // 图片查看窗口（独立窗口，可移动、可调整大小、支持缩放）
    Window {
        id: imageViewWindow
        title: "图片预览"
        modality: Qt.ApplicationModal
        flags: Qt.Window | Qt.FramelessWindowHint  // 无边框窗口
        color: "transparent"
        
        property real zoomScale: 0.5  // 缩放比例（默认50%）
        
        // 根据图片大小自适应窗口尺寸（默认50%）
        width: {
            if (fullImage.status === Image.Ready && fullImage.implicitWidth > 0) {
                return Math.min(fullImage.implicitWidth * 0.5 + 40, Screen.width * 0.9)
            }
            return 600
        }
        height: {
            if (fullImage.status === Image.Ready && fullImage.implicitHeight > 0) {
                return Math.min(fullImage.implicitHeight * 0.5 + 120, Screen.height * 0.9)
            }
            return 500
        }
        
        // 居中显示
        Component.onCompleted: {
            x = (Screen.width - width) / 2
            y = (Screen.height - height) / 2
        }
        
        Rectangle {
            anchors.fill: parent
            color: Theme.backgroundColor  // 纯色背景：浅色主题白色，深色主题黑色
            
            // 自定义标题栏（可拖动）
            Rectangle {
                id: titleBar
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 40
                color: Theme.backgroundColor  // 与主背景同色
                
                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 15
                    text: "📷 图片预览"
                    color: Theme.textColor
                    font.pixelSize: 14
                    font.bold: true
                }
                
                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 10
                    spacing: 5
                    
                    // 最小化按钮
                    Button {
                        width: 30
                        height: 30
                        text: "─"
                        font.pixelSize: 16
                        
                        background: Rectangle {
                            color: parent.hovered ? Qt.rgba(0, 0, 0, 0.1) : "transparent"
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: Theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font: parent.font
                        }
                        
                        onClicked: imageViewWindow.showMinimized()
                    }
                    
                    // 最大化/还原按钮
                    Button {
                        width: 30
                        height: 30
                        text: imageViewWindow.visibility === Window.Maximized ? "❐" : "□"
                        font.pixelSize: 16
                        
                        background: Rectangle {
                            color: parent.hovered ? Qt.rgba(0, 0, 0, 0.1) : "transparent"
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: Theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font: parent.font
                        }
                        
                        onClicked: {
                            if (imageViewWindow.visibility === Window.Maximized) {
                                imageViewWindow.showNormal()
                            } else {
                                imageViewWindow.showMaximized()
                            }
                        }
                    }
                    
                    // 关闭按钮
                    Button {
                        width: 30
                        height: 30
                        text: "×"
                        font.pixelSize: 20
                        font.bold: true
                        
                        background: Rectangle {
                            color: parent.hovered ? "#ff4444" : "transparent"
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: parent.parent.hovered ? "white" : Theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font: parent.font
                        }
                        
                        onClicked: imageViewWindow.close()
                    }
                }
                
                // 拖动区域
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 120
                    property point clickPos: Qt.point(0, 0)
                    
                    onPressed: {
                        clickPos = Qt.point(mouse.x, mouse.y)
                    }
                    
                    onPositionChanged: {
                        if (pressed) {
                            var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y)
                            imageViewWindow.x += delta.x
                            imageViewWindow.y += delta.y
                        }
                    }
                    
                    onDoubleClicked: {
                        if (imageViewWindow.visibility === Window.Maximized) {
                            imageViewWindow.showNormal()
                        } else {
                            imageViewWindow.showMaximized()
                        }
                    }
                }
            }
            
            // 工具栏（缩放控制）
            Rectangle {
                id: toolbar
                anchors.top: titleBar.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 60
                color: Theme.backgroundColor  // 与主背景同色
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    
                    Text {
                        text: "缩放:"
                        color: Theme.textColor
                    }
                    
                    Button {
                        text: "-"
                        implicitWidth: 30
                        onClicked: {
                            if (zoomSlider.value > zoomSlider.from) {
                                zoomSlider.value -= 0.25
                            }
                        }
                    }
                    
                    Slider {
                        id: zoomSlider
                        Layout.fillWidth: true
                        from: 0.25
                        to: 4.0
                        value: 0.5  // 默认50%
                        stepSize: 0.25
                        
                        onValueChanged: {
                            imageViewWindow.zoomScale = value
                        }
                    }
                    
                    Button {
                        text: "+"
                        implicitWidth: 30
                        onClicked: {
                            if (zoomSlider.value < zoomSlider.to) {
                                zoomSlider.value += 0.25
                            }
                        }
                    }
                    
                    Text {
                        text: Math.round(zoomSlider.value * 100) + "%"
                        color: Theme.textColor
                        Layout.preferredWidth: 50
                    }
                    
                    Button {
                        text: "适配"
                        onClicked: {
                            zoomSlider.value = 1.0
                        }
                    }
                    
                    Button {
                        text: "原始"
                        onClicked: {
                            if (fullImage.status === Image.Ready && fullImage.implicitWidth > 0) {
                                var fitScale = Math.min(
                                    (imageFlickable.width - 40) / fullImage.implicitWidth,
                                    (imageFlickable.height - 40) / fullImage.implicitHeight
                                )
                                zoomSlider.value = 1.0 / fitScale
                            }
                        }
                    }
                }
            }
            
            Flickable {
                id: imageFlickable
                anchors.top: toolbar.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 10
                contentWidth: fullImage.width
                contentHeight: fullImage.height
                clip: true
                
                ScrollBar.vertical: ScrollBar { }
                ScrollBar.horizontal: ScrollBar { }
                
                Image {
                    id: fullImage
                    source: root.hasImage ? ("file:///" + root.imagePath) : ""
                    fillMode: Image.PreserveAspectFit  // 保持比例，完整显示
                    smooth: true
                    cache: false
                    
                    // 根据缩放比例调整显示区域尺寸
                    width: implicitWidth * imageViewWindow.zoomScale
                    height: implicitHeight * imageViewWindow.zoomScale
                    
                    BusyIndicator {
                        anchors.centerIn: parent
                        running: fullImage.status === Image.Loading
                        visible: running
                    }
                    
                    Text {
                        anchors.centerIn: parent
                        visible: fullImage.status === Image.Error
                        text: "❌ 图片加载失败\n路径: " + root.imagePath
                        color: Theme.textColor
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                    }

                    // 错误处理：停止重试加载
                    onStatusChanged: {
                        if (status === Image.Error) {
                            // 延迟清空source以停止重试，避免持续报错
                            fullImageErrorTimer.restart()
                        }
                    }

                    Timer {
                        id: fullImageErrorTimer
                        interval: 100
                        onTriggered: {
                            if (fullImage.status === Image.Error) {
                                fullImage.source = ""
                            }
                        }
                    }
                }
            }
        }
    }
}
