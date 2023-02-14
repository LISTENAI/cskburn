project(io)

add_library(
    ${PROJECT_NAME} STATIC
    src/io.c
    src/fsio.c
    src/memio.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
