cskburn
==========

跨平台的烧录工具。

## 编译环境

Windows:

* Visual Studio 16 2019
* CMake

Linux/macOS:

* GCC
* CMake
* libusb

## 使用

```
用法: cskburn [<选项>] <burner> <地址1> <文件1> [<地址2> <文件2>...]

选项:
  -h, --help				显示帮助
  -V, --version				显示版本号
  -w, --wait				等待设备插入，并自动开始烧录

用例:
  cskburn -w burner.img 0x0 flashboot.bin 0x10000 master.bin 0x100000 respack.bin
```
