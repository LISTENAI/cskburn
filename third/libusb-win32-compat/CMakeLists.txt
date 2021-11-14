project(usb-win32-compat)

add_library(
    ${PROJECT_NAME} STATIC
    src/compat.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} usb-win32)
