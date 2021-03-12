#ifndef TASKFRAMEWORK_TF_HPP
#define TASKFRAMEWORK_TF_HPP
#include <cstdio>
#include <thread>
#include <vector>
#include <unordered_map>
#include <queue>
#include <functional>
#include <mutex>
#include <atomic>
#include <cassert>

namespace tf {
using TaskIdx = int;
class Context;
}
#include "taskflow.hpp"
#include "taskpool.hpp"
#include "scheduler.hpp"
#include "xstream_pool.hpp"
#include "context.hpp"

#endif // TASKFRAMEWORK_TF_HPP
