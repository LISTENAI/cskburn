project(serial)

set(SRCS "")
if(WIN32)
    list(APPEND SRCS "src/serial_win32.c")
else()
    list(APPEND SRCS "src/serial_posix.c")
    if(UNIX AND NOT APPLE)
        list(APPEND SRCS "src/set_baud_linux.c")
    elseif(APPLE)
        list(APPEND SRCS "src/set_baud_darwin.c")
    else()
        list(APPEND SRCS "src/set_baud_posix.c")
    endif()
endif()

add_library(${PROJECT_NAME} STATIC ${SRCS})

target_link_libraries(${PROJECT_NAME} portable)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
