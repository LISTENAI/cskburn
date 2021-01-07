project(portable)

add_library(portable INTERFACE)

target_include_directories(
    portable INTERFACE
    "${portable_SOURCE_DIR}/include"
)
