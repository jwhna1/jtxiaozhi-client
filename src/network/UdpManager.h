/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T22:15:00Z
File: UdpManager.h
Desc: UDP音频通道管理器（线程方式，完整加密音频发送/接收）
*/

#ifndef UDP_MANAGER_H
#define UDP_MANAGER_H

#include "NetworkTypes.h"
#include "../audio/AudioEncryptor.h"
#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <memory>

namespace xiaozhi {
namespace network {

/**
 * @brief UDP工作线程
 */
class UdpWorker : public QObject {
    Q_OBJECT

public:
    explicit UdpWorker(QObject* parent = nullptr);
    ~UdpWorker();

public slots:
    /**
     * @brief 连接UDP服务器（在工作线程中执行）
     */
    void connectToUdp(const UdpConfig& config);

    /**
     * @brief 断开UDP连接
     */
    void disconnect();

    /**
     * @brief 发送加密音频数据
     * @param opus_data Opus编码后的音频数据
     */
    void sendAudioData(const QByteArray& opus_data);

    /**
     * @brief 发送测试音频
     */
    void sendTestAudio(const QString& sessionId);

signals:
    /**
     * @brief UDP连接成功
     */
    void udpConnected();

    /**
     * @brief 收到解密后的音频数据（Opus格式）
     */
    void audioDataReceived(const QByteArray& opus_data);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * @brief 处理UDP接收数据
     */
    void onReadyRead();

private:
    /**
     * @brief 生成正弦波测试音频（16kHz PCM, 440Hz A4音符）
     */
    QByteArray generateTestAudio();

    QUdpSocket* m_socket;
    UdpConfig m_config;
    bool m_connected;
    std::unique_ptr<audio::AudioEncryptor> m_encryptor;  // 音频加密器
};

/**
 * @brief UDP音频通道管理器（完整加密音频发送/接收）
 */
class UdpManager : public QObject {
    Q_OBJECT

public:
    explicit UdpManager(QObject* parent = nullptr);
    ~UdpManager();

    /**
     * @brief 连接UDP服务器
     */
    void connectToUdp(const UdpConfig& config);

    /**
     * @brief 断开UDP连接
     */
    void disconnect();

    /**
     * @brief 发送加密音频数据
     * @param opus_data Opus编码后的音频数据
     */
    void sendAudioData(const QByteArray& opus_data);

    /**
     * @brief 发送测试音频
     */
    void sendTestAudio(const QString& sessionId);

signals:
    /**
     * @brief UDP连接成功
     */
    void udpConnected();

    /**
     * @brief 收到解密后的音频数据（Opus格式）
     */
    void audioDataReceived(const QByteArray& opus_data);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

    // 内部信号（用于线程通信）
    void connectToUdpInternal(const UdpConfig& config);
    void disconnectInternal();
    void sendAudioDataInternal(const QByteArray& opus_data);
    void sendTestAudioInternal(const QString& sessionId);

private:
    QThread* m_workerThread;
    UdpWorker* m_worker;
};

} // namespace network
} // namespace xiaozhi

#endif // UDP_MANAGER_H


