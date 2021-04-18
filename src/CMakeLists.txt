target_include_directories(tf-lib PUBLIC .)
target_sources(tf-lib PRIVATE
        context.cpp
        xstreamPool.cpp)

configure_file(config.hpp.in config.hpp @ONLY)
target_include_directories(tf-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Add the libcuckoo interface target
add_subdirectory(libcuckoo)

IF (HASH_TABLE_TYPE STREQUAL libcuckoo)
    ADD_DEFINITIONS(-DLIBCUCKOO)
ENDIF()