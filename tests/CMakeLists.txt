add_executable(test_power test_power.c)
target_link_libraries(test_power PRIVATE playos)
add_test(NAME test_power COMMAND test_power)
