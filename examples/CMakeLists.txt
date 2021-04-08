add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world PRIVATE tf-lib)

find_package(Eigen3)
if(EIGEN3_FOUND)
    add_executable(cholesky cholesky.cpp)
    target_link_libraries(cholesky PRIVATE tf-lib)
    target_include_directories(cholesky PRIVATE ${EIGEN3_INCLUDE_DIR})
endif()

add_executable(qr_gram-schmidt qr_gram-schmidt.cpp)
target_link_libraries(qr_gram-schmidt PRIVATE tf-lib)