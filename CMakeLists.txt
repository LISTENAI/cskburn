cmake_minimum_required(VERSION 3.10)

project(CSKBurnKit)
enable_testing()

if(WIN32)
    add_subdirectory(third/libusb-win32)
    add_subdirectory(third/libusb-win32-compat)
else()
    add_subdirectory(third/libusb)
endif()

add_subdirectory(liblog)
add_subdirectory(libportable)
add_subdirectory(libserial)
add_subdirectory(libcskburn_usb)
add_subdirectory(libcskburn_serial)
add_subdirectory(cskburn)

add_subdirectory(unity)
