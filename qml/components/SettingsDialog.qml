import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * 设置对话框 - 卡片式设计，支持未来扩展
 */
Dialog {
    id: root
    
    title: "⚙️ 设置"
    modal: true
    width: 580
    height: 520
    anchors.centerIn: parent
    
    // 自定义属性
    property var audioDeviceManager: null
    
    // 背景
    background: Rectangle {
        color: "#f5f5f5"
        radius: 12
        border.width: 0
    }
    
    // 头部
    header: Rectangle {
        height: 60
        color: "white"
        radius: 12
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            
            Text {
                text: "⚙️ 系统设置"
                font.pixelSize: 18
                font.bold: true
                color: "#333333"
                Layout.fillWidth: true
            }
            
            Button {
                text: "✖"
                flat: true
                font.pixelSize: 16
                onClicked: root.reject()
                
                background: Rectangle {
                    color: parent.hovered ? "#f0f0f0" : "transparent"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "#333333"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
    
    // 内容区域
    contentItem: Flickable {
        anchors.fill: parent
        contentHeight: contentColumn.height
        clip: true
        
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
        
        ColumnLayout {
            id: contentColumn
            width: parent.width
            spacing: 16
            
            // 音频设置卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: audioCard.height + 30
                color: "white"
                radius: 8
                                border.width: 0
                
                ColumnLayout {
                    id: audioCard
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        margins: 15
                    }
                    spacing: 12
                    
                    // 卡片标题
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        
                        Text {
                            text: "🎙️"
                            font.pixelSize: 20
                        }
                        
                        Text {
                            text: "音频设备"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                            Layout.fillWidth: true
                        }
                        
                        Rectangle {
                            width: 60
                            height: 24
                            color: "#e8f5e9"
                            radius: 4
                            
                            Text {
                                anchors.centerIn: parent
                                text: "音频"
                                font.pixelSize: 11
                                color: "#2e7d32"
                            }
                        }
                    }
                    
                    // 分隔线
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#e0e0e0"
                    }
                    
                    // 输入设备
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        
                        Text {
                            text: "🎤 输入设备（麦克风）"
                            font.pixelSize: 13
                            font.bold: true
                            color: "#333333"
                        }
                        
                        ComboBox {
                            id: inputDeviceCombo
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            
                            model: audioDeviceManager ? audioDeviceManager.inputDevices : []
                            textRole: "name"
                            valueRole: "id"
                            
                            Component.onCompleted: {
                                if (audioDeviceManager) {
                                    var devices = audioDeviceManager.inputDevices
                                    for (var i = 0; i < devices.length; i++) {
                                        if (devices[i].id === audioDeviceManager.currentInputDevice) {
                                            currentIndex = i
                                            break
                                        }
                                    }
                                }
                            }
                            
                            background: Rectangle {
                                color: parent.pressed ? "#e0e0e0" : (parent.hovered ? "#f5f5f5" : "white")
                                                                border.width: 0
                                radius: 6
                            }
                            
                            contentItem: Text {
                                leftPadding: 12
                                text: {
                                    if (inputDeviceCombo.currentIndex >= 0 && inputDeviceCombo.model) {
                                        var device = inputDeviceCombo.model[inputDeviceCombo.currentIndex]
                                        return device.name + (device.isDefault ? " (默认)" : "")
                                    }
                                    return "选择输入设备..."
                                }
                                color: "#333333"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            
                            delegate: ItemDelegate {
                                width: inputDeviceCombo.width
                                height: 36
                                
                                contentItem: RowLayout {
                                    spacing: 8
                                    
                                    Text {
                                        text: "🎤"
                                        font.pixelSize: 14
                                    }
                                    
                                    Text {
                                        text: modelData.name + (modelData.isDefault ? " (默认)" : "")
                                        color: "#333333"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                
                                background: Rectangle {
                                    color: parent.highlighted ? "#e3f2fd" : (parent.hovered ? "#f0f0f0" : "transparent")
                                }
                            }
                        }
                        
                        Text {
                            text: "用于录制和发送语音"
                            font.pixelSize: 11
                            color: "#666666"
                        }
                    }
                    
                    // 输出设备
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Layout.topMargin: 8
                        
                        Text {
                            text: "🔊 输出设备（扬声器）"
                            font.pixelSize: 13
                            font.bold: true
                            color: "#333333"
                        }
                        
                        ComboBox {
                            id: outputDeviceCombo
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            
                            model: audioDeviceManager ? audioDeviceManager.outputDevices : []
                            textRole: "name"
                            valueRole: "id"
                            
                            Component.onCompleted: {
                                if (audioDeviceManager) {
                                    var devices = audioDeviceManager.outputDevices
                                    for (var i = 0; i < devices.length; i++) {
                                        if (devices[i].id === audioDeviceManager.currentOutputDevice) {
                                            currentIndex = i
                                            break
                                        }
                                    }
                                }
                            }
                            
                            background: Rectangle {
                                color: parent.pressed ? "#e0e0e0" : (parent.hovered ? "#f5f5f5" : "white")
                                                                border.width: 0
                                radius: 6
                            }
                            
                            contentItem: Text {
                                leftPadding: 12
                                text: {
                                    if (outputDeviceCombo.currentIndex >= 0 && outputDeviceCombo.model) {
                                        var device = outputDeviceCombo.model[outputDeviceCombo.currentIndex]
                                        return device.name + (device.isDefault ? " (默认)" : "")
                                    }
                                    return "选择输出设备..."
                                }
                                color: "#333333"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            
                            delegate: ItemDelegate {
                                width: outputDeviceCombo.width
                                height: 36
                                
                                contentItem: RowLayout {
                                    spacing: 8
                                    
                                    Text {
                                        text: "🔊"
                                        font.pixelSize: 14
                                    }
                                    
                                    Text {
                                        text: modelData.name + (modelData.isDefault ? " (默认)" : "")
                                        color: "#333333"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                
                                background: Rectangle {
                                    color: parent.highlighted ? "#e3f2fd" : (parent.hovered ? "#f0f0f0" : "transparent")
                                }
                            }
                        }
                        
                        Text {
                            text: "用于播放接收的语音"
                            font.pixelSize: 11
                            color: "#666666"
                        }
                    }
                }
            }
            
            // 预留：通用设置卡片（未来扩展）
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: generalCard.height + 30
                color: "white"
                radius: 8
                                border.width: 0
                
                ColumnLayout {
                    id: generalCard
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        margins: 15
                    }
                    spacing: 12
                    
                    // 卡片标题
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        
                        Text {
                            text: "⚙️"
                            font.pixelSize: 20
                        }
                        
                        Text {
                            text: "通用设置"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                            Layout.fillWidth: true
                        }
                        
                        Rectangle {
                            width: 60
                            height: 24
                            color: "#e3f2fd"
                            radius: 4
                            
                            Text {
                                anchors.centerIn: parent
                                text: "通用"
                                font.pixelSize: 11
                                color: "#1976d2"
                            }
                        }
                    }
                    
                    // 分隔线
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#e0e0e0"
                    }
                    
                    // WebSocket协议开关
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 12
                        
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            
                            Text {
                                text: "🌐 WebSocket协议"
                                font.pixelSize: 13
                                font.bold: true
                                color: "#333333"
                            }
                            
                            Text {
                                text: "启用后将使用WebSocket进行音频通话（需要服务器支持）"
                                font.pixelSize: 11
                                color: "#666666"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                        
                        Switch {
                            id: websocketSwitch
                            Layout.alignment: Qt.AlignVCenter
                            checked: appModel ? appModel.websocketEnabled : false
                            
                            indicator: Rectangle {
                                implicitWidth: 48
                                implicitHeight: 24
                                x: websocketSwitch.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: 12
                                color: websocketSwitch.checked ? "#4CAF50" : "#BDBDBD"
                                
                                Rectangle {
                                    x: websocketSwitch.checked ? parent.width - width - 2 : 2
                                    y: (parent.height - height) / 2
                                    width: 20
                                    height: 20
                                    radius: 10
                                    color: "white"
                                    
                                    Behavior on x {
                                        NumberAnimation { duration: 100 }
                                    }
                                }
                            }
                            
                            onToggled: {
                                if (appModel) {
                                    appModel.websocketEnabled = checked
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 底部按钮
    footer: Rectangle {
        height: 70
        color: "white"
        radius: 12
        
        RowLayout {
            anchors {
                fill: parent
                margins: 16
            }
            spacing: 12
            
            Item {
                Layout.fillWidth: true
            }
            
            // 取消按钮
            Button {
                text: "取消"
                Layout.preferredWidth: 100
                Layout.preferredHeight: 40
                
                background: Rectangle {
                    color: parent.pressed ? "#e0e0e0" : (parent.hovered ? "#f5f5f5" : "white")
                                        border.width: 0
                    radius: 6
                }
                
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 14
                    color: "#333333"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: root.reject()
            }
            
            // 确定按钮
            Button {
                text: "确定保存"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 40
                
                background: Rectangle {
                    color: parent.pressed ? "#1565c0" : (parent.hovered ? "#1976d2" : "#2196f3")
                    radius: 6
                }
                
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 14
                    font.bold: true
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    confirmDialog.open()
                }
            }
        }
    }
    
    // 确认保存对话框
    Dialog {
        id: confirmDialog
        title: "确认保存"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No
        
        background: Rectangle {
            color: "white"
            radius: 8
                        border.width: 0
        }
        
        Label {
            text: "确定要保存音频设备设置吗？"
            font.pixelSize: 14
            color: "#333333"
        }
        
        onAccepted: {
            // 保存设置
            if (audioDeviceManager) {
                if (inputDeviceCombo.currentIndex >= 0 && inputDeviceCombo.model) {
                    var inputDevice = inputDeviceCombo.model[inputDeviceCombo.currentIndex]
                    audioDeviceManager.currentInputDevice = inputDevice.id
                }
                
                if (outputDeviceCombo.currentIndex >= 0 && outputDeviceCombo.model) {
                    var outputDevice = outputDeviceCombo.model[outputDeviceCombo.currentIndex]
                    audioDeviceManager.currentOutputDevice = outputDevice.id
                }
                
                audioDeviceManager.saveToConfig()
            }
            
            root.accept()
        }
    }
}
