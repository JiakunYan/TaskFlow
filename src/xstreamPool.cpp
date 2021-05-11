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
  MLOG_DBG_Log(MLOG_LOG_TRACE, "%s is done!\n", p_task->name.c_str());
}

// XstreamPool Constructor
XStreamPool::XStreamPool(Context *context_, int nxstreams_) : isEverStarted(false) {
  this->context = context_;
  this->nxstreams = nxstreams_;
  this->xstreams = (ABT_xstream *)calloc(nxstreams_, sizeof(ABT_xstream));
  this->scheds = (ABT_sched *)calloc(nxstreams_, sizeof(ABT_sched));
}


// XstreamPool Destructor
XStreamPool::~XStreamPool() {
  free(xstreams);
  free(scheds);
}

void XStreamPool::init() {
  int ret;
//  ret = ABT_xstream_self(&xstreams[0]);
//  TF_CHECK_ABT(ret);

  if (context->config.sched_type == ABT_SCHED_PRIO) {
    /* Create 4 pools for each xstreams. */
    int npools = context->config.sched_prio_npools * nxstreams;
    pools = (ABT_pool *)malloc(npools * sizeof(ABT_pool));
    for (int i = 0; i < nxstreams * context->config.sched_prio_npools; ++i) {
      ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPSC, ABT_TRUE,
                            &pools[i]);
    }
    for (int i = 0; i < nxstreams; ++i) {
      ABT_sched_create_basic(ABT_SCHED_PRIO, context->config.sched_prio_npools,
                             &pools[i * context->config.sched_prio_npools],
                             ABT_SCHED_CONFIG_NULL, &scheds[i]);
    }
  } else if (context->config.sched_type == ABT_SCHED_BASIC) {
    pools = (ABT_pool *)malloc(nxstreams * sizeof(ABT_pool));
    for (int i = 0; i < nxstreams; i++) {
      ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPSC, ABT_TRUE,
                            &pools[i]);
      ABT_sched_create_basic(ABT_SCHED_BASIC, 1, &pools[i],
                             ABT_SCHED_CONFIG_NULL, &scheds[i]);
    }
  } else {
    MLOG_Assert(context->config.sched_type == ABT_SCHED_RANDWS, "Unknown sched type!\n");
    pools = (ABT_pool *)malloc(nxstreams * sizeof(ABT_pool));
    for (int i = 0; i < nxstreams; i++) {
      ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE,
                            &pools[i]);
    }
    ABT_pool* my_pools = (ABT_pool *)malloc(nxstreams * sizeof(ABT_pool));
    for (int i = 0; i < nxstreams; i++) {
      for (int k = 0; k < nxstreams; k++) {
        my_pools[k] = pools[(i + k) % nxstreams];
      }
      ABT_sched_create_basic(ABT_SCHED_RANDWS, nxstreams, my_pools,
                             ABT_SCHED_CONFIG_NULL, &scheds[i]);
    }
    free(my_pools);
  }
}

void XStreamPool::finalize() {
  int ret;
  for (int i = 0; i < nxstreams; ++i) {
    ret = ABT_xstream_free(&xstreams[i]);
    TF_CHECK_ABT(ret);
  }
  free(pools);
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
  int ret;
  int stream_id = p_task->affinity % nxstreams;
  int idx;
  if (context->config.sched_type == ABT_SCHED_PRIO) {
    idx = stream_id * context->config.sched_prio_npools + (int)p_task->priority % context->config.sched_prio_npools;
  } else {
    idx = stream_id;
  }
  ret = ABT_task_create(pools[idx], runTaskWrapper,
                        p_task, nullptr);
  TF_CHECK_ABT(ret);
}
}