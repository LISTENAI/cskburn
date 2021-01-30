cmake_minimum_required(VERSION 3.10)

project(CSKBurnKit)

if(WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/source-charset:utf-8>")
    add_subdirectory(windows/getopt)
    add_subdirectory(windows/libusb-win32-compat)
endif()

add_subdirectory(libportable)
add_subdirectory(libcskburn_usb)
add_subdirectory(cskburn)
