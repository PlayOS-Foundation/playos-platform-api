add_executable(test_power test_power.c)
target_include_directories(test_power PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_power PRIVATE playos)
add_test(NAME test_power COMMAND test_power)

# gamepad_db is compiled directly (it is not linked into the stub build).
add_executable(test_gamepad_db
    test_gamepad_db.c
    ${CMAKE_SOURCE_DIR}/src/backends/gamepad_db.c
)
target_include_directories(test_gamepad_db PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/backends
)
add_test(NAME test_gamepad_db COMMAND test_gamepad_db)
