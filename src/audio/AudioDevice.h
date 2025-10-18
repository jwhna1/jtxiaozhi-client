/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:32:00Z
File: AudioDevice.h
Desc: 音频设备管理器（基础录音/播放启动停止功能）
*/

#ifndef AUDIO_DEVICE_H
#define AUDIO_DEVICE_H

#include "AudioTypes.h"
#include <QObject>
#include <QAudioSource>
#include <QAudioSink>
#include <QIODevice>
#include <QTimer>
#include <QByteArray>
#include <memory>

namespace xiaozhi {
namespace audio {

/**
 * @brief 音频设备管理器
 */
class AudioDevice : public QObject {
    Q_OBJECT

public:
    explicit AudioDevice(QObject* parent = nullptr);
    ~AudioDevice();

    /**
     * @brief 启动录音
     */
    bool startRecording();

    /**
     * @brief 停止录音
     */
    void stopRecording();

    /**
     * @brief 启动播放
     */
    bool startPlayback();

    /**
     * @brief 停止播放
     */
    void stopPlayback();

    /**
     * @brief 是否正在录音
     */
    bool isRecording() const { return m_recording; }

    /**
     * @brief 是否正在播放
     */
    bool isPlaying() const { return m_playing; }

    /**
     * @brief 设置音频配置
     */
    void setAudioConfig(const AudioConfig& config);

    /**
     * @brief 获取音频配置
     */
    AudioConfig audioConfig() const { return m_config; }

    /**
     * @brief 写入PCM数据到播放设备
     * @param data PCM原始数据（不包含任何头部），采样率/声道需与当前配置一致
     */
    void writeAudioData(const QByteArray& data);

signals:
    /**
     * @brief 音频数据就绪（录音）
     */
    void audioReady(const QByteArray& data);

    /**
     * @brief 录音开始
     */
    void recordingStarted();

    /**
     * @brief 录音停止
     */
    void recordingStopped();

    /**
     * @brief 播放开始
     */
    void playbackStarted();

    /**
     * @brief 播放停止
     */
    void playbackStopped();

    /**
     * @brief 播放完成（所有数据已播放完毕）
     */
    void playbackFinished();

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * @brief 处理录音数据
     */
    void handleAudioData();

    /**
     * @brief 尝试将待播放缓冲写入输出设备（基于bytesFree进行节流）
     */
    void drainPlaybackQueue();

private:
    AudioConfig m_config;
    std::unique_ptr<QAudioSource> m_audioSource;
    std::unique_ptr<QAudioSink> m_audioSink;
    QIODevice* m_inputDevice;
    QIODevice* m_outputDevice;
    bool m_recording;
    bool m_playing;

    // 播放缓冲与节流
    QByteArray m_pendingPlaybackBuffer;
    QTimer m_drainFallbackTimer;
};

} // namespace audio
} // namespace xiaozhi

#endif // AUDIO_DEVICE_H


