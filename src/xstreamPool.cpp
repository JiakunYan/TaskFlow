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
  MLOG_Log(MLOG_LOG_TRACE, "%s is done!\n", p_task->name.c_str());
}

// XstreamPool Constructor
XStreamPool::XStreamPool(Context *context_, int nxstreams_) : isEverStarted(false) {
  this->context = context_;
  this->nxstreams = nxstreams_;
  this->num_pools = (int *)calloc(nxstreams_, sizeof(int));
  this->xstreams = (ABT_xstream *)calloc(nxstreams_, sizeof(ABT_xstream));
  this->scheds = (ABT_sched *)calloc(nxstreams_, sizeof(ABT_sched));
  this->pools = (ABT_pool **)calloc(nxstreams_, sizeof(ABT_pool*));
}


// XstreamPool Destructor
XStreamPool::~XStreamPool() {
  free(num_pools);
  free(xstreams);
  free(pools);
}

void XStreamPool::init() {
  int ret;
//  ret = ABT_xstream_self(&xstreams[0]);
//  TF_CHECK_ABT(ret);

  /* Create 4 pools for each xstreams. */
  for (int i = 0; i < nxstreams; i++) {
    num_pools[i] = 4; // So there will be 4 candidate pools in each "pool"
    pools[i] = (ABT_pool *)malloc(num_pools[i] * sizeof(ABT_pool));
    for  (int k = 0; k < num_pools[i]; k++) {
      ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPSC, ABT_TRUE, &pools[i][k]);
    }
  }

  for (int i = 0; i < nxstreams; i++) {
    ABT_sched_create_basic(ABT_SCHED_PRIO, num_pools[i], pools[i], ABT_SCHED_CONFIG_NULL, &scheds[i]);
  }
}

void XStreamPool::finalize() {
  int ret;
  for (int i = 0; i < nxstreams; ++i) {
    ret = ABT_xstream_free(&xstreams[i]);
    TF_CHECK_ABT(ret);
  }
  for (int i = 0; i < nxstreams; i++) {
    if (pools[i])
      free(pools[i]);
  }
}

void XStreamPool::start() {
  int ret;
  if (!isEverStarted) {
    for (int i = 0; i < nxstreams; i++) {
      ret = ABT_xstream_create(scheds[i], &xstreams[i]);
      TF_CHECK_ABT(ret);
    }
    isEverStarted = true;
  } else {
    for (int i = 0; i < nxstreams; i++) {
      ret = ABT_xstream_revive(xstreams[i]);
      TF_CHECK_ABT(ret);
    }
  }
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
  ret = ABT_task_create(pools[i++ % nxstreams][static_cast<int>(p_task->priority)], runTaskWrapper,
                        p_task, nullptr);
  TF_CHECK_ABT(ret);
}
}