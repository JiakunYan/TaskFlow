#ifndef TASKFLOW_XSTREAM_POOL_HPP
#define TASKFLOW_XSTREAM_POOL_HPP

namespace tf {
class XStreamPool {
public:
  explicit XStreamPool(Context *context_, int nxstreams_);
  ~XStreamPool();
  void start();
  void join();
private:
  Context *context;
  int nxstreams;
  std::vector<std::thread> xstreams;
};

void xstream_fn(Context *context, int id);
}
#endif // TASKFLOW_XSTREAM_POOL_HPP
