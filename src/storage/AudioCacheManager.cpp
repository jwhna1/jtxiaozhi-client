/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-12T08:30:00Z
File: AudioCacheManager.cpp
Desc: 音频缓存管理模块实现
*/

#include "AudioCacheManager.h"
#include "../utils/Logger.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDataStream>

namespace xiaozhi {
namespace storage {

AudioCacheManager::AudioCacheManager(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
}

AudioCacheManager::~AudioCacheManager() {
}

bool AudioCacheManager::initialize(const QString& basePath) {
    if (m_initialized) {
        return true;
    }

    // 使用传入路径（AppModel 会传入程序目录/cache/audio）
    m_basePath = basePath;

    // 确保基础目录存在
    QDir baseDir(m_basePath);
    if (!baseDir.exists()) {
        if (!baseDir.mkpath(".")) {
            utils::Logger::instance().error("❌ 无法创建音频缓存目录: " + m_basePath);
            emit errorOccurred("无法创建音频缓存目录");
            return false;
        }
    }

    m_initialized = true;
    utils::Logger::instance().info("✅ 音频缓存管理器初始化成功: " + m_basePath);
    return true;
}

QString AudioCacheManager::saveAudioCache(const QString& deviceId, 
                                         const QByteArray& pcmData, 
                                         qint64 timestamp,
                                         int sampleRate, int channels) {
    if (!m_initialized) {
        utils::Logger::instance().error("音频缓存管理器未初始化");
        return QString();
    }

    if (pcmData.isEmpty()) {
        utils::Logger::instance().warn("PCM数据为空，跳过保存");
        return QString();
    }

    // 确保设备目录存在
    if (!ensureDeviceDirectory(deviceId)) {
        return QString();
    }

    // 生成文件名
    QString fileName = generateAudioFileName("tts", timestamp);
    QString deviceDir = QDir(m_basePath).filePath(deviceId);
    QString filePath = QDir(deviceDir).filePath(fileName);

    // 创建文件头
    QByteArray header = createPcmHeader(sampleRate, channels);
    QByteArray fullData = header + pcmData;

    // 写入文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QString error = "无法创建音频文件: " + file.errorString();
        utils::Logger::instance().error(error);
        emit errorOccurred(error);
        return QString();
    }

    qint64 written = file.write(fullData);
    file.close();

    if (written != fullData.size()) {
        QString error = "音频文件写入不完整";
        utils::Logger::instance().error(error);
        emit errorOccurred(error);
        return QString();
    }

    // 返回相对路径
    QString relativePath = deviceId + "/" + fileName;
    return relativePath;
}

QByteArray AudioCacheManager::loadAudioCache(const QString& audioPath) {
    if (!m_initialized) {
        utils::Logger::instance().error("音频缓存管理器未初始化");
        return QByteArray();
    }

    auto fullPath = resolveFullPath(audioPath);
    QFile file(fullPath);

    if (!file.open(QIODevice::ReadOnly)) {
        QString error = "无法打开音频文件: " + file.errorString();
        utils::Logger::instance().error(error);
        emit errorOccurred(error);
        return QByteArray();
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 8) {
        utils::Logger::instance().error("音频文件格式错误（文件太小）");
        return QByteArray();
    }

    // 解析文件头
    QByteArray header = data.left(8);
    QByteArray pcmData = data.mid(8);

    int sampleRate, channels;
    if (!parsePcmHeader(header, sampleRate, channels)) {
        utils::Logger::instance().error("音频文件头解析失败");
        return QByteArray();
    }

    utils::Logger::instance().debug(QString("加载音频缓存: %1 (%2字节, %3Hz, %4声道)")
        .arg(audioPath).arg(pcmData.size()).arg(sampleRate).arg(channels));

    return pcmData;
}

bool AudioCacheManager::clearDeviceCache(const QString& deviceId) {
    if (!m_initialized) {
        return false;
    }

    QString deviceDir = QDir(m_basePath).filePath(deviceId);
    QDir dir(deviceDir);

    if (!dir.exists()) {
        return true; // 目录不存在，认为清理成功
    }

    QStringList files = dir.entryList(QDir::Files);
    bool allSuccess = true;

    for (const QString& file : files) {
        if (!dir.remove(file)) {
            utils::Logger::instance().warn("无法删除音频文件: " + file);
            allSuccess = false;
        }
    }

    if (allSuccess && files.size() > 0) {
        if (!dir.rmdir(deviceDir)) {
            utils::Logger::instance().warn("无法删除设备目录: " + deviceDir);
            allSuccess = false;
        }
    }

    if (allSuccess) {
        utils::Logger::instance().info("✅ 清理设备音频缓存: " + deviceId);
    }

    return allSuccess;
}

QString AudioCacheManager::getAudioPath(const QString& deviceId, qint64 timestamp) {
    QString fileName = generateAudioFileName("tts", timestamp);
    return deviceId + "/" + fileName;
}

bool AudioCacheManager::getAudioInfo(const QString& audioPath, int& sampleRate, int& channels) {
    QString fullPath = QDir(m_basePath).filePath(audioPath);
    QFile file(fullPath);
    
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray header = file.read(8);
    file.close();

    return parsePcmHeader(header, sampleRate, channels);
}

bool AudioCacheManager::audioFileExists(const QString& audioPath) {
    return QFile::exists(resolveFullPath(audioPath));
}

QStringList AudioCacheManager::getDeviceAudioFiles(const QString& deviceId) {
    QStringList files;
    
    if (!m_initialized) {
        return files;
    }

    QString deviceDir = QDir(m_basePath).filePath(deviceId);
    QDir dir(deviceDir);

    if (!dir.exists()) {
        return files;
    }

    QStringList fileNames = dir.entryList(QDir::Files, QDir::Time);
    for (const QString& fileName : fileNames) {
        files.append(deviceId + "/" + fileName);
    }

    return files;
}
QString AudioCacheManager::resolveFullPath(const QString& audioPath) const {
    // 仅使用程序目录，不再兼容AppData
    return QDir(m_basePath).filePath(audioPath);
}

bool AudioCacheManager::ensureDeviceDirectory(const QString& deviceId) {
    QString deviceDir = QDir(m_basePath).filePath(deviceId);
    QDir dir(deviceDir);

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            QString error = "无法创建设备音频目录: " + deviceDir;
            utils::Logger::instance().error(error);
            emit errorOccurred(error);
            return false;
        }
    }

    return true;
}

QString AudioCacheManager::generateAudioFileName(const QString& type, qint64 timestamp) {
    return QString("%1_%2.pcm").arg(type).arg(timestamp);
}

QByteArray AudioCacheManager::createPcmHeader(int sampleRate, int channels) {
    QByteArray header(8, 0);
    QDataStream stream(&header, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<qint32>(sampleRate);
    stream << static_cast<qint32>(channels);
    return header;
}

bool AudioCacheManager::parsePcmHeader(const QByteArray& header, int& sampleRate, int& channels) {
    if (header.size() < 8) {
        return false;
    }

    QDataStream stream(header);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    qint32 rate, ch;
    stream >> rate;
    stream >> ch;

    sampleRate = static_cast<int>(rate);
    channels = static_cast<int>(ch);

    return sampleRate > 0 && channels > 0;
}

} // namespace storage
} // namespace xiaozhi

