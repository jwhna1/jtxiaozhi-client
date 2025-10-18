/*
Project: jtxiaozhi-client
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-18T11:20:00Z
File: version_info.h
Desc: 版本信息与标准文件头定义
*/

#ifndef VERSION_INFO_H
#define VERSION_INFO_H

#include <QString>

namespace xiaozhi {
namespace version {

// 项目元信息常量
constexpr const char* PROJECT_NAME = "小智跨平台客户端-社区版";
constexpr const char* PROJECT_NAME_EN = "jtxiaozhi-client";
constexpr const char* VERSION = "v0.1.0";
constexpr const char* AUTHOR = "jtserver团队";
constexpr const char* EMAIL = "jwhna1@gmail.com";

/**
 * @brief 获取当前ISO8601格式的UTC时间戳
 * @return 格式: YYYY-MM-DDThh:mm:ssZ
 */
QString current_time_iso8601();

/**
 * @brief 获取完整的版本信息字符串
 * @return 包含项目名称、版本号和作者的字符串
 */
QString full_version_string();

} // namespace version
} // namespace xiaozhi

#endif // VERSION_INFO_H


