/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T22:00:00Z
File: OpusCodec.h
Desc: Opus音频编解码器（对齐ESP32参数：16kHz单声道60ms帧）
*/

#ifndef OPUS_CODEC_H
#define OPUS_CODEC_H

#include <QByteArray>
#include <memory>
#include <opus/opus.h>

namespace xiaozhi {
namespace audio {

/**
 * @brief Opus音频编解码器
 * 
 * 编码参数（设备端）：
 * - 采样率: 16000 Hz
 * - 声道: 1（单声道）
 * - 帧时长: 60ms
 * - 比特率: 24000 bps
 * 
 * 解码参数（服务器端）：
 * - 采样率: 24000 Hz
 * - 声道: 1（单声道）
 * - 帧时长: 60ms
 */
class OpusCodec {
public:
    /**
     * @brief 构造函数
     */
    OpusCodec();
    
    /**
     * @brief 析构函数
     */
    ~OpusCodec();

    // ========== 编码器 ==========
    
    /**
     * @brief 初始化编码器
     * @param sample_rate 采样率（默认16000）
     * @param channels 声道数（默认1）
     * @param bitrate 比特率（默认24000）
     * @return 成功返回true
     */
    bool initEncoder(int sample_rate = 16000, int channels = 1, int bitrate = 24000);
    
    /**
     * @brief 编码PCM音频数据
     * @param pcm_data PCM数据（16位有符号整数）
     * @return Opus编码数据，失败返回空数组
     */
    QByteArray encode(const QByteArray& pcm_data);
    
    /**
     * @brief 获取编码器每帧样本数
     * @return 样本数（16kHz * 60ms = 960）
     */
    int getEncoderFrameSize() const { return encoder_frame_size_; }

    // ========== 解码器 ==========
    
    /**
     * @brief 初始化解码器
     * @param sample_rate 采样率（默认24000）
     * @param channels 声道数（默认1）
     * @return 成功返回true
     */
    bool initDecoder(int sample_rate = 24000, int channels = 1);
    
    /**
     * @brief 解码Opus音频数据
     * @param opus_data Opus编码数据
     * @return PCM数据（16位有符号整数），失败返回空数组
     */
    QByteArray decode(const QByteArray& opus_data);
    
    /**
     * @brief 重置解码器状态（清除内部缓冲）
     */
    void resetDecoderState();
    
    /**
     * @brief ESP32对齐：动态切换解码器采样率
     * @param target_sample_rate 目标采样率（16000 或 24000）
     * @return 如果重新创建了解码器返回true
     */
    bool setDecoderSampleRate(int target_sample_rate);
    
    /**
     * @brief 获取解码器每帧样本数
     * @return 样本数（24kHz * 60ms = 1440）
     */
    int getDecoderFrameSize() const { return decoder_frame_size_; }
    
    /**
     * @brief 获取解码器当前采样率
     * @return 采样率（16000或24000）
     */
    int getDecoderSampleRate() const { return decoder_sample_rate_; }

    // ========== 状态查询 ==========
    
    /**
     * @brief 编码器是否已初始化
     */
    bool isEncoderReady() const { return encoder_ != nullptr; }
    
    /**
     * @brief 解码器是否已初始化
     */
    bool isDecoderReady() const { return decoder_ != nullptr; }

private:
    // 编码器
    OpusEncoder* encoder_;
    int encoder_sample_rate_;
    int encoder_channels_;
    int encoder_frame_size_;     // 每帧样本数
    
    // 解码器
    OpusDecoder* decoder_;
    int decoder_sample_rate_;
    int decoder_channels_;
    int decoder_frame_size_;     // 每帧样本数
    
    // 常量
    static constexpr int FRAME_DURATION_MS = 60;  // 帧时长（毫秒）
    static constexpr int MAX_PACKET_SIZE = 4000;  // Opus包最大尺寸
};

} // namespace audio
} // namespace xiaozhi

#endif // OPUS_CODEC_H

