#include "tf.hpp"
#include "context.hpp"

namespace tf {
Config::Config() {
  char *p = getenv("TF_CONF_sched_type");
  if (p == nullptr);
  else if (strcmp(p, "basic") == 0) {
    sched_type = ABT_SCHED_BASIC;
  } else if (strcmp(p, "prio") == 0) {
    sched_type = ABT_SCHED_PRIO;
  } else if (strcmp(p, "randws") == 0) {
    sched_type = ABT_SCHED_RANDWS;
  } else {
    MLOG_Log(MLOG_LOG_WARN, "Unknown TF_CONF_sched_type %s (option: basic/prio/randws; default: prio)\n", p);
    sched_type = ABT_SCHED_PRIO;
  }
}

Context::Context(int nxstreams_)
    : nTaskInFlight(0),
      isDone(false),
      currTermSingalNum(0),
      nxstreams(nxstreams_),
      xstreamPool(this, nxstreams_) {
  int ret;
  MLOG_Init();
  ret = ABT_init(0, nullptr);
  TF_CHECK_ABT(ret);
  xstreamPool.init();
}

Context::~Context() {
  int ret;
  xstreamPool.finalize();
  ret = ABT_finalize();
  TF_CHECK_ABT(ret);
}
} // namespace tf