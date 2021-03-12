cmake_minimum_required(VERSION 3.13)
project(taskFramework VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

add_library(tf-lib STATIC)
target_link_libraries(tf-lib PRIVATE Threads::Threads)

add_subdirectory(src)
add_subdirectory(examples)