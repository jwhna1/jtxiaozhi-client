/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:30:00Z
File: Config.h
Desc: 配置管理（MAC地址生成、设备配置持久化）
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QSettings>
#include <QStringList>
#include <memory>

namespace xiaozhi {
namespace utils {

/**
 * @brief 设备配置结构
 */
struct DeviceConfig {
    QString deviceId;        // 设备唯一ID
    QString deviceName;      // 设备名称
    QString macAddress;      // MAC地址
    QString otaUrl;          // OTA服务器地址
    
    DeviceConfig() = default;
    DeviceConfig(const QString& id, const QString& name, const QString& mac, const QString& ota)
        : deviceId(id), deviceName(name), macAddress(mac), otaUrl(ota) {}
};

/**
 * @brief 配置管理类
 */
class Config {
public:
    /**
     * @brief 获取Config单例
     */
    static Config& instance();

    /**
     * @brief 生成符合ESP32标准的MAC地址（02:xx:xx:xx:xx:xx格式，小写）
     * @return 生成的MAC地址字符串
     */
    static QString generateMacAddress();

    /**
     * @brief 验证MAC地址格式
     * @param mac MAC地址字符串
     * @return 格式正确返回true，否则false
     */
    static bool validateMacAddress(const QString& mac);

    /**
     * @brief 验证OTA URL格式
     * @param url OTA服务器地址
     * @return 格式正确返回true（http://或https://开头），否则false
     */
    static bool validateOtaUrl(const QString& url);

    /**
     * @brief 保存设备配置
     * @param config 设备配置
     */
    void saveDeviceConfig(const DeviceConfig& config);

    /**
     * @brief 加载设备配置
     * @param deviceId 设备ID
     * @return 设备配置，如果不存在返回空配置
     */
    DeviceConfig loadDeviceConfig(const QString& deviceId);

    /**
     * @brief 删除设备配置
     * @param deviceId 设备ID
     */
    void removeDeviceConfig(const QString& deviceId);

    /**
     * @brief 获取所有已保存的设备ID列表
     * @return 设备ID列表
     */
    QStringList getAllDeviceIds();

    /**
     * @brief 清空所有设备配置
     */
    void clearAllDevices();

    /**
     * @brief 获取/设置默认OTA URL
     */
    QString getDefaultOtaUrl() const;
    void setDefaultOtaUrl(const QString& url);

    /**
     * @brief 获取/设置主题模式
     */
    bool isDarkTheme() const;
    void setDarkTheme(bool dark);

    /**
     * @brief MQTT端口协议缓存（自动协商后记录）
     */
    bool getMqttPortProtocol(int port) const;  // 返回true=TLS, false=TCP
    void setMqttPortProtocol(int port, bool useTLS);
    bool hasMqttPortProtocol(int port) const;

    /**
     * @brief 获取/设置音频输入设备ID
     */
    QString getAudioInputDevice() const;
    void setAudioInputDevice(const QString& deviceId, const QString& deviceName = "");

    /**
     * @brief 获取/设置音频输出设备ID
     */
    QString getAudioOutputDevice() const;
    void setAudioOutputDevice(const QString& deviceId, const QString& deviceName = "");
    
    /**
     * @brief 获取音频设备名称
     */
    QString getAudioInputDeviceName() const;
    QString getAudioOutputDeviceName() const;

    /**
     * @brief 删除拷贝构造和赋值
     */
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

private:
    Config();
    ~Config() = default;

    std::unique_ptr<QSettings> m_settings;
    static constexpr const char* DEFAULT_OTA_URL = "https://api.tenclass.net/xiaozhi/ota/";
};

} // namespace utils
} // namespace xiaozhi

#endif // CONFIG_H

