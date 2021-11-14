project(usb)

set(POSIX_PLATFORM_SRC
    "os/events_posix.h"
    "os/events_posix.c"
    "os/threads_posix.h"
    "os/threads_posix.c"
)

set(PLATFORM_SRC ${POSIX_PLATFORM_SRC})

set(OS_DARWIN_SRC
    "os/darwin_usb.h"
    "os/darwin_usb.c"
)

set(OS_LINUX_SRC
    "os/linux_usbfs.h"
    "os/linux_usbfs.c"
)

if(APPLE)
    set(OS_SRC ${OS_DARWIN_SRC})
elseif(UNIX OR ANDROID)
    set(OS_SRC ${OS_LINUX_SRC})
    list(APPEND OS_SRC os/linux_netlink.c)
endif()

set(SOURCES
    "libusbi.h"
    "version.h"
    "version_nano.h"
    "core.c"
    "descriptor.c"
    "hotplug.c"
    "io.c"
    "strerror.c"
    "sync.c"
    ${PLATFORM_SRC}
    ${OS_SRC}
)

list(TRANSFORM SOURCES PREPEND "libusb/libusb/")

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/libusb/libusb
)

if(APPLE)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/config_darwin)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework CoreFoundation")
    target_link_libraries(${PROJECT_NAME} "-framework Security")
elseif(ANDROID)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libusb/android)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
    target_link_libraries(${PROJECT_NAME} PUBLIC -llog)
elseif(UNIX)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/config_linux)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
endif()
