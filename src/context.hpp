#ifndef TASKFLOW_CONTEXT_HPP
#define TASKFLOW_CONTEXT_HPP

namespace tf {
class Context {
public:
  Context(int nxstreams_);
  ~Context();

  template <typename TaskIdx>
  void signal(TaskClass<TaskIdx>& taskClass, TaskIdx taskIdx) {
    Task *p_task = taskClass.signal(taskIdx);
    if (p_task != nullptr) {
      p_task->p_context = this;
      ++nTaskInFlight;
      xstreamPool.pushReadyTask(p_task);
    }
  }

  void start() {
    xstreamPool.start();
  }

  void join() {
    while (nTaskInFlight.load() != 0) {
      ABT_thread_yield();
    }
    xstreamPool.join();
  }

  inline int rank_me() {
    int ret, rank;
    ABT_xstream self;
    ret = ABT_xstream_self(&self);
    TF_CHECK_ABT(ret);
    ret = ABT_xstream_get_rank(self, &rank);
    TF_CHECK_ABT(ret);
    return rank;
  }

  inline int rank_n() {
    return nxstreams;
  }

private:
  friend void runTaskWrapper(void *args);
  int nxstreams;
  XStreamPool xstreamPool;
  std::atomic<int64_t> nTaskInFlight;
};
}

#endif // TASKFLOW_CONTEXT_HPP
