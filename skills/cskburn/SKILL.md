---
name: cskburn
description: >
  Use cskburn CLI to flash firmware onto LISTENAI CSK series chips (Venus/CSK6, ARCS/LS26) via serial port.
  Trigger this skill whenever the user mentions cskburn, flashing firmware to CSK chips, burning images to
  LISTENAI devices, serial flashing, or troubleshooting burn/flash failures on CSK hardware.
  Also trigger when the user says "烧录", "刷固件", "烧 flash", "flash 固件", or references cskburn errors
  or flash-related issues on LISTENAI/CSK/Venus/ARCS/LS26 devices — even if they don't mention cskburn by name.
---

# cskburn — CSK 芯片烧录工具

`cskburn` 是聆思 CSK 系列芯片的命令行烧录工具，实现了串口 OTA 协议的跨平台参考实现。

## 支持的芯片系列

| `--chip` 值 | 芯片型号 | 芯片系列 |
|---|---|---|
| `venus` | CSK6 | Venus |
| `arcs` | LS26 | ARCS |

如果从项目上下文无法推断芯片系列，必须询问用户。

## 命令格式

### 烧录（核心操作）

```
cskburn -C <chip> -s <serial_port> -b <baud_rate> --verify-all <addr1> <file1> [<addr2> <file2> ...]
```

**始终默认带上 `--verify-all`**，除非用户明确要求跳过校验。

### 校验

```
cskburn -C <chip> -s <serial_port> --verify <addr>:<size> [--verify <addr>:<size> ...]
```

### 擦除

```
cskburn -C <chip> -s <serial_port> --erase <addr>:<size> [--erase <addr>:<size> ...]
cskburn -C <chip> -s <serial_port> --erase-all
```

### 读取芯片 ID

```
cskburn -C <chip> -s <serial_port> --chip-id
```

## 串口接线要求

烧录除了 TXD/RXD 之外，还**必须**连接 DTR 和 RTS 线。它们负责自动拉低 BOOT 引脚并复位芯片，是进入烧录模式的必要条件。

**DTR/RTS 引脚映射因芯片和板型而异：**

| 芯片系列 / 板型 | RTS 接 | DTR 接 |
|---|---|---|
| Venus (CSK6) | BOOT | RESET |
| ARCS / LS26 ARCS-MINI（直连）| RESET | BOOT |
| ARCS / LS26 ARCS-EVB（差分）| — | — |

ARCS-EVB 用交叉耦合的 NPN 三极管对（S8050），DTR/RTS 差分驱动 PRST 和 RXD，不是直接连到复位/BOOT 引脚。cskburn 默认的 `--reset-strategy auto` 会在 LS26 上自动在 ARCS-MINI 和 ARCS-EVB 两种电路之间切换重试，一般不需要手动指定。

> **这是 probe 超时最常见的原因。** 遇到 probe 超时时，首先检查 DTR/RTS 是否正确连接，以及引脚映射是否与芯片系列匹配。

## 关键参数

### 串口 (`-s, --serial <port>`)

指定串口设备路径。典型值：

- Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`
- macOS: `/dev/cu.usbserial-*`, `/dev/cu.usbmodem*`, `/dev/cu.wchusbserial*` 等（前缀取决于 USB-串口芯片型号）
- Windows (MSYS2): `COM3` 等

**发现串口**：

```bash
# Linux
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null

# macOS — 列出所有串口设备，前缀因芯片而异
ls /dev/cu.* 2>/dev/null | grep -v Bluetooth
```

如果有多个串口设备，列出给用户选择。

### 波特率 (`-b, --baud <rate>`)

- cskburn 默认值为 `3000000`，但并非所有串口芯片都能稳定支持
- `1500000` 是大多数串口芯片都能支持的保险值
- 个别项目可以用到 `6000000`
- **不要硬编码默认值**——从项目上下文（README、构建脚本、Makefile 等）推断，推断不出时用 `1500000` 作为安全起点

### 芯片系列 (`-C, --chip <family>`)

从项目上下文推断芯片系列的线索：

- 项目 README 或文档中提到的芯片型号（CSK6xxx → venus，LS26xxx → arcs）
- 链接脚本 (`.ld`) 中的内存布局
- SDK 或工具链名称（如包含 venus、arcs 等关键字）
- 构建系统配置文件

推断不出时，**必须询问用户**。

### 烧录地址与文件

烧录命令需要 `<addr> <file>` 对，地址为十六进制（如 `0x0`、`0x100000`）。

**推断来源**（按优先级）：

1. 用户直接指定
2. 项目的分区配置文件或链接脚本
3. 项目 README 中的烧录说明
4. 项目构建产物目录中的约定文件名

如果无法确定地址，**必须询问用户**，不要猜测——错误的地址会导致设备变砖。

### 其他常用选项

| 选项 | 说明 |
|---|---|
| `--verify-all` | 烧录后校验所有分区（默认应带上） |
| `-v, --verbose` | 输出详细日志（排查问题时建议开启） |
| `--chip-id` | 读取芯片唯一 ID |

## Agentic 执行流程

在 Claude Code 中执行烧录时，遵循以下流程：

### 1. 确认环境

**检查 cskburn 是否可用，不可用则自动下载：**

```bash
which cskburn || echo "cskburn not found, will download"
```

如果 cskburn 不在 PATH 中，从 GitHub Releases 自动下载最新版本：

```bash
# 检测平台并下载对应二进制
OS="$(uname -s)"
ARCH="$(uname -m)"

