#ifndef TASKFLOW_XSTREAM_POOL_HPP
#define TASKFLOW_XSTREAM_POOL_HPP

namespace tf {
class XStreamPool {
public:
  explicit XStreamPool(Context *context_, int nxstreams_);
  ~XStreamPool();
  void init();
  void finalize();
  void start();
  void join();
  void pushReadyTask(Task *p_task);

private:
  Context *context;
  int nxstreams;
  int* num_pools;
  ABT_xstream* xstreams;
  ABT_pool** pools;
};
} // namespace tf
#endif // TASKFLOW_XSTREAM_POOL_HPP
