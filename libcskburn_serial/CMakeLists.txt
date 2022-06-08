project(cskburn_serial)

set(SRCS
    src/core.c
    src/cmd.c
    src/slip.c
    src/burner_4_img.c
)

if(FEATURE_MD5_CHALLENGE)
    list(APPEND SRCS src/mbedtls_md5.c)
endif()

add_library(${PROJECT_NAME} STATIC ${SRCS})

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} portable)
target_link_libraries(${PROJECT_NAME} serial)

if(FEATURE_MD5_CHALLENGE)
    target_compile_options(${PROJECT_NAME} PRIVATE -DFEATURE_MD5_CHALLENGE)
endif()

if(FEATURE_SEQ_QUEST)
    target_compile_options(${PROJECT_NAME} PRIVATE -DFEATURE_SEQ_QUEST)
endif()

add_subdirectory(test)
add_test(NAME ${PROJECT_NAME} COMMAND libcskburn_serial_test)
