cmake_minimum_required(VERSION 3.13)
project(TaskFlow VERSION 0.3.0 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)


LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")

add_subdirectory(modules/log)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(benchmarks)