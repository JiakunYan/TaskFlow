#ifndef TASKFLOW_CONTEXT_HPP
#define TASKFLOW_CONTEXT_HPP

namespace tf {
class Context {
public:
  Context(int nxstreams_, int termSingalNum = 0);
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

  void signalTerm() {
    int curr = ++currTermSingalNum;
    if (curr == totalTermSingalNum) {
      isDone = true;
    }
  }

  void start() {
    xstreamPool.start();
  }

  void join() {
    while ((totalTermSingalNum != 0 && !isDone.load()) ||
           (totalTermSingalNum == 0 && nTaskInFlight != 0)) {
      sched_yield();
    }
    xstreamPool.join();
  }

  bool tryJoin() {
    if ((totalTermSingalNum != 0 && !isDone.load()) ||
        (totalTermSingalNum == 0 && nTaskInFlight != 0)) {
      return false;
    } else {
      xstreamPool.join();
      return true;
    }
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
  int totalTermSingalNum;
  std::atomic<int> currTermSingalNum;
  std::atomic<bool> isDone;
  int nxstreams;
  XStreamPool xstreamPool;
  std::atomic<int64_t> nTaskInFlight;
};
}

#endif // TASKFLOW_CONTEXT_HPP
