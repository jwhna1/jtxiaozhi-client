/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-12T06:30:00Z
File: AppModel.cpp
Desc: 应用模型实现（使用MAC生成固定deviceId）
*/

#include "AppModel.h"
#include "../audio/ConversationManager.h"
#include "../utils/Config.h"
#include "../utils/Logger.h"
#include "../version/version_info.h"
#include <QUuid>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QCoreApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(appModel, "xiaozhi.appmodel")

namespace xiaozhi {
namespace models {

AppModel::AppModel(QObject* parent)
    : QObject(parent)
    , m_isDarkTheme(false)
    , m_websocketEnabled(false)
    , m_audioDevice(std::make_unique<audio::AudioDevice>(this))
    , m_audioDeviceManager(std::make_unique<audio::AudioDeviceManager>(this))
    , m_updateManager(std::make_unique<network::UpdateManager>(this))
    , m_appDatabase(std::make_unique<storage::AppDatabase>(this))
    , m_audioCacheManager(std::make_unique<storage::AudioCacheManager>(this))
    , m_imageCacheManager(std::make_unique<storage::ImageCacheManager>(this))
{
    // 初始化数据库（仅使用程序目录，移除AppData依赖）
    QString programDir = QCoreApplication::applicationDirPath();
    QString dbPath = programDir + "/data/app.db";
    
    // 确保data目录存在
    QDir dbDir(programDir + "/data");
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            utils::Logger::instance().error(" 无法在程序目录创建data文件夹: " + dbDir.absolutePath());
        }
    }
    
    if (m_appDatabase->initialize(dbPath)) {
        utils::Logger::instance().info(" 数据库初始化成功: " + dbPath);
    } else {
        utils::Logger::instance().error(" 数据库初始化失败: " + dbPath);
    }

    // 初始化音频缓存（仅使用程序目录/cache/audio）
    QString cachePath = programDir + "/cache/audio";
    if (!m_audioCacheManager->initialize(cachePath)) {
        utils::Logger::instance().error(" 音频缓存管理器初始化失败: " + cachePath);
    }

    // 初始化图片缓存管理器
    QString imageCachePath = programDir + "/cache/image";
    if (!m_imageCacheManager->initialize(imageCachePath)) {
        utils::Logger::instance().error(" 图片缓存管理器初始化失败: " + imageCachePath);
    }

    // 加载主题设置
    m_isDarkTheme = utils::Config::instance().isDarkTheme();

    // 初始化更新管理器
    m_updateManager->setCurrentVersion(xiaozhi::version::VERSION);
    utils::Logger::instance().info(QString(" 更新管理器初始化完成，当前版本: %1").arg(xiaozhi::version::VERSION));

    // 加载WebSocket设置
    QString wsEnabledStr = m_appDatabase->getSetting("websocket_enabled").toString();
    m_websocketEnabled = (wsEnabledStr == "true");
    utils::Logger::instance().info(QString("WebSocket协议: %1")
        .arg(m_websocketEnabled ? "已启用" : "已禁用"));

    // 加载已保存的设备
    loadSavedDevices();

    // 如果没有设备，创建默认设备
    if (m_deviceSessions.isEmpty()) {
        QString defaultMac = utils::Config::generateMacAddress();
        addDevice("智能体小智", utils::Config::instance().getDefaultOtaUrl(), defaultMac);
    }

    addLog("🚀 小智客户端启动");

    // 启动时静默检查更新（延迟3秒，避免影响启动速度）
    QTimer::singleShot(3000, [this]() {
        if (m_updateManager) {
            m_updateManager->setCurrentVersion(xiaozhi::version::VERSION);
            m_updateManager->checkForUpdates(true); // 静默模式
        }
    });
}

AppModel::~AppModel() {
}

// ========== 属性访问器实现 ==========

bool AppModel::connected() const {
    auto device = const_cast<AppModel*>(this)->getCurrentDevice();
    return device ? device->isConnected() : false;
}

bool AppModel::udpConnected() const {
    auto device = const_cast<AppModel*>(this)->getCurrentDevice();
    return device ? device->isUdpConnected() : false;
}

QString AppModel::activationCode() const {
    auto device = const_cast<AppModel*>(this)->getCurrentDevice();
    return device ? device->activationCode() : QString();
}

