/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-12T12:00:00Z
File: ImageCacheManager.cpp
Desc: 图片缓存管理器实现
*/

#include "ImageCacheManager.h"
#include "../utils/Logger.h"
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace xiaozhi {
namespace storage {

ImageCacheManager::ImageCacheManager(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
}

ImageCacheManager::~ImageCacheManager()
{
}

bool ImageCacheManager::initialize(const QString& basePath) {
    m_basePath = basePath;
    
    // 创建基础目录
    QDir dir;
    if (!dir.mkpath(m_basePath)) {
        QString error = QString("无法创建图片缓存目录: %1").arg(m_basePath);
        utils::Logger::instance().error(error);
        emit errorOccurred(error);
        return false;
    }
    
    m_initialized = true;
    utils::Logger::instance().info(QString("✅ 图片缓存管理器初始化成功: %1").arg(m_basePath));
    return true;
}

bool ImageCacheManager::ensureDeviceDirectory(const QString& deviceId) {
    if (!m_initialized) {
        return false;
    }
    
    QString deviceDir = QDir(m_basePath).filePath(deviceId);
    QDir dir;
    
    if (!dir.exists(deviceDir)) {
        if (!dir.mkpath(deviceDir)) {
            QString error = QString("无法创建设备图片目录: %1").arg(deviceDir);
            utils::Logger::instance().error(error);
            emit errorOccurred(error);
            return false;
        }
    }
    
    return true;
}

QString ImageCacheManager::generateImageFileName(qint64 timestamp, const QString& extension) {
    return QString("image_%1.%2").arg(timestamp).arg(extension);
}

QString ImageCacheManager::saveImageCache(const QString& deviceId, 
                                         const QString& sourceImagePath, 
                                         qint64 timestamp) {
    if (!m_initialized) {
        emit errorOccurred("图片缓存管理器未初始化");
        return QString();
    }
    
    if (!ensureDeviceDirectory(deviceId)) {
        return QString();
    }
    
    // 获取原始图片的扩展名
    QFileInfo sourceInfo(sourceImagePath);
    QString extension = sourceInfo.suffix().toLower();
    if (extension.isEmpty()) {
        extension = "jpg";  // 默认扩展名
    }
    
    // 生成目标文件名
    QString fileName = generateImageFileName(timestamp, extension);
    QString relativePath = QString("%1/%2").arg(deviceId, fileName);
    QString fullPath = QDir(m_basePath).filePath(relativePath);
    
    // 复制图片文件
    QFile sourceFile(sourceImagePath);
    if (!sourceFile.exists()) {
        QString error = QString("源图片文件不存在: %1").arg(sourceImagePath);
        utils::Logger::instance().error(error);
        emit errorOccurred(error);
        return QString();
    }
    
    // 如果目标文件已存在，先删除
    if (QFile::exists(fullPath)) {
        QFile::remove(fullPath);
    }
    
    // 复制文件
    if (!sourceFile.copy(fullPath)) {
        QString error = QString("复制图片失败: %1 -> %2").arg(sourceImagePath, fullPath);
        utils::Logger::instance().error(error);
        emit errorOccurred(error);
        return QString();
    }
    
    utils::Logger::instance().info(QString("保存图片缓存: %1 (%2 bytes)")
        .arg(relativePath).arg(sourceInfo.size()));
    
    return relativePath;
}

QString ImageCacheManager::resolveFullPath(const QString& imagePath) const {
    if (imagePath.isEmpty()) {
        return QString();
    }
    
    // 如果已经是绝对路径，直接返回
    QFileInfo info(imagePath);
    if (info.isAbsolute()) {
        return imagePath;
    }
    
    // 相对路径，解析为绝对路径
    QString fullPath = QDir(m_basePath).filePath(imagePath);
    
    // 检查文件是否存在
    if (QFile::exists(fullPath)) {
        return fullPath;
    }
    
    utils::Logger::instance().warn(QString("图片文件不存在: %1").arg(fullPath));
    return QString();
}

bool ImageCacheManager::imageFileExists(const QString& imagePath) {
    QString fullPath = resolveFullPath(imagePath);
    return !fullPath.isEmpty() && QFile::exists(fullPath);
}

bool ImageCacheManager::clearDeviceCache(const QString& deviceId) {
    if (!m_initialized) {
        return false;
    }
    
    QString deviceDir = QDir(m_basePath).filePath(deviceId);
    QDir dir(deviceDir);
    
    if (!dir.exists()) {
        return true;  // 目录不存在，认为清理成功
    }
    
    // 删除目录中的所有文件
    QStringList files = dir.entryList(QDir::Files);
    for (const QString& file : files) {
        QString filePath = dir.filePath(file);
        if (!QFile::remove(filePath)) {
            utils::Logger::instance().warn(QString("无法删除图片文件: %1").arg(filePath));
        }
    }
    
    utils::Logger::instance().info(QString("清理设备图片缓存: %1 (%2个文件)")
        .arg(deviceId).arg(files.size()));
    
    return true;
}

QStringList ImageCacheManager::getDeviceImageFiles(const QString& deviceId) {
    QStringList result;
    
    if (!m_initialized) {
        return result;
    }
    
    QString deviceDir = QDir(m_basePath).filePath(deviceId);
    QDir dir(deviceDir);
    
    if (!dir.exists()) {
        return result;
    }
    
    // 获取所有图片文件
    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.jpeg" << "*.png" << "*.gif" << "*.bmp";
    
    QStringList files = dir.entryList(nameFilters, QDir::Files);
    for (const QString& file : files) {
        result.append(QString("%1/%2").arg(deviceId, file));
    }
    
    return result;
}

} // namespace storage
} // namespace xiaozhi


