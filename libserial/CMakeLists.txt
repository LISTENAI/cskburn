project(serial)

set(SRCS "")
if (WIN32)
    list(APPEND SRCS "src/serial_win32.c")
else()
    list(APPEND SRCS "src/serial_posix.c")
endif()

add_library(serial STATIC ${SRCS})

target_include_directories(
    serial PUBLIC
    "${serial_SOURCE_DIR}/include"
)

install(TARGETS serial DESTINATION lib)
install(FILES include/serial.h DESTINATION include)