QString AppModel::statusMessage() const {
    auto device = const_cast<AppModel*>(this)->getCurrentDevice();
    if (!device) {
        return "未选中设备";
    }

    //  根据实际协议显示状态
    if (device->isConnected()) {
        if (device->isWebSocketMode()) {
            return " WebSocket已连接";
        } else if (device->isUdpConnected()) {
            return "已连接（MQTT + UDP）";
        } else {
            return "MQTT已连接";
        }
    } else {
        return "未连接";
    }
}

bool AppModel::isDarkTheme() const {
    return m_isDarkTheme;
}

void AppModel::setIsDarkTheme(bool dark) {
    if (m_isDarkTheme != dark) {
        m_isDarkTheme = dark;
        utils::Config::instance().setDarkTheme(dark);
        emit isDarkThemeChanged();
    }
}

bool AppModel::websocketEnabled() const {
    return m_websocketEnabled;
}

void AppModel::setWebsocketEnabled(bool enabled) {
    if (m_websocketEnabled != enabled) {
        m_websocketEnabled = enabled;
        // 保存到数据库
        m_appDatabase->setSetting("websocket_enabled", enabled ? "true" : "false");
        emit websocketEnabledChanged();
        
        //  立即更新所有已存在设备的WebSocket设置
        for (auto it = m_deviceSessions.begin(); it != m_deviceSessions.end(); ++it) {
            it.value()->updateWebSocketEnabled(enabled);
        }
        
        utils::Logger::instance().info(QString(" WebSocket协议%1（下次连接时生效）")
            .arg(enabled ? "已启用" : "已禁用"));
    }
}

void AppModel::reconnectAllDevices() {
    utils::Logger::instance().info(" 重新连接所有设备以切换协议...");
    
    for (auto it = m_deviceSessions.begin(); it != m_deviceSessions.end(); ++it) {
        auto device = it.value();
        QString deviceId = device->deviceId();
        QString deviceName = device->deviceName();
        QString macAddress = device->macAddress();
        QString otaUrl = device->otaUrl();
        
        // 断开旧连接
        bool wasConnected = device->isConnected();
        device->disconnect();
        
        // 重新创建设备会话（使用新的WebSocket设置）
        auto newDevice = std::make_shared<network::DeviceSession>(
            deviceId, deviceName, macAddress, otaUrl, 
            m_audioDevice.get(), m_websocketEnabled, this);
        
        // 连接信号
        connect(newDevice.get(), &network::DeviceSession::statusChanged,
                this, &AppModel::onDeviceStatusChanged);
        connect(newDevice.get(), &network::DeviceSession::logMessage,
                this, &AppModel::onDeviceLogMessage);
        connect(newDevice.get(), &network::DeviceSession::activationCodeReceived,
                this, &AppModel::onDeviceActivationCode);
        connect(newDevice.get(), &network::DeviceSession::connectionStateChanged,
                this, &AppModel::onDeviceConnectionStateChanged);
        connect(newDevice.get(), &network::DeviceSession::chatMessageReceived,
                this, &AppModel::onChatMessageReceived);
        
        // 替换设备会话
        m_deviceSessions[deviceId] = newDevice;
        
        // 如果之前是连接状态，自动重连
        if (wasConnected) {
            utils::Logger::instance().info(QString("🔗 自动重连设备: %1").arg(deviceName));
            newDevice->getOtaConfig();
        }
    }
    
    // 如果当前设备变了，更新UI
    if (!m_currentDeviceId.isEmpty()) {
        emit conversationManagerChanged();
        emit connectedChanged();
        emit udpConnectedChanged();
    }
    
    utils::Logger::instance().info(QString(" 协议切换完成，使用%1")
        .arg(m_websocketEnabled ? "WebSocket" : "MQTT+UDP"));
}

QStringList AppModel::deviceList() const {
    QStringList list;
    for (auto it = m_deviceSessions.constBegin(); it != m_deviceSessions.constEnd(); ++it) {
        list.append(it.value()->deviceName());
    }
    return list;
}

QVariantList AppModel::deviceInfoList() const {
    QVariantList list;
    for (auto it = m_deviceSessions.constBegin(); it != m_deviceSessions.constEnd(); ++it) {
        QVariantMap info;
        info["deviceId"] = it.key();
        info["deviceName"] = it.value()->deviceName();
        info["connected"] = it.value()->isConnected();
        info["udpConnected"] = it.value()->isUdpConnected();
        list.append(info);
    }
    return list;
}

QString AppModel::currentDeviceName() const {
    auto device = const_cast<AppModel*>(this)->getCurrentDevice();
    return device ? device->deviceName() : QString();
}

