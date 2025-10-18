/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T23:20:00Z
File: UdpManager.cpp
Desc: UDP音频通道管理器实现（完整加密音频发送/接收）
*/

#include "UdpManager.h"
#include "../utils/Logger.h"
#include <QHostAddress>
#include <QThread>
#include <QtMath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

namespace xiaozhi {
namespace network {

// ============================================================================
// UdpWorker实现
// ============================================================================

UdpWorker::UdpWorker(QObject* parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_connected(false)
    , m_encryptor(nullptr)
{
    // 连接接收信号
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpWorker::onReadyRead);
}

UdpWorker::~UdpWorker() {
    if (m_socket) {
        m_socket->close();
    }
}

void UdpWorker::connectToUdp(const UdpConfig& config) {
    // 如果已经连接，先断开（避免重复连接）
    if (m_connected) {
        utils::Logger::instance().info("⚠️ UDP已连接，忽略重复连接请求");
        return;
    }

    m_config = config;

    // 初始化音频加密器
    m_encryptor = std::make_unique<audio::AudioEncryptor>();
    if (!m_encryptor->initialize(config.key, config.nonce)) {
        emit errorOccurred("音频加密器初始化失败");
        return;
    }

    // 音频加密器初始化成功

    try {
        // 如果socket已经bind过，先关闭重新创建
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->close();
            m_socket->deleteLater();
            m_socket = new QUdpSocket(this);
            connect(m_socket, &QUdpSocket::readyRead, this, &UdpWorker::onReadyRead);
        }

        // 绑定本地端口（可选，让系统自动分配）
        if (!m_socket->bind()) {
            emit errorOccurred("UDP Socket绑定失败");
            return;
        }

        // UDP不需要connect，直接标记为已连接即可
        m_connected = true;
        // 已移除UDP连接详情日志（敏感信息）
        emit udpConnected();

    } catch (const std::exception& e) {
        emit errorOccurred(QString("UDP连接失败: %1").arg(e.what()));
    }
}

void UdpWorker::disconnect() {
    if (m_socket) {
        m_socket->close();
        m_connected = false;
    }
    m_encryptor.reset();
}

void UdpWorker::sendAudioData(const QByteArray& opus_data) {
    if (!m_connected || !m_encryptor) {
        emit errorOccurred("UDP未连接或加密器未初始化");
        return;
    }

    // 使用加密器封装音频包（包含加密）
    uint32_t timestamp = QDateTime::currentMSecsSinceEpoch();
    QByteArray encrypted_packet = m_encryptor->encrypt(opus_data, timestamp);
    if (encrypted_packet.isEmpty()) {
        emit errorOccurred("音频包加密失败");
        return;
    }

    // 发送加密后的数据包
    qint64 sent = m_socket->writeDatagram(
        encrypted_packet,
        QHostAddress(m_config.server),
        m_config.port
    );

    if (sent < 0) {
        emit errorOccurred(QString("UDP发送失败: %1").arg(m_socket->errorString()));
    }
    // 已移除UDP发送详情日志（敏感信息）
}

void UdpWorker::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;

        qint64 received = m_socket->readDatagram(
            datagram.data(),
            datagram.size(),
            &sender,
            &senderPort
        );

        if (received < 0) {
            utils::Logger::instance().error("UDP接收数据失败");
            continue;
        }

        // 已移除UDP接收详情日志（敏感信息）

        if (!m_encryptor) {
            utils::Logger::instance().error("加密器未初始化");
            continue;
        }

        // 解密音频包
        uint32_t timestamp, sequence;
        QByteArray opus_data = m_encryptor->decrypt(datagram, timestamp, sequence);
        if (opus_data.isEmpty()) {
            utils::Logger::instance().error("音频包解密失败");
            continue;
        }

        // 发送解密后的Opus数据
        emit audioDataReceived(opus_data);
    }
}

