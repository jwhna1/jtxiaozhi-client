/*
Project: jtxiaozhi-client
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-18T11:10:00Z
File: UpdateManager.cpp
Desc: 版本升级管理器实现
*/

#include "UpdateManager.h"
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QApplication>
#include <QMessageBox>
#include <QRegularExpression>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QProcess>
#include <QDesktopServices>

Q_LOGGING_CATEGORY(updateManager, "xiaozhi.update")

namespace xiaozhi {
namespace network {

const QString UpdateManager::GITHUB_API_URL = "https://api.github.com/repos/jwhna1/jtxiaozhi-client/releases";
const QString UpdateManager::GITHUB_REPO_URL = "https://github.com/jwhna1/jtxiaozhi-client";
const int UpdateManager::AUTO_CHECK_INTERVAL_MS = 24 * 60 * 60 * 1000; // 24小时

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_autoCheckTimer(new QTimer(this))
    , m_status(NoUpdate)
    , m_downloadProgress(0)
    , m_autoCheckEnabled(true)
    , m_silentMode(false)
{
    // 设置默认当前版本
    setCurrentVersion("v0.1.0");

    // 配置自动检查定时器
    m_autoCheckTimer->setInterval(AUTO_CHECK_INTERVAL_MS);
    connect(m_autoCheckTimer, &QTimer::timeout, this, [this]() {
        checkForUpdates(true); // 静默检查
    });

    // 启动自动检查
    if (m_autoCheckEnabled) {
        m_autoCheckTimer->start();
    }

    qCInfo(updateManager) << "UpdateManager initialized, auto-check:" << m_autoCheckEnabled;
}

UpdateManager::~UpdateManager()
{
    if (m_autoCheckTimer->isActive()) {
        m_autoCheckTimer->stop();
    }
}

void UpdateManager::checkForUpdates(bool silent)
{
    m_silentMode = silent;
    setStatus(Checking);
    setUpdateStatusText(silent ? "正在检查更新..." : "检查更新中...");

    qCInfo(updateManager) << "Checking for updates, silent mode:" << silent;

    QNetworkReply *reply = createGetRequest(QUrl(GITHUB_API_URL));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onCheckFinished(reply);
    });
}

void UpdateManager::downloadUpdate()
{
    if (!m_releaseInfo.isValid()) {
        emit downloadFailed("没有可用的更新信息");
        return;
    }

    setStatus(Downloading);
    setUpdateStatusText("正在下载更新...");
    setDownloadProgress(0);

    qCInfo(updateManager) << "Starting download from:" << m_releaseInfo.downloadUrl;

    QNetworkReply *reply = createGetRequest(QUrl(m_releaseInfo.downloadUrl));
    connect(reply, &QNetworkReply::downloadProgress, this, &UpdateManager::onDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onDownloadFinished(reply);
    });
}

void UpdateManager::installUpdate(const QString &filePath)
{
    qCInfo(updateManager) << "Installing update from:" << filePath;

#ifdef Q_OS_WIN
    // Windows: 执行安装程序
    QString command = QString("\"%1\"").arg(filePath);
    QProcess::startDetached(command);
    QApplication::quit();
#else
    // 其他平台: 提示用户手动安装
    QMessageBox::information(nullptr, "安装更新",
                           "请手动下载并安装新版本。\n下载链接将自动打开。");
    QDesktopServices::openUrl(QUrl(GITHUB_REPO_URL));
#endif
}

void UpdateManager::installUpdate()
{
    QString filePath = getDownloadPath() + "/" + m_releaseInfo.fileName;
    if (QFile::exists(filePath)) {
        installUpdate(filePath);
    } else {
        emit downloadFailed("安装文件不存在，请重新下载");
    }
}

void UpdateManager::setAutoCheckEnabled(bool enabled)
{
    if (m_autoCheckEnabled != enabled) {
        m_autoCheckEnabled = enabled;
        emit autoCheckEnabledChanged();

        if (enabled) {
            m_autoCheckTimer->start();
        } else {
            m_autoCheckTimer->stop();
        }

        qCInfo(updateManager) << "Auto-check enabled:" << enabled;
    }
}

