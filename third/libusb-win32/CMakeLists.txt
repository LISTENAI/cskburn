project(usb-win32)

set(LIBUSB "libusb-win32/libusb")

set(CFLAGS -O2 -Wall -DWINVER=0x500)

set(DLL_TARGET "libusb0")
set(DLL_CFLAGS ${CFLAGS} -DLOG_APPNAME=\"${DLL_TARGET}-dll\" -DTARGETTYPE=DYNLINK)
set(DLL_LDFLAGS
    -s -mdll
    -Wl,--kill-at
    -Wl,--out-implib,$(LIB_TARGET).a
    -Wl,--enable-stdcall-fixup
    -L. -lcfgmgr32 -lsetupapi -lgdi32
)

set(DLL_SOURCES
    "usb.c"
    "error.c"
    "descriptors.c"
    "windows.c"
    "install.c"
    "registry.c"
)

list(TRANSFORM DLL_SOURCES PREPEND ${LIBUSB}/src/)
add_library(${PROJECT_NAME} STATIC ${DLL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/${LIBUSB}/src)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/${LIBUSB})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/${LIBUSB}/src/driver)

target_compile_options(${PROJECT_NAME} PRIVATE ${DLL_CFLAGS})
target_link_options(${PROJECT_NAME} PRIVATE ${DLL_LDFLAGS})
