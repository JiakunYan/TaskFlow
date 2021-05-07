#include "tf.hpp"

namespace tf {
Context::Context(int nxstreams_, int termSingalNum)
    : nTaskInFlight(0),
      isDone(false),
      currTermSingalNum(0),
      totalTermSingalNum(termSingalNum),
      nxstreams(nxstreams_),
      xstreamPool(this, nxstreams_) {
  int ret;
  MLOG_Init();
  TFC_init();
  ret = ABT_init(0, nullptr);
  TF_CHECK_ABT(ret);
  xstreamPool.init();
}

Context::~Context() {
  int ret;
  xstreamPool.finalize();
  ret = ABT_finalize();
  TF_CHECK_ABT(ret);
  TFC_finalize();
}
} // namespace tf