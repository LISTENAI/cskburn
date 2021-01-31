project(usb-win32-compat)

add_library(
    usb-win32-compat STATIC
    src/compat.c
)

target_include_directories(
    usb-win32-compat PUBLIC
    "${usb-win32-compat_SOURCE_DIR}/include"
)

set(LIBUSB_WIN32_DIR ${CMAKE_SOURCE_DIR}/windows/libusb-win32-1.2.6.0)
target_include_directories(usb-win32-compat PUBLIC ${LIBUSB_WIN32_DIR}/include)
target_link_libraries(usb-win32-compat ${LIBUSB_WIN32_DIR}/lib64/libusb.lib)
