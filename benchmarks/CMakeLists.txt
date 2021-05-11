function(add_ttor_executable EXEC)
    add_executable(${EXEC} ${ARGN})
    target_link_libraries(${EXEC} PRIVATE
            TaskTorrent::TaskTorrent
            Threads::Threads)
    target_compile_definitions(${EXEC} PRIVATE TTOR_MPI)
    target_include_directories(${EXEC} PRIVATE ${MPI_CXX_INCLUDE_PATH})
    target_compile_options(${EXEC} PRIVATE ${MPI_CXX_COMPILE_FLAGS})
    target_link_libraries(${EXEC} PRIVATE ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS})
endfunction()

function(add_mpi_executable EXEC)
    add_executable(${EXEC} ${ARGN})
    target_include_directories(${EXEC} PRIVATE ${MPI_CXX_INCLUDE_PATH})
    target_compile_options(${EXEC} PRIVATE ${MPI_CXX_COMPILE_FLAGS})
    target_link_libraries(${EXEC} PRIVATE ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} mlog-lib)
endfunction()

find_package(MPI REQUIRED)
find_package(Eigen3 REQUIRED)

add_executable(micro_deps micro_deps.cpp)
target_link_libraries(micro_deps PRIVATE tf-lib)
add_executable(micro_comm_lat micro_comm_lat.cpp)
target_link_libraries(micro_comm_lat PRIVATE tf-lib)
add_executable(micro_comm_bw micro_comm_bw.cpp)
target_link_libraries(micro_comm_bw PRIVATE tf-lib)

add_executable(cholesky_dist ../benchmarks/cholesky_dist.cpp)
target_link_libraries(cholesky_dist PRIVATE tf-lib)
target_include_directories(cholesky_dist PRIVATE ${EIGEN3_INCLUDE_DIR})

add_executable(stencil_1D_dist ../benchmarks/stencil_1D_dist.cpp)
target_link_libraries(stencil_1D_dist PRIVATE tf-lib)

add_executable(stencil_2D_dist ../benchmarks/stencil_2D_dist.cpp)
target_link_libraries(stencil_2D_dist PRIVATE tf-lib)

add_mpi_executable(mpi_lat mpi_lat.cpp)
add_mpi_executable(mpi_bw mpi_bw.cpp)

find_package(TaskTorrent)
if (TaskTorrent_FOUND)
    find_package(Threads REQUIRED)
    add_ttor_executable(ttor_micro_deps ttor_micro_deps.cpp)
    add_ttor_executable(ttor_cholesky ttor_cholesky.cpp)
    target_include_directories(ttor_cholesky PRIVATE ${EIGEN3_INCLUDE_DIR})
endif()

#find_package(OpenMP)
#if (OpenMP_FOUND)
#    add_executable(omp_wait omp_wait.cpp)
#    target_link_libraries(omp_wait  PRIVATE OpenMP::OpenMP_CXX)
#endif()

find_package(PaRSEC HINTS
        ${PaRSEC_ROOT}
        ${CMAKE_SOURCE_DIR}/experiments/external/parsec-3.0.2012-install)
if (PaRSEC_FOUND)
    function(parsec_addexample lang target)
        add_executable(${target} ${ARGN})
        set_target_properties(${target} PROPERTIES
                LINKER_LANGUAGE ${lang})
        target_link_libraries(${target} PRIVATE PaRSEC::parsec)
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_include_directories(${target} PRIVATE ${MPI_CXX_INCLUDE_PATH})
        target_compile_options(${target} PRIVATE ${MPI_CXX_COMPILE_FLAGS})
        target_link_libraries(${target} PRIVATE ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS})
    endfunction(parsec_addexample)

    find_package(MPI REQUIRED)
    include(ParsecCompilePTG)

    parsec_addexample(C parsec_micro_deps)
    target_ptg_sources(parsec_micro_deps PRIVATE parsec_micro_deps.jdf)
endif()