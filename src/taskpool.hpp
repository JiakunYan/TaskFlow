#ifndef TASKFLOW_TASKPOOL_HPP
#define TASKFLOW_TASKPOOL_HPP

namespace tf {
/**
 * @brief Taskpool is used to manage all ready tasks.
 *
 * Taskpool is shared by all xstreams.
 */
class Taskpool {
public:
  Taskpool(Context* context_, int nxstreams_);

  Task *getNextReadyTask();
  void putReadyTask(Task *task);

private:
  Context *context;
  int nxstreams;

  using PQueue = std::priority_queue<Task *, std::vector<Task *>, Task::OpLess>;
  PQueue readyTasks;
  std::mutex readyTasksLock;
};
}
#endif // TASKFLOW_TASKPOOL_HPP
