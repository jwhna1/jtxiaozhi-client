/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:30:00Z
File: Logger.h
Desc: 日志工具（线程安全、支持emoji风格）
*/

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <memory>

namespace xiaozhi {
namespace utils {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

/**
 * @brief 线程安全的日志工具类（单例模式）
 */
class Logger {
public:
    /**
     * @brief 获取Logger单例
     */
    static Logger& instance();

    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param message 日志消息
     * @param deviceName 设备名称（可选，用于多设备场景）
     */
    void log(LogLevel level, const QString& message, const QString& deviceName = QString());

    /**
     * @brief 便捷方法：调试日志
     */
    void debug(const QString& message, const QString& deviceName = QString());

    /**
     * @brief 便捷方法：信息日志
     */
    void info(const QString& message, const QString& deviceName = QString());

    /**
     * @brief 便捷方法：警告日志
     */
    void warn(const QString& message, const QString& deviceName = QString());

    /**
     * @brief 便捷方法：错误日志
     */
    void error(const QString& message, const QString& deviceName = QString());

    /**
     * @brief 设置日志文件路径
     */
    void setLogFilePath(const QString& path);

    /**
     * @brief 设置是否输出到控制台
     */
    void setConsoleOutput(bool enabled);

    /**
     * @brief 设置是否输出到文件
     */
    void setFileOutput(bool enabled);

    /**
     * @brief 设置调试模式（控制是否显示详细调试信息）
     */
    void setDebugMode(bool enabled);

    /**
     * @brief 获取调试模式状态
     */
    bool isDebugMode() const;

    /**
     * @brief 删除拷贝构造和赋值
     */
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    /**
     * @brief 将日志级别转换为字符串
     */
    QString levelToString(LogLevel level) const;

    QMutex m_mutex;                      // 互斥锁（线程安全）
    std::unique_ptr<QFile> m_logFile;    // 日志文件
    std::unique_ptr<QTextStream> m_logStream; // 文件输出流
    bool m_consoleOutput;                // 是否输出到控制台
    bool m_fileOutput;                   // 是否输出到文件
    bool m_debugMode;                    // 是否启用调试模式（显示详细信息）
};

} // namespace utils
} // namespace xiaozhi

// 便捷宏定义
#define LOG_DEBUG(msg, device) xiaozhi::utils::Logger::instance().debug(msg, device)
#define LOG_INFO(msg, device) xiaozhi::utils::Logger::instance().info(msg, device)
#define LOG_WARN(msg, device) xiaozhi::utils::Logger::instance().warn(msg, device)
#define LOG_ERROR(msg, device) xiaozhi::utils::Logger::instance().error(msg, device)

#endif // LOGGER_H