void UpdateManager::setCurrentVersion(const QString &version)
{
    if (m_currentVersion != version) {
        m_currentVersion = version;
        emit currentVersionChanged();
        qCInfo(updateManager) << "Current version set to:" << version;
    }
}

void UpdateManager::onCheckFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString error = QString("检查更新失败: %1").arg(reply->errorString());
        qCWarning(updateManager) << error;

        // 静默模式下不显示网络错误
        if (!m_silentMode) {
            setUpdateStatusText(error);
            setStatus(DownloadFailed);
            emit checkFailed(error);
        } else {
            // 静默模式下重置为无更新状态
            setStatus(NoUpdate);
        }
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    parseReleasesResponse(data);
    reply->deleteLater();
}

void UpdateManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
        setDownloadProgress(progress);
        setUpdateStatusText(QString("下载中... %1%").arg(progress));
    }
}

void UpdateManager::onDownloadFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString error = QString("下载失败: %1").arg(reply->errorString());
        qCWarning(updateManager) << error;
        setUpdateStatusText(error);
        setStatus(DownloadFailed);
        emit downloadFailed(error);
        reply->deleteLater();
        return;
    }

    // 保存文件
    QString fileName = m_releaseInfo.fileName;
    if (fileName.isEmpty()) {
        // 从URL中提取文件名
        QUrl url(reply->url());
        fileName = url.fileName();
        if (fileName.isEmpty()) {
            fileName = "update.exe"; // 默认文件名
        }
    }

    QString filePath = getDownloadPath() + "/" + fileName;
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        QString error = QString("无法保存文件: %1").arg(file.errorString());
        qCWarning(updateManager) << error;
        setUpdateStatusText(error);
        setStatus(DownloadFailed);
        emit downloadFailed(error);
        reply->deleteLater();
        return;
    }

    file.write(reply->readAll());
    file.close();

    setUpdateStatusText("下载完成，准备安装");
    setStatus(InstallReady);
    setDownloadProgress(100);

    emit downloadCompleted(filePath);
    reply->deleteLater();
}

void UpdateManager::onNetworkError(QNetworkReply::NetworkError error)
{
    qCWarning(updateManager) << "Network error:" << error;
}

int UpdateManager::compareVersions(const QString &v1, const QString &v2)
{
    QString num1 = extractVersionNumber(v1);
    QString num2 = extractVersionNumber(v2);

    QStringList parts1 = num1.split('.');
    QStringList parts2 = num2.split('.');

    // 补齐版本号位数
    while (parts1.size() < 2) parts1.append("0");
    while (parts2.size() < 2) parts2.append("0");

    for (int i = 0; i < 2; ++i) {
        int num1_part = parts1[i].toInt();
        int num2_part = parts2[i].toInt();

        if (num1_part < num2_part) return -1;
        if (num1_part > num2_part) return 1;
    }

    return 0; // 版本相同
}

QString UpdateManager::extractVersionNumber(const QString &versionTag)
{
    // 移除 'v' 前缀并提取版本号
    QString version = versionTag.trimmed();
    if (version.startsWith('v', Qt::CaseInsensitive)) {
        version = version.mid(1);
    }

    // 使用正则表达式提取版本号
    QRegularExpression regex(R"((\d+)\.(\d+)(?:\.(\d+))?)");
    QRegularExpressionMatch match = regex.match(version);

    if (match.hasMatch()) {
        QString major = match.captured(1);
        QString minor = match.captured(2);
        QString patch = match.captured(3);

        // 按照规则：最小版本号两位数 (v0.1.00)
        if (patch.isEmpty()) {
            return QString("%1.%2").arg(major).arg(minor);
        } else {
            // 确保补丁版本是两位数
            int patchNum = patch.toInt();
            return QString("%1.%2.%3").arg(major).arg(minor).arg(patchNum, 2, 10, QChar('0'));
        }
    }

    return version; // 如果无法解析，返回原始字符串
}

