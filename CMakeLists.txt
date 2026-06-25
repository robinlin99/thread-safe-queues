cmake_minimum_required(VERSION 3.16)
project(queue_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release build by default for meaningful benchmarks
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Lock-based queue benchmark
add_executable(bench_lock bench_lock.cpp)
target_include_directories(bench_lock PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED)
target_link_libraries(bench_lock PRIVATE Threads::Threads)

# Lock-free queue benchmark
add_executable(bench_lock_free bench_lock_free.cpp)
target_include_directories(bench_lock_free PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bench_lock_free PRIVATE Threads::Threads)

# --- Unit tests ---
enable_testing()

add_executable(test_lock_queue tests/test_lock_queue.cpp)
target_include_directories(test_lock_queue PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_lock_queue PRIVATE Threads::Threads)
add_test(NAME test_lock_queue COMMAND test_lock_queue)

add_executable(test_lock_free_queue tests/test_lock_free_queue.cpp)
target_include_directories(test_lock_free_queue PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_lock_free_queue PRIVATE Threads::Threads)
add_test(NAME test_lock_free_queue COMMAND test_lock_free_queue)

add_executable(test_single_threaded_queue tests/test_single_threaded_queue.cpp)
target_include_directories(test_single_threaded_queue PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_single_threaded_queue COMMAND test_single_threaded_queue)

# Single-threaded queue sample
add_executable(sample_single_threaded_queue sample_single_threaded_queue.cpp)
target_include_directories(sample_single_threaded_queue PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
