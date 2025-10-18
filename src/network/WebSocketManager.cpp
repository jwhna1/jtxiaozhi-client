/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-12T16:00:00Z
File: WebSocketManager.cpp
Desc: WebSocket通信管理器实现（完全复刻ESP32 websocket_protocol逻辑）
*/

#include "WebSocketManager.h"
#include "../utils/Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtEndian>
#include <QSslConfiguration>

namespace xiaozhi {
namespace network {

// ============================================================================
// 二进制协议定义（对齐ESP32固件）
// ============================================================================

#pragma pack(push, 1)

struct BinaryProtocol2 {
    quint16 version;        // 协议版本（大端序）
    quint16 type;           // 消息类型（0: OPUS, 1: JSON）
    quint32 reserved;       // 保留字段
    quint32 timestamp;      // 时间戳（大端序）
    quint32 payload_size;   // 数据长度（大端序）
    // payload data follows
};

struct BinaryProtocol3 {
    quint8 type;            // 消息类型（0: OPUS）
    quint8 reserved;        // 保留字段
    quint16 payload_size;   // 数据长度（大端序）
    // payload data follows
};

#pragma pack(pop)

// ============================================================================
// WebSocketManager实现
// ============================================================================

WebSocketManager::WebSocketManager(QObject* parent)
    : QObject(parent)
    , m_webSocket(nullptr)
    , m_helloReceived(false)
    , m_helloTimer(new QTimer(this))
{
    m_helloTimer->setSingleShot(true);
    connect(m_helloTimer, &QTimer::timeout, this, &WebSocketManager::onHelloTimeout);
}

WebSocketManager::~WebSocketManager() {
    disconnect();
}

bool WebSocketManager::connectToServer(const WebSocketConfig& config, 
                                      const QString& deviceId, 
                                      const QString& clientId) {
    QMutexLocker locker(&m_mutex);

    if (!config.isValid()) {
        utils::Logger::instance().error("WebSocket配置无效");
        emit errorOccurred("WebSocket配置无效");
        return false;
    }

    // 断开旧连接
    if (m_webSocket != nullptr) {
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    m_config = config;
    m_deviceId = deviceId;
    m_clientId = clientId;
    m_helloReceived = false;
    m_sessionId.clear();

    // 创建WebSocket
    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    // 配置SSL（如果是wss://）
    if (m_config.url.startsWith("wss://")) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        // 在生产环境中，应该验证证书
        // 这里为了兼容性暂时不验证
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        m_webSocket->setSslConfiguration(sslConfig);
        utils::Logger::instance().info("🔒 WebSocket使用SSL/TLS加密");
    }

    // 设置HTTP Headers（对齐ESP32）
    QNetworkRequest request(m_config.url);
    
    // Authorization header
    if (!m_config.token.isEmpty()) {
        QString authHeader = m_config.token;
        // 如果token不包含空格，添加"Bearer "前缀
        if (!authHeader.contains(" ")) {
            authHeader = "Bearer " + authHeader;
        }
        request.setRawHeader("Authorization", authHeader.toUtf8());
    }
    
    // Protocol-Version header
    request.setRawHeader("Protocol-Version", QString::number(m_config.version).toUtf8());
    
    // Device-Id header
    request.setRawHeader("Device-Id", m_deviceId.toUtf8());
    
    // Client-Id header
    request.setRawHeader("Client-Id", m_clientId.toUtf8());

    // 连接信号
    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketManager::onWebSocketConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketManager::onWebSocketDisconnected);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &WebSocketManager::onWebSocketError);
    connect(m_webSocket, &QWebSocket::textMessageReceived, 
            this, &WebSocketManager::onWebSocketTextMessageReceived);
    connect(m_webSocket, &QWebSocket::binaryMessageReceived, 
            this, &WebSocketManager::onWebSocketBinaryMessageReceived);

    // 已移除WebSocket连接URL日志（敏感信息）

    // 发起连接
    m_webSocket->open(request);
    
    // 启动Hello超时计时器
    m_helloTimer->start(HELLO_TIMEOUT_MS);

    return true;
}

