project(mbedtls)

add_library(${PROJECT_NAME} STATIC
    library/md5.c
    library/platform_util.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
