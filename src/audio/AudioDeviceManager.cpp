#include "AudioDeviceManager.h"
#include "../utils/Logger.h"
#include "../utils/Config.h"
#include <QMediaDevices>

namespace xiaozhi {
namespace audio {

AudioDeviceManager::AudioDeviceManager(QObject* parent)
    : QObject(parent)
{
    // 初始化设备列表
    refreshDevices();
    
    // 从配置加载
    loadFromConfig();
    
    // TODO: Qt 6.9可能需要使用定时器定期刷新设备列表
    // 因为QMediaDevices在某些Qt版本中没有提供设备变化信号
}

AudioDeviceManager::~AudioDeviceManager() {
}

void AudioDeviceManager::refreshDevices() {
    updateInputDevices();
    updateOutputDevices();
    
    // 音频设备刷新完成
}

QVariantList AudioDeviceManager::inputDevices() const {
    QVariantList result;
    for (const QAudioDevice& device : m_inputDeviceList) {
        QVariantMap deviceMap;
        deviceMap["id"] = device.id();
        deviceMap["name"] = device.description();
        deviceMap["isDefault"] = device.isDefault();
        result.append(deviceMap);
    }
    return result;
}

QVariantList AudioDeviceManager::outputDevices() const {
    QVariantList result;
    for (const QAudioDevice& device : m_outputDeviceList) {
        QVariantMap deviceMap;
        deviceMap["id"] = device.id();
        deviceMap["name"] = device.description();
        deviceMap["isDefault"] = device.isDefault();
        result.append(deviceMap);
    }
    return result;
}

void AudioDeviceManager::setCurrentInputDevice(const QString& deviceId) {
    utils::Logger::instance().info(QString("setCurrentInputDevice调用: %1").arg(deviceId));

    QAudioDevice device = findDeviceById(m_inputDeviceList, deviceId);
    if (device.isNull()) {
        utils::Logger::instance().warn(QString("未找到输入设备: %1").arg(deviceId));
        return;
    }

    m_currentInputDeviceId = deviceId;
    emit currentInputDeviceChanged();
    
    utils::Logger::instance().info(QString(" 已设置输入设备: %1 (ID: %2)").arg(device.description(), deviceId));
}

void AudioDeviceManager::setCurrentOutputDevice(const QString& deviceId) {
    utils::Logger::instance().info(QString("setCurrentOutputDevice调用: %1").arg(deviceId));

    QAudioDevice device = findDeviceById(m_outputDeviceList, deviceId);
    if (device.isNull()) {
        utils::Logger::instance().warn(QString("未找到输出设备: %1").arg(deviceId));
        return;
    }

    m_currentOutputDeviceId = deviceId;
    emit currentOutputDeviceChanged();
    
    utils::Logger::instance().info(QString(" 已设置输出设备: %1 (ID: %2)").arg(device.description(), deviceId));
}

QAudioDevice AudioDeviceManager::getInputDevice() const {
    if (m_currentInputDeviceId.isEmpty()) {
        // 返回默认设备
        return QMediaDevices::defaultAudioInput();
    }
    
    QAudioDevice device = findDeviceById(m_inputDeviceList, m_currentInputDeviceId);
    if (device.isNull()) {
        utils::Logger::instance().warn("当前输入设备无效，使用默认设备");
        return QMediaDevices::defaultAudioInput();
    }
    
    return device;
}

QAudioDevice AudioDeviceManager::getOutputDevice() const {
    if (m_currentOutputDeviceId.isEmpty()) {
        // 返回默认设备
        return QMediaDevices::defaultAudioOutput();
    }
    
    QAudioDevice device = findDeviceById(m_outputDeviceList, m_currentOutputDeviceId);
    if (device.isNull()) {
        utils::Logger::instance().warn("当前输出设备无效，使用默认设备");
        return QMediaDevices::defaultAudioOutput();
    }
    
    return device;
}

void AudioDeviceManager::loadFromConfig() {
    QString inputDeviceId = utils::Config::instance().getAudioInputDevice();
    QString outputDeviceId = utils::Config::instance().getAudioOutputDevice();
    
    if (!inputDeviceId.isEmpty()) {
        setCurrentInputDevice(inputDeviceId);
    } else {
        // 使用默认设备
        QAudioDevice defaultInput = QMediaDevices::defaultAudioInput();
        if (!defaultInput.isNull()) {
            m_currentInputDeviceId = defaultInput.id();
            emit currentInputDeviceChanged();
        }
    }
    
    if (!outputDeviceId.isEmpty()) {
        setCurrentOutputDevice(outputDeviceId);
    } else {
        // 使用默认设备
        QAudioDevice defaultOutput = QMediaDevices::defaultAudioOutput();
        if (!defaultOutput.isNull()) {
            m_currentOutputDeviceId = defaultOutput.id();
            emit currentOutputDeviceChanged();
        }
    }
    
    // 音频设备配置已加载
}

void AudioDeviceManager::saveToConfig() {
    // 保存音频设备配置
    
    // 获取设备对象以获取名称
    QAudioDevice inputDevice = findDeviceById(m_inputDeviceList, m_currentInputDeviceId);
    QAudioDevice outputDevice = findDeviceById(m_outputDeviceList, m_currentOutputDeviceId);
    
    QString inputDeviceName = inputDevice.isNull() ? "" : inputDevice.description();
    QString outputDeviceName = outputDevice.isNull() ? "" : outputDevice.description();
    
    utils::Logger::instance().info(QString("输入设备: %1").arg(inputDeviceName));
    utils::Logger::instance().info(QString("  ID: %1").arg(m_currentInputDeviceId));
    utils::Logger::instance().info(QString("输出设备: %1").arg(outputDeviceName));
    utils::Logger::instance().info(QString("  ID: %1").arg(m_currentOutputDeviceId));
    
    // 保存ID和名称
    utils::Config::instance().setAudioInputDevice(m_currentInputDeviceId, inputDeviceName);
    utils::Config::instance().setAudioOutputDevice(m_currentOutputDeviceId, outputDeviceName);
    
    // 验证保存
    QString savedInputId = utils::Config::instance().getAudioInputDevice();
    QString savedInputName = utils::Config::instance().getAudioInputDeviceName();
    QString savedOutputId = utils::Config::instance().getAudioOutputDevice();
    QString savedOutputName = utils::Config::instance().getAudioOutputDeviceName();
    
    // 音频设备配置已完整保存
}

void AudioDeviceManager::updateInputDevices() {
    m_inputDeviceList = QMediaDevices::audioInputs();
    emit inputDevicesChanged();
}

void AudioDeviceManager::updateOutputDevices() {
    m_outputDeviceList = QMediaDevices::audioOutputs();
    emit outputDevicesChanged();
}

QAudioDevice AudioDeviceManager::findDeviceById(const QList<QAudioDevice>& devices, const QString& id) const {
    for (const QAudioDevice& device : devices) {
        if (device.id() == id) {
            return device;
        }
    }
    return QAudioDevice(); // 返回空设备
}

} // namespace audio
} // namespace xiaozhi