void WebSocketManager::disconnect() {
    QMutexLocker locker(&m_mutex);

    m_helloTimer->stop();

    if (m_webSocket != nullptr) {
        m_webSocket->close();
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    m_helloReceived = false;
    m_sessionId.clear();
}

bool WebSocketManager::isConnected() const {
    return m_webSocket != nullptr && 
           m_webSocket->state() == QAbstractSocket::ConnectedState &&
           m_helloReceived;
}

bool WebSocketManager::sendAudioData(const QByteArray& opusData, quint32 timestamp) {
    QMutexLocker locker(&m_mutex);

    if (!isConnected()) {
        return false;
    }

    QByteArray packet = buildBinaryPacket(opusData, timestamp);
    
    // 检查数据包是否构建成功
    if (packet.isEmpty()) {
        utils::Logger::instance().error("构建音频数据包失败");
        return false;
    }
    
    qint64 sent = m_webSocket->sendBinaryMessage(packet);
    
    return sent > 0;
}

bool WebSocketManager::sendJsonMessage(const QString& jsonData) {
    QMutexLocker locker(&m_mutex);

    if (!isConnected()) {
        return false;
    }

    //  安全检查：确保JSON数据有效
    if (jsonData.isEmpty()) {
        utils::Logger::instance().error("JSON消息为空，无法发送");
        return false;
    }

    qint64 sent = m_webSocket->sendTextMessage(jsonData);
    
    if (sent <= 0) {
        // 限制日志长度，避免过长消息导致问题
        int len = jsonData.length();
        QString logMsg = (len > 0 && len <= 200) ? jsonData : 
                        (len > 200) ? jsonData.left(200) + "..." : "(empty)";
        utils::Logger::instance().error(QString("发送JSON消息失败: %1").arg(logMsg));
        return false;
    }

    return true;
}

bool WebSocketManager::sendStartListening(const QString& mode) {
    // 对齐ESP32固件：{"session_id":"xxx","type":"listen","state":"start","mode":"manual|auto|realtime"}
    QJsonObject json;
    json["session_id"] = m_sessionId;
    json["type"] = "listen";
    json["state"] = "start";
    json["mode"] = mode.isEmpty() ? QString("manual") : mode;  // 默认manual模式
    
    QJsonDocument doc(json);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    QString jsonStr = QString::fromUtf8(jsonBytes);
    
    utils::Logger::instance().debug(QString("📤 WebSocket发送listen消息: %1").arg(jsonStr));
    return sendJsonMessage(jsonStr);
}

bool WebSocketManager::sendStopListening() {
    // 对齐ESP32固件：{"session_id":"xxx","type":"listen","state":"stop"}
    QJsonObject json;
    json["session_id"] = m_sessionId;
    json["type"] = "listen";
    json["state"] = "stop";
    
    QJsonDocument doc(json);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    QString jsonStr = QString::fromUtf8(jsonBytes);
    
    return sendJsonMessage(jsonStr);
}

bool WebSocketManager::sendAbortSpeaking() {
    QJsonObject json;
    json["type"] = "abort";
    json["session_id"] = m_sessionId;
    
    QJsonDocument doc(json);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    QString jsonStr = QString::fromUtf8(jsonBytes);
    
    return sendJsonMessage(jsonStr);
}

// ============================================================================
// Private slots
// ============================================================================

void WebSocketManager::onWebSocketConnected() {
    utils::Logger::instance().info(" WebSocket TCP连接已建立，正在发送Hello消息...");
    
    // 发送客户端Hello消息
    if (!sendClientHello()) {
        utils::Logger::instance().error("发送Hello消息失败");
        disconnect();
        emit errorOccurred("发送Hello消息失败");
    }
}

void WebSocketManager::onWebSocketDisconnected() {
    utils::Logger::instance().info("🔌 WebSocket连接已断开");
    m_helloReceived = false;
    m_helloTimer->stop();
    emit disconnected();
}

void WebSocketManager::onWebSocketError(QAbstractSocket::SocketError error) {
    QString errorStr = m_webSocket ? m_webSocket->errorString() : "Unknown error";
    utils::Logger::instance().error(QString("❌ WebSocket错误: %1 (code: %2)")
        .arg(errorStr).arg(static_cast<int>(error)));
    emit errorOccurred(errorStr);
}

void WebSocketManager::onWebSocketTextMessageReceived(const QString& message) {
    //  安全检查：确保消息有效
    if (message.isEmpty()) {
        utils::Logger::instance().warn("收到空的文本消息");
        return;
    }
    
    // 解析JSON消息
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    
    if (!doc.isObject()) {
        // 限制日志长度
        int len = message.length();
        QString logMsg = (len > 0 && len <= 200) ? message : 
                        (len > 200) ? message.left(200) + "..." : "(empty)";
        utils::Logger::instance().warn(QString("收到无效的JSON消息: %1").arg(logMsg));
        return;
    }

    QJsonObject jsonObj = doc.object();
    QString type = jsonObj["type"].toString();

    if (type == "hello") {
        // 服务器Hello响应
        parseServerHello(message);
    } else {
        // 其他JSON消息（stt, tts, llm, system等）
        emit jsonMessageReceived(message);
    }
}

void WebSocketManager::onWebSocketBinaryMessageReceived(const QByteArray& message) {
    // 解析二进制音频数据
    QByteArray opusData = parseBinaryPacket(message);
    
    if (!opusData.isEmpty()) {
        emit audioDataReceived(opusData);
    }
}

void WebSocketManager::onHelloTimeout() {
    if (!m_helloReceived) {
        utils::Logger::instance().error(" 等待服务器Hello响应超时");
        disconnect();
        emit errorOccurred("服务器Hello响应超时");
    }
}

// ============================================================================
// Private methods
// ============================================================================

bool WebSocketManager::sendClientHello() {
    // 构建客户端Hello消息（对齐ESP32固件）
    QJsonObject json;
    json["type"] = "hello";
    json["version"] = m_config.version;
    json["transport"] = "websocket";

    // features
    QJsonObject features;
    features["aec"] = false;  // 暂不支持AEC
    features["mcp"] = true;
    json["features"] = features;

    // audio_params
    QJsonObject audioParams;
    audioParams["format"] = "opus";
    audioParams["sample_rate"] = 16000;  // 客户端上行采样率
    audioParams["channels"] = 1;
    audioParams["frame_duration"] = 60;
    json["audio_params"] = audioParams;

    QJsonDocument doc(json);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    QString helloMessage = QString::fromUtf8(jsonBytes);

    // 已移除Hello消息内容日志（敏感信息）

    qint64 sent = m_webSocket->sendTextMessage(helloMessage);
    return sent > 0;
}

void WebSocketManager::parseServerHello(const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject json = doc.object();

    // 验证transport
    QString transport = json["transport"].toString();
    if (transport != "websocket") {
        utils::Logger::instance().error(QString("不支持的transport: %1").arg(transport));
        disconnect();
        emit errorOccurred("不支持的transport");
        return;
    }

    // 提取session_id
    m_sessionId = json["session_id"].toString();

    // 提取服务器音频参数
    QJsonObject audioParams = json["audio_params"].toObject();
    if (!audioParams.isEmpty()) {
        m_config.serverSampleRate = audioParams["sample_rate"].toInt(24000);
        m_config.serverChannels = audioParams["channels"].toInt(1);
        m_config.serverFrameDuration = audioParams["frame_duration"].toInt(60);
    }

    m_helloReceived = true;
    m_helloTimer->stop();

    // 已移除WebSocket握手详情和服务器参数日志（敏感信息）

    emit connected();
}

QByteArray WebSocketManager::buildBinaryPacket(const QByteArray& opusData, quint32 timestamp) {
    // 安全检查：数据大小必须合理
    if (opusData.isEmpty() || opusData.size() > 1024 * 1024) {  // 最大1MB
        utils::Logger::instance().error(QString("Opus数据大小异常: %1").arg(opusData.size()));
        return QByteArray();
    }
    
    if (m_config.version == 2) {
        // Version 2: 使用BinaryProtocol2
        int packetSize = static_cast<int>(sizeof(BinaryProtocol2)) + opusData.size();
        if (packetSize < 0) {  // 溢出检查
            utils::Logger::instance().error("数据包大小溢出");
            return QByteArray();
        }
        
        QByteArray packet;
        packet.resize(packetSize);

        BinaryProtocol2* header = reinterpret_cast<BinaryProtocol2*>(packet.data());
        header->version = qToBigEndian<quint16>(m_config.version);
        header->type = qToBigEndian<quint16>(0);  // 0 = OPUS
        header->reserved = 0;
        header->timestamp = qToBigEndian<quint32>(timestamp);
        header->payload_size = qToBigEndian<quint32>(static_cast<quint32>(opusData.size()));

        memcpy(packet.data() + sizeof(BinaryProtocol2), opusData.data(), static_cast<size_t>(opusData.size()));
        return packet;

    } else if (m_config.version == 3) {
        // Version 3: 使用BinaryProtocol3
        if (opusData.size() > 65535) {
            utils::Logger::instance().error(QString("Version 3不支持大于65535字节的数据: %1").arg(opusData.size()));
            return QByteArray();
        }
        
        int packetSize = static_cast<int>(sizeof(BinaryProtocol3)) + opusData.size();
        if (packetSize < 0) {  // 溢出检查
            utils::Logger::instance().error("数据包大小溢出");
            return QByteArray();
        }
        
        QByteArray packet;
        packet.resize(packetSize);

        BinaryProtocol3* header = reinterpret_cast<BinaryProtocol3*>(packet.data());
        header->type = 0;  // 0 = OPUS
        header->reserved = 0;
        header->payload_size = qToBigEndian<quint16>(static_cast<quint16>(opusData.size()));

        memcpy(packet.data() + sizeof(BinaryProtocol3), opusData.data(), static_cast<size_t>(opusData.size()));
        return packet;

    } else {
        // Version 1: 直接发送Opus数据
        return opusData;
    }
}

QByteArray WebSocketManager::parseBinaryPacket(const QByteArray& data) {
    if (m_config.version == 2) {
        // Version 2: 解析BinaryProtocol2
        if (data.size() < static_cast<int>(sizeof(BinaryProtocol2))) {
            utils::Logger::instance().warn("二进制数据包太小（Version 2）");
            return QByteArray();
        }

        const BinaryProtocol2* header = reinterpret_cast<const BinaryProtocol2*>(data.data());
        quint32 payloadSize = qFromBigEndian<quint32>(header->payload_size);
        
        // 安全检查：payloadSize必须合理
        if (payloadSize == 0 || payloadSize > 1024 * 1024) {  // 最大1MB
            utils::Logger::instance().warn(QString("无效的payload大小: %1").arg(payloadSize));
            return QByteArray();
        }

        int expectedSize = static_cast<int>(sizeof(BinaryProtocol2)) + static_cast<int>(payloadSize);
        if (data.size() < expectedSize) {
            utils::Logger::instance().warn(QString("二进制数据包长度不匹配（Version 2）: 期望%1, 实际%2")
                .arg(expectedSize).arg(data.size()));
            return QByteArray();
        }

        return data.mid(sizeof(BinaryProtocol2), static_cast<int>(payloadSize));

    } else if (m_config.version == 3) {
        // Version 3: 解析BinaryProtocol3
        if (data.size() < static_cast<int>(sizeof(BinaryProtocol3))) {
            utils::Logger::instance().warn("二进制数据包太小（Version 3）");
            return QByteArray();
        }

        const BinaryProtocol3* header = reinterpret_cast<const BinaryProtocol3*>(data.data());
        quint16 payloadSize = qFromBigEndian<quint16>(header->payload_size);
        
        // 安全检查：payloadSize必须合理
        if (payloadSize == 0 || payloadSize > 65535) {
            utils::Logger::instance().warn(QString("无效的payload大小: %1").arg(payloadSize));
            return QByteArray();
        }

        int expectedSize = static_cast<int>(sizeof(BinaryProtocol3)) + static_cast<int>(payloadSize);
        if (data.size() < expectedSize) {
            utils::Logger::instance().warn(QString("二进制数据包长度不匹配（Version 3）: 期望%1, 实际%2")
                .arg(expectedSize).arg(data.size()));
            return QByteArray();
        }

        return data.mid(sizeof(BinaryProtocol3), static_cast<int>(payloadSize));

    } else {
        // Version 1: 直接返回Opus数据
        return data;
    }
}

} // namespace network
} // namespace xiaozhi

