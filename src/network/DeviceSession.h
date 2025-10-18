/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-12T06:30:00Z
File: DeviceSession.h
Desc: 设备会话管理器（每个智能体设备独立实例，网络隔离，基于MAC生成固定UUID）
*/

#ifndef DEVICE_SESSION_H
#define DEVICE_SESSION_H

#include "NetworkTypes.h"
#include "OtaManager.h"
#include "MqttManager.h"
#include "UdpManager.h"
#include "WebSocketManager.h"
#include "../audio/ConversationManager.h"
#include "../audio/AudioDevice.h"
#include <QObject>
#include <QString>
#include <memory>

// 前向声明
namespace xiaozhi {
namespace models {
struct ChatMessage;
}
}

namespace xiaozhi {
namespace network {

/**
 * @brief 设备会话管理器（核心隔离类）
 * 
 * 设计理念：每个设备有完全独立的网络连接和数据，互不干扰
 */
class DeviceSession : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param deviceId 设备唯一标识
     * @param deviceName 设备名称
     * @param macAddress MAC地址
     * @param otaUrl OTA服务器地址
     * @param audioDevice 音频设备
     * @param websocketEnabled 是否启用WebSocket协议
     * @param parent 父对象
     */
    explicit DeviceSession(const QString& deviceId,
                          const QString& deviceName,
                          const QString& macAddress,
                          const QString& otaUrl,
                          audio::AudioDevice* audioDevice,
                          bool websocketEnabled = false,
                          QObject* parent = nullptr);
    ~DeviceSession();

    // ========== 属性访问器 ==========
    
    QString deviceId() const { return m_deviceId; }
    QString deviceName() const { return m_deviceName; }
    QString macAddress() const { return m_macAddress; }
    QString otaUrl() const { return m_otaUrl; }
    QString activationCode() const { return m_activationCode; }
    QString sessionId() const { return m_sessionId; }
    bool isConnected() const { return m_mqttConnected || m_websocketConnected; }
    bool isUdpConnected() const { return m_udpConnected; }
    bool isWebSocketMode() const { return m_websocketEnabled && m_otaConfig.hasWebSocket; }
    
    /**
     * @brief 获取对话管理器
     */
    audio::ConversationManager* conversationManager() const { 
        return m_conversationManager.get(); 
    }
    
    /**
     * @brief 更新WebSocket启用状态（运行时更新）
     */
    void updateWebSocketEnabled(bool enabled) {
        m_websocketEnabled = enabled;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 基于MAC地址生成确定性UUID
     * @param macAddress MAC地址
     * @return 固定的UUID字符串
     */
    static QString generateUuidFromMac(const QString& macAddress);

    // ========== 设备操作 ==========

    /**
     * @brief 获取OTA配置
     */
    void getOtaConfig();

    /**
     * @brief 连接MQTT
     */
    void connectMqtt();

    /**
     * @brief 申请UDP音频通道
     */
    void requestAudioChannel();

    /**
     * @brief 发送文本消息
     */
    void sendTextMessage(const QString& text);

    /**
     * @brief 发送图片识别消息
     * @param imagePath 图片文件路径
     * @param text 可选的文字描述
     */
    void sendImageMessage(const QString& imagePath, const QString& text = QString());

    /**
     * @brief 发送测试音频
     */
    void sendTestAudio();

    /**
     * @brief 断开所有连接
     */
    void disconnect();

signals:
    /**
     * @brief 状态变化
     * @param deviceId 设备ID
     * @param status 状态消息
     */
    void statusChanged(const QString& deviceId, const QString& status);

    /**
     * @brief 日志消息
     * @param deviceId 设备ID
     * @param message 日志消息
     */
    void logMessage(const QString& deviceId, const QString& message);

    /**
     * @brief 激活码收到
     * @param deviceId 设备ID
     * @param code 激活码
     */
    void activationCodeReceived(const QString& deviceId, const QString& code);

    /**
     * @brief 连接状态变化
     * @param deviceId 设备ID
     * @param connected MQTT连接状态
     * @param udpConnected UDP连接状态
     */
    void connectionStateChanged(const QString& deviceId, bool connected, bool udpConnected);

    /**
     * @brief 聊天消息接收
     * @param deviceId 设备ID
     * @param message 聊天消息
     * @param pcmData PCM音频数据（可选）
     */
    void chatMessageReceived(const QString& deviceId, const xiaozhi::models::ChatMessage& message, const QByteArray& pcmData = QByteArray());

private slots:
    // OTA回调
    void onOtaConfigReceived(const OtaConfig& config);
    void onOtaError(const QString& error);

    // MQTT回调
    void onMqttConnected();
    void onMqttDisconnected(int rc);
    void onMqttMessage(const QJsonObject& message);
    void onUdpConfigReceived(const UdpConfig& config, const QString& sessionId);
    void onMqttError(const QString& error);

    // UDP回调
    void onUdpConnected();
    void onUdpAudioData(const QByteArray& data);
    void onUdpError(const QString& error);

    // WebSocket回调
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketError(const QString& error);

    // 消息回调
    void onTtsMessageStarted(const QString& text, qint64 timestamp);
    void onTtsMessageCompleted(const QString& text, const QByteArray& pcmData, qint64 timestamp);
    void onSttMessageCompleted(const QString& text, qint64 timestamp);

private:
    // 设备基本信息
    QString m_deviceId;
    QString m_deviceName;
    QString m_macAddress;
    QString m_otaUrl;
    QString m_uuid;  // 设备UUID

    // 会话信息
    QString m_activationCode;
    QString m_sessionId;
    bool m_mqttConnected;
    bool m_udpConnected;
    bool m_websocketConnected;
    bool m_websocketEnabled;  // 用户设置：是否启用WebSocket

    // 独立的网络管理器实例（每设备一套）
    std::unique_ptr<OtaManager> m_otaManager;
    std::unique_ptr<MqttManager> m_mqttManager;
    std::unique_ptr<UdpManager> m_udpManager;
    std::unique_ptr<WebSocketManager> m_websocketManager;

    // OTA配置缓存
    OtaConfig m_otaConfig;
    
    // 对话管理器
    std::unique_ptr<audio::ConversationManager> m_conversationManager;
    audio::AudioDevice* m_audioDevice;  // 外部引用
};

} // namespace network
} // namespace xiaozhi

#endif // DEVICE_SESSION_H


