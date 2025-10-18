import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: root
    title: "编辑设备"
    modal: true
    anchors.centerIn: parent
    width: 500
    height: 350
    
    property string deviceId: ""
    property string deviceName: ""
    property string otaUrl: ""
    property string macAddress: ""
    
    background: Rectangle {
        color: Theme.backgroundColor
        border.width: 0
        radius: 5
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        
        // 设备名称输入
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            
            Text {
                text: "设备名称："
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textColor
            }
            
            TextField {
                id: deviceNameField
                Layout.fillWidth: true
                placeholderText: "请输入设备名称"
                text: root.deviceName
            }
        }
        
        // OTA地址输入
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            
            Text {
                text: "OTA服务器地址："
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textColor
            }
            
            TextField {
                id: otaUrlField
                Layout.fillWidth: true
                placeholderText: "https://api.tenclass.net/xiaozhi/ota/"
                text: root.otaUrl
            }
        }
        
        // MAC地址显示（只读）
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            
            Text {
                text: "MAC地址（不可修改）："
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textColor
            }
            
            TextField {
                id: macAddressField
                Layout.fillWidth: true
                text: root.macAddress
                readOnly: true
                opacity: 0.6
            }
        }
        
        // 错误提示
        Text {
            id: errorText
            Layout.fillWidth: true
            text: ""
            color: Theme.errorColor
            font.pixelSize: Theme.fontSizeSmall
            wrapMode: Text.WordWrap
            visible: text !== ""
        }
        
        Item {
            Layout.fillHeight: true
        }
        
        // 按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Item {
                Layout.fillWidth: true
            }
            
            Button {
                text: "取消"
                onClicked: root.close()
            }
            
            Button {
                text: "保存"
                highlighted: true
                onClicked: {
                    // 验证输入
                    var errors = []
                    
                    if (deviceNameField.text.trim() === "") {
                        errors.push("设备名称不能为空")
                    }
                    
                    var otaUrl = otaUrlField.text.trim()
                    if (!otaUrl.startsWith("http://") && !otaUrl.startsWith("https://")) {
                        errors.push("OTA地址格式不正确（必须以http://或https://开头）")
                    }
                    
                    if (errors.length > 0) {
                        errorText.text = errors.join("\n")
                        return
                    }
                    
                    // 更新设备（这里需要调用AppModel的方法）
                    appModel.updateDevice(
                        root.deviceId,
                        deviceNameField.text.trim(),
                        otaUrl
                    )
                    
                    errorText.text = ""
                    root.close()
                }
            }
        }
    }
    
    onOpened: {
        deviceNameField.text = root.deviceName
        otaUrlField.text = root.otaUrl
        macAddressField.text = root.macAddress
        errorText.text = ""
    }
}


