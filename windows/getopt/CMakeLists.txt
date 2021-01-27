project(getopt)

add_library(
    getopt STATIC
    getopt.c
)

target_include_directories(
    getopt PUBLIC
    "${getopt_SOURCE_DIR}"
)
