# 贡献指南

感谢你对 cskburn 项目的关注！本文档提供了参与项目开发的指南。

## 开发环境设置

### 前置要求

#### Windows (MSYS2)
```bash
# 安装 MSYS2，然后在 MinGW64 终端中运行:
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-ninja
pacman -S mingw-w64-x86_64-gcc
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y cmake ninja-build gcc libudev-dev
```

#### macOS
```bash
brew install cmake ninja
xcode-select --install
```

### 克隆仓库

```bash
git clone https://github.com/LISTENAI/cskburn.git
cd cskburn
git submodule update --init --recursive
```

### 构建项目

```bash
# 配置构建
cmake -B build -G Ninja

# 编译 (Release 模式)
cmake --build build --config Release

# 编译 (Debug 模式)
cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

### 运行程序

```bash
# Linux/macOS
./build/cskburn/cskburn --help

# Windows
./build/cskburn/cskburn.exe --help
```

## 代码规范

### C 代码风格

项目使用基于 Google C Style 的代码风格，配置文件为 `.clang-format`。

**格式化代码**:
```bash
# 格式化单个文件
clang-format -i path/to/file.c

# 格式化所有源文件
find . -name "*.c" -o -name "*.h" | grep -v "third/" | xargs clang-format -i
```

**关键规范**:
- 缩进: 4 个空格 (或等效的制表符)
- 每行最多 100 字符
- 函数返回类型单独一行
- 左大括号在同一行 (Linux 风格)
- 指针类型: `type *ptr` (星号靠右)

**示例**:
```c
// 正确
int
cskburn_serial_open(cskburn_serial_device_t **dev, const char *path)
{
    if (dev == NULL || path == NULL) {
        return -1;
    }
    
    // 实现...
    return 0;
}

// 错误
int cskburn_serial_open(cskburn_serial_device_t** dev, const char* path) {  // 返回类型应该单独一行
  if(dev==NULL||path==NULL) {  // 缺少空格
    return -1;
  }
  return 0;
}
```

### 命名约定

#### 函数命名
- 使用 `模块名_动作_对象` 格式
- 全小写，单词用下划线分隔

```c
// 正确
int cskburn_serial_open(...);
void libslip_encode_frame(...);

// 错误
int openSerial(...);  // 驼峰命名
int open(...);        // 名称太通用
```

#### 类型命名
- 结构体类型使用 `_t` 后缀
- 枚举类型使用 `_t` 后缀
- 类型名全小写，单词用下划线分隔

```c
// 正确
typedef struct _serial_device_t serial_device_t;
typedef enum { ... } cskburn_chip_t;

// 错误
typedef struct SerialDevice SerialDevice;  // 驼峰命名
```

#### 常量和宏
- 全大写，单词用下划线分隔
- 使用有意义的前缀避免冲突

```c
// 正确
#define SERIAL_BAUD_RATE_DEFAULT 115200
#define MAX_RETRY_COUNT 3

// 错误
#define baudRate 115200  // 应该全大写
#define MAX 3            // 名称太通用
```

#### 变量命名
- 全小写，单词用下划线分隔
- 使用描述性名称

```c
// 正确
int retry_count = 0;
uint8_t *buffer = NULL;

// 错误
int rc = 0;      // 太简短
int RetryCount;  // 驼峰命名
```

### 注释规范

#### 文件头注释
```c
/**
 * @file cskburn_serial.c
 * @brief CSK 芯片串口烧录实现
 * 
 * 实现了基于 SLIP 协议的串口烧录功能，支持 Castor、Venus、ARCS 系列芯片。
 */
```

#### 函数注释
```c
/**
 * @brief 打开串口烧录设备
 * 
 * 打开指定的串口设备并初始化烧录上下文。
 * 
 * @param[out] dev    设备句柄指针，成功时会被设置
 * @param[in]  path   串口设备路径，如 "/dev/ttyUSB0" 或 "COM3"
 * @param[in]  chip   芯片类型 (CASTOR/VENUS/ARCS)
 * @param[in]  timeout 操作超时时间，单位毫秒，0 表示使用默认值
 * 
 * @return 0 成功，负数表示错误码:
 *         -EINVAL: 参数无效
 *         -ENODEV: 设备不存在
 *         -ENOMEM: 内存不足
 * 
 * @note 调用者需要在使用完毕后调用 cskburn_serial_close() 释放资源
 * 
 * @see cskburn_serial_close()
 */
int
cskburn_serial_open(cskburn_serial_device_t **dev, const char *path,
                    cskburn_serial_chip_t chip, int32_t timeout);
```

#### 代码注释
```c
// 单行注释用于简单说明
int retry = 0;  // 重试次数

/*
 * 多行注释用于复杂逻辑说明
 * 这里实现了自动波特率检测:
 * 1. 发送同步包
 * 2. 等待响应
 * 3. 如果失败，降低波特率重试
 */
```

## 提交规范

### Commit 消息格式

使用约定式提交 (Conventional Commits) 格式:

```
<类型>(<范围>): <简短描述>

