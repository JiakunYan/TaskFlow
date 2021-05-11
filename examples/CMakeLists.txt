add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world PRIVATE tf-lib)

add_executable(hello_world_am hello_world_am.cpp)
target_link_libraries(hello_world_am PRIVATE tf-lib)

add_executable(hello_world_dist hello_world_dist.cpp)
target_link_libraries(hello_world_dist PRIVATE tf-lib)

find_package(Eigen3)
if(EIGEN3_FOUND)
    add_executable(cholesky cholesky.cpp)
    target_link_libraries(cholesky PRIVATE tf-lib)
    target_include_directories(cholesky PRIVATE ${EIGEN3_INCLUDE_DIR})
endif()

add_executable(qr_gram-schmidt qr_gram-schmidt.cpp)
target_link_libraries(qr_gram-schmidt PRIVATE tf-lib)

add_executable(stencil_2D stencil_2D.cpp)
target_link_libraries(stencil_2D PRIVATE tf-lib)

add_executable(stencil_1D stencil_1D.cpp)
target_link_libraries(stencil_1D PRIVATE tf-lib)

add_executable(stencil_1D_seq stencil_1D_seq.cpp)
target_link_libraries(stencil_1D_seq PRIVATE tf-lib)