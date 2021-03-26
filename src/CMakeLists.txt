target_include_directories(tf-lib PUBLIC .)
target_sources(tf-lib PRIVATE
        taskpool.cpp
        scheduler.cpp
        xstreamPool.cpp)