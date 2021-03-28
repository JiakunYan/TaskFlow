add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world PRIVATE tf-lib)

add_executable(cholesky cholesky.cpp)
target_link_libraries(cholesky PRIVATE tf-lib)