<div align="center">
  
 # 小智跨平台客户端 (jtxiaozhi-client)
</div>

<div align="center">

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS-lightgrey.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.9.2-green.svg)
![Version](https://img.shields.io/badge/Version-v0.1.0-red.svg)

基于虾哥xiaozhi-esp32固件源码灵感编写的现代化跨平台音频通信客户端
## 代码正在整理中，近日将开源发布
[![B站](https://img.shields.io/badge/B站-jtserver团队-ff69b4.svg)](https://space.bilibili.com/298384872)
[![GitHub](https://img.shields.io/badge/GitHub-jwhna1-181717.svg)](https://github.com/jwhna1)

</div>

## 目录

- [项目简介](#-项目简介)
- [主要特性](#-主要特性)
- [技术架构](#技术架构)
- [系统要求](#-系统要求)
- [安装指南](#-安装指南)
- [快速开始](#-快速开始)
- [使用说明](#-使用说明)
- [服务器兼容性](#-服务器兼容性)
- [贡献指南](#-贡献指南)
- [许可证](#-许可证)
- [联系我们](#-联系我们)

## 项目简介

**小智跨平台客户端**是一个基于c++ Qt/QML开发的现代化音频通信客户端，让没有esp32硬件设备想体验小智服务的朋友们有一个便捷的体验方式。本项目受到虾哥xiaozhi-esp32固件源码的启发，提供了直观易用的图形界面和强大的音频处理能力。

### 设计目标

- **现代化界面**：采用类似移动IM应用的无边框设计
- **双协议支持**：MQTT+UDP + WebSocket
- **专业音频处理**：支持OPUS编码
- **多服务器兼容**：支持官方服务器、开源服务器和商业服务器
- **多设备管理**：支持同时管理多个xiaozhi-esp32设备
- **完全免费**：永久免费使用，无任何隐藏费用

## 主要特性

### 用户界面
- **现代化设计**：无边框窗口，支持拖拽和调整大小
- **主题系统**：内置明暗主题，一键切换
- **聊天式界面**：类似微信/QQ的对话体验
- **响应式布局**：适配不同屏幕尺寸

### 网络通信
- **双协议架构**：MQTT+UDP协议 + WebSocket协议 ,双协议支持
- **自动切换**：网络故障时自动切换到备用协议
- **OTA配置**：设备自动从服务器获取连接配置
- **设备隔离**：每个设备独立的网络会话，互不干扰

### 音频处理
- **OPUS编解码**：8-48kHz采样率，60ms帧时长
- **AEC回声消除**：WebRTC AudioProcessing AEC3算法(暂未支持，后续版本添加)
- **音频缓存**：本地音频文件管理和缓存
- **实时通信**：低延迟音频传输和播放

### 设备管理
- **多设备支持**：同时连接和管理多个xiaozhi-esp32设备
- **设备配置**：图形化设备添加、编辑和删除
- **会话管理**：独立的设备会话和聊天记录
- **状态监控**：实时显示设备连接状态

### 数据存储
- **SQLite数据库**：聊天记录和配置信息持久化
- **文件缓存**：音频和图片文件智能缓存管理
- **配置同步**：设备配置信息自动保存和恢复

## 技术架构

### 分层架构设计

```
┌─────────────────────────────────────────┐
│              UI 层 (Qt Quick/QML)       │
│  • 主界面组件  • 主题系统  • 用户交互     │
├─────────────────────────────────────────┤
│            业务层 (C++)                 │
│  • AppModel  • 设备管理  • 消息处理      │
├─────────────────────────────────────────┤
│            网络层 (C++)                  │
│  • MQTT/UDP  • WebSocket  • OTA配置      │
├─────────────────────────────────────────┤
│            音频层 (C++)                  │
│  • OPUS编解码  • AEC处理  • 音频设备管理  │
├─────────────────────────────────────────┤
│            存储层 (C++)                  │
│  • SQLite数据库  • 文件缓存  • 配置管理   │
└─────────────────────────────────────────┘
```

### 核心技术栈

- **编程语言**：C++20 + QML
- **GUI框架**：Qt 6.9.2 (Core, Quick, QuickControls2, Network, Multimedia, Sql, WebSockets)
- **音频处理**：OPUS编解码器，WebRTC AudioProcessing
- **网络通信**：MQTT (Paho), WebSocket, UDP
- **数据存储**：SQLite, JSON (nlohmann)
- **构建系统**：CMake 3.26+, vcpkg包管理
- **加密通信**：OpenSSL

### 项目结构

```
jtxiaozhi-client/
├── src/                          # C++源代码
│   ├── main.cpp                  # 程序入口
│   ├── models/                   # 数据模型层
│   │   ├── AppModel.h/.cpp       # 核心应用模型
│   │   └── ChatMessage.h         # 聊天消息模型
│   ├── network/                  # 网络通信层
│   │   ├── DeviceSession.h/.cpp  # 设备会话管理
│   │   ├── MqttManager.h/.cpp    # MQTT协议管理
│   │   ├── UdpManager.h/.cpp     # UDP音频传输
│   │   ├── WebSocketManager.h/.cpp # WebSocket协议
│   │   └── OtaManager.h/.cpp     # OTA配置管理
│   ├── audio/                    # 音频处理层
│   │   ├── AudioDevice.h/.cpp    # 音频设备
│   │   ├── AudioDeviceManager.h/.cpp # 音频设备管理
│   │   ├── ConversationManager.h/.cpp # 对话管理
│   │   ├── OpusCodec.h/.cpp      # OPUS编解码
│   │   └── AudioEncryptor.h/.cpp # 音频加密
│   ├── storage/                  # 数据存储层
│   │   ├── AppDatabase.h/.cpp    # 应用数据库
│   │   ├── AudioCacheManager.h/.cpp # 音频缓存
│   │   └── ImageCacheManager.h/.cpp # 图片缓存
│   ├── utils/                    # 工具类
│   │   ├── Logger.h/.cpp         # 日志系统
│   │   └── Config.h/.cpp         # 配置管理
│   └── version/                  # 版本信息
│       └── version_info.h/.cpp
├── qml/                          # QML界面文件
│   ├── main.qml                  # 主窗口
│   ├── components/               # UI组件
│   │   ├── SideBar.qml           # 侧边栏
│   │   ├── ChatArea.qml          # 聊天区域
│   │   ├── MessageInput.qml      # 消息输入
│   │   ├── TitleBar.qml          # 标题栏
│   │   ├── DeviceItem.qml        # 设备项
│   │   └── AboutDialog.qml       # 关于对话框
│   └── theme/                    # 主题系统
│       └── Theme.qml             # 主题管理器
├── resources/                    # 资源文件
│   ├── icons/                    # 图标资源
│   └── resources.qrc             # 资源配置文件
├── CMakeLists.txt                # 构建配置
├── vcpkg.json                    # 依赖管理
└── README.md                     # 项目说明
```

## 系统要求

### 最低系统要求

- **操作系统**：
  - Windows 10/11 (x64)
  - macOS 10.15+ (Intel/Apple Silicon)
- **内存**：4GB RAM（推荐8GB+）
- **存储空间**：500MB可用空间
- **网络**：稳定的互联网连接

### 开发环境要求

- **编译器**：
  - MSVC 2022 (Windows)
  - Clang 15+ (macOS)
  - GCC 13+ (Linux)
- **Qt版本**：Qt 6.9.2 或更高版本
- **CMake**：3.26 或更高版本
- **vcpkg**：最新版本（用于依赖管理）

## 安装指南

### 预编译版本安装（推荐）

1. **下载安装包**
   ```bash
   # 从GitHub Releases下载对应平台的安装包
   # Windows: jtxiaozhi-client-v0.1.0-win64.exe
   # macOS: jtxiaozhi-client-v0.1.0-macos.dmg
   ```

2. **安装程序**
   - **Windows**：双击.exe文件，按照安装向导完成安装
   - **macOS**：打开.dmg文件，将应用拖拽到Applications文件夹

3. **启动应用**
   - Windows：开始菜单 → 小智跨平台客户端
   - macOS：Launchpad → 小智跨平台客户端

### 从源码编译安装

#### Windows (MSVC 2022)

```bash
# 1. 克隆仓库
git clone https://github.com/jwhna1/jtxiaozhi-client.git
cd jtxiaozhi-client

# 2. 安装vcpkg依赖
vcpkg install

# 3. 构建项目
mkdir build
cd build
cmake .. -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# 4. 运行程序
.\Release\jtxiaozhi-client.exe
```

#### macOS (Clang)

```bash
# 1. 安装Qt 6.9.2+
brew install qt@6

# 2. 克隆仓库
git clone https://github.com/jwhna1/jtxiaozhi-client.git
cd jtxiaozhi-client

# 3. 安装依赖
vcpkg install

# 4. 构建项目
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)

# 5. 运行程序
./jtxiaozhi-client
```

## 快速开始

### 1. 首次启动

启动应用后，您将看到现代化的主界面：

- **左侧边栏**：设备列表管理区域
- **中央聊天区**：对话和消息显示
- **底部输入区**：文本和语音输入

### 2. 添加xiaozhi-esp32设备

1. 点击侧边栏顶部的 **"+"** 按钮
2. 填写设备信息：
   - **设备名称**：为您的设备起个名字
   - **OTA服务器地址**：xiaozhi-esp32的OTA服务器URL
   - **MAC地址**：设备的物理MAC地址（可选，会自动生成）
3. 点击 **确定** 保存设备

### 3. 连接设备

双击设备列表中的设备，或选中设备后点击连接按钮：

1. **自动获取OTA配置**：程序会自动从OTA服务器获取连接配置
2. **建立MQTT连接**：使用获取的配置连接MQTT服务器
3. **申请UDP音频通道**：建立低延迟音频传输通道
4. **发送Hello消息**：与设备建立会话

### 4. 开始对话

连接成功后，您可以：

- **文字对话**：在底部输入框输入文字，按回车发送
- **语音对话**：点击麦克风按钮开始语音录制
- **图片识别**：点击图片按钮发送图片进行识别
- **测试音频**：发送测试音频验证连接

## 使用说明

### 界面介绍

#### 主界面布局
```
┌──────────────────────────────────────────────────┐
│  标题栏 (可拖拽)                    [_][□][×]     │
├─────────────┬────────────────────────────────────┤
│             │                                    │
│   设备列表   │           聊天区域                  │ 
│             │                                    │
│   • 设备A    │  ┌──────────────────────────────┐ │
│   • 设备B    │  │        聊天消息               │ │
│   • [+]     │  │                               │ │
│             │  │  用户: 你好小智                │ │
│             │  │  小智: 您好！我是小智助手...    │ │
│             │  └──────────────────────────────┘  │
├─────────────┼────────────────────────────────────┤
│             │        消息输入框                 │
│   关于按钮   │   [输入文字...]  [麦克风][摄像头][发送]     
└─────────────┴────────────────────────────────────┘
```

#### 设备状态指示
- **离线**：设备未连接
- **连接中**：正在建立连接
- **在线**：设备已连接并可正常通信
- **对话中**：正在进行语音对话

### 功能详解

#### 设备管理
- **添加设备**：点击"+"按钮，填写设备信息
- **编辑设备**：右键设备 → 编辑，修改设备配置
- **删除设备**：右键设备 → 删除，移除设备配置
- **设备连接**：双击设备自动连接，或右键选择连接

#### 消息类型
- **文本消息**：支持中英文文本发送和接收
- **语音消息**：支持实时语音录制和播放
- **图片消息**：支持图片识别功能
- **系统消息**：连接状态、错误提示等

#### 音频功能
- **语音录制**：按住麦克风按钮录音，松开发送
- **音频播放**：自动播放接收的语音消息
- **音量调节**：在设置中调节播放音量
- **音频设备**：支持选择不同的音频输入输出设备

### 设置选项

#### 通用设置
- **主题切换**：明色/暗色主题
- **语言设置**：中文/英文界面
- **自动连接**：启动时自动连接上次设备

#### 网络设置
- **WebSocket启用**：启用WebSocket协议备用
- **连接超时**：网络连接超时时间
- **重试次数**：连接失败重试次数

#### 音频设置
- **输入设备**：选择麦克风设备
- **输出设备**：选择扬声器设备
- **音频质量**：音频编码质量设置
- **AEC开关**：回声消除功能开关

## 服务器兼容性

### 官方支持的服务器

| 服务器类型 | 支持状态 | 功能说明 | 备注 |
|-----------|---------|---------|------|
| **虾哥官方服务器** | 完全支持 | 所有功能 | 推荐使用 |
| **xinnan-tech开源服务器** | 基础支持 | 连接和对话 | 开源免费 |
| **jtxiaozhi-server商用版** | 高级支持 | 优化功能 | 商业授权 |

### 其他服务器兼容性

对于其他xiaozhi-esp32兼容服务器：

- **协议兼容**：基于标准MQTT+UDP协议
- **功能支持**：基础连接和文本对话功能
- **测试状态**：暂未全面测试
- **问题反馈**：遇到兼容性问题可提交issue，我们会尽快适配

### 服务器配置要求

#### 基本要求
- **MQTT服务器**：支持MQTT 3.1+协议
- **UDP音频端口**：支持UDP数据包传输
- **OTA配置接口**：提供设备配置获取接口
- **WebSocket支持**：可选的WebSocket协议

## 贡献指南

我们欢迎所有形式的贡献！

### 贡献方式

#### 报告问题
- 使用GitHub Issues报告bug
- 提供详细的复现步骤和环境信息
- 包含相关的错误日志和截图

#### 功能建议
- 在Issues中提出新功能建议
- 详细描述功能需求和使用场景
- 讨论实现方案和技术细节


#### 提交规范
- 使用清晰的提交信息
- 一次提交只做一件事
- 提交前确保代码能正常编译
- 包含相关的测试用例

### 开发环境搭建

```bash
# 1. 克隆仓库
git clone https://github.com/jwhna1/jtxiaozhi-client.git
cd jtxiaozhi-client

# 2. 安装开发依赖
vcpkg install qt6 opus nlohmann-json openssl paho-mqttpp3

# 3. 配置开发环境
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 4. 编译项目
cmake --build . --config Debug

# 5. 运行测试
.\Debug\jtxiaozhi-client.exe
```

## 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

### 许可证摘要

**允许**：
- 商业使用
- 修改
- 分发
- 私人使用

**禁止**：
- 侵权行为
- 追究责任

**要求**：
- 包含许可证和版权声明

## 联系我们

### 免费声明

> **重要提醒**：本程序为免费使用，交流无套路，任何向您收取费用的行为与本团队无关。我们承诺软件永久免费，不会以任何形式收取使用费用。如遇到有人收费，请警惕并向我们举报。

### 联系方式

| 联系方式 | 详情 |
|---------|------|
| **开发团队** | jtserver团队 |
| **商务邮箱** | jwhna1@gmail.com |
| **技术支持** | QQ: 7280051 |
| **微信客服** | cxshow066 |

### 社交媒体

| 平台 | 地址 | 内容 |
|------|------|------|
| **GitHub** | [github.com/jwhna1](https://github.com/jwhna1) | 源码仓库 |
| **B站** | [space.bilibili.com/298384872](https://space.bilibili.com/298384872) | 视频教程 |
| **项目主页** | [jtxiaozhi-client](https://github.com/jwhna1/jtxiaozhi-client) | 项目主页 |

### 参与项目

我们鼓励社区参与！您可以通过以下方式参与：

- **报告问题**：发现bug请及时反馈
- **提出建议**：功能改进和新的想法
- **完善文档**：改进使用说明和开发文档
- **代码贡献**：提交代码和功能实现
- **翻译**：帮助翻译界面到其他语言
- **推广**：向朋友推荐本项目

---

<div align="center">

**感谢您使用小智跨平台客户端！**

如果这个项目对您有帮助，请给我们一个 Star！

Made with by [jtserver团队](https://github.com/jwhna1)

</div>
