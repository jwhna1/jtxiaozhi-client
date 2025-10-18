/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-12T08:30:00Z
File: ChatMessage.h
Desc: 聊天消息数据结构定义
*/

#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <QVariantMap>

namespace xiaozhi {
namespace models {

/**
 * @brief 聊天消息数据结构
 */
struct ChatMessage {
    qint64 id;                    // 数据库ID
    QString deviceId;             // 设备UUID
    QString messageType;          // "stt" | "tts" | "text" | "image" | "activation"
    QString textContent;          // 文字内容
    QString audioFilePath;        // 音频文件路径（相对）
    QString imagePath;            // 图片文件路径（绝对路径）
    qint64 timestamp;             // 消息时间戳
    bool isFinal;                 // STT是否最终结果
    QDateTime createdAt;          // 创建时间
    
    // 运行时状态（不存数据库）
    bool isPlaying = false;       // 音频是否正在播放
    
    /**
     * @brief 默认构造函数
     */
    ChatMessage() = default;
    
    /**
     * @brief 构造函数
     */
    ChatMessage(qint64 id, const QString& deviceId, const QString& messageType,
                const QString& textContent, const QString& audioFilePath,
                qint64 timestamp, bool isFinal = true)
        : id(id), deviceId(deviceId), messageType(messageType),
          textContent(textContent), audioFilePath(audioFilePath),
          timestamp(timestamp), isFinal(isFinal), createdAt(QDateTime::fromMSecsSinceEpoch(timestamp))
    {}
    
    /**
     * @brief 转换为QVariantMap（用于QML）
     */
    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["id"] = id;
        map["deviceId"] = deviceId;
        map["messageType"] = messageType;
        map["textContent"] = textContent;
        map["audioFilePath"] = audioFilePath;
        map["imagePath"] = imagePath;
        map["timestamp"] = timestamp;
        map["isFinal"] = isFinal;
        map["isPlaying"] = isPlaying;
        map["createdAt"] = createdAt.toString("yyyy-MM-dd hh:mm:ss");
        return map;
    }
    
    /**
     * @brief 从QVariantMap创建
     */
    static ChatMessage fromVariantMap(const QVariantMap& map) {
        ChatMessage msg;
        msg.id = map["id"].toLongLong();
        msg.deviceId = map["deviceId"].toString();
        msg.messageType = map["messageType"].toString();
        msg.textContent = map["textContent"].toString();
        msg.audioFilePath = map["audioFilePath"].toString();
        msg.imagePath = map["imagePath"].toString();
        msg.timestamp = map["timestamp"].toLongLong();
        msg.isFinal = map["isFinal"].toBool();
        msg.isPlaying = map["isPlaying"].toBool();
        msg.createdAt = QDateTime::fromString(map["createdAt"].toString(), "yyyy-MM-dd hh:mm:ss");
        return msg;
    }
};

} // namespace models
} // namespace xiaozhi

Q_DECLARE_METATYPE(xiaozhi::models::ChatMessage)

#endif // CHAT_MESSAGE_H
