project(errors)

add_library(
    ${PROJECT_NAME} STATIC
    src/errors.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
