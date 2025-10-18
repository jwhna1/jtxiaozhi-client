/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-12T07:00:00Z
File: AudioEncryptor.cpp
Desc: 音频数据AES-CTR加密/解密实现（修复解密时使用实际payload长度）
*/

#include "AudioEncryptor.h"
#include "../utils/Logger.h"
#include <QRandomGenerator>
#include <cstring>

namespace xiaozhi {
namespace audio {

AudioEncryptor::AudioEncryptor(QObject* parent)
    : QObject(parent)
    , m_encryptCtx(nullptr)
    , m_decryptCtx(nullptr)
    , m_localSequence(0)
    , m_remoteSequence(0)
    , m_ssrc(0)
    , m_initialized(false)
{
    // SSRC将从服务器的nonce中提取
}

AudioEncryptor::~AudioEncryptor() {
    if (m_encryptCtx) {
        EVP_CIPHER_CTX_free(m_encryptCtx);
    }
    if (m_decryptCtx) {
        EVP_CIPHER_CTX_free(m_decryptCtx);
    }
}

bool AudioEncryptor::initialize(const QString& keyHex, const QString& nonceHex) {
    // 已移除音频加密器初始化详情日志（敏感信息）
    
    // 转换十六进制字符串为字节
    m_key = hexToBytes(keyHex);
    m_nonce = hexToBytes(nonceHex);
    
    if (m_key.size() != 16 || m_nonce.size() != 16) {
        utils::Logger::instance().error(QString("密钥或Nonce长度错误: key=%1, nonce=%2")
            .arg(m_key.size()).arg(m_nonce.size()));
        return false;
    }
    
    // 创建加密上下文
    m_encryptCtx = EVP_CIPHER_CTX_new();
    m_decryptCtx = EVP_CIPHER_CTX_new();
    
    if (!m_encryptCtx || !m_decryptCtx) {
        utils::Logger::instance().error("创建EVP上下文失败");
        return false;
    }
    
    // 初始化AES-128-CTR加密
    if (EVP_EncryptInit_ex(m_encryptCtx, EVP_aes_128_ctr(), nullptr, 
                          (const unsigned char*)m_key.data(), 
                          (const unsigned char*)m_nonce.data()) != 1) {
        utils::Logger::instance().error("初始化加密上下文失败");
        return false;
    }
    
    // 初始化AES-128-CTR解密（CTR模式加密和解密相同）
    if (EVP_DecryptInit_ex(m_decryptCtx, EVP_aes_128_ctr(), nullptr, 
                          (const unsigned char*)m_key.data(), 
                          (const unsigned char*)m_nonce.data()) != 1) {
        utils::Logger::instance().error("初始化解密上下文失败");
        return false;
    }
    
    m_initialized = true;
    m_localSequence = 0;
    m_remoteSequence = 0;
    
    // 从nonce中提取SSRC (位置4-7)
    // nonce结构: [type 1][flags 1][len 2][ssrc 4][timestamp 4][sequence 4]
    memcpy(&m_ssrc, m_nonce.data() + 4, 4);
    m_ssrc = ntohl_custom(m_ssrc);  // 转换为主机字节序
    
    // 加密器初始化成功
    
    return true;
}

QByteArray AudioEncryptor::encrypt(const QByteArray& audioData, uint32_t timestamp) {
    if (!m_initialized) {
        utils::Logger::instance().error("加密器未初始化");
        return QByteArray();
    }
    
    if (audioData.isEmpty()) {
        return QByteArray();
    }
    
    // 递增序列号
    uint32_t sequence = ++m_localSequence;
    
    // 构建Nonce（与ESP32完全一致）
    // ESP32代码: std::string nonce(aes_nonce_);
    //           *(uint16_t*)&nonce[2] = htons(packet->payload.size());
    //           *(uint32_t*)&nonce[8] = htonl(packet->timestamp);
    //           *(uint32_t*)&nonce[12] = htonl(++local_sequence_);
    // nonce结构就是header: [type][flags][len][ssrc][timestamp][sequence]
    QByteArray nonce = m_nonce;  // 复制原始nonce（已包含type/flags/ssrc）
    
    // 在nonce的特定位置写入payload长度、时间戳和序列号（网络字节序）
    uint16_t payloadLen = htons_custom(audioData.size());
    uint32_t ts = htonl_custom(timestamp);
    uint32_t seq = htonl_custom(sequence);
    
    memcpy(nonce.data() + 2, &payloadLen, 2);   // 位置2-3
    memcpy(nonce.data() + 8, &ts, 4);           // 位置8-11
    memcpy(nonce.data() + 12, &seq, 4);         // 位置12-15
    // 位置0-1 (type/flags) 和 位置4-7 (ssrc) 保持服务器提供的值不变
    
    // 加密音频数据
    QByteArray encrypted(audioData.size(), 0);
    int outLen = 0;
    
    // 重新初始化IV（使用修改后的nonce）
    if (EVP_EncryptInit_ex(m_encryptCtx, nullptr, nullptr, nullptr, 
                          (const unsigned char*)nonce.data()) != 1) {
        utils::Logger::instance().error("重新初始化加密IV失败");
        return QByteArray();
    }
    
    if (EVP_EncryptUpdate(m_encryptCtx, (unsigned char*)encrypted.data(), &outLen,
                         (const unsigned char*)audioData.data(), audioData.size()) != 1) {
        utils::Logger::instance().error("加密失败");
        return QByteArray();
    }
    
    // 构建完整UDP包：nonce(16字节，就是header) + 加密负载
    // 与ESP32完全一致：modified_nonce + encrypted_payload
    QByteArray packet(16 + encrypted.size(), 0);
    memcpy(packet.data(), nonce.data(), 16);  // 前16字节是修改后的nonce（就是header）
    memcpy(packet.data() + 16, encrypted.data(), encrypted.size());  // 后面是加密的payload
    
    return packet;
}

QByteArray AudioEncryptor::decrypt(const QByteArray& encryptedPacket, 
                                  uint32_t& timestamp, 
                                  uint32_t& sequence) {
    if (!m_initialized) {
        utils::Logger::instance().error("加密器未初始化");
        return QByteArray();
    }
    
    if (encryptedPacket.size() < (int)sizeof(AudioPacketHeader)) {
        utils::Logger::instance().error(QString("UDP包太小: %1字节").arg(encryptedPacket.size()));
        return QByteArray();
    }
    
    // 解析包头
    AudioPacketHeader header;
    memcpy(&header, encryptedPacket.data(), sizeof(AudioPacketHeader));
    
    header.payload_len = ntohs_custom(header.payload_len);
    header.ssrc = ntohl_custom(header.ssrc);
    timestamp = ntohl_custom(header.timestamp);
    sequence = ntohl_custom(header.sequence);
    
    // 验证包类型
    if (header.type != 0x01) {
        utils::Logger::instance().warn(QString("未知包类型: 0x%1").arg(header.type, 2, 16, QChar('0')));
        return QByteArray();
    }
    
    // 验证序列号连续性
    if (m_remoteSequence > 0 && sequence != m_remoteSequence + 1) {
        utils::Logger::instance().warn(QString("序列号跳跃: 期望=%1, 实际=%2")
            .arg(m_remoteSequence + 1).arg(sequence));
    }
    m_remoteSequence = sequence;
    
    // 提取加密负载（使用实际计算的大小，服务器可能不填payload_len字段）
    int payloadSize = encryptedPacket.size() - sizeof(AudioPacketHeader);
    if (payloadSize != header.payload_len && header.payload_len != 0) {
        // 只在payload_len非0且不匹配时才警告（服务器payload_len=0是正常的）
        utils::Logger::instance().debug(QString("负载长度不匹配: 包头=%1, 实际=%2")
            .arg(header.payload_len).arg(payloadSize));
    }
    
    // 直接使用数据包前16字节作为CTR IV（与ESP32/旧客户端一致，完全避免长度字段不一致导致的偏移）
    QByteArray nonce = encryptedPacket.left(sizeof(AudioPacketHeader));
    
    // 解密
    QByteArray decrypted(payloadSize, 0);
    int outLen = 0;
    
    // 重新初始化IV
    if (EVP_DecryptInit_ex(m_decryptCtx, nullptr, nullptr, nullptr, 
                          (const unsigned char*)nonce.constData()) != 1) {
        utils::Logger::instance().error("重新初始化解密IV失败");
        return QByteArray();
    }
    
    if (EVP_DecryptUpdate(m_decryptCtx, (unsigned char*)decrypted.data(), &outLen,
                         (const unsigned char*)(encryptedPacket.data() + sizeof(AudioPacketHeader)), 
                         payloadSize) != 1) {
        utils::Logger::instance().error("解密失败");
        return QByteArray();
    }
    
    return decrypted;
}

void AudioEncryptor::resetSequence() {
    m_localSequence = 0;
    m_remoteSequence = 0;
    utils::Logger::instance().info("序列号已重置");
}

QByteArray AudioEncryptor::hexToBytes(const QString& hex) {
    QByteArray bytes;
    QString cleanHex = hex.trimmed();
    
    // 移除可能的0x前缀
    if (cleanHex.startsWith("0x", Qt::CaseInsensitive)) {
        cleanHex = cleanHex.mid(2);
    }
    
    // 确保是偶数长度
    if (cleanHex.length() % 2 != 0) {
        cleanHex.prepend('0');
    }
    
    for (int i = 0; i < cleanHex.length(); i += 2) {
        bool ok;
        uint8_t byte = cleanHex.mid(i, 2).toUInt(&ok, 16);
        if (ok) {
            bytes.append(byte);
        }
    }
    
    return bytes;
}

uint16_t AudioEncryptor::htons_custom(uint16_t value) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
#else
    return value;
#endif
}

uint32_t AudioEncryptor::htonl_custom(uint32_t value) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
#else
    return value;
#endif
}

uint16_t AudioEncryptor::ntohs_custom(uint16_t value) {
    return htons_custom(value);  // 网络序转换是对称的
}

uint32_t AudioEncryptor::ntohl_custom(uint32_t value) {
    return htonl_custom(value);  // 网络序转换是对称的
}

} // namespace audio
} // namespace xiaozhi

