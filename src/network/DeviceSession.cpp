/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-12T06:30:00Z
File: DeviceSession.cpp
Desc: 设备会话管理器实现（使用MAC生成固定UUID，确保设备身份持久化）
*/

#include "DeviceSession.h"
#include "../models/ChatMessage.h"
#include "../utils/Logger.h"
#include <QUuid>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>

namespace xiaozhi {
namespace network {

QString DeviceSession::generateUuidFromMac(const QString& macAddress) {
    // 使用MAC地址作为种子，生成确定性的UUID（UUID v5）
    // 这样相同的MAC地址总是生成相同的UUID，保证设备标识的持久性
    QString cleanMac = macAddress.toLower().remove(':').remove('-').remove(' ');
    
    // 使用Qt的UUID v5生成（基于SHA-1的命名空间UUID）
    // 使用DNS命名空间作为基础
    QUuid namespaceDns = QUuid("{6ba7b810-9dad-11d1-80b4-00c04fd430c8}");
    QUuid deviceUuid = QUuid::createUuidV5(namespaceDns, cleanMac);
    
    return deviceUuid.toString(QUuid::WithoutBraces);
}

DeviceSession::DeviceSession(const QString& deviceId,
                             const QString& deviceName,
                             const QString& macAddress,
                             const QString& otaUrl,
                             audio::AudioDevice* audioDevice,
                             bool websocketEnabled,
                             QObject* parent)
    : QObject(parent)
    , m_deviceId(deviceId)
    , m_deviceName(deviceName)
    , m_macAddress(macAddress)
    , m_otaUrl(otaUrl)
    , m_uuid(generateUuidFromMac(macAddress))  // 基于MAC生成固定UUID
    , m_mqttConnected(false)
    , m_udpConnected(false)
    , m_websocketConnected(false)
    , m_websocketEnabled(websocketEnabled)
    , m_audioDevice(audioDevice)
{
    // 记录设备UUID（基于MAC地址生成，持久化）
    // 设备UUID已生成
    
    // 创建独立的网络管理器实例
    m_otaManager = std::make_unique<OtaManager>(this);
    m_mqttManager = std::make_unique<MqttManager>(this);
    m_udpManager = std::make_unique<UdpManager>(this);

    // 连接OTA信号
    connect(m_otaManager.get(), &OtaManager::otaConfigReceived,
            this, &DeviceSession::onOtaConfigReceived);
    connect(m_otaManager.get(), &OtaManager::errorOccurred,
            this, &DeviceSession::onOtaError);

    // 连接MQTT信号
    connect(m_mqttManager.get(), &MqttManager::connected,
            this, &DeviceSession::onMqttConnected);
    connect(m_mqttManager.get(), &MqttManager::disconnected,
            this, &DeviceSession::onMqttDisconnected);
    connect(m_mqttManager.get(), &MqttManager::messageReceived,
            this, &DeviceSession::onMqttMessage);
    connect(m_mqttManager.get(), &MqttManager::udpConfigReceived,
            this, &DeviceSession::onUdpConfigReceived);
    connect(m_mqttManager.get(), &MqttManager::errorOccurred,
            this, &DeviceSession::onMqttError);

    // 连接UDP信号
    connect(m_udpManager.get(), &UdpManager::udpConnected,
            this, &DeviceSession::onUdpConnected);
    connect(m_udpManager.get(), &UdpManager::audioDataReceived,
            this, &DeviceSession::onUdpAudioData);
    connect(m_udpManager.get(), &UdpManager::errorOccurred,
            this, &DeviceSession::onUdpError);

    // 初始日志输出到控制台
    // 设备会话创建完成
}

DeviceSession::~DeviceSession() {
    disconnect();
}

void DeviceSession::getOtaConfig() {
    emit logMessage(m_deviceId, "正在连接...");
    
    DeviceInfo deviceInfo = OtaManager::generateDeviceInfo(m_macAddress, m_uuid);
    m_otaManager->requestOtaConfig(deviceInfo, m_otaUrl);
}

void DeviceSession::connectMqtt() {
    if (!m_otaConfig.mqtt.isValid()) {
        emit logMessage(m_deviceId, "连接失败：服务器配置无效");
        return;
    }

    utils::Logger::instance().info(QString("[%1] 正在连接MQTT...").arg(m_deviceId));
    m_mqttManager->connectToMqtt(m_otaConfig.mqtt);
}

void DeviceSession::requestAudioChannel() {
    if (!m_udpConnected && m_otaConfig.udp.isValid()) {
        emit logMessage(m_deviceId, "正在建立音频通道...");
        m_udpManager->connectToUdp(m_otaConfig.udp);
    } else if (m_udpConnected) {
        emit logMessage(m_deviceId, "音频通道已就绪");
    } else {
        emit logMessage(m_deviceId, "音频通道未配置");
    }
}

void DeviceSession::sendTextMessage(const QString& text) {
    // 检查连接状态（支持WebSocket和MQTT）
    if (!isConnected()) {
        emit logMessage(m_deviceId, "未连接，无法发送消息");
        return;
    }
    
    if (m_sessionId.isEmpty()) {
        emit logMessage(m_deviceId, "会话未建立，请先建立音频通道");
        utils::Logger::instance().warn(QString("[%1] session_id为空，无法发送文本消息").arg(m_deviceId));
        return;
    }

    // 构建文本消息JSON（对齐ESP32格式）
    QJsonObject message;
    message["session_id"] = m_sessionId;
    message["type"] = "text";
    message["text"] = text;
    
    QJsonDocument doc(message);
    QString jsonStr = doc.toJson(QJsonDocument::Compact);
    
    // 根据协议类型发送
    if (m_websocketConnected && m_websocketManager) {
        // WebSocket协议：直接发送JSON消息
        m_websocketManager->sendJsonMessage(jsonStr);
    } else if (m_mqttConnected && m_mqttManager) {
        // MQTT协议：发送完整格式消息
        message["version"] = 3;
        message["state"] = "";
        message["mode"] = "";
        message["transport"] = "mqtt_udp";
        
        // 音频参数
        QJsonObject audioParams;
        audioParams["format"] = "opus";
        audioParams["sample_rate"] = m_conversationManager->serverSampleRate();
        audioParams["channels"] = m_conversationManager->serverChannels();
        audioParams["frame_duration"] = 60;
        message["audio_params"] = audioParams;
        
        message["data"] = QJsonObject();
        message["payload"] = QJsonObject();

        // 发送到MQTT
        QString topic = m_otaConfig.mqtt.publish_topic;
        m_mqttManager->sendRawMessage(topic, message);
    }
}

void DeviceSession::sendTestAudio() {
    if (!m_udpConnected) {
        emit logMessage(m_deviceId, "音频通道未连接");
        return;
    }

    // 技术日志只输出到控制台
    utils::Logger::instance().info(QString("[%1] 发送测试音频").arg(m_deviceId));
    m_udpManager->sendTestAudio(m_sessionId);
}

void DeviceSession::sendImageMessage(const QString& imagePath, const QString& text) {
    // 检查连接状态（支持WebSocket和MQTT）
    if (!isConnected()) {
        emit logMessage(m_deviceId, "未连接，无法发送消息");
        return;
    }
    
    if (m_sessionId.isEmpty()) {
        emit logMessage(m_deviceId, "会话未建立，请先建立音频通道");
        utils::Logger::instance().warn(QString("[%1] session_id为空，无法发送图片消息").arg(m_deviceId));
        return;
    }

    // 读取图片文件
    QFile imageFile(imagePath);
    if (!imageFile.open(QIODevice::ReadOnly)) {
        emit logMessage(m_deviceId, QString("无法读取图片: %1").arg(imagePath));
        utils::Logger::instance().error(QString("[%1] 图片读取失败: %2").arg(m_deviceId, imagePath));
        return;
    }

    QByteArray imageData = imageFile.readAll();
    imageFile.close();

    // 转换为Base64
    QString base64Image = imageData.toBase64();
    
    // 获取图片格式（扩展名）
    QString format = QFileInfo(imagePath).suffix().toLower();
    if (format.isEmpty()) {
        format = "jpg";  // 默认格式
    }

    QString fileName = QFileInfo(imagePath).fileName();
    
    // 根据协议类型发送
    if (m_websocketConnected && m_websocketManager) {
        // WebSocket协议：简化的JSON格式（对齐ESP32）
        QJsonObject message;
        message["session_id"] = m_sessionId;
        message["type"] = "image";
        message["text"] = text.isEmpty() ? "这张图片里有什么？" : text;
        
        // data字段
        QJsonObject dataObj;
        dataObj["image"] = base64Image;
        dataObj["url"] = "";
        dataObj["format"] = format;
        message["data"] = dataObj;
        
        QJsonDocument doc(message);
        QString jsonStr = doc.toJson(QJsonDocument::Compact);
        
        m_websocketManager->sendJsonMessage(jsonStr);
            
    } else if (m_mqttConnected && m_mqttManager) {
        // MQTT协议：完整格式消息
        QJsonObject message;
        message["type"] = "image";
        message["text"] = text.isEmpty() ? "这张图片里有什么？" : text;
        message["session_id"] = m_sessionId;
        message["version"] = 3;
        message["state"] = "";
        message["mode"] = "";
        message["transport"] = "mqtt_udp";
        
        // 音频参数
        QJsonObject audioParams;
        audioParams["format"] = "opus";
        audioParams["sample_rate"] = m_conversationManager->serverSampleRate();
        audioParams["channels"] = m_conversationManager->serverChannels();
        audioParams["frame_duration"] = 60;
        message["audio_params"] = audioParams;
        
        // data结构
        QJsonObject dataObj;
        dataObj["image"] = base64Image;
        dataObj["url"] = "";
        dataObj["format"] = format;
        message["data"] = dataObj;
        
        message["payload"] = QJsonObject();

        // 发送到MQTT
        QString topic = m_otaConfig.mqtt.publish_topic;
        m_mqttManager->sendRawMessage(topic, message);
    }
    
    emit logMessage(m_deviceId, QString("发送图片: %1").arg(fileName));
}

void DeviceSession::disconnect() {
    if (m_mqttConnected) {
        m_mqttManager->disconnect();
        m_mqttConnected = false;
    }

    if (m_udpConnected) {
        m_udpManager->disconnect();
        m_udpConnected = false;
    }

    emit connectionStateChanged(m_deviceId, false, false);
    emit logMessage(m_deviceId, "已断开连接");
}

// ========== OTA回调 ==========

void DeviceSession::onOtaConfigReceived(const OtaConfig& config) {
    m_otaConfig = config;

    emit logMessage(m_deviceId, "连接成功");

    // 提取激活码并格式化显示
    if (!config.activation.code.isEmpty()) {
        m_activationCode = config.activation.code;
        
        // 构建友好的激活消息，以气泡形式显示
        QString activationMsg = QString("\n━━━━━━━━━━━━━━━━━━━━━━\n"
                                       "设备绑定验证码\n"
                                       "━━━━━━━━━━━━━━━━━━━━━━\n\n"
                                       "验证码：%1\n\n").arg(m_activationCode);
        
        // 添加消息提示
        if (!config.activation.message.isEmpty()) {
            activationMsg += QString("%1\n\n").arg(config.activation.message);
        }
        
        // 添加控制面板URL
        if (config.bind_instructions.contains("web_url")) {
            QString webUrl = config.bind_instructions["web_url"].toString();
            if (!webUrl.isEmpty()) {
                activationMsg += QString("控制面板：%1\n\n").arg(webUrl);
            }
        }
        
        // 添加绑定步骤
        if (config.bind_instructions.contains("step1")) {
            activationMsg += "绑定步骤：\n";
            activationMsg += QString("① %1\n").arg(config.bind_instructions["step1"].toString());
            if (config.bind_instructions.contains("step2")) {
                activationMsg += QString("② %1\n").arg(config.bind_instructions["step2"].toString());
            }
            if (config.bind_instructions.contains("step3")) {
                activationMsg += QString("③ %1\n").arg(config.bind_instructions["step3"].toString());
            }
        }
        
        activationMsg += "\n━━━━━━━━━━━━━━━━━━━━━━";
        
        // 作为聊天消息显示在聊天框
        xiaozhi::models::ChatMessage activationMessage;
        activationMessage.deviceId = m_deviceId;
        activationMessage.messageType = "activation";
        activationMessage.textContent = activationMsg;
        activationMessage.audioFilePath = "";
        activationMessage.timestamp = QDateTime::currentMSecsSinceEpoch();
        activationMessage.isFinal = true;
        activationMessage.createdAt = QDateTime::currentDateTime();
        
        // 发送聊天消息
        emit chatMessageReceived(m_deviceId, activationMessage);
        emit activationCodeReceived(m_deviceId, m_activationCode);
    }

    // 已移除OTA配置详情日志（敏感信息）

    // ========== 协议选择逻辑 ==========
    // 优先级：WebSocket（如果启用且可用） > MQTT+UDP
    
    if (m_websocketEnabled && config.hasWebSocket) {
        // 使用WebSocket协议
        utils::Logger::instance().info(QString("[%1] 🌐 使用WebSocket协议").arg(m_deviceId));
        emit logMessage(m_deviceId, "正在连接WebSocket服务器...");
        
        // 创建WebSocketManager
        m_websocketManager = std::make_unique<WebSocketManager>(this);
        
        // 连接WebSocket信号
        connect(m_websocketManager.get(), &WebSocketManager::connected,
                this, &DeviceSession::onWebSocketConnected);
        connect(m_websocketManager.get(), &WebSocketManager::disconnected,
                this, &DeviceSession::onWebSocketDisconnected);
        connect(m_websocketManager.get(), &WebSocketManager::errorOccurred,
                this, &DeviceSession::onWebSocketError);
        
        // 连接WebSocket服务器
        m_websocketManager->connectToServer(config.websocket, m_macAddress, m_uuid);
        
    } else if (config.hasMqtt) {
        // 使用MQTT+UDP协议（默认）
        utils::Logger::instance().info(QString("[%1] 📡 使用MQTT+UDP协议").arg(m_deviceId));
        
        if (config.mqtt.isValid()) {
            utils::Logger::instance().info(QString("[%1] 自动连接MQTT...").arg(m_deviceId));
            QTimer::singleShot(1000, this, &DeviceSession::connectMqtt);
        } else {
            emit logMessage(m_deviceId, "服务器配置无效");
            // 已移除MQTT配置详情日志（敏感信息）
        }
    } else {
        emit logMessage(m_deviceId, "服务器未提供任何可用协议");
        utils::Logger::instance().error(QString("[%1] OTA响应中既无MQTT也无WebSocket配置").arg(m_deviceId));
    }
}

void DeviceSession::onOtaError(const QString& error) {
    emit logMessage(m_deviceId, QString("OTA错误: %1").arg(error));
}

// ========== MQTT回调 ==========

void DeviceSession::onMqttConnected() {
    // 避免重复处理（MQTT客户端可能多次触发连接回调）
    if (m_mqttConnected) {
        utils::Logger::instance().debug(" MQTT已连接，忽略重复回调");
        return;
    }

    m_mqttConnected = true;
    emit logMessage(m_deviceId, "MQTT已连接");
    emit connectionStateChanged(m_deviceId, true, m_udpConnected);

    // 发送hello消息（只发送一次）
    QTimer::singleShot(500, [this]() {
        m_mqttManager->sendHello(m_otaConfig.transport_type);
    });
}

void DeviceSession::onMqttDisconnected(int rc) {
    m_mqttConnected = false;
    if (rc != 0) {
        emit logMessage(m_deviceId, "连接已断开");
        utils::Logger::instance().warn(QString("[%1] MQTT断开连接，错误码: %2").arg(m_deviceId).arg(rc));
    } else {
        emit logMessage(m_deviceId, "已断开连接");
    }
    emit connectionStateChanged(m_deviceId, false, m_udpConnected);
}

void DeviceSession::onMqttMessage(const QJsonObject& message) {
    QString msgType = message["type"].toString();
    // 已移除MQTT消息详情日志（敏感信息）

    // 可以在这里添加更多消息处理逻辑
}

void DeviceSession::onUdpConfigReceived(const UdpConfig& config, const QString& sessionId) {
    m_sessionId = sessionId;
    m_otaConfig.udp = config;

    // 已移除会话和UDP配置详情日志（敏感信息）

    // 立即建立UDP连接
    emit logMessage(m_deviceId, QString("会话已建立 (ID: %1)").arg(sessionId.left(8) + "..."));
    m_udpManager->connectToUdp(config);

    // 创建对话管理器（此时已有MQTT、UDP配置、sessionId、服务器音频参数）
    if (!m_conversationManager && m_audioDevice) {
        m_conversationManager = std::make_unique<audio::ConversationManager>(
            m_mqttManager.get(),
            m_udpManager.get(),
            m_audioDevice,
            m_sessionId,
            config.serverSampleRate,      // 传递服务器采样率
            config.serverChannels,        // 传递服务器声道数
            config.serverFrameDuration    // 传递服务器帧时长
        );
        
        // 连接对话管理器的信号到设备会话
        connect(m_conversationManager.get(), &audio::ConversationManager::sttTextReceived,
                this, [this](const QString& text) {
            emit logMessage(m_deviceId, QString(" 识别: %1").arg(text));
        });
        
        connect(m_conversationManager.get(), &audio::ConversationManager::ttsTextReceived,
                this, [this](const QString& text) {
            emit logMessage(m_deviceId, QString(" 小智: %1").arg(text));
        });
        
        connect(m_conversationManager.get(), &audio::ConversationManager::errorOccurred,
                this, [this](const QString& error) {
            emit logMessage(m_deviceId, QString(" 对话错误: %1").arg(error));
        });
        
        // 连接消息完成信号
        connect(m_conversationManager.get(), &audio::ConversationManager::ttsMessageStarted,
                this, &DeviceSession::onTtsMessageStarted);
        connect(m_conversationManager.get(), &audio::ConversationManager::ttsMessageCompleted,
                this, &DeviceSession::onTtsMessageCompleted);
        connect(m_conversationManager.get(), &audio::ConversationManager::sttMessageCompleted,
                this, &DeviceSession::onSttMessageCompleted);
    }

    // 发送IoT描述符（延迟1秒）
    QTimer::singleShot(1000, [this]() {
        m_mqttManager->sendIotDescriptors(m_sessionId);
    });

    // 发送IoT状态（延迟1.5秒）
    QTimer::singleShot(1500, [this]() {
        m_mqttManager->sendIotStates(m_sessionId);
    });
}

void DeviceSession::onMqttError(const QString& error) {
    emit logMessage(m_deviceId, QString("MQTT错误: %1").arg(error));
}

// ========== UDP回调 ==========

void DeviceSession::onUdpConnected() {
    m_udpConnected = true;
    emit logMessage(m_deviceId, "音频通道已就绪");
    emit connectionStateChanged(m_deviceId, m_mqttConnected, true);
}

void DeviceSession::onUdpAudioData(const QByteArray& data) {
    // 接收到音频数据（可以在这里处理）
}

void DeviceSession::onUdpError(const QString& error) {
    emit logMessage(m_deviceId, QString("UDP错误: %1").arg(error));
}

// ========== 消息完成回调 ==========

void DeviceSession::onTtsMessageStarted(const QString& text, qint64 timestamp) {
    // 过滤空消息（服务器有时会发送空文本的 stop 消息）
    if (text.trimmed().isEmpty()) {
        return;
    }
    
    // 立即发送消息到UI（不带音频数据），让文字先显示
    xiaozhi::models::ChatMessage msg;
    msg.deviceId = m_deviceId;
    msg.messageType = "tts";
    msg.textContent = text;
    msg.audioFilePath = "";
    msg.timestamp = timestamp;
    msg.isFinal = false;  // 标记为非最终（音频还在播放）
    msg.createdAt = QDateTime::fromMSecsSinceEpoch(timestamp);
    
    emit chatMessageReceived(m_deviceId, msg, QByteArray());  // 不传递PCM数据
}

void DeviceSession::onTtsMessageCompleted(const QString& text, const QByteArray& pcmData, qint64 timestamp) {
    // 过滤空消息（服务器有时会发送空文本的 stop 消息）
    if (text.trimmed().isEmpty()) {
        return;
    }
    
    // 音频播放完成，只保存音频（UI已经在 onTtsMessageStarted 中显示了）
    // 注意：这里仍然发送消息，但 AppModel 会识别相同 timestamp 并更新音频路径
    xiaozhi::models::ChatMessage msg;
    msg.deviceId = m_deviceId;
    msg.messageType = "tts";
    msg.textContent = text;
    msg.audioFilePath = "";  // 将在AppModel中设置
    msg.timestamp = timestamp;
    msg.isFinal = true;
    msg.createdAt = QDateTime::fromMSecsSinceEpoch(timestamp);
    
    // 将PCM数据作为额外参数传递（通过信号参数）
    emit chatMessageReceived(m_deviceId, msg, pcmData);
}

void DeviceSession::onSttMessageCompleted(const QString& text, qint64 timestamp) {
    xiaozhi::models::ChatMessage msg;
    msg.deviceId = m_deviceId;
    msg.messageType = "stt";
    msg.textContent = text;
    msg.audioFilePath = "";  // STT暂不保存音频
    msg.timestamp = timestamp;
    msg.isFinal = true;
    msg.createdAt = QDateTime::fromMSecsSinceEpoch(timestamp);
    
    emit chatMessageReceived(m_deviceId, msg);
    
    utils::Logger::instance().info(QString("[%1] STT消息完成: %2")
        .arg(m_deviceId, text));
}

// ========== WebSocket回调 ==========

void DeviceSession::onWebSocketConnected() {
    m_websocketConnected = true;
    m_sessionId = m_websocketManager->sessionId();
    
    emit logMessage(m_deviceId, "WebSocket已连接");
    emit connectionStateChanged(m_deviceId, true, false);  // WebSocket模式，UDP标志设为false
    
    // 创建WebSocket模式的对话管理器
    if (!m_conversationManager && m_audioDevice) {
        m_conversationManager = std::make_unique<audio::ConversationManager>(
            m_websocketManager.get(),
            m_audioDevice,
            m_sessionId,
            m_websocketManager->serverSampleRate(),
            m_websocketManager->serverChannels(),
            m_websocketManager->serverFrameDuration()
        );
        
        // 连接对话管理器的信号
        connect(m_conversationManager.get(), &audio::ConversationManager::sttTextReceived,
                this, [this](const QString& text) {
            emit logMessage(m_deviceId, QString(" 识别: %1").arg(text));
        });
        
        connect(m_conversationManager.get(), &audio::ConversationManager::ttsTextReceived,
                this, [this](const QString& text) {
            emit logMessage(m_deviceId, QString(" 小智: %1").arg(text));
        });
        
        connect(m_conversationManager.get(), &audio::ConversationManager::errorOccurred,
                this, [this](const QString& error) {
            emit logMessage(m_deviceId, QString(" 对话错误: %1").arg(error));
        });
        
        // 连接消息完成信号
        connect(m_conversationManager.get(), &audio::ConversationManager::ttsMessageStarted,
                this, &DeviceSession::onTtsMessageStarted);
        connect(m_conversationManager.get(), &audio::ConversationManager::ttsMessageCompleted,
                this, &DeviceSession::onTtsMessageCompleted);
        connect(m_conversationManager.get(), &audio::ConversationManager::sttMessageCompleted,
                this, &DeviceSession::onSttMessageCompleted);
    }
}

void DeviceSession::onWebSocketDisconnected() {
    m_websocketConnected = false;
    emit logMessage(m_deviceId, "WebSocket已断开");
    emit connectionStateChanged(m_deviceId, false, false);
    
    utils::Logger::instance().warn(QString("[%1] WebSocket连接断开").arg(m_deviceId));
}

void DeviceSession::onWebSocketError(const QString& error) {
    emit logMessage(m_deviceId, QString("WebSocket错误: %1").arg(error));
    utils::Logger::instance().error(QString("[%1] WebSocket错误: %2").arg(m_deviceId, error));
}

} // namespace network
} // namespace xiaozhi

