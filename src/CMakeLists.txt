target_include_directories(tf-lib PUBLIC .)
target_sources(tf-lib PRIVATE
        taskflow.cpp
        taskpool.cpp
        scheduler.cpp
        xstream_pool.cpp)