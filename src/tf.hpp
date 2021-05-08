#ifndef TASKFLOW_TF_HPP
#define TASKFLOW_TF_HPP
#include <cstdio>
#include <thread>
#include <vector>
#include <unordered_map>
#include <queue>
#include <functional>
#include <mutex>
#include <atomic>
#include <cassert>

#include "abt.h"
#include "mlog.h"

#define AUTOMATIC 1
#define MUST_SUCCEED 1

namespace tf {
void runTaskWrapper(void *args);
class Context;

struct buffer_t {
  void *address;
  int64_t size;
};
}

#include "config.hpp"
#include "backend/backend.h"
#include "utils.hpp"
#include "views.hpp"
#include "serialization.hpp"
#include "hashes.hpp"
#include "taskClass.hpp"
#include "xstreamPool.hpp"
#include "context.hpp"
#include "communication.hpp"

#endif // TASKFLOW_TF_HPP
