target_include_directories(tf-lib PUBLIC .)
target_sources(tf-lib PRIVATE
        taskpool.cpp
        scheduler.cpp
        xstreamPool.cpp)

configure_file(config.hpp.in config.hpp @ONLY)
target_include_directories(tf-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})