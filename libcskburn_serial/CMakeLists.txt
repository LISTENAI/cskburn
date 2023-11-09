project(cskburn_serial)

set(SRCS
    src/core.c
    src/cmd.c
    src/slip.c
    src/burner_4_img.c
    src/burner_6_img.c
)

add_library(${PROJECT_NAME} STATIC ${SRCS})

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} portable)
target_link_libraries(${PROJECT_NAME} serial)
target_link_libraries(${PROJECT_NAME} io)

if($ENV{TRACE_DATA})
    target_compile_options(${PROJECT_NAME} PRIVATE -DTRACE_DATA=$ENV{TRACE_DATA})
endif()

if($ENV{TRACE_SLIP})
    target_compile_options(${PROJECT_NAME} PRIVATE -DTRACE_SLIP=$ENV{TRACE_SLIP})
endif()
