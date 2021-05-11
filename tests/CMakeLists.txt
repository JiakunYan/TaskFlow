add_executable(test_am_empty test_am_empty.cpp)
target_link_libraries(test_am_empty PRIVATE tf-lib)

find_package(Eigen3)
if(EIGEN3_FOUND)
    add_executable(test_am_Eigen3 test_am_Eigen3.cpp)
    target_link_libraries(test_am_Eigen3 PRIVATE tf-lib)
    target_include_directories(test_am_Eigen3 PRIVATE ${EIGEN3_INCLUDE_DIR})
endif()