# CLAUDE.md

This file provides guidance to AI coding agents when working with code in this repository.

## Project Overview

cskburn is a cross-platform C firmware burning tool for LISTENAI CSK series chips. It implements the serial OTA protocol and supports burning via serial port or USB.

## Build

```bash
cmake -G Ninja -B build
cmake --build build --config Release
```

Requires CMake + Ninja. On Windows, use MSYS2 MinGW64.

Debug tracing options and compile-time flags are defined in the root and per-library `CMakeLists.txt` files.

## Architecture

```
cskburn (CLI executable)
├── libcskburn_serial — Serial OTA burning backend
│   ├── libserial — Cross-platform serial I/O (POSIX / Win32)
│   ├── libslip — SLIP packet framing
│   ├── libio — Abstract reader/writer for firmware data
│   └── mbedtls — MD5 for firmware verification
├── libcskburn_usb — USB burning backend
│   └── libusb — USB device communication
├── libio — File and memory I/O abstractions
├── liblog — Logging
└── libportable — Cross-platform utilities (sleep, monotonic clock)
```

### Serial Protocol Stack

Application commands → `cskburn_serial` (CSK OTA protocol) → `slip` (packet framing) → `serial` (raw I/O) → OS serial drivers

### Key Abstractions

- **reader_t / writer_t** (`libio/include/io.h`): Stream interfaces decoupling firmware source (file/memory) from burning logic
- **chip_features_t** (`cskburn/src/main.c`): Per-chip feature table. Inspect this struct to understand supported chips and their differences.
- **Platform-specific serial**: `libserial/src/` splits implementations by OS, including POSIX/Win32 serial I/O and per-platform baud rate configuration.
- **RTS/DTR pin mapping**: Pin mappings differ across chip families. See `libcskburn_serial/src/core.c` for the relevant logic.

### Embedded Bootloaders

Pre-compiled bootloader binaries (`burner_*.bin`) under `libcskburn_serial/` are loaded into RAM before burning. For external burner loading logic, see the `--burner` argument handling in `cskburn/src/main.c`.

### Third-Party Dependencies

All vendored as git submodules under `third/` (libusb, mbedtls, etc).

### CI

See `.github/workflows/build.yml` for the multi-platform cross-compilation matrix.

## Conventions

- README and code comments are in Chinese.
- Commit messages use Chinese with a component prefix. Refer to `git log` for style.

## Git Commit Requirements

- Use reasonable granularity to auto-commit code changes.
- Always use `git commit -s` to include the current user's Signed-off-by.
- Append a `Co-Authored-By` trailer with your own model name and version (e.g., `Co-Authored-By: Claude Sonnet 4 <noreply@anthropic.com>`).
