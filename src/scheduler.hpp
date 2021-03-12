#ifndef TASKFRAMEWORK_SCHEDULER_HPP
#define TASKFRAMEWORK_SCHEDULER_HPP

namespace tf {

/**
 * @brief Scheduler is used to manage all the ready tasks. It decides which
 * ready task should be run next.
 *
 * Scheduler is shared by all xstreams.
 */
class Scheduler {
public:
  Scheduler(Context *context_, int nxstreams_);
  /**
   * Every xstream (thread) will run this function.
   */
  void run(int id);
  Task *getNextReadyTask();
  void putReadyTask(Task *task);
  bool testCompletion();
private:
  int nxstreams;
  Context *context;

  using PQueue = std::priority_queue<Task *, std::vector<Task *>, Task::OpLess>;
  PQueue readyTasks;
  std::mutex readyTasksLock;
  std::atomic<int> tasks_in_flight;
};
}
#endif // TASKFRAMEWORK_SCHEDULER_HPP