<详细描述>

<页脚>
```

#### 类型 (Type)
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式修改 (不影响功能)
- `refactor`: 代码重构
- `test`: 添加或修改测试
- `chore`: 构建过程或辅助工具的变动
- `perf`: 性能优化

#### 示例
```
feat(serial): 添加 CSK6 NAND 烧录支持

实现了 CSK6 芯片的 NAND Flash 烧录功能，包括:
- NAND 初始化
- 分区烧录
- 坏块处理

Closes #123
```

```
fix(usb): 修复设备检测超时问题

在某些 USB 控制器上，设备枚举可能需要更长时间。
将超时时间从 5 秒增加到 10 秒。

Fixes #456
```

### Pull Request 规范

1. **标题**: 使用清晰的标题描述变更
2. **描述**: 提供详细的变更说明
   - 变更的目的
   - 实现方法
   - 测试方法
   - 相关 issue
3. **检查清单**: 确保完成以下项目
   - [ ] 代码已通过 `clang-format` 格式化
   - [ ] 添加了必要的注释
   - [ ] 更新了相关文档
   - [ ] 代码可以正常编译
   - [ ] 功能经过测试
   - [ ] 没有引入新的警告

## 开发流程

### 1. 创建分支

```bash
# 从 master 创建功能分支
git checkout master
git pull origin master
git checkout -b feature/your-feature-name

# 或创建修复分支
git checkout -b fix/issue-number-description
```

### 2. 开发和测试

```bash
# 修改代码...

# 构建项目
cmake --build build

# 运行测试 (当有测试时)
cmake --build build --target test

# 格式化代码
clang-format -i modified_file.c
```

### 3. 提交变更

```bash
# 添加修改的文件
git add path/to/modified/files

# 提交 (使用规范的提交消息)
git commit -m "feat(module): add new feature"

# 如果需要修改上一次提交
git commit --amend
```

### 4. 推送和创建 PR

```bash
# 推送到远程仓库
git push origin feature/your-feature-name

# 在 GitHub 上创建 Pull Request
```

## 测试指南

### 手动测试

#### 串口烧录测试
```bash
# 测试串口连接
./cskburn -s /dev/ttyUSB0 --chip-id

# 测试烧录功能
./cskburn -s /dev/ttyUSB0 -b 1500000 0x0 firmware.bin

# 测试验证功能
./cskburn -s /dev/ttyUSB0 --verify 0x0:1024
```

#### USB 烧录测试
```bash
# 检测设备
./cskburn -u - -c

# 烧录固件
./cskburn -u - 0x0 firmware.bin
```

### 单元测试 (计划中)

```bash
# 运行所有测试
cmake --build build --target test

# 运行特定模块测试
./build/tests/test_slip
./build/tests/test_serial
```

## 调试技巧

### 启用详细日志

```bash
# 使用 -v 参数启用详细日志
./cskburn -v -s /dev/ttyUSB0 0x0 firmware.bin

# 使用 --trace 启用跟踪日志
./cskburn --trace -s /dev/ttyUSB0 0x0 firmware.bin
```

### 使用调试器

```bash
# 使用 GDB
gdb --args ./build-debug/cskburn/cskburn -s /dev/ttyUSB0 0x0 firmware.bin

# 使用 LLDB (macOS)
lldb -- ./build-debug/cskburn/cskburn -s /dev/ttyUSB0 0x0 firmware.bin
```

### 内存检查

```bash
# Linux - 使用 Valgrind
valgrind --leak-check=full ./build-debug/cskburn/cskburn --help

# 使用 AddressSanitizer
cmake -B build-asan -G Ninja -DCMAKE_C_FLAGS="-fsanitize=address -g"
cmake --build build-asan
./build-asan/cskburn/cskburn --help
```

## 常见问题

### Q: 编译时找不到 libusb
**A**: 确保已经正确初始化 git submodules:
```bash
git submodule update --init --recursive
```

### Q: 串口权限问题 (Linux)
**A**: 将当前用户添加到 dialout 组:
```bash
sudo usermod -a -G dialout $USER
# 注销后重新登录生效
```

### Q: 如何添加新的芯片支持?
**A**: 参考 `ARCHITECTURE.md` 中的"扩展性"章节。

## 代码审查清单

在提交 PR 之前，请确保:

- [ ] 代码符合项目编码规范
- [ ] 函数复杂度合理 (< 100 行)
- [ ] 添加了必要的错误处理
- [ ] 资源正确管理 (无内存泄漏)
- [ ] 添加了适当的注释
- [ ] 更新了相关文档
- [ ] 代码通过编译 (无警告)
- [ ] 功能经过测试
- [ ] Commit 消息清晰准确

## 获取帮助

- **Issue 跟踪**: https://github.com/LISTENAI/cskburn/issues
- **讨论**: 在 Issue 中提问或讨论

## 许可证

通过向本项目贡献代码，你同意你的贡献将根据 Apache-2.0 License 授权。
