//
// Created by jiakunyan on 3/12/21.
//
#include <cassert>
#include "tf.hpp"
using namespace std;

namespace tf {
void runTaskWrapper(void *args) {
  auto p_task = static_cast<Task*>(args);
  p_task->run();
  --p_task->p_context->nTaskInFlight;
}

XStreamPool::XStreamPool(Context *context_, int nxstreams_)
    : context(context_),
      nxstreams(nxstreams_),
      xstreams(nxstreams_),
      pools(nxstreams_) {}

XStreamPool::~XStreamPool() {
  xstreams.clear();
  pools.clear();
}

void XStreamPool::init() {
  int ret;
//  ret = ABT_xstream_self(&xstreams[0]);
//  TF_CHECK_ABT(ret);

  /* Create pools. */
  for (int i = 0; i < nxstreams; i++) {
    ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, &pools[i]);
  }

  ABT_sched *scheds = (ABT_sched*)malloc(sizeof(ABT_sched)*nxstreams);
  for (int i = 0; i < nxstreams; i++) {
    ABT_sched_create_basic(ABT_SCHED_PRIO, 1, &pools[i], ABT_SCHED_CONFIG_NULL, &scheds[i]);
  }

  for (int i = 0; i < nxstreams; i++) {
    ret = ABT_xstream_create(scheds[i], &xstreams[i]);
    TF_CHECK_ABT(ret);
  }

  for (int i = 0; i < nxstreams; i++) {
    ret = ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
    TF_CHECK_ABT(ret);
  }
}

void XStreamPool::finalize() {
  int ret;
  for (int i = 0; i < nxstreams; ++i) {
    ret = ABT_xstream_free(&xstreams[i]);
    TF_CHECK_ABT(ret);
  }
}

void XStreamPool::start() {
}

void XStreamPool::join() {
  int ret;
  for (int i = 0; i < nxstreams; ++i) {
    ret = ABT_xstream_join(xstreams[i]);
    TF_CHECK_ABT(ret);
  }
}

void XStreamPool::pushReadyTask(Task *p_task) {
  static std::atomic<int64_t> i(0);
  int ret;
  ret = ABT_task_create(pools[i++ % nxstreams], runTaskWrapper,
                        p_task, nullptr);
  TF_CHECK_ABT(ret);
}
}