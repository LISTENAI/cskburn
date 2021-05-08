project(log)

add_library(
    ${PROJECT_NAME} STATIC
    src/log.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