QVariantList AppModel::chatMessages() const {
    // 直接返回缓存的QVariantList，避免每次重新创建
    return m_chatMessagesCache;
}

QObject* AppModel::conversationManager() const {
    auto device = const_cast<AppModel*>(this)->getCurrentDevice();
    if (device) {
        auto cm = device->conversationManager();
        if (cm) {
            return cm;
        }
        // 移除日志：ConversationManager在UDP连接后才创建，之前查询为null是正常的
    }
    return nullptr;
}

// ========== QML可调用方法实现 ==========

QVariantMap AppModel::canAddDevice(const QString& otaUrl) {
    QVariantMap result;
    result["canAdd"] = true;
    result["errorMessage"] = "";
    
    // 检查设备总数限制（最多2个）
    if (m_deviceSessions.size() >= 2) {
        result["canAdd"] = false;
        result["errorMessage"] = "最多只能添加2个智能体设备";
        return result;
    }
    
    // 检查虾哥官方服务器限制（只能1个）
    QString officialServerUrl = "https://api.tenclass.net/xiaozhi/ota/";
    if (otaUrl == officialServerUrl) {
        // 检查是否已经有虾哥官方服务器
        for (auto it = m_deviceSessions.constBegin(); it != m_deviceSessions.constEnd(); ++it) {
            if (it.value()->otaUrl() == officialServerUrl) {
                result["canAdd"] = false;
                result["errorMessage"] = "虾哥官方服务器只能添加一个，请选择其他服务器";
                return result;
            }
        }
    }
    
    return result;
}

void AppModel::addDevice(const QString& name, const QString& otaUrl, const QString& macAddress) {
    // 先检查是否可以添加设备
    QVariantMap checkResult = canAddDevice(otaUrl);
    if (!checkResult["canAdd"].toBool()) {
        addLog(QString(" 添加设备失败: %1").arg(checkResult["errorMessage"].toString()));
        return;
    }
    
    //  使用MAC地址生成固定的设备ID（与DeviceSession的UUID保持一致）
    QString deviceId = network::DeviceSession::generateUuidFromMac(macAddress);

    // 创建设备会话（传递音频设备和WebSocket启用状态）
    auto device = std::make_shared<network::DeviceSession>(
        deviceId, name, macAddress, otaUrl, m_audioDevice.get(), m_websocketEnabled, this);

    // 连接设备信号
    connect(device.get(), &network::DeviceSession::statusChanged,
            this, &AppModel::onDeviceStatusChanged);
    connect(device.get(), &network::DeviceSession::logMessage,
            this, &AppModel::onDeviceLogMessage);
    connect(device.get(), &network::DeviceSession::activationCodeReceived,
            this, &AppModel::onDeviceActivationCode);
    connect(device.get(), &network::DeviceSession::connectionStateChanged,
            this, &AppModel::onDeviceConnectionStateChanged);
    connect(device.get(), &network::DeviceSession::chatMessageReceived,
            this, &AppModel::onChatMessageReceived);

    // 添加到会话列表
    m_deviceSessions[deviceId] = device;

    // 保存设备配置
    saveDeviceConfig(deviceId);

    // 如果是第一个设备，自动选中
    if (m_currentDeviceId.isEmpty()) {
        m_currentDeviceId = deviceId;
        emit currentDeviceIdChanged();
        emit currentDeviceNameChanged();
    }

    emit deviceListChanged();
    addLog(QString("➕ 添加设备: %1").arg(name));

    // 自动获取OTA配置
    device->getOtaConfig();
}

void AppModel::removeDevice(const QString& deviceId) {
    if (!m_deviceSessions.contains(deviceId)) {
        return;
    }

    // 断开连接
    m_deviceSessions[deviceId]->disconnect();

    // 移除设备
    QString deviceName = m_deviceSessions[deviceId]->deviceName();
    m_deviceSessions.remove(deviceId);

    // 删除配置
    utils::Config::instance().removeDeviceConfig(deviceId);

    // 如果删除的是当前设备，选择第一个设备
    if (m_currentDeviceId == deviceId) {
        if (!m_deviceSessions.isEmpty()) {
            m_currentDeviceId = m_deviceSessions.firstKey();
        } else {
            m_currentDeviceId.clear();
        }
        emit currentDeviceIdChanged();
        emit currentDeviceNameChanged();
        emit connectedChanged();
        emit udpConnectedChanged();
        emit activationCodeChanged();
        emit statusMessageChanged();
    }

    emit deviceListChanged();
    addLog(QString("➖ 删除设备: %1").arg(deviceName));
}

