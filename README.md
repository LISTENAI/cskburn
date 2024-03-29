cskburn
==========

[![release][release-img]][release-url] [![downloads][downloads-img]][downloads-url] [![license][license-img]][license-url] [![issues][issues-img]][issues-url] [![stars][stars-img]][stars-url] [![commits][commits-img]][commits-url]

聆思 CSK 系列芯片烧录工具，[串口 OTA 协议](https://docs.listenai.com/AIsolution/dsp/firmware_development/OTA_service#5-ota%E5%8D%8F%E8%AE%AE)的一个跨平台参考实现。

## 使用

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
  -b, --baud
    baud rate used for serial burning (default: 3000000)
  -C, --chip
    chip family, acceptable values: 3/4/6 (default: 4)
  --chip-id
    read unique chip ID
  --verify-all
    verify all partitions after burning
  -n, --nand
    burn to NAND flash (CSK6 only)
  --timeout <ms>
    override timeout for each operation, acceptable values:
    -1: no timeout
     0: use default strategy
    >0: timeout in milliseconds

Advanced operations (serial only):
  --erase <addr:size>
    erase specified flash region
  --erase-all
    erase the entire flash
  --verify <addr:size>
    verify specified flash region

Example:
    cskburn -C 6 -s /dev/cu.usbserial-0001 -b 15000000 --verify-all 0x0 app.bin 0x100000 res.bin
```

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
