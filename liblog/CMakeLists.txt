project(log)

add_library(
    log STATIC
    src/log.c
)

target_include_directories(
    log PUBLIC
    "${log_SOURCE_DIR}/include"
)
