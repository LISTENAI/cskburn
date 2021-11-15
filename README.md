cskburn
==========

[![release][release-img]][release-url] [![downloads][downloads-img]][downloads-url] [![license][license-img]][license-url] [![issues][issues-img]][issues-url] [![stars][stars-img]][stars-url] [![commits][commits-img]][commits-url]

聆思 CSK 系列芯片烧录工具，[串口 OTA 协议](https://docs.listenai.com/AIsolution/dsp/firmware_development/OTA_service#5-ota%E5%8D%8F%E8%AE%AE)的一个跨平台参考实现。

## 使用

```
用法: cskburn [<选项>] <地址1> <文件1> [<地址2> <文件2>...]

烧录选项:
  -u, --usb (-|<总线>:<设备>)		使用指定 USB 设备烧录。传 - 表示自动选取第一个 CSK 设备
  -s, --serial <端口>			使用指定串口设备 (如 /dev/cu.usbserial-0001) 烧录

其它选项:
  -h, --help				显示帮助
  -V, --version				显示版本号
  -v, --verbose				显示详细日志
  -w, --wait				等待设备插入，并自动开始烧录
  -b, --baud				串口烧录所使用的波特率 (默认为 3000000)
  -R, --repeat				循环等待设备插入，并自动开始烧录
  -c, --check				检查设备是否插入 (不进行烧录)
  -C, --chip				指定芯片系列，支持3/4/6，默认为 4

用例:
  cskburn -w 0x0 flashboot.bin 0x10000 master.bin 0x100000 respack.bin
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
