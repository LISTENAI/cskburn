project(getopt)

add_library(
    ${PROJECT_NAME} STATIC
    getopt.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}
)
