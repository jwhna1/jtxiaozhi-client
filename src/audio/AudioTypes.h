/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:32:00Z
File: AudioTypes.h
Desc: 音频相关数据结构定义
*/

#ifndef AUDIO_TYPES_H
#define AUDIO_TYPES_H

#include <QAudioFormat>

namespace xiaozhi {
namespace audio {

/**
 * @brief 音频格式配置
 */
struct AudioConfig {
    int sampleRate = 16000;        // 采样率（Hz）
    int sampleSize = 16;           // 采样大小（位）
    int channelCount = 1;          // 声道数
    QAudioFormat::SampleFormat sampleFormat = QAudioFormat::Int16; // 采样格式

    /**
     * @brief 转换为QAudioFormat
     */
    QAudioFormat toQAudioFormat() const {
        QAudioFormat format;
        format.setSampleRate(sampleRate);
        format.setChannelCount(channelCount);
        format.setSampleFormat(sampleFormat);
        return format;
    }
};

} // namespace audio
} // namespace xiaozhi

#endif // AUDIO_TYPES_H


