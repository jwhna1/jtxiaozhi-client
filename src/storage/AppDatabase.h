/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-12T08:30:00Z
File: AppDatabase.h
Desc: 统一数据库管理模块（SQLite）- 管理聊天消息、设备配置、应用设置
*/

#ifndef APP_DATABASE_H
#define APP_DATABASE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

// 前向声明
namespace xiaozhi {
namespace models {
struct ChatMessage;
}
namespace utils {
struct DeviceConfig;
}
}

namespace xiaozhi {
namespace storage {

/**
 * @brief 统一数据库管理类
 * 
 * 管理所有应用数据的持久化：
 * - 聊天消息
 * - 设备配置
 * - 应用设置
 * - MQTT端口缓存
 * - 音频设备配置
 */
class AppDatabase : public QObject {
    Q_OBJECT

public:
    explicit AppDatabase(QObject* parent = nullptr);
    ~AppDatabase();

    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径
     * @return 成功返回true
     */
    bool initialize(const QString& dbPath);

    /**
     * @brief 关闭数据库连接
     */
    void close();

    // ========== 消息操作 ==========

    /**
     * @brief 插入聊天消息
     */
    qint64 insertMessage(const QString& deviceId, const QString& type, 
                        const QString& text, const QString& audioPath, 
                        const QString& imagePath,
                        qint64 timestamp, bool isFinal = true);

    /**
     * @brief 获取设备聊天消息
     */
    QList<xiaozhi::models::ChatMessage> getMessages(const QString& deviceId, int limit = 100);

    /**
     * @brief 更新消息的音频路径
     */
    bool updateMessageAudioPath(qint64 messageId, const QString& audioPath);

    /**
     * @brief 清空设备聊天消息
     */
    bool clearMessages(const QString& deviceId);

    /**
     * @brief 获取设备消息数量
     */
    int getMessageCount(const QString& deviceId);

    /**
     * @brief 获取设备最后消息时间
     */
    qint64 getLastMessageTime(const QString& deviceId);

    // ========== 设备配置操作（替代Config类的设备管理） ==========

    /**
     * @brief 保存设备配置
     */
    bool saveDeviceConfig(const xiaozhi::utils::DeviceConfig& config);

    /**
     * @brief 加载设备配置
     */
    xiaozhi::utils::DeviceConfig loadDeviceConfig(const QString& deviceId);

    /**
     * @brief 获取所有设备ID
     */
    QStringList getAllDeviceIds();

    /**
     * @brief 删除设备配置
     */
    bool removeDeviceConfig(const QString& deviceId);

    // ========== 应用设置操作（替代Config类的设置管理） ==========

    /**
     * @brief 设置应用配置项
     */
    void setSetting(const QString& key, const QVariant& value, const QString& category = "general");

    /**
     * @brief 获取应用配置项
     */
    QVariant getSetting(const QString& key, const QVariant& defaultValue = QVariant());

    /**
     * @brief 是否深色主题
     */
    bool isDarkTheme();

    /**
     * @brief 设置深色主题
     */
    void setDarkTheme(bool dark);

    // ========== MQTT端口缓存 ==========

    /**
     * @brief 设置MQTT端口成功状态
     */
    void setMqttPortSuccess(int port, bool isSsl);

    /**
     * @brief 获取MQTT端口状态
     */
    bool getMqttPortStatus(int port);

    // ========== 音频设备配置 ==========

    /**
     * @brief 保存音频设备配置
     */
    void saveAudioDevice(const QString& type, const QString& deviceId, const QString& deviceName);

    /**
     * @brief 获取音频设备ID
     */
    QString getAudioDeviceId(const QString& type);

    /**
     * @brief 获取音频设备名称
     */
    QString getAudioDeviceName(const QString& type);

signals:
    /**
     * @brief 数据库错误
     */
    void errorOccurred(const QString& error);

private:
    /**
     * @brief 创建数据库表
     */
    bool createTables();

    /**
     * @brief 数据库架构迁移（添加新列等）
     */
    void migrateDatabaseSchema();

    /**
     * @brief 执行SQL语句
     */
    bool executeQuery(const QString& sql, const QVariantList& params = QVariantList());

    /**
     * @brief 检查数据库连接
     */
    bool checkConnection();

    /**
     * @brief 记录SQL错误
     */
    void logSqlError(const QString& operation, const QSqlError& error);

    QSqlDatabase m_database;
    bool m_initialized;
    QString m_dbPath;
};

} // namespace storage
} // namespace xiaozhi

#endif // APP_DATABASE_H
