/*
Project: jtxiaozhi-client
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-18T11:20:00Z
File: version_info.cpp
Desc: 版本信息实现
*/

#include "version_info.h"
#include <QDateTime>

namespace xiaozhi {
namespace version {

QString current_time_iso8601() {
    // 获取当前UTC时间并格式化为ISO8601
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QString full_version_string() {
    return QString("%1 %2 - %3 (%4)")
        .arg(PROJECT_NAME)
        .arg(VERSION)
        .arg(AUTHOR)
        .arg(EMAIL);
}

} // namespace version
} // namespace xiaozhi