void AppModel::updateDevice(const QString& deviceId, const QString& name, const QString& otaUrl) {
    if (!m_deviceSessions.contains(deviceId)) {
        return;
    }

    // 更新设备信息
    auto device = m_deviceSessions[deviceId];
    QString oldName = device->deviceName();
    
    // 加载旧配置
    auto oldConfig = utils::Config::instance().loadDeviceConfig(deviceId);
    
    // 更新配置
    utils::DeviceConfig newConfig(deviceId, name, oldConfig.macAddress, otaUrl);
    utils::Config::instance().saveDeviceConfig(newConfig);
    
    // 断开旧连接
    bool wasConnected = device->isConnected();
    device->disconnect();
    
    // 重新创建设备会话（传递WebSocket启用状态）
    auto newDevice = std::make_shared<network::DeviceSession>(deviceId, name, oldConfig.macAddress, otaUrl, m_audioDevice.get(), m_websocketEnabled, this);
    
    // 连接信号（使用现有的信号）
    connect(newDevice.get(), &network::DeviceSession::statusChanged,
            this, [this](const QString& devId, const QString& status) {
        if (m_currentDeviceId == devId) {
            emit statusMessageChanged();
        }
    });
    
    connect(newDevice.get(), &network::DeviceSession::logMessage,
            this, [this](const QString& devId, const QString& message) {
        addLog(QString("[%1] %2").arg(devId, message));
    });
    
    connect(newDevice.get(), &network::DeviceSession::activationCodeReceived,
            this, [this](const QString& devId, const QString& code) {
        if (m_currentDeviceId == devId) {
            emit activationCodeChanged();
        }
    });
    // 确保聊天消息信号连接（之前遗漏导致UI没有气泡）
    connect(newDevice.get(), &network::DeviceSession::chatMessageReceived,
            this, &AppModel::onChatMessageReceived);
    
    connect(newDevice.get(), &network::DeviceSession::connectionStateChanged,
            this, [this](const QString& devId, bool mqttConnected, bool udpConn) {
        if (m_currentDeviceId == devId) {
            emit connectedChanged();
            emit udpConnectedChanged();
            emit conversationManagerChanged();
        }
    });
    
    // 替换旧设备会话
    m_deviceSessions[deviceId] = newDevice;
    
    // 通知UI更新
    emit deviceListChanged();
    if (m_currentDeviceId == deviceId) {
        emit currentDeviceNameChanged();
    }
    
    addLog(QString("✏️ 更新设备: %1 → %2").arg(oldName, name));
    
    // 如果之前已连接，自动重新获取配置
    if (wasConnected) {
        newDevice->getOtaConfig();
    }
}

QVariantMap AppModel::getDeviceInfo(const QString& deviceId) {
    QVariantMap info;
    
    if (!m_deviceSessions.contains(deviceId)) {
        return info;
    }
    
    auto device = m_deviceSessions[deviceId];
    auto config = utils::Config::instance().loadDeviceConfig(deviceId);
    
    info["deviceId"] = deviceId;
    info["deviceName"] = device->deviceName();
    info["macAddress"] = device->macAddress();
    info["otaUrl"] = config.otaUrl;
    info["connected"] = device->isConnected();
    info["udpConnected"] = device->isUdpConnected();
    
    return info;
}

void AppModel::selectDevice(const QString& deviceId) {
    if (m_deviceSessions.contains(deviceId) && m_currentDeviceId != deviceId) {
        m_currentDeviceId = deviceId;
        emit currentDeviceIdChanged();
        emit currentDeviceNameChanged();
        emit connectedChanged();
        emit udpConnectedChanged();
        emit activationCodeChanged();
        emit statusMessageChanged();
        emit conversationManagerChanged();  // 通知对话管理器变化

        // 加载新设备的聊天记录
        loadChatMessages(deviceId);

        addLog(QString("👉 选中设备: %1").arg(m_deviceSessions[deviceId]->deviceName()));
    }
}

void AppModel::connectDevice(const QString& deviceId) {
    // 先选中设备
    if (deviceId != m_currentDeviceId) {
        selectDevice(deviceId);
    }
    
    // 触发OTA配置获取（之后会自动连接MQTT）
    auto device = getCurrentDevice();
    if (device) {
        addLog(QString(" 开始连接设备: %1").arg(device->deviceName()));
        addLog(" 步骤1: 获取OTA配置...");
        device->getOtaConfig();
    }
}