void UdpWorker::sendTestAudio(const QString& sessionId) {
    if (!m_connected) {
        emit errorOccurred("UDP未连接");
        return;
    }

    // 生成测试音频数据
    QByteArray audioData = generateTestAudio();

    // 分块发送（1024字节/块）
    const int chunkSize = 1024;
    int totalChunks = (audioData.size() + chunkSize - 1) / chunkSize;

    for (int i = 0; i < totalChunks; ++i) {
        int start = i * chunkSize;
        int end = qMin(start + chunkSize, audioData.size());
        QByteArray chunk = audioData.mid(start, end - start);

        // 构建音频包（简化版本，实际ESP32使用二进制格式）
        QJsonObject audioPacket;
        audioPacket["type"] = "audio";
        audioPacket["session_id"] = sessionId;
        audioPacket["sequence"] = i;
        audioPacket["total_chunks"] = totalChunks;

        // 转换为字节列表
        QJsonArray dataArray;
        for (auto byte : chunk) {
            dataArray.append(static_cast<int>(static_cast<unsigned char>(byte)));
        }
        audioPacket["data"] = dataArray;

        QJsonDocument doc(audioPacket);
        QByteArray packetData = doc.toJson(QJsonDocument::Compact);

        // 发送数据包
        m_socket->writeDatagram(
            packetData,
            QHostAddress(m_config.server),
            m_config.port
        );

        // 小延迟避免网络拥塞
        QThread::msleep(10);
    }
}

QByteArray UdpWorker::generateTestAudio() {
    // 生成正弦波测试音（16kHz PCM, 440Hz A4音符, 1秒）
    const int sampleRate = 16000;
    const double duration = 1.0;
    const double frequency = 440.0; // A4音符
    const int numSamples = static_cast<int>(sampleRate * duration);

    QByteArray audioData;
    audioData.reserve(numSamples * 2); // 16位PCM

    for (int i = 0; i < numSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double sample = 32767.0 * 0.3 * qSin(2.0 * M_PI * frequency * t);
        qint16 sampleValue = static_cast<qint16>(sample);

        // 小端序
        audioData.append(static_cast<char>(sampleValue & 0xFF));
        audioData.append(static_cast<char>((sampleValue >> 8) & 0xFF));
    }

    return audioData;
}

// ============================================================================
// UdpManager实现
// ============================================================================

UdpManager::UdpManager(QObject* parent)
    : QObject(parent)
    , m_workerThread(new QThread(this))
    , m_worker(new UdpWorker())
{
    // 将worker移到工作线程
    m_worker->moveToThread(m_workerThread);

    // 连接信号
    connect(this, &UdpManager::connectToUdpInternal,
            m_worker, &UdpWorker::connectToUdp);
    connect(this, &UdpManager::disconnectInternal,
            m_worker, &UdpWorker::disconnect);
    connect(this, &UdpManager::sendAudioDataInternal,
            m_worker, &UdpWorker::sendAudioData);
    connect(this, &UdpManager::sendTestAudioInternal,
            m_worker, &UdpWorker::sendTestAudio);

    connect(m_worker, &UdpWorker::udpConnected,
            this, &UdpManager::udpConnected);
    connect(m_worker, &UdpWorker::audioDataReceived,
            this, &UdpManager::audioDataReceived);
    connect(m_worker, &UdpWorker::errorOccurred,
            this, &UdpManager::errorOccurred);

    // 启动工作线程
    m_workerThread->start();
}

UdpManager::~UdpManager() {
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_worker;
}

void UdpManager::connectToUdp(const UdpConfig& config) {
    emit connectToUdpInternal(config);
}

void UdpManager::disconnect() {
    emit disconnectInternal();
}

void UdpManager::sendAudioData(const QByteArray& opus_data) {
    emit sendAudioDataInternal(opus_data);
}

void UdpManager::sendTestAudio(const QString& sessionId) {
    emit sendTestAudioInternal(sessionId);
}

} // namespace network
} // namespace xiaozhi


