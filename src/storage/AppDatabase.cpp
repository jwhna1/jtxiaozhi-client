/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-12T08:30:00Z
File: AppDatabase.cpp
Desc: 统一数据库管理模块实现
*/

#include "AppDatabase.h"
#include "../models/ChatMessage.h"
#include "../utils/Config.h"
#include "../utils/Logger.h"
#include <QDir>
#include <QStandardPaths>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>

namespace xiaozhi {
namespace storage {

AppDatabase::AppDatabase(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
}

AppDatabase::~AppDatabase() {
    close();
}

bool AppDatabase::initialize(const QString& dbPath) {
    if (m_initialized) {
        return true;
    }

    m_dbPath = dbPath;

    // 确保目录存在
    QDir dir = QFileInfo(dbPath).absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            utils::Logger::instance().error("无法创建数据库目录: " + dir.absolutePath());
            emit errorOccurred("无法创建数据库目录");
            return false;
        }
    }

    // 创建数据库连接
    m_database = QSqlDatabase::addDatabase("QSQLITE", "xiaozhi_app_db");
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        utils::Logger::instance().error("无法打开数据库: " + m_database.lastError().text());
        emit errorOccurred("无法打开数据库: " + m_database.lastError().text());
        return false;
    }

    // 创建表
    if (!createTables()) {
        utils::Logger::instance().error("创建数据库表失败");
        emit errorOccurred("创建数据库表失败");
        return false;
    }

    m_initialized = true;
    // 数据库初始化成功
    return true;
}

void AppDatabase::close() {
    if (m_database.isOpen()) {
        m_database.close();
    }
    m_initialized = false;
}

bool AppDatabase::createTables() {
    QStringList createTableSqls = {
        // 聊天消息表
        R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT NOT NULL,
            message_type TEXT NOT NULL,
            text_content TEXT,
            audio_file_path TEXT,
            image_file_path TEXT,
            timestamp INTEGER NOT NULL,
            is_final BOOLEAN DEFAULT 1,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        )",
        
        // 设备配置表
        R"(
        CREATE TABLE IF NOT EXISTS device_configs (
            device_id TEXT PRIMARY KEY,
            device_name TEXT NOT NULL,
            mac_address TEXT NOT NULL,
            ota_url TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        )",
        
        // 应用设置表
        R"(
        CREATE TABLE IF NOT EXISTS app_settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            value_type TEXT DEFAULT 'string',
            category TEXT,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        )",
        
        // MQTT端口缓存表
        R"(
        CREATE TABLE IF NOT EXISTS mqtt_port_cache (
            port INTEGER PRIMARY KEY,
            is_ssl BOOLEAN NOT NULL,
            last_success_time DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        )",
        
        // 音频设备配置表
        R"(
        CREATE TABLE IF NOT EXISTS audio_device_config (
            device_type TEXT PRIMARY KEY,
            device_id TEXT NOT NULL,
            device_name TEXT NOT NULL,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        )"
    };

    // 创建索引
    QStringList createIndexSqls = {
        "CREATE INDEX IF NOT EXISTS idx_device_time ON messages(device_id, timestamp)",
        "CREATE INDEX IF NOT EXISTS idx_message_type ON messages(message_type)",
        "CREATE INDEX IF NOT EXISTS idx_settings_category ON app_settings(category)"
    };

    // 执行创建表语句
    for (const QString& sql : createTableSqls) {
        if (!executeQuery(sql)) {
            return false;
        }
    }

    // 执行创建索引语句
    for (const QString& sql : createIndexSqls) {
        if (!executeQuery(sql)) {
            return false;
        }
    }

    // 数据库迁移：添加 image_file_path 列（如果不存在）
    migrateDatabaseSchema();

    return true;
}

void AppDatabase::migrateDatabaseSchema() {
    if (!checkConnection()) {
        return;
    }

    // 检查 messages 表是否有 image_file_path 列
    QSqlQuery query(m_database);
    query.prepare("PRAGMA table_info(messages)");
    
    if (!query.exec()) {
        utils::Logger::instance().warn("无法检查表结构");
        return;
    }

    bool hasImageColumn = false;
    while (query.next()) {
        QString columnName = query.value("name").toString();
        if (columnName == "image_file_path") {
            hasImageColumn = true;
            break;
        }
    }

    // 如果没有 image_file_path 列，添加它
    if (!hasImageColumn) {
        QString alterSql = "ALTER TABLE messages ADD COLUMN image_file_path TEXT";
        QSqlQuery alterQuery(m_database);
        
        if (alterQuery.exec(alterSql)) {
            // 数据库迁移成功
        } else {
            utils::Logger::instance().error(QString("数据库迁移失败: %1").arg(alterQuery.lastError().text()));
        }
    }
}

