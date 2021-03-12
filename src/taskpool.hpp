#ifndef TASKFRAMEWORK_TASKPOOL_HPP
#define TASKFRAMEWORK_TASKPOOL_HPP

namespace tf {
/**
 * @brief Taskpool is used to manage all the task dependency. It decides which
 * tasks are ready based on already completed tasks.
 *
 * Taskpool is shared by all xstreams.
 */
class Taskpool {
public:
  Taskpool(Context* context_, int nxstreams_);
  /**
   * @brief mark a task as completed
   * @param[in] task the completed task
   */
  void markAsCompleted(Taskflow& taskflow, TaskIdx taskIdx);

  /**
   * @brief signal a task
   * @param task the task to be signaled
   * @return whether the task becomes ready
   */
  bool signal(Taskflow& taskflow, TaskIdx taskIdx);

private:
  int nxstreams;
  Context *context;

  struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
      auto h1 = std::hash<T1>{}(p.first);
      auto h2 = std::hash<T2>{}(p.second);

      // Mainly for demonstration purposes, i.e. works but is overly simple
      // In the real world, use sth. like boost.hash_combine
      return h1 ^ h2;
    }
  };

  using DepMap = std::unordered_map<std::pair<int, TaskIdx>, int, pair_hash>;
  DepMap depCounter;
  std::mutex depCounterLock;
};
}
#endif // TASKFRAMEWORK_TASKPOOL_HPP
