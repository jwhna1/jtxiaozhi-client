/*
Project: jtxiaozhi-client
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-18T11:10:00Z
File: UpdateManager.h
Desc: 版本升级管理器 - 检测、下载和安装新版本
*/

#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>

namespace xiaozhi {
namespace network {

/**
 * @brief 版本信息结构
 */
struct ReleaseInfo {
    QString tagName;          // 版本号 (v0.1.0)
    QString versionName;      // 版本名称
    QString body;             // 更新内容
    bool isPrerelease;        // 是否为预发布版本
    QDateTime publishedAt;    // 发布时间
    QString downloadUrl;      // 下载链接
    QString fileName;         // 文件名
    qint64 fileSize;          // 文件大小

    bool isValid() const {
        return !tagName.isEmpty() && !downloadUrl.isEmpty();
    }
};

/**
 * @brief 版本升级管理器
 */
class UpdateManager : public QObject {
    Q_OBJECT

public:
    enum UpdateStatus {
        NoUpdate = 0,          // 无更新
        UpdateAvailable = 1,   // 有更新
        Checking = 2,          // 检查中
        Downloading = 3,       // 下载中
        DownloadFailed = 4,    // 下载失败
        InstallReady = 5       // 安装就绪
    };
    Q_ENUM(UpdateStatus)

    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager();

    // 属性访问器
    UpdateStatus status() const { return m_status; }
    QString currentVersion() const { return m_currentVersion; }
    QString latestVersion() const { return m_latestVersion; }
    QString updateStatusText() const { return m_updateStatusText; }
    int downloadProgress() const { return m_downloadProgress; }
    ReleaseInfo releaseInfo() const { return m_releaseInfo; }
    bool autoCheckEnabled() const { return m_autoCheckEnabled; }

public slots:
    // 检查更新
    void checkForUpdates(bool silent = false);

    // 下载更新
    void downloadUpdate();

    // 安装更新
    void installUpdate();
    void installUpdate(const QString &filePath);

    // 设置自动检查
    void setAutoCheckEnabled(bool enabled);

    // 设置当前版本
    void setCurrentVersion(const QString &version);

signals:
    // 状态变化信号
    void statusChanged();
    void currentVersionChanged();
    void latestVersionChanged();
    void updateStatusTextChanged();
    void downloadProgressChanged();
    void releaseInfoChanged();
    void autoCheckEnabledChanged();

    // 更新结果信号
    void updateAvailable(const ReleaseInfo &info);
    void noUpdateAvailable();
    void downloadCompleted(const QString &filePath);
    void downloadFailed(const QString &error);
    void checkFailed(const QString &error);

private slots:
    void onCheckFinished(QNetworkReply *reply);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(QNetworkReply *reply);
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    // 版本号比较
    int compareVersions(const QString &v1, const QString &v2);
    QString extractVersionNumber(const QString &versionTag);

    // 网络请求
    QNetworkReply *createGetRequest(const QUrl &url);
    void parseReleasesResponse(const QByteArray &data);

    // 文件操作
    QString getDownloadPath();
    void cleanOldDownloads();

    // 状态管理
    void setStatus(UpdateStatus status);
    void setUpdateStatusText(const QString &text);
    void setDownloadProgress(int progress);

private:
    QNetworkAccessManager *m_networkManager;
    QTimer *m_autoCheckTimer;

    UpdateStatus m_status;
    QString m_currentVersion;
    QString m_latestVersion;
    QString m_updateStatusText;
    int m_downloadProgress;
    ReleaseInfo m_releaseInfo;
    bool m_autoCheckEnabled;
    bool m_silentMode;

    static const QString GITHUB_API_URL;
    static const QString GITHUB_REPO_URL;
    static const int AUTO_CHECK_INTERVAL_MS;
};

} // namespace network
} // namespace xiaozhi

#endif // UPDATE_MANAGER_H