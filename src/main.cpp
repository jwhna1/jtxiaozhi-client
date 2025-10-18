/*
Project: 小智跨平台客户端 (jtxiaozhi-client)
Version: v0.1.0
Author: jtserver团队
Email: jwhna1@gmail.com
Updated: 2025-10-11T15:35:00Z
File: main.cpp
Desc: 程序入口
*/

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QIcon>
#include "version/version_info.h"
#include "models/AppModel.h"
#include "utils/Logger.h"
#include "network/UpdateManager.h"

int main(int argc, char *argv[]) {
    // 启用高DPI缩放
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    
    QGuiApplication app(argc, argv);

    // 设置应用信息
    app.setOrganizationName(xiaozhi::version::AUTHOR);
    app.setOrganizationDomain("jtserver.net");
    app.setApplicationName(xiaozhi::version::PROJECT_NAME_EN);
    app.setApplicationVersion(xiaozhi::version::VERSION);

    // 设置Quick风格
    QQuickStyle::setStyle("Basic");

    // 输出版本信息
    xiaozhi::utils::Logger::instance().info(xiaozhi::version::full_version_string());

    // 创建应用模型
    xiaozhi::models::AppModel appModel;

    // 创建QML引擎
    QQmlApplicationEngine engine;

    // 注册UpdateManager类型到QML
    qmlRegisterType<xiaozhi::network::UpdateManager>("XiaozhiClient", 1, 0, "UpdateManager");

    // 注册AppModel到QML上下文
    engine.rootContext()->setContextProperty("appModel", &appModel);

    // 加载主QML文件
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        xiaozhi::utils::Logger::instance().error("无法加载QML文件");
        return -1;
    }

    xiaozhi::utils::Logger::instance().info(" 应用启动成功");

    return app.exec();
}

