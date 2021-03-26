#ifndef TASKFLOW_SCHEDULER_HPP
#define TASKFLOW_SCHEDULER_HPP

namespace tf {

/**
 * @brief Scheduler is used to run the ready tasks.
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
  bool testCompletion();
private:
  int nxstreams;
  Context *context;
  std::atomic<int> tasks_in_flight;
};
}
#endif // TASKFLOW_SCHEDULER_HPP
