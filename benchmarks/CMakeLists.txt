add_executable(micro_deps micro_deps.cpp)
target_link_libraries(micro_deps PRIVATE tf-lib)

find_package(TaskTorrent)
if (TaskTorrent_FOUND)
    find_package(Threads REQUIRED)
    add_executable(ttor_micro_deps ttor_micro_deps.cpp)
    target_link_libraries(ttor_micro_deps PRIVATE
            TaskTorrent::TaskTorrent
            Threads::Threads)
    target_compile_definitions(ttor_micro_deps PRIVATE TTOR_SHARED)
endif()

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