void AppModel::getOtaConfig() {
    auto device = getCurrentDevice();
    if (device) {
        device->getOtaConfig();
    }
}

void AppModel::connectMqtt() {
    auto device = getCurrentDevice();
    if (device) {
        device->connectMqtt();
    }
}

void AppModel::requestAudioChannel() {
    auto device = getCurrentDevice();
    if (device) {
        device->requestAudioChannel();
    }
}

void AppModel::sendTextMessage(const QString& text) {
    auto device = getCurrentDevice();
    if (device && !text.trimmed().isEmpty()) {
        device->sendTextMessage(text);
        
        // 立即在UI显示用户发送的文本消息
        xiaozhi::models::ChatMessage userMsg;
        userMsg.deviceId = m_currentDeviceId;
        userMsg.messageType = "text";
        userMsg.textContent = text;
        userMsg.audioFilePath = "";
        userMsg.timestamp = QDateTime::currentMSecsSinceEpoch();
        userMsg.isFinal = true;
        userMsg.createdAt = QDateTime::currentDateTime();
        
        saveChatMessage(userMsg, QByteArray());
    }
}

void AppModel::sendTestAudio() {
    auto device = getCurrentDevice();
    if (device) {
        device->sendTestAudio();
    }
}

void AppModel::sendImageMessage(const QString& imagePath, const QString& text) {
    auto device = getCurrentDevice();
    if (device && !imagePath.trimmed().isEmpty()) {
        // 处理文件URL（去掉 file:/// 前缀）
        QString localPath = imagePath;
        if (localPath.startsWith("file:///")) {
            localPath = localPath.mid(8);  // 移除 "file:///"
        }
        
        device->sendImageMessage(localPath, text);
        
        // 保存图片到缓存目录
        qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        QString imageRelativePath;
        QString imageAbsolutePath;
        if (m_imageCacheManager) {
            imageRelativePath = m_imageCacheManager->saveImageCache(m_currentDeviceId, localPath, timestamp);
            // 立即解析为绝对路径用于UI显示
            if (!imageRelativePath.isEmpty()) {
                imageAbsolutePath = m_imageCacheManager->resolveFullPath(imageRelativePath);
            }
        }
        
        // 立即在UI显示用户发送的消息（带图片标识和预览）
        xiaozhi::models::ChatMessage userMsg;
        userMsg.deviceId = m_currentDeviceId;
        userMsg.messageType = "image";
        userMsg.textContent = QString("📷 %1").arg(text.isEmpty() ? "发送图片" : text);
        userMsg.audioFilePath = "";
        userMsg.imagePath = imageAbsolutePath;  // UI使用绝对路径
        userMsg.timestamp = timestamp;
        userMsg.isFinal = true;
        userMsg.createdAt = QDateTime::currentDateTime();
        
        // 注意：saveChatMessage会保存到数据库，我们需要传递相对路径给数据库
        // 但UI需要绝对路径，所以这里需要特殊处理
        saveChatMessage(userMsg, QByteArray());
    }
}

void AppModel::disconnectDevice(const QString& deviceId) {
    if (m_deviceSessions.contains(deviceId)) {
        m_deviceSessions[deviceId]->disconnect();
    }
}

void AppModel::disconnectAll() {
    for (auto& device : m_deviceSessions) {
        device->disconnect();
    }
    addLog("🔌 已断开所有设备");
}

void AppModel::toggleTheme() {
    setIsDarkTheme(!m_isDarkTheme);
    addLog(m_isDarkTheme ? " 切换到深色主题" : " 切换到浅色主题");
}

void AppModel::startAudioRecording() {
    if (m_audioDevice->startRecording()) {
        addLog("🎙️ 开始录音");
    } else {
        addLog(" 启动录音失败");
    }
}

void AppModel::stopAudioRecording() {
    m_audioDevice->stopRecording();
    addLog("🔇 停止录音");
}

QString AppModel::generateRandomMac() {
    return utils::Config::generateMacAddress();
}

QString AppModel::getVersion() const {
    return QString::fromUtf8(xiaozhi::version::VERSION);
}

QString AppModel::getAppTitle() const {
    return QString("%1 %2")
        .arg(QString::fromUtf8(xiaozhi::version::PROJECT_NAME))
        .arg(QString::fromUtf8(xiaozhi::version::VERSION));
}

