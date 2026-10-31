cmake_minimum_required(VERSION 3.14)
project(RealTimeTaskScheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release by default: the verdict experiment runs 132,000 simulations.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Put the executables in the project root so "./rtts demo" works right after
# building, and the default output paths (viewer/data, results) line up.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${config} CONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_SOURCE_DIR})
endforeach()

# ---------------------------------------------------------------- the library
# Everything except main() lives in a library, so the tests can link to it.
add_library(rtts_core
    src/queues/binary_heap.cpp
    src/queues/fibonacci_heap.cpp
    src/queues/sorted_list.cpp
    src/queues/rb_tree_queue.cpp
    src/generator.cpp
    src/theory.cpp
    src/metrics.cpp
    src/scheduler.cpp
    src/verdict.cpp
    src/dataset.cpp
    src/experiments.cpp
    src/op_benchmark.cpp
    src/terminal_view.cpp
    src/json_export.cpp
)
target_include_directories(rtts_core PUBLIC include)

# ------------------------------------------------------------ the CLI program
add_executable(rtts src/main.cpp)
target_link_libraries(rtts PRIVATE rtts_core)

# --------------------------------------------------------------------- tests
enable_testing()
add_executable(rtts_tests
    tests/test_main.cpp
    tests/test_queues.cpp
    tests/test_theory.cpp
    tests/test_scheduler.cpp
    tests/test_verdict.cpp
    tests/test_op_benchmark.cpp
)
target_link_libraries(rtts_tests PRIVATE rtts_core)
add_test(NAME all_tests COMMAND rtts_tests)
