/*
Project: jtxiaozhi-client
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-18T11:20:00Z
File: OtaManager.h
Desc: OTA配置获取管理器（线程方式，复刻esp32_simulator_gui.py逻辑）
*/

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "NetworkTypes.h"
#include <QObject>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace xiaozhi {
namespace network {

/**
 * @brief OTA工作线程
 */
class OtaWorker : public QObject {
    Q_OBJECT

public:
    explicit OtaWorker(QObject* parent = nullptr);
    ~OtaWorker();

public slots:
    /**
     * @brief 请求OTA配置（在工作线程中执行）
     * @param deviceInfo 设备信息
     * @param otaUrl OTA服务器地址
     */
    void requestOtaConfig(const DeviceInfo& deviceInfo, const QString& otaUrl);

signals:
    /**
     * @brief OTA配置获取成功
     */
    void otaConfigReceived(const OtaConfig& config);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

private:
    QNetworkAccessManager* m_networkManager;
};

/**
 * @brief OTA配置管理器（复刻esp32_simulator_gui.py第254-356行）
 */
class OtaManager : public QObject {
    Q_OBJECT

public:
    explicit OtaManager(QObject* parent = nullptr);
    ~OtaManager();

    /**
     * @brief 生成设备信息
     * @param macAddress 设备MAC地址
     * @param uuid 设备UUID
     * @return 设备信息结构
     */
    static DeviceInfo generateDeviceInfo(const QString& macAddress, const QString& uuid);

    /**
     * @brief 请求OTA配置
     * @param deviceInfo 设备信息
     * @param otaUrl OTA服务器地址（默认使用官方服务器）
     */
    void requestOtaConfig(const DeviceInfo& deviceInfo, 
                         const QString& otaUrl = "https://api.tenclass.net/xiaozhi/ota/");

signals:
    /**
     * @brief OTA配置获取成功
     */
    void otaConfigReceived(const OtaConfig& config);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

    // 内部信号（用于线程通信）
    void requestOtaConfigInternal(const DeviceInfo& deviceInfo, const QString& otaUrl);

private:
    QThread* m_workerThread;
    OtaWorker* m_worker;
};

} // namespace network
} // namespace xiaozhi

#endif // OTA_MANAGER_H