// ========== 设备会话回调 ==========

void AppModel::onDeviceStatusChanged(const QString& deviceId, const QString& status) {
    if (deviceId == m_currentDeviceId) {
        emit statusMessageChanged();
    }
}

void AppModel::onDeviceLogMessage(const QString& deviceId, const QString& message) {
    // 获取设备名称
    QString deviceName = "Unknown";
    if (m_deviceSessions.contains(deviceId)) {
        deviceName = m_deviceSessions[deviceId]->deviceName();
    }

    // 添加日志（带设备名称）
    addLog(QString("[%1] %2").arg(deviceName, message));
}

void AppModel::onDeviceActivationCode(const QString& deviceId, const QString& code) {
    if (deviceId == m_currentDeviceId) {
        emit activationCodeChanged();
    }
}

void AppModel::onDeviceConnectionStateChanged(const QString& deviceId, bool connected, bool udpConnected) {
    if (deviceId == m_currentDeviceId) {
        emit connectedChanged();
        emit udpConnectedChanged();
        emit statusMessageChanged();
        // UDP连接后ConversationManager会创建，通知QML
        emit conversationManagerChanged();
    }
    
    // 任何设备的连接状态改变，都更新设备列表显示
    emit deviceListChanged();
}

// ========== 私有方法 ==========

void AppModel::addLog(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logMessage = QString("[%1] %2").arg(timestamp, message);
    
    m_logMessages.append(logMessage);
    
    // 限制日志数量（最多1000条）
    if (m_logMessages.size() > 1000) {
        m_logMessages.removeFirst();
    }
    
    emit logMessagesChanged();
    
    // 同时输出到Logger
    utils::Logger::instance().info(message);
}

network::DeviceSession* AppModel::getCurrentDevice() {
    if (m_currentDeviceId.isEmpty() || !m_deviceSessions.contains(m_currentDeviceId)) {
        return nullptr;
    }
    return m_deviceSessions[m_currentDeviceId].get();
}

void AppModel::loadSavedDevices() {
    QStringList deviceIds = utils::Config::instance().getAllDeviceIds();
    
    for (const QString& deviceId : deviceIds) {
        utils::DeviceConfig config = utils::Config::instance().loadDeviceConfig(deviceId);
        if (!config.deviceId.isEmpty()) {
            // 重新创建设备会话（传递音频设备和WebSocket启用状态）
            auto device = std::make_shared<network::DeviceSession>(
                config.deviceId,
                config.deviceName,
                config.macAddress,
                config.otaUrl,
                m_audioDevice.get(),
                m_websocketEnabled,
                this
            );

            // 连接设备信号
            connect(device.get(), &network::DeviceSession::statusChanged,
                    this, &AppModel::onDeviceStatusChanged);
            connect(device.get(), &network::DeviceSession::logMessage,
                    this, &AppModel::onDeviceLogMessage);
            connect(device.get(), &network::DeviceSession::activationCodeReceived,
                    this, &AppModel::onDeviceActivationCode);
            connect(device.get(), &network::DeviceSession::connectionStateChanged,
                    this, &AppModel::onDeviceConnectionStateChanged);
            // 关键：加载设备时也要连接聊天消息信号
            connect(device.get(), &network::DeviceSession::chatMessageReceived,
                    this, &AppModel::onChatMessageReceived);

            m_deviceSessions[deviceId] = device;
        }
    }

    // 选中第一个设备
    if (!m_deviceSessions.isEmpty() && m_currentDeviceId.isEmpty()) {
        m_currentDeviceId = m_deviceSessions.firstKey();
        emit currentDeviceIdChanged();
        emit currentDeviceNameChanged();
        
        // 加载第一个设备的聊天记录
        loadChatMessages(m_currentDeviceId);
        
        utils::Logger::instance().info(QString("📱 启动时自动选中设备: %1")
            .arg(m_deviceSessions[m_currentDeviceId]->deviceName()));
    }
}

void AppModel::saveDeviceConfig(const QString& deviceId) {
    if (!m_deviceSessions.contains(deviceId)) {
        return;
    }

    auto device = m_deviceSessions[deviceId];
    utils::DeviceConfig config(
        device->deviceId(),
        device->deviceName(),
        device->macAddress(),
        device->otaUrl()
    );

    utils::Config::instance().saveDeviceConfig(config);
}

// ========== 聊天消息管理 ==========

