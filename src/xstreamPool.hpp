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
  ABT_xstream* xstreams;
  ABT_sched* scheds;
  ABT_pool* pools;
  bool isEverStarted;
};
} // namespace tf
#endif // TASKFLOW_XSTREAM_POOL_HPP
