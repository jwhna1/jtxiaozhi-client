/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T23:05:00Z
File: AppModel.h
Desc: 应用模型（QML绑定的核心控制器，管理多个设备会话）
*/

#ifndef APP_MODEL_H
#define APP_MODEL_H

#include "../network/DeviceSession.h"
#include "../network/UpdateManager.h"
#include "../audio/AudioDevice.h"
#include "../audio/AudioDeviceManager.h"
#include "../models/ChatMessage.h"
#include "../storage/AppDatabase.h"
#include "../storage/AudioCacheManager.h"
#include "../storage/ImageCacheManager.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QVariantList>
#include <memory>

namespace xiaozhi {
namespace models {

/**
 * @brief 应用模型（管理多个设备会话）
 */
class AppModel : public QObject {
    Q_OBJECT

    // QML属性（针对当前选中设备）
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool udpConnected READ udpConnected NOTIFY udpConnectedChanged)
    Q_PROPERTY(QString activationCode READ activationCode NOTIFY activationCodeChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool isDarkTheme READ isDarkTheme WRITE setIsDarkTheme NOTIFY isDarkThemeChanged)
    Q_PROPERTY(bool websocketEnabled READ websocketEnabled WRITE setWebsocketEnabled NOTIFY websocketEnabledChanged)
    Q_PROPERTY(QStringList logMessages READ logMessages NOTIFY logMessagesChanged)
    Q_PROPERTY(QStringList deviceList READ deviceList NOTIFY deviceListChanged)
    Q_PROPERTY(QVariantList deviceInfoList READ deviceInfoList NOTIFY deviceListChanged)
    Q_PROPERTY(QString currentDeviceId READ currentDeviceId NOTIFY currentDeviceIdChanged)
    Q_PROPERTY(QString currentDeviceName READ currentDeviceName NOTIFY currentDeviceNameChanged)
    Q_PROPERTY(QObject* audioDeviceManager READ audioDeviceManager CONSTANT)
    Q_PROPERTY(QObject* conversationManager READ conversationManager NOTIFY conversationManagerChanged)
    Q_PROPERTY(QVariantList chatMessages READ chatMessages NOTIFY chatMessagesChanged)
    Q_PROPERTY(QObject* updateManager READ updateManager CONSTANT)

public:
    explicit AppModel(QObject* parent = nullptr);
    ~AppModel();

    // ========== 属性访问器 ==========

    bool connected() const;
    bool udpConnected() const;
    QString activationCode() const;
    QString statusMessage() const;
    bool isDarkTheme() const;
    void setIsDarkTheme(bool dark);
    bool websocketEnabled() const;
    void setWebsocketEnabled(bool enabled);
    QStringList logMessages() const { return m_logMessages; }
    QStringList deviceList() const;
    QVariantList deviceInfoList() const;
    QString currentDeviceId() const { return m_currentDeviceId; }
    QString currentDeviceName() const;
    QObject* audioDeviceManager() const { return m_audioDeviceManager.get(); }
    QObject* conversationManager() const;
    QVariantList chatMessages() const;
    QObject* updateManager() const { return m_updateManager.get(); }

    // ========== QML可调用方法 ==========

    /**
     * @brief 添加新设备
     */
    Q_INVOKABLE void addDevice(const QString& name, const QString& otaUrl, const QString& macAddress);

    /**
     * @brief 检查是否可以添加设备（数量限制和服务器限制）
     * @param otaUrl OTA服务器地址
     * @return 检查结果和错误信息
     */
    Q_INVOKABLE QVariantMap canAddDevice(const QString& otaUrl);

    /**
     * @brief 移除设备
     */
    Q_INVOKABLE void removeDevice(const QString& deviceId);

    /**
     * @brief 更新设备信息
     */
    Q_INVOKABLE void updateDevice(const QString& deviceId, const QString& name, const QString& otaUrl);

    /**
     * @brief 获取设备信息
     */
    Q_INVOKABLE QVariantMap getDeviceInfo(const QString& deviceId);

    /**
     * @brief 选中设备
     */
    Q_INVOKABLE void selectDevice(const QString& deviceId);

    /**
     * @brief 连接设备（完整流程：OTA -> 自动MQTT -> 自动hello）
     */
    Q_INVOKABLE void connectDevice(const QString& deviceId);

    /**
     * @brief 获取当前设备OTA配置
     */
    Q_INVOKABLE void getOtaConfig();

    /**
     * @brief 连接当前设备MQTT
     */
    Q_INVOKABLE void connectMqtt();

    /**
     * @brief 申请当前设备UDP通道
     */
    Q_INVOKABLE void requestAudioChannel();

