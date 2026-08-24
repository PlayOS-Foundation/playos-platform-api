add_executable(test_power test_power.c)
target_include_directories(test_power PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_power PRIVATE playos)
add_test(NAME test_power COMMAND test_power)
