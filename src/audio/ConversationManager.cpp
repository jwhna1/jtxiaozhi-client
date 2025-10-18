/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-12T07:00:00Z
File: ConversationManager.cpp
Desc: 对话状态机管理器实现（修复播放采样率不匹配问题）
*/

#include "ConversationManager.h"
#include "AudioTypes.h"
#include "../utils/Logger.h"
#include <QJsonObject>

namespace xiaozhi {
namespace audio {

ConversationManager::ConversationManager(
    network::MqttManager* mqttManager,
    network::UdpManager* udpManager,
    AudioDevice* audioDevice,
    const QString& sessionId,
    int serverSampleRate,
    int serverChannels,
    int serverFrameDuration,
    QObject* parent)
    : QObject(parent)
    , m_protocolType(ProtocolType::MqttUdp)
    , m_mqttManager(mqttManager)
    , m_udpManager(udpManager)
    , m_websocketManager(nullptr)
    , m_audioDevice(audioDevice)
    , m_codec(std::make_unique<OpusCodec>())
    , m_sessionId(sessionId)
    , m_state(ConversationState::Idle)
    , m_mode(ConversationMode::Manual)
    , m_isRecording(false)
    , m_isPlaying(false)
    , m_udpChannelOpened(false)
    , m_targetFrameSize(0)
    , m_playbackBuffer(std::make_unique<QBuffer>())
{
    // 初始化Opus编码器（16kHz单声道，客户端发送给服务器）
    if (!m_codec->initEncoder(16000, 1, 24000)) {
        utils::Logger::instance().error(" Opus编码器初始化失败");
    }

    //  修复：使用服务器参数初始化解码器（不要自动切换采样率！）
    // 服务器参数从hello消息中获取: serverSampleRate (通常是24000Hz)
    if (!m_codec->initDecoder(serverSampleRate, serverChannels)) {
        utils::Logger::instance().error(" Opus解码器初始化失败");
    }
    // 已移除Opus解码器初始化详情日志（敏感信息）

    //  配置播放设备采样率（使用服务器采样率）
    AudioConfig playbackConfig;
    playbackConfig.sampleRate = serverSampleRate;
    playbackConfig.channelCount = serverChannels;
    playbackConfig.sampleSize = 16;
    playbackConfig.sampleFormat = QAudioFormat::Int16;
    m_audioDevice->setAudioConfig(playbackConfig);
    
    // 已移除播放设备配置详情日志（敏感信息）

    // 计算目标帧大小（字节）：样本数 * 声道数 * 每样本字节数
    m_targetFrameSize = m_codec->getEncoderFrameSize() * 1 * 2;  // 960样本 * 1声道 * 2字节 = 1920字节

    // 保存服务器参数供外部读取
    m_serverSampleRate = serverSampleRate;
    m_serverChannels = serverChannels;

    // ConversationManager初始化完成

    // 连接UDP信号
    connect(m_udpManager, &network::UdpManager::udpConnected,
            this, &ConversationManager::onUdpConnected);
    connect(m_udpManager, &network::UdpManager::audioDataReceived,
            this, &ConversationManager::onUdpAudioReceived);

    // 连接MQTT信号
    connect(m_mqttManager, &network::MqttManager::messageReceived,
            this, &ConversationManager::onMqttMessageReceived);

    // 连接录音信号
    connect(m_audioDevice, &AudioDevice::audioReady,
            this, &ConversationManager::onAudioReady);
    
    // 初始化播放缓冲区
    m_playbackBuffer->open(QIODevice::ReadWrite);
}

ConversationManager::ConversationManager(
    network::WebSocketManager* websocketManager,
    AudioDevice* audioDevice,
    const QString& sessionId,
    int serverSampleRate,
    int serverChannels,
    int serverFrameDuration,
    QObject* parent)
    : QObject(parent)
    , m_protocolType(ProtocolType::WebSocket)
    , m_mqttManager(nullptr)
    , m_udpManager(nullptr)
    , m_websocketManager(websocketManager)
    , m_audioDevice(audioDevice)
    , m_codec(std::make_unique<OpusCodec>())
    , m_sessionId(sessionId)
    , m_state(ConversationState::Idle)
    , m_mode(ConversationMode::Manual)
    , m_isRecording(false)
    , m_isPlaying(false)
    , m_udpChannelOpened(false)  // WebSocket模式下，此标志表示连接状态
    , m_targetFrameSize(0)
    , m_playbackBuffer(std::make_unique<QBuffer>())
{
    // 初始化Opus编码器（16kHz单声道，客户端发送给服务器）
    if (!m_codec->initEncoder(16000, 1, 24000)) {
        utils::Logger::instance().error(" Opus编码器初始化失败");
    }

    // 初始化Opus解码器（使用服务器参数）
    if (!m_codec->initDecoder(serverSampleRate, serverChannels)) {
        utils::Logger::instance().error(" Opus解码器初始化失败");
    }
    // 已移除Opus解码器初始化详情日志（敏感信息）

    // 配置播放设备采样率
    AudioConfig playbackConfig;
    playbackConfig.sampleRate = serverSampleRate;
    playbackConfig.channelCount = serverChannels;
    playbackConfig.sampleSize = 16;
    playbackConfig.sampleFormat = QAudioFormat::Int16;
    m_audioDevice->setAudioConfig(playbackConfig);

    // 计算目标帧大小
    m_targetFrameSize = m_codec->getEncoderFrameSize() * 1 * 2;

    // 保存服务器参数
    m_serverSampleRate = serverSampleRate;
    m_serverChannels = serverChannels;

    // ConversationManager初始化完成（WebSocket模式）

    // 连接WebSocket信号
    connect(m_websocketManager, &network::WebSocketManager::connected,
            this, &ConversationManager::onWebSocketConnected);
    connect(m_websocketManager, &network::WebSocketManager::audioDataReceived,
            this, &ConversationManager::onWebSocketAudioReceived);
    connect(m_websocketManager, &network::WebSocketManager::jsonMessageReceived,
            this, &ConversationManager::onWebSocketJsonReceived);

    // 连接录音信号
    connect(m_audioDevice, &AudioDevice::audioReady,
            this, &ConversationManager::onAudioReady);

    // 初始化播放缓冲区
    m_playbackBuffer->open(QIODevice::ReadWrite);
    
    // WebSocket连接后，udpChannelOpened自动设为true
    if (m_websocketManager->isConnected()) {
        m_udpChannelOpened = true;
        emit udpChannelOpenedChanged(true);
    }
}

ConversationManager::~ConversationManager() {
    stopRecording();
    closeAudioChannel();
}

// ========== 对话控制 ==========

void ConversationManager::setMode(ConversationMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        emit modeChanged(mode);
        
        QString modeStr;
        switch (mode) {
            case ConversationMode::Auto: modeStr = "auto"; break;
            case ConversationMode::Manual: modeStr = "manual"; break;
            case ConversationMode::Realtime: modeStr = "realtime"; break;
        }
        utils::Logger::instance().info(QString(" 切换对话模式: %1").arg(modeStr));
    }
}

void ConversationManager::startConversation() {
    if (m_isRecording) {
        utils::Logger::instance().warn(" 已经在录音中");
        return;
    }

    // 检查UDP通道是否已打开
    // 如果未打开，UDP会在MQTT Hello响应后自动建立，用户需等待
    if (!m_udpChannelOpened) {
        utils::Logger::instance().warn(" UDP音频通道尚未建立，请稍候片刻后重试");
        emit errorOccurred("音频通道正在建立中，请稍候片刻后重试");
        return;
    }

    utils::Logger::instance().info(" 开始对话");

    // 开始录音
    if (!m_audioDevice->startRecording()) {
        emit errorOccurred("开始录音失败");
        return;
    }

    m_isRecording = true;
    emit isRecordingChanged(true);

    // 切换到聆听状态
    switchToListening();

    // 发送listen消息（带模式）
    QString modeStr;
    switch (m_mode) {
        case ConversationMode::Auto: modeStr = "auto"; break;
        case ConversationMode::Manual: modeStr = "manual"; break;
        case ConversationMode::Realtime: modeStr = "realtime"; break;
    }
    
    utils::Logger::instance().info(QString("📤 发送start_listening (mode: %1)").arg(modeStr));
    
    if (m_protocolType == ProtocolType::WebSocket) {
        m_websocketManager->sendStartListening(modeStr);  //  传递模式参数
    } else {
        m_mqttManager->sendStartListening(m_sessionId, modeStr);
    }
}

void ConversationManager::stopRecording() {
    if (!m_isRecording) {
        return;
    }

    utils::Logger::instance().info("⏹️ 停止录音");

    // 停止录音
    m_audioDevice->stopRecording();
    m_isRecording = false;
    emit isRecordingChanged(false);

    // 清空PCM缓冲区
    m_pcmBuffer.clear();

    // 发送stop listening消息
    if (m_protocolType == ProtocolType::WebSocket) {
        m_websocketManager->sendStopListening();
    } else {
        m_mqttManager->sendStopListening(m_sessionId);
    }

    // 切换到空闲状态
    switchToIdle();
}

void ConversationManager::abortSpeaking() {
    if (m_state != ConversationState::Speaking) {
        utils::Logger::instance().warn(" 当前不在说话状态");
        return;
    }

    utils::Logger::instance().info("⏸️ 中止说话");

    // 停止播放
    m_audioDevice->stopPlayback();
    m_isPlaying = false;
    emit isPlayingChanged(false);

    // 发送abort消息
    if (m_protocolType == ProtocolType::WebSocket) {
        m_websocketManager->sendAbortSpeaking();
    } else {
        m_mqttManager->sendAbort(m_sessionId, "user_interrupted");
    }

    // 切换回聆听状态
    switchToListening();
}

void ConversationManager::closeAudioChannel() {
    // 关闭UDP音频通道

    // 停止录音和播放
    stopRecording();
    if (m_isPlaying) {
        m_audioDevice->stopPlayback();
        m_isPlaying = false;
        emit isPlayingChanged(false);
    }

    // 断开UDP
    m_udpManager->disconnect();
    m_udpChannelOpened = false;
    emit udpChannelOpenedChanged(false);

    // 切换到空闲状态
    switchToIdle();
}

// ========== 状态切换 ==========

void ConversationManager::switchToListening() {
    if (m_state != ConversationState::Listening) {
        m_state = ConversationState::Listening;
        emit stateChanged(m_state);
        utils::Logger::instance().info("👂 切换到聆听状态");
    }
}

void ConversationManager::switchToSpeaking() {
    if (m_state != ConversationState::Speaking) {
        m_state = ConversationState::Speaking;
        emit stateChanged(m_state);
        utils::Logger::instance().info("🗣️ 切换到说话状态");

        // 停止录音（但不关闭UDP）
        if (m_isRecording) {
            m_audioDevice->stopRecording();
            m_isRecording = false;
            emit isRecordingChanged(false);
            m_pcmBuffer.clear();
        }
    }
}

void ConversationManager::switchToIdle() {
    if (m_state != ConversationState::Idle) {
        m_state = ConversationState::Idle;
        emit stateChanged(m_state);
        utils::Logger::instance().info("💤 切换到空闲状态");
    }
}

// ========== 音频处理 ==========

void ConversationManager::onAudioReady(const QByteArray& pcm_data) {
    if (!m_isRecording || m_state != ConversationState::Listening) {
        return;
    }

    // 累积PCM数据到缓冲区
    m_pcmBuffer.append(pcm_data);

    // 当缓冲区达到一帧大小时，编码并发送
    while (m_pcmBuffer.size() >= m_targetFrameSize) {
        // 提取一帧
        QByteArray frame = m_pcmBuffer.left(m_targetFrameSize);
        m_pcmBuffer.remove(0, m_targetFrameSize);

        // 编码并发送
        sendEncodedAudio(frame);
    }
}

void ConversationManager::sendEncodedAudio(const QByteArray& pcm_data) {
    // Opus编码
    QByteArray opus_data = m_codec->encode(pcm_data);
    if (opus_data.isEmpty()) {
        utils::Logger::instance().error(" Opus编码失败");
        return;
    }

    // 根据协议类型发送
    if (m_protocolType == ProtocolType::WebSocket) {
        // WebSocket发送
        m_websocketManager->sendAudioData(opus_data);
    } else {
        // UDP发送（加密在UdpManager中完成）
        m_udpManager->sendAudioData(opus_data);
    }
}

void ConversationManager::onUdpAudioReceived(const QByteArray& opus_data) {
    if (m_state != ConversationState::Speaking) {
        // 不在说话状态，忽略收到的音频
        return;
    }

    receiveDecodedAudio(opus_data);
}

void ConversationManager::receiveDecodedAudio(const QByteArray& opus_data) {
    //  直接按 Opus 帧解码（UDP负载即为纯Opus数据，无需再剥离任何头部）
    QByteArray pcm_data = m_codec->decode(opus_data);
    
    // 如果解码失败，直接跳过这个包（可能是网络包损坏）
    if (pcm_data.isEmpty()) {
        return;
    }

    // 新增：累积TTS音频数据
    if (m_isTtsAccumulating && !pcm_data.isEmpty()) {
        m_currentTtsPcm.append(pcm_data);
    }

    // 播放PCM音频
    if (!m_isPlaying) {
        m_audioDevice->startPlayback();
        m_isPlaying = true;
        emit isPlayingChanged(true);
    }

    // 将PCM数据写入音频设备进行播放
    if (m_audioDevice) {
        m_audioDevice->writeAudioData(pcm_data);
    }
}

// ========== MQTT消息处理 ==========

void ConversationManager::onMqttMessageReceived(const QJsonObject& message) {
    QString type = message["type"].toString();

    if (type == "stt") {
        // 语音识别结果
        QString text = message["text"].toString();
        bool isFinal = message["is_final"].toBool(false);
        
        utils::Logger::instance().info(QString("📝 STT: %1 (final=%2)").arg(text).arg(isFinal));
        emit sttTextReceived(text);
        
        // 修改：即使is_final为false也保存STT消息，因为服务器可能不发送final=true
        if (!text.isEmpty()) {
            qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
            emit sttMessageCompleted(text, timestamp);
        }
    }
    else if (type == "tts") {
        // TTS文本
        QString text = message["text"].toString();
        QString state = message["state"].toString();
        
        utils::Logger::instance().info(QString("💬 TTS: %1").arg(text));
        emit ttsTextReceived(text);

        if (state == "start" || state == "sentence_start") {
            // TTS开始：初始化累积
            m_currentTtsText = text;
            m_currentTtsPcm.clear();
            m_currentTtsStartTime = QDateTime::currentMSecsSinceEpoch();
            m_isTtsAccumulating = true;
            
            m_codec->resetDecoderState();
            
            // 立即发射消息开始信号，让UI显示文字（音频在后台继续播放）
            emit ttsMessageStarted(m_currentTtsText, m_currentTtsStartTime);
        }
        else if (state == "end" || state == "sentence_end" || state == "stop") {
            // TTS结束：保存完整消息
            m_isTtsAccumulating = false;
            
            // 通知DeviceSession保存消息（携带累积的PCM数据）
            emit ttsMessageCompleted(m_currentTtsText, m_currentTtsPcm, m_currentTtsStartTime);
            
            // 清空累积
            m_currentTtsText.clear();
            m_currentTtsPcm.clear();
        }

        // 切换到说话状态
        switchToSpeaking();
    }
    else if (type == "llm") {
        // LLM情感表达（可选）
        QString emotion = message["emotion"].toString();
        utils::Logger::instance().info(QString("😊 LLM Emotion: %1").arg(emotion));
    }
    else if (type == "system") {
        // 系统消息
        QString action = message["action"].toString();
        
        if (action == "audio_end") {
            // 服务器音频发送完毕，切回聆听
            // 服务器音频发送完毕
            
            if (m_isPlaying) {
                m_audioDevice->stopPlayback();
                m_playbackBuffer->close();
                m_playbackBuffer->buffer().clear();
                m_playbackBuffer->open(QIODevice::ReadWrite);
                m_isPlaying = false;
                emit isPlayingChanged(false);
            }

            // 如果在auto模式，自动切回聆听并开始录音
            if (m_mode == ConversationMode::Auto) {
                switchToListening();
                if (!m_isRecording) {
                    startConversation();
                }
            } else {
                // manual模式，切换到空闲
                switchToIdle();
            }
        }
    }
}

void ConversationManager::onUdpConnected() {
    m_udpChannelOpened = true;
    emit udpChannelOpenedChanged(true);
    // UDP音频通道已开启
}

// ========== WebSocket回调 ==========

void ConversationManager::onWebSocketConnected() {
    m_udpChannelOpened = true;  // WebSocket模式下复用此标志
    emit udpChannelOpenedChanged(true);
    // WebSocket音频通道已开启
}

void ConversationManager::onWebSocketAudioReceived(const QByteArray& opus_data) {
    // 与UDP模式相同的处理逻辑
    receiveDecodedAudio(opus_data);
}

void ConversationManager::onWebSocketJsonReceived(const QString& jsonData) {
    // 解析JSON消息
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
    if (!doc.isObject()) {
        utils::Logger::instance().warn(QString("收到无效的JSON消息: %1").arg(jsonData));
        return;
    }
    
    QJsonObject message = doc.object();
    
    // 与MQTT模式相同的处理逻辑
    onMqttMessageReceived(message);
}

} // namespace audio
} // namespace xiaozhi

