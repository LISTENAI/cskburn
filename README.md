cskburn
==========

[![release][release-img]][release-url] [![downloads][downloads-img]][downloads-url] [![license][license-img]][license-url] [![issues][issues-img]][issues-url] [![stars][stars-img]][stars-url] [![commits][commits-img]][commits-url]

聆思 CSK 系列芯片烧录工具，[串口 OTA 协议](https://docs.listenai.com/AIsolution/dsp/firmware_development/OTA_service#5-ota%E5%8D%8F%E8%AE%AE)的一个跨平台参考实现。

## 安装

### Homebrew（macOS）

```sh
brew install listenai/tap/cskburn
```

### 其他平台

前往 [Releases](https://github.com/LISTENAI/cskburn/releases) 下载对应平台的预编译包，解压后将 `cskburn` 放入 `PATH`。

## 使用

### AI Skill

太长不想看？让你的 AI 编程助手代劳：

```bash
npx skills add LISTENAI/cskburn
```

### 详细参数

```
Usage: cskburn [<options>] <addr1> <file1> [<addr2> <file2> ...]
       cskburn [<options>] --verify <addr1>:<size1> [--verify <addr2>:<size2> ...]
       cskburn [<options>] --erase <addr1>:<size1> [--erase <addr2>:<size2> ...]

Burning options:
  -u, --usb (-|<bus>:<device>)
    burn with specified USB device. Pass "-" to select first CSK device automatically
  -s, --serial <port>
    burn with specified serial device (e.g. /dev/cu.usbserial-0001)

Common options:
  -h, --help
    show help
  -V, --version
    show version
  -v, --verbose
    print verbose log

USB burning options:
  -w, --wait
    wait for device presence and start burning
  -R, --repeat
    repeatly wait for device presence and start burning
  -c, --check
    check for device presence (without burning)

Serial burning options:
  -b, --baud <rate>
    baud rate used for serial burning (default: 3000000)
  -C, --chip <family>
    chip family (default: castor), acceptable values:
      castor: Castor (CSK3/CSK4)
      venus: Venus (CSK6)
      arcs: Arcs (LS26)
      arcs_dual: Arcs Dual Flash (LS26)
      venusa: VenusA (CSK7)
  --chip-id
    read unique chip ID
  --verify-all
    verify all partitions after burning
  -n, --nand
    burn to NAND flash (CSK6 only)
  --emmc
    burn to eMMC (ARCS only)
  --probe-timeout <ms>
    timeout for probing device (default: 10000 ms)
  --reset-attempts <n>
    number of attempts to reset device during probing (default: 4)
  --reset-delay <ms>
    delay in milliseconds the reset line is held low (default: 500 ms)
  --reset-strategy <name>
    reset strategy for entering burn mode (default: auto), acceptable values:
      auto: auto-select by chip; for LS26 alternates dtr-boot and dual-npn
      dtr-boot: DTR -> BOOT, RTS -> RESET (LS26 ARCS-MINI)
      rts-boot: RTS -> BOOT, DTR -> RESET (CSK4/CSK6 default)
      rts-boot-inv: rts-boot with BOOT active high (equivalent to --update-high)
      dual-npn: cross-wired NPN pair S8050 (LS26 ARCS-EVB)
  --timeout <ms>
    override timeout for each operation (default: 0), acceptable values:
      -1: no timeout
       0: use default strategy
       n: timeout after n milliseconds (n > 0)
    this option does not affect the timeout of probing device, use --probe-timeout if needed

Advanced operations (serial only):
  --flash-index <index>
    select flash 0 or 1 (arcs_dual only; default: 0)
  --erase <addr:size>
    erase specified flash region
  --erase-all
    erase the entire flash
  --lock
    lock the selected flash after successful operations
  --unlock
    unlock the selected flash before operations
  --verify <addr:size>
    verify specified flash region

Example:
    cskburn -C venus -s /dev/cu.usbserial-0001 -b 1500000 --verify-all 0x0 app.bin 0x100000 res.bin
```

### ARCS 双 Flash 与 eMMC

`arcs` 使用单 Flash loader，`arcs_dual` 使用双 Flash loader。双 Flash
loader 将两个 16 MB Flash 作为独立器件操作，地址相对于当前选择的器件：

```sh
cskburn -C arcs_dual -s /dev/ttyACM0 --flash-index 0 0x0 flash0.bin
cskburn -C arcs_dual -s /dev/ttyACM0 --flash-index 1 0x0 flash1.bin
```

选择在擦除、写入、读取、MD5 校验和锁解锁期间保持不变。操作结束或失败时，
host 会恢复 loader 的 32 MB 映射视图。`--lock` 只在全部操作和校验成功后执行，
`--unlock` 在其他 Flash 操作前执行。

ARCS eMMC 使用 `--emmc` 选择，读写、区域擦除和 MD5 校验复用相同的地址参数：

```sh
cskburn -C arcs -s /dev/ttyACM0 --emmc --verify-all 0x0 image.bin
```

当前 eMMC wire protocol 使用 32 位地址和长度，因此 host 拒绝跨越 4 GiB
可寻址边界的单次操作；设备容量检测仍使用 64 位计算。

这些功能由所选嵌入 loader 的 capability 控制；不支持的芯片组合会在打开串口前
返回结构化 `E1003` 错误。

## 编译

```sh
cmake -G Ninja -B build
cmake --build build --config Release
```

### 编译环境

#### Windows

* MSYS2 MinGW64
* CMake
* Ninja

#### Linux/macOS

* CMake
* Ninja

#### Android

* Android NDK r22b
* CMake
* Ninja

## 协议

[Apache-2.0 License](LICENSE)

[release-img]: https://img.shields.io/github/v/release/LISTENAI/cskburn?style=flat-square
[release-url]: https://github.com/LISTENAI/cskburn/releases/latest
[downloads-img]: https://img.shields.io/github/downloads/LISTENAI/cskburn/total?style=flat-square
[downloads-url]: https://github.com/LISTENAI/cskburn/releases
[license-img]: https://img.shields.io/github/license/LISTENAI/cskburn?style=flat-square
[license-url]: LICENSE
[issues-img]: https://img.shields.io/github/issues/LISTENAI/cskburn?style=flat-square
[issues-url]: https://github.com/LISTENAI/cskburn/issues
[stars-img]: https://img.shields.io/github/stars/LISTENAI/cskburn?style=flat-square
[stars-url]: https://github.com/LISTENAI/cskburn/stargazers
[commits-img]: https://img.shields.io/github/last-commit/LISTENAI/cskburn?style=flat-square
[commits-url]: https://github.com/LISTENAI/cskburn/commits/master
