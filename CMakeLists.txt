cmake_minimum_required(VERSION 3.13)
project(TaskFlow VERSION 0.3.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")
find_package(Argobots REQUIRED)
find_package(Threads REQUIRED)

add_library(tf-lib STATIC)
target_link_libraries(tf-lib PUBLIC Argobots::Argobots Threads::Threads)

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(benchmarks)