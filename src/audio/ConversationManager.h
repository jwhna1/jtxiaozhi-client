/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T22:30:00Z
File: ConversationManager.h
Desc: 对话状态机管理器（支持auto/manual/realtime三种模式）
*/

#ifndef CONVERSATION_MANAGER_H
#define CONVERSATION_MANAGER_H

#include "AudioDevice.h"
#include "OpusCodec.h"
#include "../network/MqttManager.h"
#include "../network/UdpManager.h"
#include "../network/WebSocketManager.h"
#include <QObject>
#include <QString>
#include <QBuffer>
#include <memory>

namespace xiaozhi {
namespace audio {

/**
 * @brief 通信协议类型
 */
enum class ProtocolType {
    MqttUdp,      // MQTT + UDP（默认）
    WebSocket     // WebSocket
};

/**
 * @brief 对话状态
 */
enum class ConversationState {
    Idle,         // 空闲
    Listening,    // 聆听中（录音，向服务器发送音频）
    Speaking      // 说话中（播放服务器音频和文字）
};

/**
 * @brief 对话模式
 */
enum class ConversationMode {
    Auto,         // 自动模式（服务器控制状态切换）
    Manual,       // 手动模式（用户按钮控制）
    Realtime      // 实时模式（持续双向通信）
};

/**
 * @brief 对话状态机管理器
 * 
 * 业务流程：
 * 1. 点击录音按钮 → 开启UDP音频通道 + 开始麦克风录音
 * 2. 点击停止录音 → 关闭麦克风，UDP通道保持开启
 * 3. 聆听状态：客户端向服务器发送音频
 * 4. 说话中状态：服务器向客户端发送音频和文字（由服务器切换）
 * 5. 客户端可以中止说话状态切换回聆听
 * 6. 服务器发送完后自动切回聆听状态
 */
class ConversationManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(ConversationState state READ state NOTIFY stateChanged)
    Q_PROPERTY(ConversationMode mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool udpChannelOpened READ udpChannelOpened NOTIFY udpChannelOpenedChanged)

public:
    /**
     * @brief 构造函数（MQTT+UDP模式）
     */
    explicit ConversationManager(
        network::MqttManager* mqttManager,
        network::UdpManager* udpManager,
        AudioDevice* audioDevice,
        const QString& sessionId,
        int serverSampleRate = 24000,       // 服务器音频采样率（解码器用）
        int serverChannels = 1,             // 服务器音频声道数
        int serverFrameDuration = 60,       // 服务器音频帧时长（ms）
        QObject* parent = nullptr
    );

    /**
     * @brief 构造函数（WebSocket模式）
     */
    explicit ConversationManager(
        network::WebSocketManager* websocketManager,
        AudioDevice* audioDevice,
        const QString& sessionId,
        int serverSampleRate = 24000,
        int serverChannels = 1,
        int serverFrameDuration = 60,
        QObject* parent = nullptr
    );

    ~ConversationManager();

    // ========== 属性访问器 ==========
    
    ConversationState state() const { return m_state; }
    ConversationMode mode() const { return m_mode; }
    bool isRecording() const { return m_isRecording; }
    bool isPlaying() const { return m_isPlaying; }
    bool udpChannelOpened() const { return m_udpChannelOpened; }
    ProtocolType protocolType() const { return m_protocolType; }
    // 供外部保存音频缓存时取得真实参数
    int serverSampleRate() const { return m_serverSampleRate; }
    int serverChannels() const { return m_serverChannels; }

    // ========== QML可调用方法 ==========

    /**
     * @brief 设置对话模式
     */
    Q_INVOKABLE void setMode(ConversationMode mode);

    /**
     * @brief 开启UDP音频通道并开始录音
     */
    Q_INVOKABLE void startConversation();

    /**
     * @brief 停止录音（UDP保持开启）
     */
    Q_INVOKABLE void stopRecording();

    /**
     * @brief 中止说话状态，切换回聆听
     */
    Q_INVOKABLE void abortSpeaking();

