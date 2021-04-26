option(USE_LIBCUCKOO "Use libcuckoo hash table" ON)

target_include_directories(tf-lib PUBLIC .)
target_sources(tf-lib PRIVATE
        context.cpp
        xstreamPool.cpp)

configure_file(config.hpp.in config.hpp @ONLY)
target_include_directories(tf-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if (USE_LIBCUCKOO)
    # Add the libcuckoo interface target
    add_subdirectory(libcuckoo)
    find_package(Threads REQUIRED)
    target_link_libraries(tf-lib PUBLIC Threads::Threads)
endif()