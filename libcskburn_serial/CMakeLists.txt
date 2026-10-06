project(cskburn_serial)

set(SRCS
    src/core.c
    src/cmd.c
)

add_library(${PROJECT_NAME} STATIC ${SRCS})

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} errors)
target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} portable)
target_link_libraries(${PROJECT_NAME} serial)
target_link_libraries(${PROJECT_NAME} slip)
target_link_libraries(${PROJECT_NAME} io)

if($ENV{TRACE_DATA})
    target_compile_options(${PROJECT_NAME} PRIVATE -DTRACE_DATA=$ENV{TRACE_DATA})
endif()

include(target_embed_binary.cmake)
target_embed_binary(${PROJECT_NAME} burner_serial_castor  ${CMAKE_CURRENT_SOURCE_DIR}/burner_castor.bin)
target_embed_binary(${PROJECT_NAME} burner_serial_venus   ${CMAKE_CURRENT_SOURCE_DIR}/burner_venus.bin)
target_embed_binary(${PROJECT_NAME} burner_serial_arcs    ${CMAKE_CURRENT_SOURCE_DIR}/burner_arcs.bin)
target_embed_binary(${PROJECT_NAME} burner_serial_venusa  ${CMAKE_CURRENT_SOURCE_DIR}/burner_venusa.bin)

if(BUILD_TESTING)
    add_executable(cskburn_serial_cmd_test tests/test_cmd.c)
    target_include_directories(cskburn_serial_cmd_test PRIVATE src)
    target_link_libraries(cskburn_serial_cmd_test cskburn_serial)
    if(MSVC)
        target_compile_options(cskburn_serial_cmd_test PRIVATE /UNDEBUG)
    else()
        target_compile_options(cskburn_serial_cmd_test PRIVATE -UNDEBUG)
    endif()
    add_test(NAME cskburn_serial_cmd COMMAND cskburn_serial_cmd_test)
endif()
