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
target_link_libraries(${PROJECT_NAME} io)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} usb-win32-compat)
else()
    target_link_libraries(${PROJECT_NAME} usb)
endif()
