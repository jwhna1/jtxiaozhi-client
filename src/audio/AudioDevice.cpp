/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:32:00Z
File: AudioDevice.cpp
Desc: 音频设备管理器实现
*/

#include "AudioDevice.h"
#include "../utils/Logger.h"
#include <QAudioDevice>
#include <QMediaDevices>

namespace xiaozhi {
namespace audio {

AudioDevice::AudioDevice(QObject* parent)
    : QObject(parent)
    , m_inputDevice(nullptr)
    , m_outputDevice(nullptr)
    , m_recording(false)
    , m_playing(false)
{
    // 回退定时器用于在QAudioSink通知之前继续尝试写入
    connect(&m_drainFallbackTimer, &QTimer::timeout, this, &AudioDevice::drainPlaybackQueue);
    m_drainFallbackTimer.setInterval(5); // 5ms tick，低开销
}

AudioDevice::~AudioDevice() {
    stopRecording();
    stopPlayback();
}

bool AudioDevice::startRecording() {
    if (m_recording) {
        return true;
    }

    // 获取默认音频输入设备
    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull()) {
        emit errorOccurred("未找到音频输入设备");
        return false;
    }

    // 创建音频格式
    QAudioFormat format = m_config.toQAudioFormat();

    // 检查格式是否支持
    if (!inputDevice.isFormatSupported(format)) {
        emit errorOccurred("音频格式不支持");
        return false;
    }

    // 创建音频源
    m_audioSource = std::make_unique<QAudioSource>(inputDevice, format, this);
    
    // 启动录音
    m_inputDevice = m_audioSource->start();
    if (m_inputDevice) {
        connect(m_inputDevice, &QIODevice::readyRead, this, &AudioDevice::handleAudioData);
        m_recording = true;
        emit recordingStarted();
        return true;
    }

    emit errorOccurred("启动录音失败");
    return false;
}

void AudioDevice::stopRecording() {
    if (!m_recording) {
        return;
    }

    if (m_audioSource) {
        m_audioSource->stop();
        m_audioSource.reset();
    }

    m_inputDevice = nullptr;
    m_recording = false;
    emit recordingStopped();
}

bool AudioDevice::startPlayback() {
    if (m_playing) {
        return true;
    }

    // 获取默认音频输出设备
    QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
    if (outputDevice.isNull()) {
        emit errorOccurred("未找到音频输出设备");
        return false;
    }

    // 创建音频格式
    QAudioFormat format = m_config.toQAudioFormat();

    // 检查格式是否支持
    if (!outputDevice.isFormatSupported(format)) {
        emit errorOccurred("音频格式不支持");
        return false;
    }

    // 创建音频接收器
    m_audioSink = std::make_unique<QAudioSink>(outputDevice, format, this);

    // 启动播放
    m_outputDevice = m_audioSink->start();
    if (m_outputDevice) {
        m_playing = true;
        emit playbackStarted();
        // 启动主动排空逻辑
        drainPlaybackQueue();
        m_drainFallbackTimer.start();
        return true;
    }

    emit errorOccurred("启动播放失败");
    return false;
}

void AudioDevice::stopPlayback() {
    if (!m_playing) {
        return;
    }

    if (m_audioSink) {
        m_audioSink->stop();
        m_audioSink.reset();
    }

    m_outputDevice = nullptr;
    m_playing = false;
    m_pendingPlaybackBuffer.clear();
    m_drainFallbackTimer.stop();
    emit playbackStopped();
}

void AudioDevice::setAudioConfig(const AudioConfig& config) {
    m_config = config;
}

void AudioDevice::handleAudioData() {
    if (m_inputDevice && m_recording) {
        QByteArray data = m_inputDevice->readAll();
        if (!data.isEmpty()) {
            emit audioReady(data);
        }
    }
}

void AudioDevice::writeAudioData(const QByteArray& data) {
    if (!m_playing || !m_outputDevice) {
        return;
    }
    if (data.isEmpty()) {
        return;
    }
    // 追加到待播放缓冲，由drainPlaybackQueue按bytesFree写入
    m_pendingPlaybackBuffer.append(data);
    drainPlaybackQueue();
}

void AudioDevice::drainPlaybackQueue() {
    if (!m_playing || !m_outputDevice) {
        m_pendingPlaybackBuffer.clear();
        return;
    }
    if (m_pendingPlaybackBuffer.isEmpty()) {
        return;
    }
    // 查询可写字节，避免阻塞和写失败
    const qint64 freeBytes = m_audioSink ? m_audioSink->bytesFree() : 0;
    if (freeBytes <= 0) {
        return;
    }
    // 一次写不超过可用空间与缓冲剩余
    const qint64 toWrite = qMin<qint64>(freeBytes, m_pendingPlaybackBuffer.size());
    if (toWrite <= 0) return;

    qint64 written = m_outputDevice->write(m_pendingPlaybackBuffer.constData(), toWrite);
    if (written > 0) {
        m_pendingPlaybackBuffer.remove(0, static_cast<int>(written));
    }
}

} // namespace audio
} // namespace xiaozhi


