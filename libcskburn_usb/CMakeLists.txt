project(cskburn_usb)

add_library(
    ${PROJECT_NAME} STATIC
    src/core.c
    src/bootrom.c
    src/burner_img.c
    src/burner.c
    src/crc64.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} portable)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} usb-win32-compat)
else()
    find_package(PkgConfig)
    pkg_search_module(USB REQUIRED IMPORTED_TARGET libusb-1.0)
    target_link_libraries(${PROJECT_NAME} PkgConfig::USB)

    target_compile_options(${PROJECT_NAME} PRIVATE -DARCH_LINUX)
endif()
