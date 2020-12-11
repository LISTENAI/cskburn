project(cskburn)

add_executable(
    cskburn
    src/main.c
)

target_link_libraries(
    cskburn
    cskburn_usb
)

install(TARGETS cskburn DESTINATION bin)
