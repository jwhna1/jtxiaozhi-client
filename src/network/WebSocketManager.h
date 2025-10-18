/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-12T16:00:00Z
File: WebSocketManager.h
Desc: WebSocket通信管理器（完全复刻ESP32 websocket_protocol实现）
*/

#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include "NetworkTypes.h"
#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QMutex>

namespace xiaozhi {
namespace network {

/**
 * @brief WebSocket通信管理器
 * 
 * 实现完整的WebSocket协议，包括：
 * 1. Hello握手流程
 * 2. 音频数据收发（Opus编码，支持Version 1/2/3）
 * 3. JSON消息收发（stt, tts, llm, system等）
 * 4. 自动重连机制
 */
class WebSocketManager : public QObject {
    Q_OBJECT

public:
    explicit WebSocketManager(QObject* parent = nullptr);
    ~WebSocketManager();

    /**
     * @brief 连接到WebSocket服务器
     * @param config WebSocket配置（url, token, version）
     * @param deviceId 设备MAC地址
     * @param clientId 客户端UUID
     * @return 是否成功发起连接
     */
    bool connectToServer(const WebSocketConfig& config, const QString& deviceId, const QString& clientId);

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 是否已连接
     */
    bool isConnected() const;

    /**
     * @brief 发送音频数据（Opus编码）
     * @param opusData Opus编码的音频数据
     * @param timestamp 时间戳（仅Version 2使用）
     * @return 是否发送成功
     */
    bool sendAudioData(const QByteArray& opusData, quint32 timestamp = 0);

    /**
     * @brief 发送JSON文本消息
     * @param jsonData JSON字符串
     * @return 是否发送成功
     */
    bool sendJsonMessage(const QString& jsonData);

    /**
     * @brief 发送对话控制消息
     * @param mode 对话模式：manual/auto/realtime（仅sendStartListening需要）
     */
    bool sendStartListening(const QString& mode = "manual");
    bool sendStopListening();
    bool sendAbortSpeaking();

    /**
     * @brief 获取会话ID
     */
    QString sessionId() const { return m_sessionId; }

    /**
     * @brief 获取服务器音频参数
     */
    int serverSampleRate() const { return m_config.serverSampleRate; }
    int serverChannels() const { return m_config.serverChannels; }
    int serverFrameDuration() const { return m_config.serverFrameDuration; }

signals:
    /**
     * @brief 连接成功（Hello握手完成）
     */
    void connected();

    /**
     * @brief 连接断开
     */
    void disconnected();

    /**
     * @brief 接收到音频数据（已解码Opus）
     * @param opusData Opus编码数据
     */
    void audioDataReceived(const QByteArray& opusData);

    /**
     * @brief 接收到JSON消息
     * @param jsonData JSON字符串
     */
    void jsonMessageReceived(const QString& jsonData);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

private slots:
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketError(QAbstractSocket::SocketError error);
    void onWebSocketTextMessageReceived(const QString& message);
    void onWebSocketBinaryMessageReceived(const QByteArray& message);
    void onHelloTimeout();

private:
    /**
     * @brief 发送客户端Hello消息
     */
    bool sendClientHello();

    /**
     * @brief 解析服务器Hello响应
     */
    void parseServerHello(const QString& message);

    /**
     * @brief 构建二进制协议包（根据version）
     */
    QByteArray buildBinaryPacket(const QByteArray& opusData, quint32 timestamp);

    /**
     * @brief 解析二进制协议包（根据version）
     */
    QByteArray parseBinaryPacket(const QByteArray& data);

private:
    QWebSocket* m_webSocket;
    WebSocketConfig m_config;
    QString m_deviceId;
    QString m_clientId;
    QString m_sessionId;
    bool m_helloReceived;
    QTimer* m_helloTimer;
    QMutex m_mutex;

    static constexpr int HELLO_TIMEOUT_MS = 10000;  // 10秒超时
};

} // namespace network
} // namespace xiaozhi

#endif // WEBSOCKET_MANAGER_H

