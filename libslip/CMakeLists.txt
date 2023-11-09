project(slip)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE src/slip.c)
target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} serial)
target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} portable)

if($ENV{TRACE_SLIP})
    target_compile_options(${PROJECT_NAME} PRIVATE -DTRACE_SLIP=$ENV{TRACE_SLIP})
endif()
