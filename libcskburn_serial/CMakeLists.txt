project(cskburn_serial)

add_library(
    cskburn_serial STATIC
    src/core.c
    src/cmd.c
    src/slip.c
    src/burner_img.c
)

target_include_directories(
    cskburn_serial PUBLIC
    "${cskburn_serial_SOURCE_DIR}/include"
)

target_link_libraries(cskburn_serial log)
target_link_libraries(cskburn_serial portable)
target_link_libraries(cskburn_serial serial)
