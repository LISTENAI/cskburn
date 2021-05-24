project(libcskburn_serial_test)

add_executable(
    ${PROJECT_NAME}
    main.c
)

target_link_libraries(${PROJECT_NAME} unity)
