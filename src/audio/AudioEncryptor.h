/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T20:50:00Z
File: AudioEncryptor.h
Desc: 音频数据AES-CTR加密/解密模块（严格对齐ESP32实现）
*/

#ifndef AUDIO_ENCRYPTOR_H
#define AUDIO_ENCRYPTOR_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <openssl/evp.h>
#include <openssl/aes.h>

namespace xiaozhi {
namespace audio {

/**
 * @brief UDP音频包头结构（与ESP32完全一致）
 * 
 * 格式: |type 1byte|flags 1byte|payload_len 2bytes|ssrc 4bytes|timestamp 4bytes|sequence 4bytes|payload|
 */
#pragma pack(push, 1)
struct AudioPacketHeader {
    uint8_t type;           // 数据包类型，固定为0x01
    uint8_t flags;          // 标志位，当前未使用
    uint16_t payload_len;   // 负载长度（网络字节序）
    uint32_t ssrc;          // 同步源标识符
    uint32_t timestamp;     // 时间戳（网络字节序）
    uint32_t sequence;      // 序列号（网络字节序）
};
#pragma pack(pop)

/**
 * @brief 音频加密器类
 * 
 * 使用AES-128-CTR模式加密/解密音频数据
 * 完全对齐ESP32固件的实现（mbedtls_aes_crypt_ctr）
 */
class AudioEncryptor : public QObject {
    Q_OBJECT

public:
    explicit AudioEncryptor(QObject* parent = nullptr);
    ~AudioEncryptor();

    /**
     * @brief 初始化加密上下文
     * @param key AES密钥（16字节，十六进制字符串）
     * @param nonce 随机数（16字节，十六进制字符串）
     * @return 成功返回true
     */
    bool initialize(const QString& keyHex, const QString& nonceHex);

    /**
     * @brief 加密音频数据并封装为UDP包
     * @param audioData 原始音频数据（Opus编码后）
     * @param timestamp 时间戳（毫秒）
     * @return 加密后的完整UDP包（包含包头）
     */
    QByteArray encrypt(const QByteArray& audioData, uint32_t timestamp);

    /**
     * @brief 解密UDP音频包
     * @param encryptedPacket 加密的UDP包（包含包头）
     * @param timestamp 输出参数：解密后的时间戳
     * @param sequence 输出参数：解密后的序列号
     * @return 解密后的原始音频数据，失败返回空
     */
    QByteArray decrypt(const QByteArray& encryptedPacket, 
                      uint32_t& timestamp, 
                      uint32_t& sequence);

    /**
     * @brief 获取当前本地序列号
     */
    uint32_t getLocalSequence() const { return m_localSequence; }

    /**
     * @brief 获取远程序列号
     */
    uint32_t getRemoteSequence() const { return m_remoteSequence; }

    /**
     * @brief 重置序列号
     */
    void resetSequence();

private:
    /**
     * @brief 十六进制字符串转字节数组
     */
    QByteArray hexToBytes(const QString& hex);

    /**
     * @brief 网络字节序转换
     */
    uint16_t htons_custom(uint16_t value);
    uint32_t htonl_custom(uint32_t value);
    uint16_t ntohs_custom(uint16_t value);
    uint32_t ntohl_custom(uint32_t value);

private:
    EVP_CIPHER_CTX* m_encryptCtx;   // 加密上下文
    EVP_CIPHER_CTX* m_decryptCtx;   // 解密上下文
    
    QByteArray m_key;               // AES密钥（16字节）
    QByteArray m_nonce;             // 随机数（16字节）
    
    uint32_t m_localSequence;       // 本地发送序列号
    uint32_t m_remoteSequence;      // 远程接收序列号
    uint32_t m_ssrc;                // 同步源标识符
    
    bool m_initialized;             // 是否已初始化
};

} // namespace audio
} // namespace xiaozhi

#endif // AUDIO_ENCRYPTOR_H


