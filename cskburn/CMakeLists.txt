project(cskburn)

add_executable(
    cskburn
    src/main.c
)

target_link_libraries(cskburn portable)

if(WIN32)
    target_link_libraries(cskburn getopt)
endif()

target_link_libraries(
    cskburn
    cskburn_usb
    cskburn_serial
)

execute_process(COMMAND git describe --tags --abbrev=0 OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-list --count HEAD OUTPUT_VARIABLE GIT_INCREMENT OUTPUT_STRIP_TRAILING_WHITESPACE)

target_compile_options(
    cskburn PRIVATE
    -DGIT_TAG="${GIT_TAG}"
    -DGIT_COMMIT="${GIT_COMMIT}"
    -DGIT_INCREMENT=${GIT_INCREMENT}
)

install(TARGETS cskburn DESTINATION bin)
