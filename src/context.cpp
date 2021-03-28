#include "tf.hpp"

namespace tf {
Context::Context(int nxstreams_)
    : nTaskInFlight(0),
      nxstreams(nxstreams_),
      xstreamPool(this, nxstreams_) {
  int ret;
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