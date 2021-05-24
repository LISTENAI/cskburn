project(libcskburn_serial_test)

add_executable(
    ${PROJECT_NAME}
    main.c
    test_slip_decode.c
    ../src/slip.c
)

target_include_directories(${PROJECT_NAME} PRIVATE ../src)

target_link_libraries(${PROJECT_NAME} unity)
