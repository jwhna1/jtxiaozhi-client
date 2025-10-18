import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    color: Theme.sidebarBackgroundColor
    border.width: 0
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // 顶部：用户头像 + 添加设备按钮
        Row {
            Layout.fillWidth: true
            spacing: 10
            
            // 用户头像
            Rectangle {
                width: 50
                height: 50
                radius: 25
                color: Theme.borderColor
                
                Text {
                    anchors.centerIn: parent
                    text: "我"
                    font.pixelSize: 20
                    color: Theme.textColor
                }
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            // 添加设备按钮
            Button {
                text: "+"
                width: 40
                height: 40
                font.pixelSize: 24
                enabled: appModel.deviceList.length < 2
                opacity: enabled ? 1.0 : 0.5
                onClicked: {
                    if (enabled) {
                        addDeviceDialog.open()
                    }
                }
                
                // 工具提示
                ToolTip.visible: !enabled && hovered
                ToolTip.text: "最多只能添加2个智能体"
            }
        }
        
        // 设备数量提示
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Theme.highlightColor
            radius: 4
            visible: appModel.deviceList.length >= 1
            
            Text {
                anchors.centerIn: parent
                text: "智能体: " + appModel.deviceList.length + "/2"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textColor
            }
        }
        
        // 设备列表
        ListView {
            id: deviceListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 5
            
            model: ListModel {
                id: deviceListModel
            }
            
            delegate: DeviceItem {
                width: deviceListView.width
                deviceId: model.deviceId
                deviceName: model.deviceName
                connected: model.connected
                selected: appModel.currentDeviceId === model.deviceId
                
                onClicked: {
                    appModel.selectDevice(model.deviceId)
                }
                
                onDoubleClicked: {
                    // 双击设备自动连接OTA
                    appModel.selectDevice(model.deviceId)
                    appModel.connectDevice(model.deviceId)
                }
                
                onDeleteClicked: {
                    appModel.removeDevice(model.deviceId)
                }
                
                onEditClicked: {
                    var deviceInfo = appModel.getDeviceInfo(model.deviceId)
                    editDeviceDialog.deviceId = deviceInfo.deviceId
                    editDeviceDialog.deviceName = deviceInfo.deviceName
                    editDeviceDialog.otaUrl = deviceInfo.otaUrl
                    editDeviceDialog.macAddress = deviceInfo.macAddress
                    editDeviceDialog.open()
                }
            }
        }
        
        // 底部：关于按钮
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            text: "ℹ 关于"
            font.pixelSize: Theme.fontSizeMedium
            
            background: Rectangle {
                color: parent.hovered ? Theme.highlightColor : Theme.buttonColor
                radius: 6
                border.width: 0
            }
            
            contentItem: Text {
                text: parent.text
                color: parent.hovered ? Theme.textColor : Theme.buttonTextColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: parent.font.pixelSize
            }
            
            onClicked: aboutDialog.open()
        }
    }
    
    // 添加设备对话框
    AddDeviceDialog {
        id: addDeviceDialog
    }
    
    // 关于对话框
    AboutDialog {
        id: aboutDialog
    }
    
    // 编辑设备对话框
    EditDeviceDialog {
        id: editDeviceDialog
    }
    
    // 监听设备列表变化
    Connections {
        target: appModel
        function onDeviceListChanged() {
            updateDeviceList()
        }
        
        // 监听连接状态变化
        function onConnectedChanged() {
            updateDeviceList()
        }
        
        function onUdpConnectedChanged() {
            updateDeviceList()
        }
    }
    
    // 更新设备列表
    function updateDeviceList() {
        deviceListModel.clear()
        var devices = appModel.deviceInfoList
        for (var i = 0; i < devices.length; i++) {
            deviceListModel.append(devices[i])
        }
    }
    
    Component.onCompleted: {
        updateDeviceList()
    }
}

