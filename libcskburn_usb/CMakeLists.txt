project(cskburn_usb)

add_library(
    cskburn_usb STATIC
    src/core.c
)

target_include_directories(
    cskburn_usb PUBLIC
    "${cskburn_usb_SOURCE_DIR}/include"
)

find_package(PkgConfig)
pkg_search_module(USB REQUIRED IMPORTED_TARGET libusb-1.0)
target_link_libraries(cskburn_usb PkgConfig::USB)

install(TARGETS cskburn_usb DESTINATION lib)
install(FILES include/cskburn_usb.h DESTINATION include)
