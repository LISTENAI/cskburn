project(portable)

add_library(
    ${PROJECT_NAME} STATIC
    src/time_monotonic.c
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
