#ifndef TASKFLOW_CONTEXT_HPP
#define TASKFLOW_CONTEXT_HPP

namespace tf {
class Context {
public:
  Context(int nxstreams_)
      : xstreamPool(this, nxstreams_),
        taskpool(this, nxstreams_),
        scheduler(this, nxstreams_) {}

  template <typename TaskIdx>
  void signal(Taskflow<TaskIdx>& taskflow, TaskIdx taskIdx) {
    Task *p_task = taskflow.signal(taskIdx);
    if (p_task != nullptr) {
      taskpool.putReadyTask(p_task);
    }
  }

  void start() {
    xstreamPool.start();
  }

  void join() {
    xstreamPool.join();
  }

private:
  friend class Scheduler;
  friend void xstream_fn(Context *context, int id);
  XStreamPool xstreamPool;
  Taskpool taskpool;
  Scheduler scheduler;
};
}

#endif // TASKFLOW_CONTEXT_HPP
