/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:30:00Z
File: NetworkTypes.h
Desc: 网络相关数据结构定义
*/

#ifndef NETWORK_TYPES_H
#define NETWORK_TYPES_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace xiaozhi {
namespace network {

/**
 * @brief 设备信息结构（用于OTA请求）
 */
struct DeviceInfo {
    int version = 2;
    int flash_size = 4194304;
    int psram_size = 0;
    int minimum_free_heap_size = 123456;
    QString mac_address;
    QString uuid;
    QString chip_model_name = "c++client";
    
    struct ChipInfo {
        int model = 1;
        int cores = 2;
        int revision = 0;
        int features = 0;
    } chip_info;
    
    struct Application {
        QString name = "jtxiaozhi-client";
        QString version = "0.1.0";
        QString compile_time;
        QString idf_version = "5.1.0";
        QString elf_sha256 = "simulator_sha256";
    } application;
    
    struct PartitionTable {
        struct App {
            QString label = "app";
            int type = 1;
            int subtype = 2;
            int address = 0x10000;
            int size = 0x100000;
        } app;
    } partition_table;
    
    struct Ota {
        QString label = "ota_0";
    } ota;
    
    struct Board {
        QString name = "jtxiaozhi-client";
        QString version = "1.0";
    } board;
    
    /**
     * @brief 转换为JSON对象
     */
    QJsonObject toJson() const;
};

/**
 * @brief 激活信息结构
 */
struct ActivationInfo {
    QString code;           // 激活码
    QString message;        // 激活消息
    QString challenge;      // 挑战码
    int timeout_ms = 0;     // 超时时间（毫秒）
    
    ActivationInfo() = default;
    
    static ActivationInfo fromJson(const QJsonObject& json);
};

/**
 * @brief MQTT配置结构
 */
struct MqttConfig {
    QString endpoint;       // MQTT服务器地址（host:port）
    QString client_id;      // 客户端ID
    QString username;       // 用户名
    QString password;       // 密码
    QString publish_topic;  // 发布主题
    QString subscribe_topic; // 订阅主题
    
    MqttConfig() = default;
    
    static MqttConfig fromJson(const QJsonObject& json);
    
    bool isValid() const {
        return !endpoint.isEmpty() && !client_id.isEmpty();
    }
};

/**
 * @brief UDP配置结构
 */
struct UdpConfig {
    QString server;         // UDP服务器地址
    int port = 8080;        // UDP端口
    QString key;            // 加密密钥（可选）
    QString nonce;          // 随机数（可选）
    
    // 音频参数（从服务器Hello响应获取）
    int serverSampleRate = 24000;   // 服务器发送音频的采样率（解码器需要）
    int serverChannels = 1;         // 服务器音频声道数
    int serverFrameDuration = 60;   // 服务器音频帧时长（ms）
    
    UdpConfig() = default;
    
    static UdpConfig fromJson(const QJsonObject& json);
    
    bool isValid() const {
        return !server.isEmpty() && port > 0;
    }
};

/**
 * @brief WebSocket配置结构（备用协议）
 */
struct WebSocketConfig {
    QString url;            // WebSocket服务器地址（ws:// 或 wss://）
    QString token;          // 认证令牌（Bearer token）
    int version = 1;        // 协议版本（1/2/3）
    
    // 音频参数（从服务器Hello响应获取）
    int serverSampleRate = 24000;   // 服务器发送音频的采样率
    int serverChannels = 1;         // 服务器音频声道数
    int serverFrameDuration = 60;   // 服务器音频帧时长（ms）
    
    WebSocketConfig() = default;
    
    static WebSocketConfig fromJson(const QJsonObject& json);
    
    bool isValid() const {
        return !url.isEmpty();
    }
};

/**
 * @brief OTA配置响应结构（包含所有可能的配置）
 */
struct OtaConfig {
    ActivationInfo activation;      // 激活信息
    MqttConfig mqtt;                // MQTT配置
    UdpConfig udp;                  // UDP配置
    WebSocketConfig websocket;      // WebSocket配置（可选）
    bool hasMqtt = false;           // 是否包含MQTT配置
    bool hasWebSocket = false;      // 是否包含WebSocket配置
    QString protocol_type = "mqtt"; // 协议类型：mqtt 或 websocket
    QString transport_type = "udp"; // 传输类型：udp 或 websocket
    QJsonObject bind_instructions;  // 绑定说明（包含web_url, step1, step2, step3等）
    
    OtaConfig() = default;
    
    static OtaConfig fromJson(const QJsonObject& json);
};

/**
 * @brief 音频参数结构（用于hello消息）
 */
struct AudioParams {
    QString format = "opus";        // 音频格式
    int sample_rate = 16000;        // 采样率
    int channels = 1;               // 声道数
    int frame_duration = 60;        // 帧时长（毫秒）
    
    QJsonObject toJson() const;
};

} // namespace network
} // namespace xiaozhi

#endif // NETWORK_TYPES_H