    /**
     * @brief 关闭UDP音频通道
     */
    Q_INVOKABLE void closeAudioChannel();

signals:
    /**
     * @brief 状态变化
     */
    void stateChanged(ConversationState state);

    /**
     * @brief 模式变化
     */
    void modeChanged(ConversationMode mode);

    /**
     * @brief 录音状态变化
     */
    void isRecordingChanged(bool recording);

    /**
     * @brief 播放状态变化
     */
    void isPlayingChanged(bool playing);

    /**
     * @brief UDP通道状态变化
     */
    void udpChannelOpenedChanged(bool opened);

    /**
     * @brief 收到STT文本
     */
    void sttTextReceived(const QString& text);

    /**
     * @brief 收到TTS文本
     */
    void ttsTextReceived(const QString& text);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

    /**
     * @brief TTS消息开始（收到第一条文字，用于立即显示UI）
     */
    void ttsMessageStarted(const QString& text, qint64 timestamp);

    /**
     * @brief TTS消息完成（包含累积的PCM数据）
     */
    void ttsMessageCompleted(const QString& text, const QByteArray& pcmData, qint64 timestamp);

    /**
     * @brief STT消息完成
     */
    void sttMessageCompleted(const QString& text, qint64 timestamp);

private slots:
    /**
     * @brief UDP连接成功
     */
    void onUdpConnected();

    /**
     * @brief 收到UDP音频数据
     */
    void onUdpAudioReceived(const QByteArray& opus_data);

    /**
     * @brief 录音设备数据就绪
     */
    void onAudioReady(const QByteArray& pcm_data);

    /**
     * @brief 收到MQTT消息
     */
    void onMqttMessageReceived(const QJsonObject& message);

    /**
     * @brief WebSocket连接成功
     */
    void onWebSocketConnected();

    /**
     * @brief 收到WebSocket音频数据
     */
    void onWebSocketAudioReceived(const QByteArray& opus_data);

    /**
     * @brief 收到WebSocket JSON消息
     */
    void onWebSocketJsonReceived(const QString& jsonData);

private:
    /**
     * @brief 切换到聆听状态
     */
    void switchToListening();

    /**
     * @brief 切换到说话状态
     */
    void switchToSpeaking();

    /**
     * @brief 切换到空闲状态
     */
    void switchToIdle();

    /**
     * @brief 发送编码加密后的音频数据
     */
    void sendEncodedAudio(const QByteArray& pcm_data);

    /**
     * @brief 接收解密解码后的音频数据
     */
    void receiveDecodedAudio(const QByteArray& opus_data);

    // 协议类型
    ProtocolType m_protocolType;
    
    // 网络管理器（根据协议类型使用）
    network::MqttManager* m_mqttManager;          // MQTT+UDP模式
    network::UdpManager* m_udpManager;            // MQTT+UDP模式
    network::WebSocketManager* m_websocketManager; // WebSocket模式
    
    // 音频设备
    AudioDevice* m_audioDevice;
    
    // 编解码器
    std::unique_ptr<OpusCodec> m_codec;
    
    // 会话ID
    QString m_sessionId;
    
    // 状态
    ConversationState m_state;
    ConversationMode m_mode;
    bool m_isRecording;
    bool m_isPlaying;
    bool m_udpChannelOpened;
    
    // PCM缓冲区（用于累积到一帧）
    QByteArray m_pcmBuffer;
    int m_targetFrameSize;  // 目标帧大小（字节）
    
    // 播放缓冲区
    std::unique_ptr<QBuffer> m_playbackBuffer;
    
    // TTS消息累积（多个音频包合并为一个气泡）
    QString m_currentTtsText;           // 当前TTS文字
    QByteArray m_currentTtsPcm;         // 累积的PCM数据
    qint64 m_currentTtsStartTime;       // TTS开始时间
    bool m_isTtsAccumulating = false;   // 是否正在累积TTS

    // 服务器音频参数（用于外部持久化）
    int m_serverSampleRate = 24000;
    int m_serverChannels = 1;
};

} // namespace audio
} // namespace xiaozhi

#endif // CONVERSATION_MANAGER_H

