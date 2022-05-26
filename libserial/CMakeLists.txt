project(serial)

set(SRCS "")
if(WIN32)
    list(APPEND SRCS "src/serial_win32.c")
    list(APPEND SRCS "src/usb_info_win32.c")
else()
    list(APPEND SRCS "src/serial_posix.c")
    if(UNIX AND NOT APPLE)
        list(APPEND SRCS "src/set_baud_linux.c")
        list(APPEND SRCS "src/usb_info_linux.c")
    elseif(APPLE)
        list(APPEND SRCS "src/set_baud_darwin.c")
        list(APPEND SRCS "src/usb_info_darwin.c")
    else()
        list(APPEND SRCS "src/set_baud_posix.c")
        list(APPEND SRCS "src/usb_info_posix.c")
    endif()
endif()

add_library(${PROJECT_NAME} STATIC ${SRCS})

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} "-lsetupapi")
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework CoreFoundation")
endif()
