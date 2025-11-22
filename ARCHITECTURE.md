# cskburn 架构文档

## 项目概述

cskburn 是一个跨平台的聆思 CSK 系列芯片烧录工具，实现了串口 OTA 协议的参考实现。项目采用 C 语言开发，支持 Windows、Linux 和 macOS 平台。

## 架构设计

### 整体架构

项目采用模块化分层架构，分为以下几个层次：

```
┌─────────────────────────────────────────┐
│          cskburn (应用层)                │
│    命令行界面和主要业务逻辑              │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│      烧录协议层 (Protocol Layer)         │
│  ┌─────────────────┐  ┌──────────────┐ │
│  │ libcskburn_usb  │  │ libcskburn_  │ │
│  │   USB烧录协议   │  │   serial     │ │
│  │                 │  │ 串口烧录协议 │ │
│  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│         公共库层 (Common Layer)          │
│  ┌────────┐ ┌────────┐ ┌─────────────┐ │
│  │ libio  │ │ liblog │ │ libportable │ │
│  │ I/O接口│ │ 日志   │ │  跨平台适配 │ │
│  └────────┘ └────────┘ └─────────────┘ │
│  ┌────────┐ ┌────────┐                 │
│  │libserial│ │libslip │                 │
│  │串口通信│ │SLIP协议│                 │
│  └────────┘ └────────┘                 │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│      第三方依赖 (Third Party)            │
│  ┌────────┐ ┌──────────┐               │
│  │ libusb │ │ mbedtls  │               │
│  │USB通信 │ │ 加密算法 │               │
│  └────────┘ └──────────┘               │
└─────────────────────────────────────────┘
```

### 核心模块说明

#### 1. cskburn (应用层)
- **职责**: 命令行解析、用户交互、业务流程控制
- **文件**: `cskburn/src/main.c` 等
- **功能**:
  - 解析命令行参数
  - 管理烧录流程
  - 读取和验证固件文件
  - 处理 Intel HEX 格式

#### 2. libcskburn_serial (串口烧录)
- **职责**: 实现串口烧录协议
- **文件**: `libcskburn_serial/src/`
- **功能**:
  - 串口设备连接和初始化
  - 烧录程序上传
  - Flash 读写操作
  - 芯片 ID 读取
  - 支持 Castor、Venus、ARCS 三种芯片系列

#### 3. libcskburn_usb (USB烧录)
- **职责**: 实现 USB 烧录协议
- **文件**: `libcskburn_usb/src/`
- **功能**:
  - USB 设备发现和连接
  - Bootrom 通信
  - 固件烧录
  - CRC64 校验

#### 4. libserial (串口通信)
- **职责**: 封装跨平台串口操作
- **文件**: `libserial/src/`
- **功能**:
  - 串口打开/关闭
  - 波特率设置
  - 数据收发
  - 平台差异处理 (POSIX/Windows)

#### 5. libslip (SLIP 协议)
- **职责**: 实现 SLIP (Serial Line Internet Protocol) 协议
- **文件**: `libslip/src/slip.c`
- **功能**:
  - 数据帧封装
  - 字符转义处理

#### 6. libio (I/O 抽象)
- **职责**: 提供统一的 I/O 接口
- **文件**: `libio/src/`
- **功能**:
  - 文件系统操作
  - 内存操作

#### 7. liblog (日志系统)
- **职责**: 统一日志管理
- **文件**: `liblog/src/`
- **功能**:
  - 多级别日志 (TRACE/DEBUG/INFO/ERROR)
  - 错误 ID 管理

#### 8. libportable (跨平台适配)
- **职责**: 提供跨平台工具函数
- **文件**: `libportable/src/`
- **功能**:
  - 时间函数
  - 睡眠函数

### 数据流

#### 烧录流程 (Serial)
```
用户输入
    ↓
main.c 解析参数
    ↓
读取固件文件 (read_parts)
    ↓
cskburn_serial_open() - 打开串口
    ↓
cskburn_serial_connect() - 建立连接
    ↓
cskburn_serial_enter() - 进入烧录模式
    ↓
cskburn_serial_write() - 写入数据
    ↓
cskburn_serial_verify() - 验证 (可选)
    ↓
完成
```

## 设计优点

1. **模块化设计**: 清晰的模块分层，职责明确
2. **跨平台支持**: 良好的平台抽象层
3. **协议解耦**: USB 和串口协议独立实现
4. **代码复用**: 公共功能提取为独立库

## 技术债务

1. **测试覆盖**: 缺少单元测试和集成测试
2. **文档不足**: API 文档和注释不够完善
3. **错误处理**: 错误处理模式不够统一
4. **代码复杂度**: main.c 文件过大 (1144 行)，存在超长函数

## 依赖关系

### 外部依赖
- **libusb**: USB 设备通信 (v1.0.24)
- **mbedtls**: MD5 校验算法
- **CMake**: 构建系统 (>= 3.10)
- **Ninja**: 编译工具

### 内部依赖图
```
cskburn
├─→ libcskburn_serial
│   ├─→ libserial
│   ├─→ libslip
│   ├─→ libio
│   ├─→ liblog
│   └─→ libportable
├─→ libcskburn_usb
│   ├─→ libusb
│   ├─→ libio
│   ├─→ liblog
│   └─→ libportable
└─→ mbedtls (MD5)
```

## 构建系统

使用 CMake 构建系统:
- 主 CMakeLists.txt 协调所有子模块
- 每个库有独立的 CMakeLists.txt
- 支持静态库构建
- 使用 Ninja 作为默认生成器

## 平台支持

### Windows
- MSYS2 MinGW64 环境
- 使用 libusb-win32 分支

### Linux/macOS
- 标准 POSIX 环境
- 使用标准 libusb

### Android
- 支持 NDK r22b
- 交叉编译配置

## 扩展性

### 添加新芯片支持
1. 在 `chip_features` 数组添加新芯片定义
2. 准备对应的 burner 二进制文件
3. 更新 CMakeLists.txt 嵌入二进制
4. 实现芯片特定的初始化代码

### 添加新协议
1. 创建新的 libcskburn_xxx 目录
2. 实现协议接口
3. 在 main.c 中集成新协议

## 安全性

- 使用 MD5 进行固件校验
- 串口通信使用 SLIP 协议保证数据完整性
- CRC64 用于 USB 传输校验
