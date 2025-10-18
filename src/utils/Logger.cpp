/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:30:00Z
File: Logger.cpp
Desc: 日志工具实现
*/

#include "Logger.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStringConverter>
#include <iostream>

namespace xiaozhi {
namespace utils {

Logger::Logger()
    : m_consoleOutput(true)
    , m_fileOutput(true)
    , m_debugMode(true)  // 默认开启调试模式
{
    // 默认日志文件路径
    QString logPath = "jtxiaozhi-client.log";
    setLogFilePath(logPath);
}

Logger::~Logger() {
    if (m_logStream && m_logFile && m_logFile->isOpen()) {
        m_logStream->flush();
        m_logFile->close();
    }
}

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::log(LogLevel level, const QString& message, const QString& deviceName) {
    QMutexLocker locker(&m_mutex);

    // 获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

    // 构建日志消息
    QString logMessage;
    if (deviceName.isEmpty()) {
        logMessage = QString("[%1] %2").arg(timestamp, message);
    } else {
        logMessage = QString("[%1] [%2] %3").arg(timestamp, deviceName, message);
    }

    // 输出到控制台
    if (m_consoleOutput) {
        std::cout << logMessage.toStdString() << std::endl;
    }

    // 输出到文件
    if (m_fileOutput && m_logStream) {
        *m_logStream << logMessage << "\n";
        m_logStream->flush();
    }
}

void Logger::debug(const QString& message, const QString& deviceName) {
    log(LogLevel::DEBUG, message, deviceName);
}

void Logger::info(const QString& message, const QString& deviceName) {
    log(LogLevel::INFO, message, deviceName);
}

void Logger::warn(const QString& message, const QString& deviceName) {
    log(LogLevel::WARN, message, deviceName);
}

void Logger::error(const QString& message, const QString& deviceName) {
    log(LogLevel::ERROR, message, deviceName);
}

void Logger::setLogFilePath(const QString& path) {
    QMutexLocker locker(&m_mutex);

    // 关闭旧的日志文件
    if (m_logStream && m_logFile && m_logFile->isOpen()) {
        m_logStream->flush();
        m_logFile->close();
    }

    // 创建新的日志文件
    m_logFile = std::make_unique<QFile>(path);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logStream = std::make_unique<QTextStream>(m_logFile.get());
        m_logStream->setEncoding(QStringConverter::Utf8);  // Qt 6使用setEncoding代替setCodec
    } else {
        qWarning() << "无法打开日志文件:" << path;
        m_logStream.reset();
    }
}

void Logger::setConsoleOutput(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_consoleOutput = enabled;
}

void Logger::setFileOutput(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_fileOutput = enabled;
}

void Logger::setDebugMode(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_debugMode = enabled;
}

bool Logger::isDebugMode() const {
    return m_debugMode;
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

} // namespace utils
} // namespace xiaozhi

