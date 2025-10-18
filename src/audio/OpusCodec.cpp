/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T22:00:00Z
File: OpusCodec.cpp
Desc: Opus音频编解码器实现
*/

#include "OpusCodec.h"
#include "../utils/Logger.h"
#include <cstring>

namespace xiaozhi {
namespace audio {

OpusCodec::OpusCodec()
    : encoder_(nullptr)
    , encoder_sample_rate_(0)
    , encoder_channels_(0)
    , encoder_frame_size_(0)
    , decoder_(nullptr)
    , decoder_sample_rate_(0)
    , decoder_channels_(0)
    , decoder_frame_size_(0)
{
}

OpusCodec::~OpusCodec() {
    if (encoder_) {
        opus_encoder_destroy(encoder_);
        encoder_ = nullptr;
    }
    
    if (decoder_) {
        opus_decoder_destroy(decoder_);
        decoder_ = nullptr;
    }
}

// ========== 编码器 ==========

bool OpusCodec::initEncoder(int sample_rate, int channels, int bitrate) {
    int error;
    
    // 创建编码器
    encoder_ = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK || !encoder_) {
        utils::Logger::instance().error(QString("❌ Opus编码器创建失败: %1").arg(opus_strerror(error)));
        return false;
    }
    
    // 设置比特率
    error = opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(bitrate));
    if (error != OPUS_OK) {
        utils::Logger::instance().error(QString("❌ 设置Opus比特率失败: %1").arg(opus_strerror(error)));
        opus_encoder_destroy(encoder_);
        encoder_ = nullptr;
        return false;
    }
    
    // 计算帧大小（样本数 = 采样率 * 帧时长 / 1000）
    encoder_sample_rate_ = sample_rate;
    encoder_channels_ = channels;
    encoder_frame_size_ = (sample_rate * FRAME_DURATION_MS) / 1000;
    
    utils::Logger::instance().info(QString(" Opus编码器初始化成功: %1Hz %2声道 %3bps 帧大小=%4样本")
        .arg(sample_rate)
        .arg(channels)
        .arg(bitrate)
        .arg(encoder_frame_size_));
    
    return true;
}

QByteArray OpusCodec::encode(const QByteArray& pcm_data) {
    if (!encoder_) {
        utils::Logger::instance().error("❌ Opus编码器未初始化");
        return QByteArray();
    }
    
    // 检查输入数据大小（每个样本2字节）
    int expected_size = encoder_frame_size_ * encoder_channels_ * 2;
    if (pcm_data.size() != expected_size) {
        utils::Logger::instance().error(QString("❌ PCM数据大小不匹配: 期望%1字节, 实际%2字节")
            .arg(expected_size)
            .arg(pcm_data.size()));
        return QByteArray();
    }
    
    // 准备输出缓冲区
    QByteArray opus_data(MAX_PACKET_SIZE, 0);
    
    // 编码
    int encoded_bytes = opus_encode(
        encoder_,
        reinterpret_cast<const opus_int16*>(pcm_data.constData()),
        encoder_frame_size_,
        reinterpret_cast<unsigned char*>(opus_data.data()),
        MAX_PACKET_SIZE
    );
    
    if (encoded_bytes < 0) {
        utils::Logger::instance().error(QString("❌ Opus编码失败: %1").arg(opus_strerror(encoded_bytes)));
        return QByteArray();
    }
    
    // 调整数据大小
    opus_data.resize(encoded_bytes);
    
    return opus_data;
}

// ========== 解码器 ==========

bool OpusCodec::initDecoder(int sample_rate, int channels) {
    int error;
    
    //  ESP32对齐：直接用目标采样率初始化（16kHz或24kHz）
    decoder_ = opus_decoder_create(sample_rate, channels, &error);
    if (error != OPUS_OK || !decoder_) {
        utils::Logger::instance().error(QString("❌ Opus解码器创建失败: %1").arg(opus_strerror(error)));
        return false;
    }
    
    // 保存解码器参数
    decoder_sample_rate_ = sample_rate;
    decoder_channels_ = channels;
    decoder_frame_size_ = (sample_rate * FRAME_DURATION_MS) / 1000;
    
    utils::Logger::instance().info(QString(" Opus解码器初始化成功: %1Hz %2声道 帧大小=%3样本")
        .arg(sample_rate)
        .arg(channels)
        .arg(decoder_frame_size_));
    
    return true;
}

QByteArray OpusCodec::decode(const QByteArray& opus_data) {
    if (!decoder_) {
        utils::Logger::instance().error("❌ Opus解码器未初始化");
        return QByteArray();
    }
    
    if (opus_data.isEmpty()) {
        utils::Logger::instance().debug("Opus数据为空，跳过解码");
        return QByteArray();
    }
    
    //  ESP32对齐：准备足够大的缓冲区支持可变帧大小
    const int MAX_FRAME_SIZE = 6000;
    QByteArray pcm_data(MAX_FRAME_SIZE * decoder_channels_ * 2, 0);
    
    // 解码
    int decoded_samples = opus_decode(
        decoder_,
        reinterpret_cast<const unsigned char*>(opus_data.constData()),
        opus_data.size(),
        reinterpret_cast<opus_int16*>(pcm_data.data()),
        MAX_FRAME_SIZE,
        0  // 不使用FEC
    );
    
    if (decoded_samples < 0) {
        // 解码失败，静默跳过（可能是包损坏）
        // 不要打印日志，避免刷屏
        return QByteArray();
    }
    
    // 调整数据大小
    pcm_data.resize(decoded_samples * decoder_channels_ * 2);
    
    return pcm_data;
}

void OpusCodec::resetDecoderState() {
    if (decoder_) {
        // 重置解码器内部状态（清除缓冲区）
        opus_decoder_ctl(decoder_, OPUS_RESET_STATE);
    }
}

bool OpusCodec::setDecoderSampleRate(int target_sample_rate) {
    //  ESP32对齐：如果采样率没变，直接返回
    if (decoder_ && decoder_sample_rate_ == target_sample_rate) {
        return false;  // 没有重新创建
    }
    
    //  ESP32对齐：销毁旧解码器
    if (decoder_) {
        opus_decoder_destroy(decoder_);
        decoder_ = nullptr;
    }
    
    //  ESP32对齐：用新采样率创建解码器
    int error;
    decoder_ = opus_decoder_create(target_sample_rate, decoder_channels_, &error);
    if (error != OPUS_OK || !decoder_) {
        utils::Logger::instance().error(QString("❌ Opus解码器重建失败: %1").arg(opus_strerror(error)));
        return false;
    }
    
    // 更新解码器参数
    decoder_sample_rate_ = target_sample_rate;
    decoder_frame_size_ = (target_sample_rate * FRAME_DURATION_MS) / 1000;
    
    utils::Logger::instance().info(QString(" Opus解码器已重建: %1Hz %2声道 (ESP32对齐)")
        .arg(target_sample_rate).arg(decoder_channels_));
    
    return true;  // 重新创建了解码器
}

} // namespace audio
} // namespace xiaozhi

