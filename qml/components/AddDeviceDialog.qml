import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: root
    title: "添加智能体设备"
    modal: true
    anchors.centerIn: parent
    width: 500
    height: 400
    
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
                placeholderText: "请输入设备名称（如：智能体小智）"
                text: ""
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
                text: "https://api.tenclass.net/xiaozhi/ota/"
            }
        }
        
        // MAC地址输入
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            
            Text {
                text: "MAC地址："
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textColor
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                
                TextField {
                    id: macAddressField
                    Layout.fillWidth: true
                    placeholderText: "02:xx:xx:xx:xx:xx"
                    text: appModel.generateRandomMac()
                }
                
                Button {
                    text: "随机生成"
                    onClicked: {
                        macAddressField.text = appModel.generateRandomMac()
                    }
                }
            }
        }
        
        // 限制提示
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Theme.highlightColor
            radius: 4
            border.width: 0
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4
                
                Text {
                    Layout.fillWidth: true
                    text: " 智能体添加规则："
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.textColor
                }
                
                Text {
                    Layout.fillWidth: true
                    text: "• 最多只能添加2个智能体"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textColor
                }
                
                Text {
                    Layout.fillWidth: true
                    text: "• 虾哥官方服务器只能添加1个"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textColor
                }
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
                text: "确定"
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
                    
                    var mac = macAddressField.text.trim().toLowerCase()
                    var macRegex = /^([0-9a-f]{2}:){5}[0-9a-f]{2}$/
                    if (!macRegex.test(mac)) {
                        errors.push("MAC地址格式不正确（必须是小写的xx:xx:xx:xx:xx:xx格式）")
                    }
                    
                    if (errors.length > 0) {
                        errorText.text = errors.join("\n")
                        return
                    }
                    
                    // 检查设备添加限制
                    var checkResult = appModel.canAddDevice(otaUrl)
                    if (!checkResult.canAdd) {
                        errorText.text = checkResult.errorMessage
                        return
                    }
                    
                    // 添加设备
                    appModel.addDevice(
                        deviceNameField.text.trim(),
                        otaUrl,
                        mac
                    )
                    
                    // 清空表单
                    deviceNameField.text = ""
                    otaUrlField.text = "https://api.tenclass.net/xiaozhi/ota/"
                    macAddressField.text = appModel.generateRandomMac()
                    errorText.text = ""
                    
                    root.close()
                }
            }
        }
    }
}

