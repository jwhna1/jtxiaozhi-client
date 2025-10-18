/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-12T12:00:00Z
File: ImageCacheManager.h
Desc: 图片缓存管理器（保存和加载图片）
*/

#ifndef IMAGE_CACHE_MANAGER_H
#define IMAGE_CACHE_MANAGER_H

#include <QObject>
#include <QString>
#include <QDir>

namespace xiaozhi {
namespace storage {

/**
 * @brief 图片缓存管理器
 * 负责保存和加载发送的图片到本地缓存目录
 */
class ImageCacheManager : public QObject {
    Q_OBJECT

public:
    explicit ImageCacheManager(QObject* parent = nullptr);
    ~ImageCacheManager();

    /**
     * @brief 初始化缓存目录
     * @param basePath 基础缓存路径
     * @return 成功返回true
     */
    bool initialize(const QString& basePath);

    /**
     * @brief 保存图片到缓存（复制原图片）
     * @param deviceId 设备ID
     * @param sourceImagePath 原始图片路径
     * @param timestamp 时间戳
     * @return 相对路径（deviceId/image_timestamp.ext），失败返回空字符串
     */
    QString saveImageCache(const QString& deviceId, 
                          const QString& sourceImagePath, 
                          qint64 timestamp);

    /**
     * @brief 将相对路径解析为绝对路径
     * @param imagePath 图片文件相对路径
     * @return 完整的绝对路径
     */
    QString resolveFullPath(const QString& imagePath) const;

    /**
     * @brief 检查图片文件是否存在
     * @param imagePath 图片文件相对路径
     * @return 存在返回true
     */
    bool imageFileExists(const QString& imagePath);

    /**
     * @brief 清理设备的所有图片缓存
     * @param deviceId 设备ID
     * @return 成功返回true
     */
    bool clearDeviceCache(const QString& deviceId);

    /**
     * @brief 获取设备图片文件列表
     * @param deviceId 设备ID
     * @return 文件路径列表
     */
    QStringList getDeviceImageFiles(const QString& deviceId);

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
     * @brief 生成图片文件名
     */
    QString generateImageFileName(qint64 timestamp, const QString& extension);

    QString m_basePath;
    bool m_initialized;
};

} // namespace storage
} // namespace xiaozhi

#endif // IMAGE_CACHE_MANAGER_H