void AppModel::playAudioMessage(qint64 messageId) {
    // 查找消息
    auto it = std::find_if(m_currentChatMessages.begin(), m_currentChatMessages.end(),
                          [messageId](const xiaozhi::models::ChatMessage& msg) { return msg.id == messageId; });
    
    if (it == m_currentChatMessages.end() || it->audioFilePath.isEmpty()) {
        utils::Logger::instance().warn("音频消息不存在或没有音频文件");
        return;
    }
    
    // 停止当前播放
    stopAudioPlayback();
    
    // 读取音频参数（从缓存文件头）
    int sampleRate = 16000;
    int channels = 1;
    if (m_audioCacheManager->getAudioInfo(it->audioFilePath, sampleRate, channels) == false) {
        utils::Logger::instance().warn("无法读取音频头信息，使用默认 16k/1ch");
    }

    // 加载音频PCM数据（已剥离文件头）
    QByteArray pcmData = m_audioCacheManager->loadAudioCache(it->audioFilePath);
    if (pcmData.isEmpty()) {
        utils::Logger::instance().error("加载音频文件失败: " + it->audioFilePath);
        return;
    }
    
    // 配置音频设备
    audio::AudioConfig config;
    config.sampleRate = sampleRate;
    config.channelCount = channels;
    config.sampleSize = 16;
    config.sampleFormat = QAudioFormat::Int16;
    m_audioDevice->setAudioConfig(config);
    
    // 播放
    if (m_audioDevice->startPlayback()) {
        // 直接写入音频数据播放
        m_audioDevice->writeAudioData(pcmData);
        
        // 更新播放状态
        it->isPlaying = true;
        emit audioPlaybackStateChanged(messageId, true);
        updateChatMessagesCache();  // 更新缓存以刷新UI
        
        utils::Logger::instance().info(QString("开始播放音频消息: %1").arg(messageId));
    } else {
        utils::Logger::instance().error("启动音频播放失败（可能是不支持的音频格式或设备不可用）");
    }
}

void AppModel::stopAudioPlayback() {
    m_audioDevice->stopPlayback();
    
    // 重置所有消息的播放状态
    bool stateChanged = false;
    for (auto& msg : m_currentChatMessages) {
        if (msg.isPlaying) {
            msg.isPlaying = false;
            emit audioPlaybackStateChanged(msg.id, false);
            stateChanged = true;
        }
    }
    
    // 只有状态变化时才更新缓存
    if (stateChanged) {
        updateChatMessagesCache();  // 更新缓存以刷新UI
    }
}

void AppModel::clearChatHistory(const QString& deviceId) {
    if (m_appDatabase) {
        m_appDatabase->clearMessages(deviceId);
    }
    
    if (m_audioCacheManager) {
        m_audioCacheManager->clearDeviceCache(deviceId);
    }
    
    // 如果是当前设备，重新加载消息
    if (deviceId == m_currentDeviceId) {
        loadChatMessages(deviceId);
    }
    
    utils::Logger::instance().info(QString("清空设备聊天记录: %1").arg(deviceId));
}

void AppModel::updateChatMessagesCache() {
    m_chatMessagesCache.clear();
    for (const xiaozhi::models::ChatMessage& msg : m_currentChatMessages) {
        m_chatMessagesCache.append(msg.toVariantMap());
    }
    emit chatMessagesChanged();
}

void AppModel::loadChatMessages(const QString& deviceId) {
    if (!m_appDatabase) {
        return;
    }
    
    m_currentChatMessages = m_appDatabase->getMessages(deviceId, 100);
    
    // 将相对路径转换为绝对路径
    if (m_audioCacheManager || m_imageCacheManager) {
        for (auto& msg : m_currentChatMessages) {
            // 解析音频路径
            if (!msg.audioFilePath.isEmpty() && m_audioCacheManager) {
                QString absolutePath = m_audioCacheManager->resolveFullPath(msg.audioFilePath);
                msg.audioFilePath = absolutePath;
            }
            
            // 解析图片路径
            if (!msg.imagePath.isEmpty() && m_imageCacheManager) {
                QString absolutePath = m_imageCacheManager->resolveFullPath(msg.imagePath);
                // 验证文件是否存在
                if (QFileInfo::exists(absolutePath)) {
                    msg.imagePath = absolutePath;
                } else {
                    qCWarning(appModel) << "加载消息时发现图片文件不存在:" << absolutePath;
                    msg.imagePath = "";  // 清空无效路径
                }
            }
        }
    }
    
    // 更新QML缓存并发射信号
    updateChatMessagesCache();
}

