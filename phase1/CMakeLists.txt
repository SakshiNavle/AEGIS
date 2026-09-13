cmake_minimum_required(VERSION 3.15)

project(aegis_cacgf C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(cacgf STATIC src/cacgf.c src/command_policy.c src/freshness.c
    src/vehicle_context.c src/delay_queue.c src/guardcan.c)
target_include_directories(cacgf PUBLIC include)

if (MSVC)
    target_compile_options(cacgf PRIVATE /W4)
else()
    target_compile_options(cacgf PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(test_cacgf tests/test_cacgf.c)
target_link_libraries(test_cacgf PRIVATE cacgf)

add_executable(test_freshness tests/test_freshness.c)
target_link_libraries(test_freshness PRIVATE cacgf)

add_executable(test_vehicle_context tests/test_vehicle_context.c)
target_link_libraries(test_vehicle_context PRIVATE cacgf)

add_executable(test_delay_queue tests/test_delay_queue.c)
target_link_libraries(test_delay_queue PRIVATE cacgf)

add_executable(test_guardcan tests/test_guardcan.c)
target_link_libraries(test_guardcan PRIVATE cacgf)

add_executable(test_integration tests/test_integration.c)
target_link_libraries(test_integration PRIVATE cacgf)

enable_testing()
add_test(NAME cacgf_unit_tests COMMAND test_cacgf)
add_test(NAME freshness_unit_tests COMMAND test_freshness)
add_test(NAME vehicle_context_unit_tests COMMAND test_vehicle_context)
add_test(NAME delay_queue_unit_tests COMMAND test_delay_queue)
add_test(NAME guardcan_unit_tests COMMAND test_guardcan)
add_test(NAME integration_tests COMMAND test_integration)