bool AppDatabase::executeQuery(const QString& sql, const QVariantList& params) {
    if (!checkConnection()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(sql);

    for (const QVariant& param : params) {
        query.addBindValue(param);
    }

    if (!query.exec()) {
        logSqlError("执行查询", query.lastError());
        return false;
    }

    return true;
}

bool AppDatabase::checkConnection() {
    if (!m_database.isOpen()) {
        utils::Logger::instance().error("数据库连接已关闭");
        emit errorOccurred("数据库连接已关闭");
        return false;
    }
    return true;
}

void AppDatabase::logSqlError(const QString& operation, const QSqlError& error) {
    QString errorMsg = QString("%1失败: %2").arg(operation, error.text());
    utils::Logger::instance().error(errorMsg);
    emit errorOccurred(errorMsg);
}

// ========== 消息操作实现 ==========

qint64 AppDatabase::insertMessage(const QString& deviceId, const QString& type, 
                                 const QString& text, const QString& audioPath, 
                                 const QString& imagePath,
                                 qint64 timestamp, bool isFinal) {
    if (!checkConnection()) {
        return -1;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO messages (device_id, message_type, text_content, audio_file_path, image_file_path, timestamp, is_final)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(deviceId);
    query.addBindValue(type);
    query.addBindValue(text);
    query.addBindValue(audioPath);
    query.addBindValue(imagePath);
    query.addBindValue(timestamp);
    query.addBindValue(isFinal);

    if (!query.exec()) {
        logSqlError("插入消息", query.lastError());
        return -1;
    }

    qint64 insertId = query.lastInsertId().toLongLong();
    return insertId;
}

QList<xiaozhi::models::ChatMessage> AppDatabase::getMessages(const QString& deviceId, int limit) {
    QList<xiaozhi::models::ChatMessage> messages;
    
    if (!checkConnection()) {
        return messages;
    }

    QSqlQuery query(m_database);
    // 子查询：先取最近的N条（降序），再按时间升序排列（旧→新）
    query.prepare(R"(
        SELECT * FROM (
            SELECT id, device_id, message_type, text_content, audio_file_path, image_file_path,
                   timestamp, is_final, created_at
            FROM messages 
            WHERE device_id = ? 
            ORDER BY timestamp DESC 
            LIMIT ?
        ) AS recent_messages
        ORDER BY timestamp ASC
    )");
    
    query.addBindValue(deviceId);
    query.addBindValue(limit);

    if (!query.exec()) {
        logSqlError("查询消息", query.lastError());
        return messages;
    }

    while (query.next()) {
        xiaozhi::models::ChatMessage msg;
        msg.id = query.value("id").toLongLong();
        msg.deviceId = query.value("device_id").toString();
        msg.messageType = query.value("message_type").toString();
        msg.textContent = query.value("text_content").toString();
        msg.audioFilePath = query.value("audio_file_path").toString();
        msg.imagePath = query.value("image_file_path").toString();
        msg.timestamp = query.value("timestamp").toLongLong();
        msg.isFinal = query.value("is_final").toBool();
        msg.createdAt = query.value("created_at").toDateTime();
        
        messages.append(msg);
    }

    return messages;
}

bool AppDatabase::updateMessageAudioPath(qint64 messageId, const QString& audioPath) {
    if (!checkConnection()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("UPDATE messages SET audio_file_path = ? WHERE id = ?");
    query.addBindValue(audioPath);
    query.addBindValue(messageId);

    if (!query.exec()) {
        logSqlError("更新消息音频路径", query.lastError());
        return false;
    }
    
    return true;
}

bool AppDatabase::clearMessages(const QString& deviceId) {
    if (!checkConnection()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM messages WHERE device_id = ?");
    query.addBindValue(deviceId);

    if (!query.exec()) {
        logSqlError("清空消息", query.lastError());
        return false;
    }

    return true;
}

int AppDatabase::getMessageCount(const QString& deviceId) {
    if (!checkConnection()) {
        return 0;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM messages WHERE device_id = ?");
    query.addBindValue(deviceId);

    if (!query.exec() || !query.next()) {
        logSqlError("查询消息数量", query.lastError());
        return 0;
    }

    return query.value(0).toInt();
}

qint64 AppDatabase::getLastMessageTime(const QString& deviceId) {
    if (!checkConnection()) {
        return 0;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT MAX(timestamp) FROM messages WHERE device_id = ?");
    query.addBindValue(deviceId);

    if (!query.exec() || !query.next()) {
        logSqlError("查询最后消息时间", query.lastError());
        return 0;
    }

    return query.value(0).toLongLong();
}

// ========== 设备配置操作实现 ==========

bool AppDatabase::saveDeviceConfig(const xiaozhi::utils::DeviceConfig& config) {
    if (!checkConnection()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO device_configs (device_id, device_name, mac_address, ota_url, updated_at)
        VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP)
    )");
    
    query.addBindValue(config.deviceId);
    query.addBindValue(config.deviceName);
    query.addBindValue(config.macAddress);
    query.addBindValue(config.otaUrl);

    if (!query.exec()) {
        logSqlError("保存设备配置", query.lastError());
        return false;
    }

    return true;
}

xiaozhi::utils::DeviceConfig AppDatabase::loadDeviceConfig(const QString& deviceId) {
    xiaozhi::utils::DeviceConfig config;
    
    if (!checkConnection()) {
        return config;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT device_name, mac_address, ota_url FROM device_configs WHERE device_id = ?");
    query.addBindValue(deviceId);

    if (!query.exec()) {
        logSqlError("加载设备配置", query.lastError());
        return config;
    }

    if (query.next()) {
        config.deviceId = deviceId;
        config.deviceName = query.value("device_name").toString();
        config.macAddress = query.value("mac_address").toString();
        config.otaUrl = query.value("ota_url").toString();
    }

    return config;
}

QStringList AppDatabase::getAllDeviceIds() {
    QStringList deviceIds;
    
    if (!checkConnection()) {
        return deviceIds;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT device_id FROM device_configs ORDER BY created_at");

    if (!query.exec()) {
        logSqlError("查询设备ID列表", query.lastError());
        return deviceIds;
    }

    while (query.next()) {
        deviceIds.append(query.value("device_id").toString());
    }

    return deviceIds;
}

bool AppDatabase::removeDeviceConfig(const QString& deviceId) {
    if (!checkConnection()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM device_configs WHERE device_id = ?");
    query.addBindValue(deviceId);

    if (!query.exec()) {
        logSqlError("删除设备配置", query.lastError());
        return false;
    }

    return true;
}

// ========== 应用设置操作实现 ==========

void AppDatabase::setSetting(const QString& key, const QVariant& value, const QString& category) {
    if (!checkConnection()) {
        return;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO app_settings (key, value, value_type, category, updated_at)
        VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP)
    )");
    
    query.addBindValue(key);
    query.addBindValue(value.toString());
    query.addBindValue(value.typeName());
    query.addBindValue(category);

    if (!query.exec()) {
        logSqlError("设置应用配置", query.lastError());
    }
}

QVariant AppDatabase::getSetting(const QString& key, const QVariant& defaultValue) {
    if (!checkConnection()) {
        return defaultValue;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT value, value_type FROM app_settings WHERE key = ?");
    query.addBindValue(key);

    if (!query.exec() || !query.next()) {
        return defaultValue;
    }

    QString valueStr = query.value("value").toString();
    QString typeStr = query.value("value_type").toString();

    if (typeStr == "int") {
        return valueStr.toInt();
    } else if (typeStr == "bool") {
        return valueStr == "true";
    } else {
        return valueStr;
    }
}

bool AppDatabase::isDarkTheme() {
    return getSetting("darkTheme", false).toBool();
}

void AppDatabase::setDarkTheme(bool dark) {
    setSetting("darkTheme", dark, "general");
}

// ========== MQTT端口缓存实现 ==========

void AppDatabase::setMqttPortSuccess(int port, bool isSsl) {
    if (!checkConnection()) {
        return;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO mqtt_port_cache (port, is_ssl, last_success_time)
        VALUES (?, ?, CURRENT_TIMESTAMP)
    )");
    
    query.addBindValue(port);
    query.addBindValue(isSsl);

    if (!query.exec()) {
        logSqlError("设置MQTT端口状态", query.lastError());
    }
}

bool AppDatabase::getMqttPortStatus(int port) {
    if (!checkConnection()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT is_ssl FROM mqtt_port_cache WHERE port = ?");
    query.addBindValue(port);

    if (!query.exec() || !query.next()) {
        return false;
    }

    return query.value("is_ssl").toBool();
}

// ========== 音频设备配置实现 ==========

void AppDatabase::saveAudioDevice(const QString& type, const QString& deviceId, const QString& deviceName) {
    if (!checkConnection()) {
        return;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO audio_device_config (device_type, device_id, device_name, updated_at)
        VALUES (?, ?, ?, CURRENT_TIMESTAMP)
    )");
    
    query.addBindValue(type);
    query.addBindValue(deviceId);
    query.addBindValue(deviceName);

    if (!query.exec()) {
        logSqlError("保存音频设备配置", query.lastError());
    }
}

QString AppDatabase::getAudioDeviceId(const QString& type) {
    if (!checkConnection()) {
        return QString();
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT device_id FROM audio_device_config WHERE device_type = ?");
    query.addBindValue(type);

    if (!query.exec() || !query.next()) {
        return QString();
    }

    return query.value("device_id").toString();
}

QString AppDatabase::getAudioDeviceName(const QString& type) {
    if (!checkConnection()) {
        return QString();
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT device_name FROM audio_device_config WHERE device_type = ?");
    query.addBindValue(type);

    if (!query.exec() || !query.next()) {
        return QString();
    }

    return query.value("device_name").toString();
}

} // namespace storage
} // namespace xiaozhi
