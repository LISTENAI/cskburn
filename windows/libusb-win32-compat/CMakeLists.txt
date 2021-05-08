project(usb-win32-compat)

add_library(
    ${PROJECT_NAME} STATIC
    src/compat.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

set(LIBUSB_WIN32_DIR ${CMAKE_SOURCE_DIR}/windows/libusb-win32-1.2.6.0)
target_include_directories(${PROJECT_NAME} PUBLIC ${LIBUSB_WIN32_DIR}/include)
target_link_libraries(${PROJECT_NAME} ${LIBUSB_WIN32_DIR}/lib64/libusb.lib)