    /**
     * @brief 当前设备发送文本
     */
    Q_INVOKABLE void sendTextMessage(const QString& text);

    /**
     * @brief 当前设备发送测试音频
     */
    Q_INVOKABLE void sendTestAudio();

    /**
     * @brief 发送图片识别消息
     * @param imagePath 图片文件路径
     * @param text 可选的文字描述
     */
    Q_INVOKABLE void sendImageMessage(const QString& imagePath, const QString& text = QString());

    /**
     * @brief 断开指定设备
     */
    Q_INVOKABLE void disconnectDevice(const QString& deviceId);

    /**
     * @brief 播放音频消息
     */
    Q_INVOKABLE void playAudioMessage(qint64 messageId);

    /**
     * @brief 停止音频播放
     */
    Q_INVOKABLE void stopAudioPlayback();

    /**
     * @brief 清空聊天记录
     */
    Q_INVOKABLE void clearChatHistory(const QString& deviceId);

    /**
     * @brief 断开所有设备
     */
    Q_INVOKABLE void disconnectAll();

    /**
     * @brief 切换主题
     */
    Q_INVOKABLE void toggleTheme();

    /**
     * @brief 启动录音
     */
    Q_INVOKABLE void startAudioRecording();

    /**
     * @brief 停止录音
     */
    Q_INVOKABLE void stopAudioRecording();

    /**
     * @brief 生成随机MAC地址
     */
    Q_INVOKABLE QString generateRandomMac();

    /**
     * @brief 获取程序版本号
     */
    Q_INVOKABLE QString getVersion() const;

    /**
     * @brief 获取程序完整标题（名称+版本）
     */
    Q_INVOKABLE QString getAppTitle() const;

signals:
    void connectedChanged();
    void udpConnectedChanged();
    void activationCodeChanged();
    void statusMessageChanged();
    void isDarkThemeChanged();
    void websocketEnabledChanged();
    void logMessagesChanged();
    void deviceListChanged();
    void currentDeviceIdChanged();
    void currentDeviceNameChanged();
    void conversationManagerChanged();
    void chatMessagesChanged();
    void audioPlaybackStateChanged(qint64 messageId, bool playing);

private slots:
    // 设备会话回调
    void onDeviceStatusChanged(const QString& deviceId, const QString& status);
    void onDeviceLogMessage(const QString& deviceId, const QString& message);
    void onDeviceActivationCode(const QString& deviceId, const QString& code);
    void onDeviceConnectionStateChanged(const QString& deviceId, bool connected, bool udpConnected);
    void onChatMessageReceived(const QString& deviceId, const xiaozhi::models::ChatMessage& message, const QByteArray& pcmData = QByteArray());

private:
    /**
     * @brief 添加日志
     */
    void addLog(const QString& message);

    /**
     * @brief 获取当前设备会话
     */
    network::DeviceSession* getCurrentDevice();

    /**
     * @brief 加载已保存的设备
     */
    void loadSavedDevices();

    /**
     * @brief 保存设备配置
     */
    void saveDeviceConfig(const QString& deviceId);

    /**
     * @brief 加载聊天消息
     */
    void loadChatMessages(const QString& deviceId);

    /**
     * @brief 保存聊天消息到数据库
     */
    void saveChatMessage(const xiaozhi::models::ChatMessage& message, const QByteArray& pcmData = QByteArray());
    
    /**
     * @brief 重新连接所有设备（协议切换时调用）
     */
    void reconnectAllDevices();

    QMap<QString, std::shared_ptr<network::DeviceSession>> m_deviceSessions;
    QString m_currentDeviceId;
    bool m_isDarkTheme;
    bool m_websocketEnabled;
    QStringList m_logMessages;
    std::unique_ptr<audio::AudioDevice> m_audioDevice;
    std::unique_ptr<audio::AudioDeviceManager> m_audioDeviceManager;

    // 更新管理
    std::unique_ptr<network::UpdateManager> m_updateManager;

    // 存储管理
    std::unique_ptr<storage::AppDatabase> m_appDatabase;
    std::unique_ptr<storage::AudioCacheManager> m_audioCacheManager;
    std::unique_ptr<storage::ImageCacheManager> m_imageCacheManager;
    
    // 聊天消息
    QList<xiaozhi::models::ChatMessage> m_currentChatMessages;
    QVariantList m_chatMessagesCache;  // QML绑定缓存
    
    /**
     * @brief 更新聊天消息缓存（供QML绑定）
     */
    void updateChatMessagesCache();
};

} // namespace models
} // namespace xiaozhi

#endif // APP_MODEL_H

