/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:31:00Z
File: MqttManager.cpp
Desc: MQTT连接管理器实现（复刻esp32_simulator_gui.py第358-803行）
*/

#include "MqttManager.h"
#include "../utils/Logger.h"
#include "../utils/Config.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <chrono>

namespace xiaozhi {
namespace network {

// ============================================================================
// MqttCallback实现
// ============================================================================

MqttCallback::MqttCallback(QObject* parent)
    : m_parent(parent)
{
}

void MqttCallback::connected(const std::string& cause) {
    // 通过父对象发送信号
    QMetaObject::invokeMethod(m_parent, "onMqttConnected", Qt::QueuedConnection);
}

void MqttCallback::connection_lost(const std::string& cause) {
    QMetaObject::invokeMethod(m_parent, "onMqttDisconnected",
                             Qt::QueuedConnection,
                             Q_ARG(int, 7)); // 默认错误码7
}

void MqttCallback::message_arrived(mqtt::const_message_ptr msg) {
    // 避免触发库内静态EMPTY_STR/EMPTY_BIN，改用引用型API
    const auto& topicRef = msg->get_topic_ref();
    QString topic;
    if (topicRef) {
        topic = QString::fromUtf8(topicRef.data(), static_cast<int>(topicRef.size()));
    }

    const auto& payloadRef = msg->get_payload_ref();
    QString payload;
    if (payloadRef) {
        payload = QString::fromUtf8(payloadRef.data(), static_cast<int>(payloadRef.size()));
    }

    // 已移除MQTT接收消息详情日志（敏感信息）

    QMetaObject::invokeMethod(m_parent, "onMqttMessage",
                             Qt::QueuedConnection,
                             Q_ARG(QString, topic),
                             Q_ARG(QString, payload));
}

// ============================================================================
// MqttWorker实现
// ============================================================================

MqttWorker::MqttWorker(QObject* parent)
    : QObject(parent)
    , m_connected(false)
    , m_reconnectTimer(new QTimer(this))
{
    // 设置重连定时器
    m_reconnectTimer->setInterval(5000); // 5秒后重连
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &MqttWorker::attemptReconnect);
}

MqttWorker::~MqttWorker() {
    if (m_client && m_connected) {
        try {
            m_client->disconnect()->wait();
        } catch (...) {
        }
    }
}

void MqttWorker::connectToMqtt(const MqttConfig& config) {
    m_config = config;

    // 解析服务器地址和端口
    QString host;
    int port = 0;  // 0表示未指定
    bool portSpecified = false;
    
    if (config.endpoint.contains(':')) {
        QStringList parts = config.endpoint.split(':');
        host = parts[0];
        port = parts[1].toInt();
        portSpecified = true;
    } else {
        host = config.endpoint;
        portSpecified = false;
    }

    // 情况1：OTA没有指定端口（需要尝试8883和1883）
    if (!portSpecified) {
        utils::Logger::instance().info(QString(" 服务器未指定端口: %1").arg(host));
        
        // 检查8883端口缓存
        if (utils::Config::instance().hasMqttPortProtocol(8883)) {
            port = 8883;
            bool useSSL = utils::Config::instance().getMqttPortProtocol(8883);
            utils::Logger::instance().info(QString(" 使用缓存: 端口8883 → %1").arg(useSSL ? "TLS" : "TCP"));
            if (tryConnect(host, port, useSSL)) {
                return;  // 成功
            }
        }
        
        // 检查1883端口缓存
        if (utils::Config::instance().hasMqttPortProtocol(1883)) {
            port = 1883;
            bool useSSL = utils::Config::instance().getMqttPortProtocol(1883);
            utils::Logger::instance().info(QString(" 使用缓存: 端口1883 → %1").arg(useSSL ? "TLS" : "TCP"));
            if (tryConnect(host, port, useSSL)) {
                return;  // 成功
            }
        }
        
        // 无缓存：先尝试8883（TLS）
        utils::Logger::instance().info(" 未指定端口：优先尝试8883(TLS)");
        port = 8883;
        if (tryConnect(host, port, true)) {
            utils::Config::instance().setMqttPortProtocol(port, true);
            utils::Logger::instance().info(" 端口8883(TLS)连接成功并已缓存");
            return;
        }
        
        // 8883失败，尝试1883（TCP）
        utils::Logger::instance().warn("8883(TLS)连接失败，尝试1883(TCP)");
        port = 1883;
        if (tryConnect(host, port, false)) {
            utils::Config::instance().setMqttPortProtocol(port, false);
            utils::Logger::instance().info(" 端口1883(TCP)连接成功并已缓存");
            return;
        }
        
        // 两个标准端口都失败
        emit errorOccurred(QString("MQTT连接失败: 无法连接到%1 (尝试了8883和1883端口)").arg(host));
        return;
    }

    // 情况2：OTA指定了端口（原有逻辑）
    bool useSSL;
    bool tryAlternate = false;
    
    if (utils::Config::instance().hasMqttPortProtocol(port)) {
        // 有缓存：直接使用缓存的协议
        useSSL = utils::Config::instance().getMqttPortProtocol(port);
        utils::Logger::instance().info(QString(" 使用缓存协议: 端口%1 → %2")
            .arg(port).arg(useSSL ? "TLS" : "TCP"));
    } else {
        // 无缓存：根据端口号智能判断优先协议
        if (port >= 8000 && port < 9000) {
            // 8xxx端口：优先TLS
            useSSL = true;
            utils::Logger::instance().info(QString(" 端口%1(8xxx)：优先尝试TLS").arg(port));
        } else if (port >= 1000 && port < 2000) {
            // 1xxx端口：优先TCP
            useSSL = false;
            utils::Logger::instance().info(QString(" 端口%1(1xxx)：优先尝试TCP").arg(port));
        } else {
            // 其他端口：默认尝试TLS
            useSSL = true;
            utils::Logger::instance().info(QString(" 端口%1：默认优先TLS").arg(port));
        }
        tryAlternate = true;  // 标记需要备选尝试
    }

    // 尝试连接
    bool success = tryConnect(host, port, useSSL);
    
    if (!success && tryAlternate) {
        // 首选协议失败，尝试备选协议
        utils::Logger::instance().warn(QString("%1连接失败，尝试切换到%2")
            .arg(useSSL ? "TLS" : "TCP")
            .arg(useSSL ? "TCP" : "TLS"));
        
        useSSL = !useSSL;
        success = tryConnect(host, port, useSSL);
    }
    
    if (success) {
        // 连接成功，保存协议到缓存
        utils::Config::instance().setMqttPortProtocol(port, useSSL);
        utils::Logger::instance().info(QString(" 端口%1协议已缓存: %2")
            .arg(port).arg(useSSL ? "TLS" : "TCP"));
    } else {
        emit errorOccurred(QString("MQTT连接失败: 无法连接到%1:%2").arg(host).arg(port));
    }
}

bool MqttWorker::tryConnect(const QString& host, int port, bool useSSL) {
    try {
        QString protocol = useSSL ? "ssl" : "tcp";
        QString serverUri = QString("%1://%2:%3").arg(protocol, host).arg(port);
        
        // 已移除敏感的MQTT连接详情日志

        // 创建MQTT客户端
        m_client = std::make_unique<mqtt::async_client>(
            serverUri.toStdString(),
            m_config.client_id.toStdString()
        );

        // 设置回调
        m_callback = std::make_unique<MqttCallback>(this);
        m_client->set_callback(*m_callback);

        // 配置连接选项
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(120); // 120秒心跳
        connOpts.set_clean_session(true);
        
        if (!m_config.username.isEmpty()) {
            connOpts.set_user_name(m_config.username.toStdString());
        }
        if (!m_config.password.isEmpty()) {
            connOpts.set_password(m_config.password.toStdString());
        }

        // 只在8883端口时启用TLS/SSL配置
        if (useSSL) {
            // SSL/TLS加密已启用
            
            mqtt::ssl_options sslopts;
            sslopts.set_verify(true);
            sslopts.set_enable_server_cert_auth(true);
            
            // 1. 优先从Qt资源系统读取（打包到程序中）
            QString certPath = ":/cacert.pem";
            if (QFile::exists(certPath)) {
                // 从资源中提取到临时文件（OpenSSL需要文件路径）
                QString tempCertPath = QDir::temp().filePath("xiaozhi_cacert.pem");
                // 如果临时文件已存在，先删除
                if (QFile::exists(tempCertPath)) {
                    QFile::remove(tempCertPath);
                }
                if (QFile::copy(certPath, tempCertPath)) {
                    QFile::setPermissions(tempCertPath, QFile::ReadOwner | QFile::WriteOwner);
                    sslopts.set_trust_store(tempCertPath.toStdString());
                    // 已加载CA证书
                } else {
                    // 尝试外部证书
                }
            }
            
            // 如果内置证书加载失败，尝试外部证书
            if (!QFile::exists(certPath) || !QFile::exists(QDir::temp().filePath("xiaozhi_cacert.pem"))) {
                // 2. 尝试程序目录
                certPath = QCoreApplication::applicationDirPath() + "/cacert.pem";
                if (QFile::exists(certPath)) {
                    sslopts.set_trust_store(certPath.toStdString());
                    // 已加载外部CA证书
                } else {
                    // 3. 尝试当前目录
                    certPath = "cacert.pem";
                    if (QFile::exists(certPath)) {
                        sslopts.set_trust_store(certPath.toStdString());
                        // 已加载CA证书
                    } else {
                        // 未找到CA证书
                    }
                }
            }
            
            connOpts.set_ssl(sslopts);
        } else {
            // TCP连接
        }

        // 同步连接（设置超时为5秒）
        m_client->connect(connOpts)->wait_for(std::chrono::seconds(5));
        
        m_connected = true;
        emit connected();
        
        return true;  // 连接成功

    } catch (const mqtt::exception& e) {
        // 连接失败
        return false;
    } catch (...) {
        // 连接失败
        return false;
    }
}

void MqttWorker::disconnect() {
    if (m_client && m_connected) {
        try {
            m_client->disconnect()->wait();
            m_connected = false;
        } catch (const mqtt::exception& e) {
            emit errorOccurred(QString("MQTT断开失败: %1").arg(e.what()));
        }
    }
}

void MqttWorker::sendHello(const QString& transportType) {
    QJsonObject helloMsg;
    helloMsg["type"] = "hello";
    helloMsg["version"] = 3;
    helloMsg["transport"] = transportType;

    // 音频参数
    AudioParams audioParams;
    helloMsg["audio_params"] = audioParams.toJson();

    publish(m_config.publish_topic, helloMsg, 0);
}

void MqttWorker::sendPong(const QString& clientId) {
    QJsonObject pongMsg;
    pongMsg["type"] = "pong";
    pongMsg["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    pongMsg["client_id"] = clientId;

    publish(m_config.publish_topic, pongMsg, 0);
}

void MqttWorker::sendTextMessage(const QString& text, const QString& clientId) {
    QJsonObject textMsg;
    textMsg["type"] = "text";
    textMsg["content"] = text;
    textMsg["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    textMsg["client_id"] = clientId;

    publish(m_config.publish_topic, textMsg, 0);
}

void MqttWorker::sendIotDescriptors(const QString& sessionId) {
    // IoT设备描述符（模拟ESP32扬声器）
    QJsonArray descriptors;
    QJsonObject speaker;
    speaker["name"] = "Speaker";
    speaker["description"] = "扬声器";

    QJsonObject properties;
    QJsonObject volumeProp;
    volumeProp["name"] = "volume";
    volumeProp["description"] = "当前音量值";
    volumeProp["type"] = "number";
    volumeProp["min"] = 0;
    volumeProp["max"] = 100;
    properties["volume"] = volumeProp;
    speaker["properties"] = properties;

    QJsonObject methods;
    QJsonObject setVolumeMethod;
    setVolumeMethod["name"] = "SetVolume";
    setVolumeMethod["description"] = "设置音量";
    
    QJsonObject parameters;
    QJsonObject volumeParam;
    volumeParam["name"] = "volume";
    volumeParam["description"] = "0到100之间的整数";
    volumeParam["type"] = "number";
    volumeParam["required"] = true;
    parameters["volume"] = volumeParam;
    setVolumeMethod["parameters"] = parameters;
    methods["SetVolume"] = setVolumeMethod;
    speaker["methods"] = methods;

    descriptors.append(speaker);

    QJsonObject iotMsg;
    iotMsg["session_id"] = sessionId;
    iotMsg["type"] = "iot";
    iotMsg["descriptors"] = descriptors;

    publish(m_config.publish_topic, iotMsg, 0);
}

void MqttWorker::sendIotStates(const QString& sessionId) {
    // IoT设备状态
    QJsonArray states;
    QJsonObject speakerState;
    speakerState["name"] = "Speaker";
    
    QJsonObject state;
    state["volume"] = 50; // 默认音量50
    speakerState["state"] = state;
    
    states.append(speakerState);

    QJsonObject iotMsg;
    iotMsg["session_id"] = sessionId;
    iotMsg["type"] = "iot";
    iotMsg["update"] = true;
    iotMsg["states"] = states;

    publish(m_config.publish_topic, iotMsg, 0);
}

void MqttWorker::sendStartListening(const QString& sessionId, const QString& mode) {
    QJsonObject listenMsg;
    listenMsg["session_id"] = sessionId;
    listenMsg["type"] = "listen";
    listenMsg["state"] = "start";
    listenMsg["mode"] = mode;  // "manual" or "auto"

    publish(m_config.publish_topic, listenMsg, 0);
}

void MqttWorker::sendStopListening(const QString& sessionId) {
    QJsonObject listenMsg;
    listenMsg["session_id"] = sessionId;
    listenMsg["type"] = "listen";
    listenMsg["state"] = "stop";

    publish(m_config.publish_topic, listenMsg, 0);
}

void MqttWorker::sendAbort(const QString& sessionId, const QString& reason) {
    QJsonObject abortMsg;
    abortMsg["session_id"] = sessionId;
    abortMsg["type"] = "abort";
    
    if (!reason.isEmpty()) {
        abortMsg["reason"] = reason;
    }

    publish(m_config.publish_topic, abortMsg, 0);
}

void MqttWorker::sendGoodbye(const QString& sessionId) {
    QJsonObject goodbyeMsg;
    goodbyeMsg["session_id"] = sessionId;
    goodbyeMsg["type"] = "goodbye";

    publish(m_config.publish_topic, goodbyeMsg, 0);
}

void MqttWorker::sendMcpMessage(const QString& sessionId, const QJsonObject& payload) {
    QJsonObject mcpMsg;
    mcpMsg["session_id"] = sessionId;
    mcpMsg["type"] = "mcp";
    mcpMsg["payload"] = payload;

    publish(m_config.publish_topic, mcpMsg, 0);
}

void MqttWorker::sendRawMessage(const QString& topic, const QJsonObject& message) {
    publish(topic, message, 0);
}

void MqttWorker::onMqttConnected() {
    emit connected();
}

void MqttWorker::onMqttDisconnected(int rc) {
    m_connected = false;
    emit disconnected(rc);
    
    // 启动自动重连
    if (rc != 0) {
        m_reconnectTimer->start();
    }
}

void MqttWorker::onMqttMessage(const QString& topic, const QString& payload) {
    try {
        // 解析JSON消息
        QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
        if (!doc.isObject()) {
            return;
        }

        QJsonObject message = doc.object();
        QString msgType = message["type"].toString();

        // 处理hello响应
        if (msgType == "hello") {
            // 提取会话ID
            QString sessionId = message["session_id"].toString();
            
            // 提取UDP配置
            if (message.contains("udp")) {
                QJsonObject udpObj = message["udp"].toObject();
                UdpConfig udpConfig = UdpConfig::fromJson(udpObj);
                
                //  提取服务器的音频参数（用于解码器和播放设备）
                if (message.contains("audio_params")) {
                    QJsonObject audioParams = message["audio_params"].toObject();
                    udpConfig.serverSampleRate = audioParams["sample_rate"].toInt(24000);
                    udpConfig.serverChannels = audioParams["channels"].toInt(1);
                    udpConfig.serverFrameDuration = audioParams["frame_duration"].toInt(60);
                    
                    // 已移除服务器音频参数日志（敏感信息）
                }
                
                emit udpConfigReceived(udpConfig, sessionId);
            }
        }
        // 处理ping消息
        else if (msgType == "ping") {
            sendPong(m_config.client_id);
        }
        // 处理其他ESP32支持的消息
        else {
            handleEsp32Message(message);
        }

        // 发送消息给上层
        emit messageReceived(message);

    } catch (const std::exception& e) {
        // 忽略异常，避免连接断开
    }
}

void MqttWorker::attemptReconnect() {
    if (!m_connected && !m_config.endpoint.isEmpty()) {
        connectToMqtt(m_config);
    }
}

bool MqttWorker::publish(const QString& topic, const QJsonObject& message, int qos) {
    if (!m_client || !m_connected) {
        return false;
    }

    try {
        QJsonDocument doc(message);
        QString payloadStr = doc.toJson(QJsonDocument::Compact);
        
        // 已移除MQTT发送消息详情日志（敏感信息）
        
        auto msg = mqtt::make_message(topic.toStdString(), payloadStr.toStdString());
        msg->set_qos(qos);
        
        m_client->publish(msg)->wait();
        return true;

    } catch (const mqtt::exception& e) {
        utils::Logger::instance().error(QString("MQTT发送失败: %1").arg(e.what()));
        return false;
    }
}

void MqttWorker::handleEsp32Message(const QJsonObject& message) {
    QString msgType = message["type"].toString();
    
    // ESP32只处理以下消息类型：tts, stt, llm, iot, system, alert
    // mcp消息完全忽略
    if (msgType == "mcp") {
        // 不做任何处理，完全模拟ESP32行为
        return;
    }
    
    // 其他消息类型由上层处理
}

// ============================================================================
// MqttManager实现
// ============================================================================

MqttManager::MqttManager(QObject* parent)
    : QObject(parent)
    , m_workerThread(new QThread(this))
    , m_worker(new MqttWorker())
{
    // 将worker移到工作线程
    m_worker->moveToThread(m_workerThread);

    // 连接信号
    connect(this, &MqttManager::connectToMqttInternal,
            m_worker, &MqttWorker::connectToMqtt);
    connect(this, &MqttManager::disconnectInternal,
            m_worker, &MqttWorker::disconnect);
    connect(this, &MqttManager::sendHelloInternal,
            m_worker, &MqttWorker::sendHello);
    connect(this, &MqttManager::sendPongInternal,
            m_worker, &MqttWorker::sendPong);
    connect(this, &MqttManager::sendTextMessageInternal,
            m_worker, &MqttWorker::sendTextMessage);
    connect(this, &MqttManager::sendIotDescriptorsInternal,
            m_worker, &MqttWorker::sendIotDescriptors);
    connect(this, &MqttManager::sendIotStatesInternal,
            m_worker, &MqttWorker::sendIotStates);
    connect(this, &MqttManager::sendStartListeningInternal,
            m_worker, &MqttWorker::sendStartListening);
    connect(this, &MqttManager::sendStopListeningInternal,
            m_worker, &MqttWorker::sendStopListening);
    connect(this, &MqttManager::sendAbortInternal,
            m_worker, &MqttWorker::sendAbort);
    connect(this, &MqttManager::sendGoodbyeInternal,
            m_worker, &MqttWorker::sendGoodbye);
    connect(this, &MqttManager::sendMcpMessageInternal,
            m_worker, &MqttWorker::sendMcpMessage);
    connect(this, &MqttManager::sendRawMessageInternal,
            m_worker, &MqttWorker::sendRawMessage);

    connect(m_worker, &MqttWorker::connected,
            this, &MqttManager::connected);
    connect(m_worker, &MqttWorker::disconnected,
            this, &MqttManager::disconnected);
    connect(m_worker, &MqttWorker::messageReceived,
            this, &MqttManager::messageReceived);
    connect(m_worker, &MqttWorker::udpConfigReceived,
            this, &MqttManager::udpConfigReceived);
    connect(m_worker, &MqttWorker::errorOccurred,
            this, &MqttManager::errorOccurred);

    // 启动工作线程
    m_workerThread->start();
}

MqttManager::~MqttManager() {
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_worker;
}

void MqttManager::connectToMqtt(const MqttConfig& config) {
    emit connectToMqttInternal(config);
}

void MqttManager::disconnect() {
    emit disconnectInternal();
}

void MqttManager::sendHello(const QString& transportType) {
    emit sendHelloInternal(transportType);
}

void MqttManager::sendPong(const QString& clientId) {
    emit sendPongInternal(clientId);
}

void MqttManager::sendTextMessage(const QString& text, const QString& clientId) {
    emit sendTextMessageInternal(text, clientId);
}

void MqttManager::sendIotDescriptors(const QString& sessionId) {
    emit sendIotDescriptorsInternal(sessionId);
}

void MqttManager::sendIotStates(const QString& sessionId) {
    emit sendIotStatesInternal(sessionId);
}

void MqttManager::sendStartListening(const QString& sessionId, const QString& mode) {
    emit sendStartListeningInternal(sessionId, mode);
}

void MqttManager::sendStopListening(const QString& sessionId) {
    emit sendStopListeningInternal(sessionId);
}

void MqttManager::sendAbort(const QString& sessionId, const QString& reason) {
    emit sendAbortInternal(sessionId, reason);
}

void MqttManager::sendGoodbye(const QString& sessionId) {
    emit sendGoodbyeInternal(sessionId);
}

void MqttManager::sendMcpMessage(const QString& sessionId, const QJsonObject& payload) {
    emit sendMcpMessageInternal(sessionId, payload);
}

void MqttManager::sendRawMessage(const QString& topic, const QJsonObject& message) {
    emit sendRawMessageInternal(topic, message);
}

} // namespace network
} // namespace xiaozhi