case "${OS}-${ARCH}" in
  Linux-x86_64)   ASSET="cskburn-linux-x64.tar.xz" ;;
  Linux-aarch64)  ASSET="cskburn-linux-arm64.tar.xz" ;;
  Linux-armv7l)   ASSET="cskburn-linux-arm.tar.xz" ;;
  Darwin-x86_64)  ASSET="cskburn-darwin-x64.tar.xz" ;;
  Darwin-arm64)   ASSET="cskburn-darwin-arm64.tar.xz" ;;
  *)              echo "Unsupported platform: ${OS}-${ARCH}"; exit 1 ;;
esac

curl -fSL "https://github.com/LISTENAI/cskburn/releases/latest/download/${ASSET}" -o /tmp/cskburn.tar.xz
mkdir -p /tmp/cskburn-bin
tar xf /tmp/cskburn.tar.xz -C /tmp/cskburn-bin
chmod +x /tmp/cskburn-bin/cskburn
# 使用 /tmp/cskburn-bin/cskburn 作为 cskburn 路径，或移到 PATH 中
```

> Windows 下的 asset 名为 `cskburn-win32-x64.zip`，需用 `unzip` 解压。

**探测串口设备：**

```bash
# Linux
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null

# macOS — 串口设备前缀因 USB-串口芯片而异（cu.usbserial、cu.usbmodem、cu.wchusbserial 等）
ls /dev/cu.* 2>/dev/null | grep -v Bluetooth
```

macOS 上不要假设串口前缀，直接列出所有 `/dev/cu.*` 设备（排除 Bluetooth）让用户选择。

### 2. 收集烧录参数

从对话和项目上下文中确定：

- [ ] 芯片系列（`--chip` 值）
- [ ] 串口设备路径
- [ ] 波特率
- [ ] 烧录地址和固件文件路径（确认文件存在）

**任何不确定的参数都必须向用户确认，尤其是烧录地址。**

### 3. 执行烧录

```bash
# 先确认固件文件存在
ls -la <file1> <file2> ...

# 执行烧录（默认带 --verify-all）
cskburn -C <chip> -s <port> -b <baud> --verify-all <addr1> <file1> [<addr2> <file2> ...]
```

### 4. 检查结果

- 烧录成功：cskburn 退出码为 0
- 烧录失败：分析 stderr 输出，参考下方故障排查

## 故障排查

cskburn 的错误输出统一形如 `ERROR [E####]: <说明>[: <上下文>]`，方括号里的编号（如 `E3001`）对应一种确定的根因。项目根目录的 `TROUBLESHOOTING.md` 有完整的编号速查表，下面只列最常见的几类。

### 串口连接失败（`E3xxx`）

| 编号 | 含义 | 排查 |
|---|---|---|
| `E3001` | 串口不存在 | 检查设备是否已插；Linux `ls /dev/ttyUSB* /dev/ttyACM*`，macOS `ls /dev/cu.* \| grep -v Bluetooth`；确认驱动已装 |
| `E3002` | 没权限访问 | Linux：`sudo usermod -aG dialout $USER` 后重新登录；临时用 `sudo chmod 666 /dev/ttyUSB0` |
| `E3003` | 串口被占用 | `lsof /dev/ttyUSB0` 查占用进程；关掉串口终端/IDE 监视器/其他 cskburn 实例 |
| `E3004` | 打开失败（其他原因） | 换 USB 口、换线、重插；确认用的是数据线而非充电线 |

### 连不上芯片（`E4xxx`）

**症状**：`E4001` — Device did not respond after reset

按优先级排查：

1. **DTR/RTS 接线**（最常见）：两根线都要接上，且引脚映射与芯片匹配——Venus 是 RTS→BOOT、DTR→RESET；ARCS 直连板相反。
2. **芯片系列选对**：`-C venus` vs `-C arcs`，选错会导致握手协议对不上。
3. **TX/RX 接线**：是否交叉连接，GND 是否接好。
4. **供电**：确认开发板供电充足，尤其是外接模组。
5. **手动进下载模式**：按住 BOOT 再按一下复位。
6. **特殊复位电路**：auto 模式会覆盖 ARCS-MINI 和 ARCS-EVB 两种；如果仍失败可用 `--reset-strategy <name>` 显式指定：`dtr-boot`（ARCS-MINI）、`dual-npn`（ARCS-EVB）、`rts-boot-inv`（反相 BOOT，等同旧的 `--update-high`）。

### 烧录中途失败、波特率问题（`E5xxx` / `E7xxx`）

**典型编号**：

- `E5001` / `E5005` — 切换波特率被设备拒绝
- `E5002` / `E5006` — 切换波特率后连不上
- `E7003` — 写 flash 中途失败

**核心排查**：**降低波特率**。从当前值降到 `1500000`，还不行就 `921600`、`115200`。

USB-串口芯片对最高波特率有限制：

- CP2102: 最高 1Mbps
- CH340: 最高 2Mbps
- FT232R: 最高 3Mbps
- CP2104/FT232H: 更高

另外检查 USB 线质量和长度，避开 USB Hub 直连主板 USB 口。

### 校验失败（`E8003`）

**症状**：`E8003` — Verification failed: MD5 does not match

加了 `--verify-all` 时，烧完后设备端 MD5 与源文件不一致，说明写入过程有数据错误：

1. 先重新烧一次，看是否偶发。
2. 稳定复现时多半是 flash 坏块或电源不稳，需硬件层面排查。
