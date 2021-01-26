project(cskburn_usb)

add_library(
    cskburn_usb STATIC
    src/core.c
    src/bootrom.c
    src/burner.c
    src/crc64.c
)

target_include_directories(
    cskburn_usb PUBLIC
    "${cskburn_usb_SOURCE_DIR}/include"
)

target_link_libraries(cskburn_usb portable)

if(WIN32)
    set(LIBUSB_DIR ${CMAKE_SOURCE_DIR}/windows/libusb-1.0.24)
    target_include_directories(cskburn_usb PRIVATE ${LIBUSB_DIR}/include/libusb-1.0)
    target_link_libraries(cskburn_usb ${LIBUSB_DIR}/lib64/libusb-1.0.lib)
else()
    find_package(PkgConfig)
    pkg_search_module(USB REQUIRED IMPORTED_TARGET libusb-1.0)
    target_link_libraries(cskburn_usb PkgConfig::USB)
endif()

install(TARGETS cskburn_usb DESTINATION lib)
install(FILES include/cskburn_usb.h DESTINATION include)
