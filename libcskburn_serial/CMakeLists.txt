project(cskburn_serial)

add_library(
    ${PROJECT_NAME} STATIC
    src/core.c
    src/cmd.c
    src/slip.c
    src/burner_img.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} portable)
target_link_libraries(${PROJECT_NAME} serial)
