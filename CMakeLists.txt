cmake_minimum_required(VERSION 3.10)

project(CSKBurnKit)
enable_testing()

set(CMAKE_C_STANDARD 11)
if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

if(WIN32)
    add_subdirectory(third/libusb-win32)
    add_subdirectory(third/libusb-win32-compat)
else()
    add_subdirectory(third/libusb)
endif()

add_subdirectory(third/mbedtls)

add_subdirectory(liberrors)
add_subdirectory(libio)
add_subdirectory(liblog)
add_subdirectory(libportable)
add_subdirectory(libserial)
add_subdirectory(libslip)
add_subdirectory(libcskburn_usb)
add_subdirectory(libcskburn_serial)
add_subdirectory(cskburn)