void AppModel::saveChatMessage(const xiaozhi::models::ChatMessage& message, const QByteArray& pcmData) {
    if (!m_appDatabase) {
        return;
    }
    
    // 检查是否已存在相同 timestamp 的消息（避免重复添加）
    auto existingIt = std::find_if(m_currentChatMessages.begin(), m_currentChatMessages.end(),
                                    [&message](const xiaozhi::models::ChatMessage& msg) {
                                        return msg.timestamp == message.timestamp && 
                                               msg.deviceId == message.deviceId &&
                                               msg.messageType == message.messageType;
                                    });
    
    QString audioPath;
    
    // 如果有PCM数据，保存到音频缓存
    if (!pcmData.isEmpty() && m_audioCacheManager) {
        int sampleRate = 16000;  // 默认采样率
        int channels = 1;        // 默认单声道

        // 从ConversationManager获取实际采样率（如果有的话）
        auto device = getCurrentDevice();
        if (device && device->conversationManager()) {
            auto cm = static_cast<audio::ConversationManager*>(device->conversationManager());
            if (cm) {
                sampleRate = cm->serverSampleRate();
                channels = cm->serverChannels();
            }
        }
        
        audioPath = m_audioCacheManager->saveAudioCache(
            message.deviceId, pcmData, message.timestamp, sampleRate, channels);
    }
    
    // 如果消息已存在，更新音频路径（内存和数据库）
    if (existingIt != m_currentChatMessages.end()) {
        existingIt->audioFilePath = audioPath;
        existingIt->isFinal = message.isFinal;
        
        // 更新数据库中的音频路径
        if (!audioPath.isEmpty() && existingIt->id > 0) {
            m_appDatabase->updateMessageAudioPath(existingIt->id, audioPath);
        }
        
        updateChatMessagesCache();  // 更新UI
        return;
    }
    
    // 创建完整的消息对象
    xiaozhi::models::ChatMessage fullMessage = message;
    fullMessage.audioFilePath = audioPath;
    
    // 数据库中保存相对路径（音频和图片）
    QString dbAudioPath = audioPath;  // 音频已经是相对路径
    QString dbImagePath = message.imagePath;
    
    // 验证图片文件是否存在，如果不存在则清空路径
    if (!dbImagePath.isEmpty()) {
        QString fullImagePath;
        if (QFileInfo(dbImagePath).isAbsolute()) {
            fullImagePath = dbImagePath;
            // 从绝对路径中提取相对路径部分（deviceId/image_xxx.png）
            QDir cacheDir(QCoreApplication::applicationDirPath() + "/cache/image");
            dbImagePath = cacheDir.relativeFilePath(dbImagePath);
        } else {
            // 相对路径需要拼接完整路径
            fullImagePath = QCoreApplication::applicationDirPath() + "/cache/image/" + dbImagePath;
        }

        // 检查文件是否存在
        if (!QFileInfo::exists(fullImagePath)) {
            qCWarning(appModel) << "图片文件不存在，清空图片路径:" << fullImagePath;
            dbImagePath = "";
        }
    }
    
    // 保存到数据库（使用相对路径）
    qint64 messageId = m_appDatabase->insertMessage(
        fullMessage.deviceId,
        fullMessage.messageType,
        fullMessage.textContent,
        dbAudioPath,
        dbImagePath,
        fullMessage.timestamp,
        fullMessage.isFinal
    );
    
    // 数据库插入成功
    
    if (messageId > 0) {
        fullMessage.id = messageId;
        
        // 保存聊天消息
        
        // 如果是当前设备，添加到当前消息列表
        if (fullMessage.deviceId == m_currentDeviceId) {
            m_currentChatMessages.append(fullMessage);
            updateChatMessagesCache();  // 更新QML缓存
        } else {
            utils::Logger::instance().info(QString(" 消息设备ID不匹配，未添加到UI列表"));
        }
    }
}

void AppModel::onChatMessageReceived(const QString& deviceId, const xiaozhi::models::ChatMessage& message, const QByteArray& pcmData) {
    
    // 保存消息到数据库（包含PCM数据）
    saveChatMessage(message, pcmData);
    
    // 如果是当前设备，立即显示
    if (deviceId == m_currentDeviceId) {
        // 消息已经通过saveChatMessage添加到m_currentChatMessages
    }
}

} // namespace models
} // namespace xiaozhi

