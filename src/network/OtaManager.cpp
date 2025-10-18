/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:30:00Z
File: OtaManager.cpp
Desc: OTA配置获取管理器实现（复刻esp32_simulator_gui.py第254-356行）
*/

#include "OtaManager.h"
#include "../utils/Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QEventLoop>
#include <QTimer>

namespace xiaozhi {
namespace network {

// ============================================================================
// OtaWorker实现
// ============================================================================

OtaWorker::OtaWorker(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

OtaWorker::~OtaWorker() {
}

void OtaWorker::requestOtaConfig(const DeviceInfo& deviceInfo, const QString& otaUrl) {
    // 构建JSON请求体
    QJsonObject jsonData = deviceInfo.toJson();
    QJsonDocument doc(jsonData);
    QByteArray postData = doc.toJson();

    // 构建HTTP请求
    QNetworkRequest request;
    request.setUrl(QUrl(otaUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Activation-Version", "1");
    request.setRawHeader("Device-Id", deviceInfo.mac_address.toUtf8());
    request.setRawHeader("Client-Id", deviceInfo.uuid.toUtf8());
    request.setRawHeader("User-Agent", "esp32s3/1.6.2");
    request.setRawHeader("Accept-Language", "zh-CN");

    // 发送POST请求（同步等待响应）
    QNetworkReply* reply = m_networkManager->post(request, postData);

    // 使用事件循环等待响应（模拟Python的同步请求）
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    // 设置30秒超时
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);
    
    loop.exec();

    // 处理响应
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        
        // 已移除敏感的OTA响应日志（服务器通讯细节）
        
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        
        if (responseDoc.isObject()) {
            QJsonObject responseObj = responseDoc.object();
            
            // 解析OTA配置
            OtaConfig config = OtaConfig::fromJson(responseObj);
            
            emit otaConfigReceived(config);
        } else {
            emit errorOccurred("OTA响应格式错误");
        }
    } else {
        emit errorOccurred(QString("OTA请求失败: %1").arg(reply->errorString()));
    }

    reply->deleteLater();
}

// ============================================================================
// OtaManager实现
// ============================================================================

OtaManager::OtaManager(QObject* parent)
    : QObject(parent)
    , m_workerThread(new QThread(this))
    , m_worker(new OtaWorker())
{
    // 将worker移到工作线程
    m_worker->moveToThread(m_workerThread);

    // 连接信号
    connect(this, &OtaManager::requestOtaConfigInternal,
            m_worker, &OtaWorker::requestOtaConfig);
    
    connect(m_worker, &OtaWorker::otaConfigReceived,
            this, &OtaManager::otaConfigReceived);
    
    connect(m_worker, &OtaWorker::errorOccurred,
            this, &OtaManager::errorOccurred);

    // 启动工作线程
    m_workerThread->start();
}

OtaManager::~OtaManager() {
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_worker;
}

DeviceInfo OtaManager::generateDeviceInfo(const QString& macAddress, const QString& uuid) {
    DeviceInfo info;
    info.mac_address = macAddress;
    info.uuid = uuid;
    info.application.compile_time = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "Z";
    
    return info;
}

void OtaManager::requestOtaConfig(const DeviceInfo& deviceInfo, const QString& otaUrl) {
    // 通过信号触发工作线程中的请求
    emit requestOtaConfigInternal(deviceInfo, otaUrl);
}

// ============================================================================
// DeviceInfo JSON转换实现
// ============================================================================

QJsonObject DeviceInfo::toJson() const {
    QJsonObject json;
    json["version"] = version;
    json["flash_size"] = flash_size;
    json["psram_size"] = psram_size;
    json["minimum_free_heap_size"] = minimum_free_heap_size;
    json["mac_address"] = mac_address;
    json["uuid"] = uuid;
    json["chip_model_name"] = chip_model_name;

    // chip_info
    QJsonObject chipInfoObj;
    chipInfoObj["model"] = chip_info.model;
    chipInfoObj["cores"] = chip_info.cores;
    chipInfoObj["revision"] = chip_info.revision;
    chipInfoObj["features"] = chip_info.features;
    json["chip_info"] = chipInfoObj;

    // application
    QJsonObject appObj;
    appObj["name"] = application.name;
    appObj["version"] = application.version;
    appObj["compile_time"] = application.compile_time;
    appObj["idf_version"] = application.idf_version;
    appObj["elf_sha256"] = application.elf_sha256;
    json["application"] = appObj;

    // partition_table
    QJsonObject partitionObj;
    QJsonObject appPartObj;
    appPartObj["label"] = partition_table.app.label;
    appPartObj["type"] = partition_table.app.type;
    appPartObj["subtype"] = partition_table.app.subtype;
    appPartObj["address"] = partition_table.app.address;
    appPartObj["size"] = partition_table.app.size;
    partitionObj["app"] = appPartObj;
    json["partition_table"] = partitionObj;

    // ota
    QJsonObject otaObj;
    otaObj["label"] = ota.label;
    json["ota"] = otaObj;

    // board
    QJsonObject boardObj;
    boardObj["name"] = board.name;
    boardObj["version"] = board.version;
    json["board"] = boardObj;

    return json;
}

// ============================================================================
// ActivationInfo fromJson实现
// ============================================================================

ActivationInfo ActivationInfo::fromJson(const QJsonObject& json) {
    ActivationInfo info;
    info.code = json["code"].toString();
    info.message = json["message"].toString();
    info.challenge = json["challenge"].toString();
    info.timeout_ms = json["timeout_ms"].toInt();
    return info;
}

// ============================================================================
// MqttConfig fromJson实现
// ============================================================================

MqttConfig MqttConfig::fromJson(const QJsonObject& json) {
    MqttConfig config;
    
    // 已移除MQTT配置JSON键详情日志（敏感信息）
    
    // 尝试多个可能的字段名（endpoint, server, host, broker）
    if (json.contains("endpoint")) {
        config.endpoint = json["endpoint"].toString();
    } else if (json.contains("server")) {
        // server字段可能单独，也可能有port
        QString server = json["server"].toString();
        if (json.contains("port")) {
            int port = json["port"].toInt();
            config.endpoint = QString("%1:%2").arg(server).arg(port);
        } else {
            config.endpoint = server;
        }
    } else if (json.contains("host")) {
        // host字段可能单独，也可能有port
        QString host = json["host"].toString();
        if (json.contains("port")) {
            int port = json["port"].toInt();
            config.endpoint = QString("%1:%2").arg(host).arg(port);
        } else {
            config.endpoint = host;
        }
    } else if (json.contains("broker")) {
        config.endpoint = json["broker"].toString();
    }
    
    config.client_id = json["client_id"].toString();
    config.username = json["username"].toString();
    config.password = json["password"].toString();
    config.publish_topic = json["publish_topic"].toString();
    config.subscribe_topic = json["subscribe_topic"].toString();
    return config;
}

// ============================================================================
// UdpConfig fromJson实现
// ============================================================================

UdpConfig UdpConfig::fromJson(const QJsonObject& json) {
    UdpConfig config;
    config.server = json["server"].toString();
    config.port = json["port"].toInt(8080);
    config.key = json["key"].toString();
    config.nonce = json["nonce"].toString();
    return config;
}

// ============================================================================
// WebSocketConfig fromJson实现
// ============================================================================

WebSocketConfig WebSocketConfig::fromJson(const QJsonObject& json) {
    WebSocketConfig config;
    
    // 尝试多个可能的字段名（url, endpoint, server）
    if (json.contains("url")) {
        config.url = json["url"].toString();
    } else if (json.contains("endpoint")) {
        config.url = json["endpoint"].toString();
    } else if (json.contains("server")) {
        config.url = json["server"].toString();
    }
    
    config.token = json["token"].toString();
    config.version = json["version"].toInt(1);  // 默认版本1
    
    // 已移除WebSocket配置详情日志（敏感信息）
    
    return config;
}

// ============================================================================
// OtaConfig fromJson实现
// ============================================================================

OtaConfig OtaConfig::fromJson(const QJsonObject& json) {
    OtaConfig config;

    // 解析activation
    if (json.contains("activation")) {
        config.activation = ActivationInfo::fromJson(json["activation"].toObject());
    }

    // 解析mqtt配置（服务器可能同时提供MQTT和WebSocket配置）
    if (json.contains("mqtt")) {
        config.mqtt = MqttConfig::fromJson(json["mqtt"].toObject());
        config.hasMqtt = config.mqtt.isValid();
        
        if (config.hasMqtt) {
            config.protocol_type = "mqtt";
            config.transport_type = "udp";
            utils::Logger::instance().info("OTA响应包含有效的MQTT配置");
        }
    }

    // 解析websocket配置（可以与MQTT共存）
    if (json.contains("websocket")) {
        config.websocket = WebSocketConfig::fromJson(json["websocket"].toObject());
        config.hasWebSocket = config.websocket.isValid();
        
        if (config.hasWebSocket) {
            utils::Logger::instance().info("OTA响应包含有效的WebSocket配置");
            
            // 如果没有MQTT配置，则WebSocket作为主要协议
            if (!config.hasMqtt) {
                config.protocol_type = "websocket";
                config.transport_type = "websocket";
            }
        }
    }

    // 解析udp配置
    if (json.contains("udp")) {
        config.udp = UdpConfig::fromJson(json["udp"].toObject());
    }

    // 解析绑定说明
    if (json.contains("bind_instructions")) {
        config.bind_instructions = json["bind_instructions"].toObject();
    }

    return config;
}

// ============================================================================
// AudioParams toJson实现
// ============================================================================

QJsonObject AudioParams::toJson() const {
    QJsonObject json;
    json["format"] = format;
    json["sample_rate"] = sample_rate;
    json["channels"] = channels;
    json["frame_duration"] = frame_duration;
    return json;
}

} // namespace network
} // namespace xiaozhi

