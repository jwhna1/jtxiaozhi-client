#ifndef AUDIO_DEVICE_MANAGER_H
#define AUDIO_DEVICE_MANAGER_H

#include <QObject>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QList>
#include <QVariantList>

namespace xiaozhi {
namespace audio {

/**
 * @brief 音频设备管理器
 * 
 * 负责枚举、管理和选择音频输入/输出设备
 */
class AudioDeviceManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList inputDevices READ inputDevices NOTIFY inputDevicesChanged)
    Q_PROPERTY(QVariantList outputDevices READ outputDevices NOTIFY outputDevicesChanged)
    Q_PROPERTY(QString currentInputDevice READ currentInputDevice WRITE setCurrentInputDevice NOTIFY currentInputDeviceChanged)
    Q_PROPERTY(QString currentOutputDevice READ currentOutputDevice WRITE setCurrentOutputDevice NOTIFY currentOutputDeviceChanged)

public:
    explicit AudioDeviceManager(QObject* parent = nullptr);
    ~AudioDeviceManager();

    /**
     * @brief 刷新设备列表
     */
    Q_INVOKABLE void refreshDevices();
    
    /**
     * @brief 保存音频设备设置到配置（QML可调用）
     */
    Q_INVOKABLE void saveToConfig();

    /**
     * @brief 获取输入设备列表（QML可用）
     * @return 设备列表 [{id: "device_id", name: "设备名称"}, ...]
     */
    QVariantList inputDevices() const;

    /**
     * @brief 获取输出设备列表（QML可用）
     */
    QVariantList outputDevices() const;

    /**
     * @brief 获取当前输入设备ID
     */
    QString currentInputDevice() const { return m_currentInputDeviceId; }

    /**
     * @brief 设置当前输入设备
     */
    void setCurrentInputDevice(const QString& deviceId);

    /**
     * @brief 获取当前输出设备ID
     */
    QString currentOutputDevice() const { return m_currentOutputDeviceId; }

    /**
     * @brief 设置当前输出设备
     */
    void setCurrentOutputDevice(const QString& deviceId);

    /**
     * @brief 获取输入设备对象（C++使用）
     */
    QAudioDevice getInputDevice() const;

    /**
     * @brief 获取输出设备对象（C++使用）
     */
    QAudioDevice getOutputDevice() const;

    /**
     * @brief 从配置加载音频设备设置
     */
    void loadFromConfig();

signals:
    void inputDevicesChanged();
    void outputDevicesChanged();
    void currentInputDeviceChanged();
    void currentOutputDeviceChanged();

private:
    void updateInputDevices();
    void updateOutputDevices();
    QAudioDevice findDeviceById(const QList<QAudioDevice>& devices, const QString& id) const;

    QList<QAudioDevice> m_inputDeviceList;
    QList<QAudioDevice> m_outputDeviceList;
    QString m_currentInputDeviceId;
    QString m_currentOutputDeviceId;
};

} // namespace audio
} // namespace xiaozhi

#endif // AUDIO_DEVICE_MANAGER_H


