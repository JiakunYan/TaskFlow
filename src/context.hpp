#ifndef TASKFRAMEWORK_CONTEXT_HPP
#define TASKFRAMEWORK_CONTEXT_HPP

namespace tf {
class Context {
public:
  Context(int nxstreams_)
      : xstream_pool(this, nxstreams_),
        taskpool(this, nxstreams_),
        scheduler(this, nxstreams_) {}
  void signal(Taskflow& taskflow, TaskIdx taskIdx) {
    taskpool.signal(taskflow, taskIdx);
  }
  void start() {
    xstream_pool.start();
  }
  void join() {
    xstream_pool.join();
  }

  XStreamPool xstream_pool;
  Taskpool taskpool;
  Scheduler scheduler;
};
}

#endif // TASKFRAMEWORK_CONTEXT_HPP
