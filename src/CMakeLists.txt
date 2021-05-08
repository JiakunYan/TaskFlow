find_package(Argobots REQUIRED)
find_package(MPI REQUIRED)
find_package(Threads REQUIRED)

option(TF_USE_LIBCUCKOO "Use libcuckoo hash table" ON)

add_library(tf-lib STATIC)
target_include_directories(tf-lib PUBLIC .)
target_sources(tf-lib PRIVATE
        context.cpp
        xstreamPool.cpp
        communication.cpp)
target_link_libraries(tf-lib PUBLIC
        Argobots::Argobots
        Threads::Threads
        mlog-lib
        )
target_include_directories(tf-lib PUBLIC ${MPI_CXX_INCLUDE_PATH})
target_compile_options(tf-lib PUBLIC ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(tf-lib PUBLIC ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS})

configure_file(config.hpp.in config.hpp @ONLY)
target_include_directories(tf-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if (USE_LIBCUCKOO)
    # Add the libcuckoo interface target
    add_subdirectory(libcuckoo)
    find_package(Threads REQUIRED)
    target_link_libraries(tf-lib PUBLIC Threads::Threads)
endif()