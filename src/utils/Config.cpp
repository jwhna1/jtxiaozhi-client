/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:30:00Z
File: Config.cpp
Desc: 配置管理实现
*/

#include "Config.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

namespace xiaozhi {
namespace utils {

Config::Config() {
    // 配置文件路径：程序所在目录下的xiaozhi_config.ini
    QString appDir = QCoreApplication::applicationDirPath();
    QString configPath = appDir + "/xiaozhi_config.ini";
    m_settings = std::make_unique<QSettings>(configPath, QSettings::IniFormat);
    
    qDebug() << "配置文件路径:" << configPath;
    // Qt 6默认使用UTF-8编码，不需要setIniCodec()
}

Config& Config::instance() {
    static Config instance;
    return instance;
}

QString Config::generateMacAddress() {
    // 生成符合ESP32标准的MAC地址（02:xx:xx:xx:xx:xx格式，小写）
    // 第一个字节固定为0x02（本地管理地址）
    QString mac = "02";
    
    // 生成剩余5个字节
    for (int i = 0; i < 5; ++i) {
        quint8 byte = QRandomGenerator::global()->bounded(256);
        mac += QString(":%1").arg(byte, 2, 16, QChar('0'));
    }
    
    return mac.toLower();
}

bool Config::validateMacAddress(const QString& mac) {
    // 验证格式：xx:xx:xx:xx:xx:xx（小写十六进制）
    QRegularExpression regex("^([0-9a-f]{2}:){5}[0-9a-f]{2}$");
    return regex.match(mac).hasMatch();
}

bool Config::validateOtaUrl(const QString& url) {
    // 验证URL格式：必须以http://或https://开头
    return url.startsWith("http://", Qt::CaseInsensitive) || 
           url.startsWith("https://", Qt::CaseInsensitive);
}

void Config::saveDeviceConfig(const DeviceConfig& config) {
    m_settings->beginGroup("Devices");
    m_settings->beginGroup(config.deviceId);
    
    m_settings->setValue("deviceName", config.deviceName);
    m_settings->setValue("macAddress", config.macAddress);
    m_settings->setValue("otaUrl", config.otaUrl);
    
    m_settings->endGroup();
    m_settings->endGroup();
    m_settings->sync();
}

DeviceConfig Config::loadDeviceConfig(const QString& deviceId) {
    DeviceConfig config;
    
    m_settings->beginGroup("Devices");
    m_settings->beginGroup(deviceId);
    
    if (m_settings->contains("deviceName")) {
        config.deviceId = deviceId;
        config.deviceName = m_settings->value("deviceName").toString();
        config.macAddress = m_settings->value("macAddress").toString();
        config.otaUrl = m_settings->value("otaUrl", DEFAULT_OTA_URL).toString();
    }
    
    m_settings->endGroup();
    m_settings->endGroup();
    
    return config;
}

void Config::removeDeviceConfig(const QString& deviceId) {
    m_settings->beginGroup("Devices");
    m_settings->remove(deviceId);
    m_settings->endGroup();
    m_settings->sync();
}

QStringList Config::getAllDeviceIds() {
    m_settings->beginGroup("Devices");
    QStringList deviceIds = m_settings->childGroups();
    m_settings->endGroup();
    
    return deviceIds;
}

void Config::clearAllDevices() {
    m_settings->beginGroup("Devices");
    m_settings->remove("");  // 删除整个组
    m_settings->endGroup();
    m_settings->sync();
}

QString Config::getDefaultOtaUrl() const {
    return m_settings->value("General/defaultOtaUrl", DEFAULT_OTA_URL).toString();
}

void Config::setDefaultOtaUrl(const QString& url) {
    m_settings->setValue("General/defaultOtaUrl", url);
    m_settings->sync();
}

bool Config::isDarkTheme() const {
    return m_settings->value("General/darkTheme", false).toBool();
}

void Config::setDarkTheme(bool dark) {
    m_settings->setValue("General/darkTheme", dark);
    m_settings->sync();
}

bool Config::getMqttPortProtocol(int port) const {
    return m_settings->value(QString("MqttPortCache/%1").arg(port), true).toBool();
}

void Config::setMqttPortProtocol(int port, bool useTLS) {
    m_settings->setValue(QString("MqttPortCache/%1").arg(port), useTLS);
    m_settings->sync();
}

bool Config::hasMqttPortProtocol(int port) const {
    return m_settings->contains(QString("MqttPortCache/%1").arg(port));
}

QString Config::getAudioInputDevice() const {
    return m_settings->value("Audio/inputDevice", "").toString();
}

void Config::setAudioInputDevice(const QString& deviceId, const QString& deviceName) {
    m_settings->setValue("Audio/inputDevice", deviceId);
    if (!deviceName.isEmpty()) {
        m_settings->setValue("Audio/inputDeviceName", deviceName);
    }
    m_settings->sync();
    qDebug() << "Config::setAudioInputDevice -" << deviceId;
    qDebug() << "设备名称:" << deviceName;
    qDebug() << "配置文件路径:" << m_settings->fileName();
}

QString Config::getAudioInputDeviceName() const {
    return m_settings->value("Audio/inputDeviceName", "").toString();
}

QString Config::getAudioOutputDevice() const {
    return m_settings->value("Audio/outputDevice", "").toString();
}

void Config::setAudioOutputDevice(const QString& deviceId, const QString& deviceName) {
    m_settings->setValue("Audio/outputDevice", deviceId);
    if (!deviceName.isEmpty()) {
        m_settings->setValue("Audio/outputDeviceName", deviceName);
    }
    m_settings->sync();
    qDebug() << "Config::setAudioOutputDevice -" << deviceId;
    qDebug() << "设备名称:" << deviceName;
    qDebug() << "配置文件路径:" << m_settings->fileName();
}

QString Config::getAudioOutputDeviceName() const {
    return m_settings->value("Audio/outputDeviceName", "").toString();
}

} // namespace utils
} // namespace xiaozhi