QNetworkReply *UpdateManager::createGetRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Xiaozhi-Client/1.0");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    return m_networkManager->get(request);
}

void UpdateManager::parseReleasesResponse(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("解析响应失败: %1").arg(parseError.errorString());
        qCWarning(updateManager) << error;
        setUpdateStatusText(error);
        setStatus(DownloadFailed);
        emit checkFailed(error);
        return;
    }

    QJsonArray releases = doc.array();
    ReleaseInfo latestRelease;

    // 查找最新的正式版本
    for (const QJsonValue &value : releases) {
        QJsonObject release = value.toObject();

        // 跳过预发布版本
        if (release["prerelease"].toBool()) {
            continue;
        }

        ReleaseInfo info;
        info.tagName = release["tag_name"].toString();
        info.versionName = release["name"].toString();
        info.body = release["body"].toString();
        info.isPrerelease = release["prerelease"].toBool();
        info.publishedAt = QDateTime::fromString(release["published_at"].toString(), Qt::ISODate);

        // 查找下载链接
        QJsonArray assets = release["assets"].toArray();
        for (const QJsonValue &assetValue : assets) {
            QJsonObject asset = assetValue.toObject();
            QString assetName = asset["name"].toString().toLower();

            // 查找Windows安装程序
            if (assetName.endsWith(".exe") && assetName.contains("setup")) {
                info.downloadUrl = asset["browser_download_url"].toString();
                info.fileName = asset["name"].toString();
                info.fileSize = asset["size"].toVariant().toLongLong();
                break;
            }
        }

        if (info.isValid()) {
            latestRelease = info;
            break; // 取第一个正式版本
        }
    }

    if (!latestRelease.isValid()) {
        if (!m_silentMode) {
            setUpdateStatusText("未找到可用版本");
        }
        setStatus(NoUpdate);
        emit noUpdateAvailable();
        return;
    }

    // 比较版本号
    int comparison = compareVersions(m_currentVersion, latestRelease.tagName);

    if (comparison < 0) {
        // 有新版本
        m_latestVersion = latestRelease.tagName;
        m_releaseInfo = latestRelease;

        setUpdateStatusText(QString("发现新版本: %1").arg(latestRelease.tagName));
        setStatus(UpdateAvailable);

        emit updateAvailable(latestRelease);
        emit latestVersionChanged();
        emit releaseInfoChanged();

        qCInfo(updateManager) << "New version available:" << latestRelease.tagName;
    } else {
        // 没有更新
        if (!m_silentMode) {
            setUpdateStatusText("已是最新版本");
        }
        setStatus(NoUpdate);
        emit noUpdateAvailable();

        qCInfo(updateManager) << "No update available, current version is latest";
    }
}

QString UpdateManager::getDownloadPath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/xiaozhi-updates";

    QDir dir;
    if (!dir.exists(path)) {
        dir.mkpath(path);
    }

    return path;
}

void UpdateManager::cleanOldDownloads()
{
    QString downloadPath = getDownloadPath();
    QDir dir(downloadPath);

    QStringList filters;
    filters << "*.exe" << "*.msi" << "*.dmg";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    // 保留最新的3个文件
    for (int i = 3; i < files.size(); ++i) {
        QFile::remove(files[i].absoluteFilePath());
        qCInfo(updateManager) << "Removed old update file:" << files[i].absoluteFilePath();
    }
}

void UpdateManager::setStatus(UpdateStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

void UpdateManager::setUpdateStatusText(const QString &text)
{
    if (m_updateStatusText != text) {
        m_updateStatusText = text;
        emit updateStatusTextChanged();
    }
}

void UpdateManager::setDownloadProgress(int progress)
{
    if (m_downloadProgress != progress) {
        m_downloadProgress = progress;
        emit downloadProgressChanged();
    }
}

} // namespace network
} // namespace xiaozhi