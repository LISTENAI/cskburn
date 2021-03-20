cskburn
==========

跨平台的烧录工具。

## 使用

```
用法: cskburn [<选项>] <地址1> <文件1> [<地址2> <文件2>...]

选项:
  -h, --help				显示帮助
  -V, --version				显示版本号
  -w, --wait				等待设备插入，并自动开始烧录
  -R, --repeat				循环等待设备插入，并自动开始烧录
  -c, --check				检查设备是否插入 (不进行烧录)

用例:
  cskburn -w 0x0 flashboot.bin 0x10000 master.bin 0x100000 respack.bin
```
