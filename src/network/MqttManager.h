/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:31:00Z
File: MqttManager.h
Desc: MQTT连接管理器（线程方式，复刻esp32_simulator_gui.py第358-803行）
*/

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "NetworkTypes.h"
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QJsonObject>
#include <mqtt/async_client.h>
#include <memory>

namespace xiaozhi {
namespace network {

/**
 * @brief MQTT回调处理类
 * 注意：此类不继承QObject，使用父对象转发信号
 */
class MqttCallback : public virtual mqtt::callback {
public:
    explicit MqttCallback(QObject* parent);

    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;

private:
    QObject* m_parent;
};

/**
 * @brief MQTT工作线程
 */
class MqttWorker : public QObject {
    Q_OBJECT

public:
    explicit MqttWorker(QObject* parent = nullptr);
    ~MqttWorker();

public slots:
    /**
     * @brief 连接MQTT服务器（在工作线程中执行）
     */
    void connectToMqtt(const MqttConfig& config);

    /**
     * @brief 断开MQTT连接
     */
    void disconnect();

    /**
     * @brief 发送hello消息
     */
    void sendHello(const QString& transportType);

    /**
     * @brief 发送pong消息
     */
    void sendPong(const QString& clientId);

    /**
     * @brief 发送文本消息
     */
    void sendTextMessage(const QString& text, const QString& clientId);

    /**
     * @brief 发送IoT描述符
     */
    void sendIotDescriptors(const QString& sessionId);

    /**
     * @brief 发送IoT状态
     */
    void sendIotStates(const QString& sessionId);

    /**
     * @brief 发送开始听筒消息
     * @param sessionId 会话ID
     * @param mode 模式：manual(手动) 或 auto(自动)
     */
    void sendStartListening(const QString& sessionId, const QString& mode = "manual");

    /**
     * @brief 发送停止听筒消息
     * @param sessionId 会话ID
     */
    void sendStopListening(const QString& sessionId);

    /**
     * @brief 发送中止消息
     * @param sessionId 会话ID
     * @param reason 中止原因（可选）
     */
    void sendAbort(const QString& sessionId, const QString& reason = "");

    /**
     * @brief 发送再见消息（关闭会话）
     * @param sessionId 会话ID
     */
    void sendGoodbye(const QString& sessionId);

    /**
     * @brief 发送MCP消息（物联网控制）
     * @param sessionId 会话ID
     * @param payload MCP JSON-RPC负载
     */
    void sendMcpMessage(const QString& sessionId, const QJsonObject& payload);

    /**
     * @brief 发送原始JSON消息
     * @param topic MQTT主题
     * @param message JSON消息对象
     */
    void sendRawMessage(const QString& topic, const QJsonObject& message);

signals:
    /**
     * @brief MQTT连接成功
     */
    void connected();

    /**
     * @brief MQTT断开连接
     */
    void disconnected(int rc);

    /**
     * @brief 收到消息
     */
    void messageReceived(const QJsonObject& message);

    /**
     * @brief 收到UDP配置
     */
    void udpConfigReceived(const UdpConfig& config, const QString& sessionId);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * @brief 处理MQTT连接成功
     */
    void onMqttConnected();

    /**
     * @brief 处理MQTT断开
     */
    void onMqttDisconnected(int rc);

    /**
     * @brief 处理收到的消息
     */
    void onMqttMessage(const QString& topic, const QString& payload);

    /**
     * @brief 尝试重连MQTT
     */
    void attemptReconnect();

private:
    /**
     * @brief 发布MQTT消息
     */
    bool publish(const QString& topic, const QJsonObject& message, int qos = 0);

    /**
     * @brief 处理ESP32支持的消息类型
     */
    void handleEsp32Message(const QJsonObject& message);

    /**
     * @brief 尝试连接MQTT（支持自动协议切换）
     */
    bool tryConnect(const QString& host, int port, bool useSSL);

    std::unique_ptr<mqtt::async_client> m_client;
    std::unique_ptr<MqttCallback> m_callback;
    MqttConfig m_config;
    bool m_connected;
    QTimer* m_reconnectTimer;
};

/**
 * @brief MQTT连接管理器（复刻esp32_simulator_gui.py第358-803行）
 */
class MqttManager : public QObject {
    Q_OBJECT

public:
    explicit MqttManager(QObject* parent = nullptr);
    ~MqttManager();

    /**
     * @brief 连接MQTT服务器
     */
    void connectToMqtt(const MqttConfig& config);

    /**
     * @brief 断开MQTT连接
     */
    void disconnect();

    /**
     * @brief 发送hello消息
     */
    void sendHello(const QString& transportType = "udp");

    /**
     * @brief 发送pong消息
     */
    void sendPong(const QString& clientId);

    /**
     * @brief 发送文本消息
     */
    void sendTextMessage(const QString& text, const QString& clientId);

    /**
     * @brief 发送IoT描述符
     */
    void sendIotDescriptors(const QString& sessionId);

    /**
     * @brief 发送IoT状态
     */
    void sendIotStates(const QString& sessionId);

    /**
     * @brief 发送开始听筒消息
     * @param sessionId 会话ID
     * @param mode 模式：manual(手动) 或 auto(自动)
     */
    void sendStartListening(const QString& sessionId, const QString& mode = "manual");

    /**
     * @brief 发送停止听筒消息
     * @param sessionId 会话ID
     */
    void sendStopListening(const QString& sessionId);

    /**
     * @brief 发送中止消息
     * @param sessionId 会话ID
     * @param reason 中止原因（可选）
     */
    void sendAbort(const QString& sessionId, const QString& reason = "");

    /**
     * @brief 发送再见消息（关闭会话）
     * @param sessionId 会话ID
     */
    void sendGoodbye(const QString& sessionId);

    /**
     * @brief 发送MCP消息（物联网控制）
     * @param sessionId 会话ID
     * @param payload MCP JSON-RPC负载
     */
    void sendMcpMessage(const QString& sessionId, const QJsonObject& payload);

    /**
     * @brief 发送原始JSON消息
     * @param topic MQTT主题
     * @param message JSON消息对象
     */
    void sendRawMessage(const QString& topic, const QJsonObject& message);

signals:
    /**
     * @brief MQTT连接成功
     */
    void connected();

    /**
     * @brief MQTT断开连接
     */
    void disconnected(int rc);

    /**
     * @brief 收到消息
     */
    void messageReceived(const QJsonObject& message);

    /**
     * @brief 收到UDP配置
     */
    void udpConfigReceived(const UdpConfig& config, const QString& sessionId);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

    // 内部信号（用于线程通信）
    void connectToMqttInternal(const MqttConfig& config);
    void disconnectInternal();
    void sendHelloInternal(const QString& transportType);
    void sendPongInternal(const QString& clientId);
    void sendTextMessageInternal(const QString& text, const QString& clientId);
    void sendIotDescriptorsInternal(const QString& sessionId);
    void sendIotStatesInternal(const QString& sessionId);
    void sendStartListeningInternal(const QString& sessionId, const QString& mode);
    void sendStopListeningInternal(const QString& sessionId);
    void sendAbortInternal(const QString& sessionId, const QString& reason);
    void sendGoodbyeInternal(const QString& sessionId);
    void sendMcpMessageInternal(const QString& sessionId, const QJsonObject& payload);
    void sendRawMessageInternal(const QString& topic, const QJsonObject& message);

private:
    QThread* m_workerThread;
    MqttWorker* m_worker;
};

} // namespace network
} // namespace xiaozhi

#endif // MQTT_MANAGER_H

