project(cskburn)

add_executable(
    ${PROJECT_NAME}
    src/main.c
    src/verify.c
    src/utils.c
    src/read_parts_bin.c
    src/read_parts_hex.c
    src/intelhex/intelhex.c
)

target_link_libraries(${PROJECT_NAME} log)
target_link_libraries(${PROJECT_NAME} portable)
target_link_libraries(${PROJECT_NAME} io)

target_link_libraries(${PROJECT_NAME} cskburn_usb)
target_link_libraries(${PROJECT_NAME} cskburn_serial)

target_link_libraries(${PROJECT_NAME} mbedtls)

execute_process(COMMAND git describe --tags --abbrev=0
    OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET RESULT_VARIABLE _git_tag_ret)
if(NOT _git_tag_ret EQUAL 0)
    set(GIT_TAG "unknown")
endif()

execute_process(COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET RESULT_VARIABLE _git_commit_ret)
if(NOT _git_commit_ret EQUAL 0)
    set(GIT_COMMIT "unknown")
endif()

execute_process(COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE GIT_INCREMENT OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET RESULT_VARIABLE _git_inc_ret)
if(NOT _git_inc_ret EQUAL 0)
    set(GIT_INCREMENT 0)
endif()

target_compile_options(
    ${PROJECT_NAME} PRIVATE
    -DGIT_TAG="${GIT_TAG}"
    -DGIT_COMMIT="${GIT_COMMIT}"
    -DGIT_INCREMENT=${GIT_INCREMENT}
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
