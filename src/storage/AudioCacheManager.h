/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-01-12T08:30:00Z
File: AudioCacheManager.h
Desc: 音频缓存管理模块（PCM文件存储与加载）
*/

#ifndef AUDIO_CACHE_MANAGER_H
#define AUDIO_CACHE_MANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDir>

namespace xiaozhi {
namespace storage {

/**
 * @brief 音频缓存管理器
 * 
 * 管理解码后的PCM音频文件存储：
 * - 按设备ID分目录存储
 * - 文件头包含音频参数（采样率、声道）
 * - 支持音频文件加载和播放
 */
class AudioCacheManager : public QObject {
    Q_OBJECT

public:
    explicit AudioCacheManager(QObject* parent = nullptr);
    ~AudioCacheManager();

    /**
     * @brief 初始化缓存目录
     * @param basePath 基础缓存路径
     * @return 成功返回true
     */
    bool initialize(const QString& basePath);

    /**
     * @brief 保存解码后的PCM音频
     * @param deviceId 设备ID
     * @param pcmData PCM音频数据
     * @param timestamp 时间戳
     * @param sampleRate 采样率
     * @param channels 声道数
     * @return 相对路径，失败返回空字符串
     */
    QString saveAudioCache(const QString& deviceId, 
                          const QByteArray& pcmData, 
                          qint64 timestamp,
                          int sampleRate, int channels);

    /**
     * @brief 加载音频文件用于播放
     * @param audioPath 音频文件相对路径
     * @return PCM数据，失败返回空数组
     */
    QByteArray loadAudioCache(const QString& audioPath);

    /**
     * @brief 清理设备的所有音频缓存
     * @param deviceId 设备ID
     * @return 成功返回true
     */
    bool clearDeviceCache(const QString& deviceId);

    /**
     * @brief 获取音频文件完整路径
     * @param deviceId 设备ID
     * @param timestamp 时间戳
     * @return 完整文件路径
     */
    QString getAudioPath(const QString& deviceId, qint64 timestamp);

    /**
     * @brief 获取音频文件信息
     * @param audioPath 音频文件相对路径
     * @param sampleRate 输出采样率
     * @param channels 输出声道数
     * @return 成功返回true
     */
    bool getAudioInfo(const QString& audioPath, int& sampleRate, int& channels);

    /**
     * @brief 检查音频文件是否存在
     * @param audioPath 音频文件相对路径
     * @return 存在返回true
     */
    bool audioFileExists(const QString& audioPath);

    /**
     * @brief 获取设备音频文件列表
     * @param deviceId 设备ID
     * @return 文件路径列表
     */
    QStringList getDeviceAudioFiles(const QString& deviceId);

    /**
     * @brief 将相对路径解析为绝对路径
     * @param audioPath 音频文件相对路径
     * @return 完整的绝对路径
     */
    QString resolveFullPath(const QString& audioPath) const;

signals:
    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

private:
    /**
     * @brief 确保设备目录存在
     */
    bool ensureDeviceDirectory(const QString& deviceId);

    /**
     * @brief 生成音频文件名
     */
    QString generateAudioFileName(const QString& type, qint64 timestamp);

    /**
     * @brief 创建PCM文件头
     */
    QByteArray createPcmHeader(int sampleRate, int channels);

    /**
     * @brief 解析PCM文件头
     */
    bool parsePcmHeader(const QByteArray& header, int& sampleRate, int& channels);

    QString m_basePath;
    bool m_initialized;
};

} // namespace storage
} // namespace xiaozhi

#endif // AUDIO_CACHE_MANAGER_H

