cmake_minimum_required(VERSION 3.10)

project(CSKBurnKit)

if(WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/source-charset:utf-8>")
    add_compile_options("$<$<CONFIG:Release>:/MT>")
    add_subdirectory(windows/getopt)

    if(NOT WITHOUT_USB)
        add_subdirectory(windows/libusb-win32-compat)
    endif()
endif()

add_subdirectory(liblog)
add_subdirectory(libportable)
add_subdirectory(libserial)
if(NOT WITHOUT_USB)
    add_subdirectory(libcskburn_usb)
endif()
add_subdirectory(libcskburn_serial)
add_subdirectory(cskburn)
