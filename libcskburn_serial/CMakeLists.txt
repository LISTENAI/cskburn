project(cskburn_serial)

set(SRCS
    src/core.c
    src/cmd.c
    src/burner_castor_img.c
    src/burner_venus_img.c
)

add_library(${PROJECT_NAME} STATIC ${SRCS})

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} portable)
target_link_libraries(${PROJECT_NAME} serial)
target_link_libraries(${PROJECT_NAME} slip)
target_link_libraries(${PROJECT_NAME} io)

if($ENV{TRACE_DATA})
    target_compile_options(${PROJECT_NAME} PRIVATE -DTRACE_DATA=$ENV{TRACE_DATA})
endif()
