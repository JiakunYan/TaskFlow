#include <utility>

#ifndef TASKFLOW_TASKCLASS_HPP
#define TASKFLOW_TASKCLASS_HPP

namespace tf {
enum TaskStatus {
  PENDING,
  READY,
  RUNNING,
  DONE,
};

struct Task {
  Context *p_context;
  std::function<void()> run;
  double priority;
  std::string name;

  struct OpLess {
    bool operator()(const Task *lhs, const Task *rhs) {
      return lhs->priority < rhs->priority;
    }
  };
};

template <typename TaskIdx>
class TaskClass {
public:
  TaskClass() : depCounter() {
    task_fn = [](TaskIdx taskIdx) {
      printf("TaskClass: undefined task function\n");
    };
    indep_fn = [](TaskIdx taskIdx) {
      printf("TaskClass: undefined indegree function\n");
      return 1;
    };
    outdep_fn = [](TaskIdx taskIdx) { (void)taskIdx; };
    priority_fn = [](TaskIdx taskIdx) { return 0.0; };
    affinity_fn = [](TaskIdx taskIdx) { return 0; };
    binding_fn = [](TaskIdx taskIdx) { return false; };
    name_fn = [](TaskIdx taskIdx) { return "AnonymousTask"; };
  }

  TaskClass &setTask(std::function<void(TaskIdx)> f) {
    task_fn = std::move(f);
    return *this;
  }
  TaskClass &setOutDep(std::function<void(TaskIdx)> f) {
    outdep_fn = std::move(f);
    return *this;
  }
  TaskClass &setInDep(std::function<int(TaskIdx)> f) {
    indep_fn = std::move(f);
    return *this;
  }
  /**
   * @brief The larger the priority is, the more important the task is
   * @param f the mapping from task index to priority
   * @return reference to the taskClass object
   */
  TaskClass &setPriority(std::function<double(TaskIdx)> f) {
    priority_fn = std::move(f);
    return *this;
  }
  /**
   * @brief set the process rank to run the task
   * @param f the mapping from task index to rank
   * @return reference to the taskClass object
   */
  TaskClass &setAffinity(std::function<int(TaskIdx)> f) {
    affinity_fn = std::move(f);
    return *this;
  }
  TaskClass & setBinding(std::function<bool(TaskIdx)> f) {
    binding_fn = std::move(f);
    return *this;
  }
  TaskClass & setName(std::function<std::string(TaskIdx)> f) {
    name_fn = std::move(f);
    return *this;
  }

  Task* makeTask(TaskIdx taskIdx) {
    Task *t = new Task();
    t->run = [this, taskIdx]() { task_fn(taskIdx); outdep_fn(taskIdx); };
    t->name = name_fn(taskIdx);
    t->priority = priority_fn(taskIdx);
    // We have not implemented the distributed memory verison yet.
    assert(affinity_fn(taskIdx) == 0);
    // Current version makes all tasks unbound.
    assert(binding_fn(taskIdx) == false);
    return t;
  }

  Task* signal(TaskIdx taskIdx, std::atomic<int64_t> &nTaskInFlight) {
    int indegree = indep_fn(taskIdx);
    if (indegree == 1) {
      MLOG_Log(MLOG_LOG_TRACE, "signal %s inDep 1/1\n", name_fn(taskIdx).c_str());
      ++nTaskInFlight;
      Task *p_task = makeTask(taskIdx);
      return p_task;
    }
    Task* ret = nullptr;

    depCounterLock.lock();
    auto search = depCounter.find(taskIdx);
    if (search == depCounter.end()) {
      assert(indegree > 1);
      ++nTaskInFlight;
      auto insert_ret = depCounter.insert(std::make_pair(taskIdx, indegree - 1));
      assert(insert_ret.second); // (taskIdx, indegree-1) was successfully inserted
      MLOG_Log(MLOG_LOG_TRACE, "signal %s inDep 1/%d\n", name_fn(taskIdx).c_str(), indegree);
    } else {
      int count = --search->second;
      MLOG_Log(MLOG_LOG_TRACE, "signal %s inDep %d/%d\n", name_fn(taskIdx).c_str(), indegree - count, indegree);
      assert(count >= 0);
      if (count == 0) {
        depCounter.erase(taskIdx);
        Task *p_task = makeTask(taskIdx);
        ret = p_task;
      }
    }
    depCounterLock.unlock();
    return ret;
  }

private:
  using DepMap = std::unordered_map<TaskIdx, int, hash_int_N<TaskIdx>>;
  DepMap depCounter;
  std::mutex depCounterLock;

  std::function<void(TaskIdx)> task_fn;
  std::function<void(TaskIdx)> outdep_fn;
  std::function<double(TaskIdx)> priority_fn;
  std::function<int(TaskIdx)> indep_fn;
  std::function<int(TaskIdx)> affinity_fn;
  std::function<bool(TaskIdx)> binding_fn;
  std::function<std::string(TaskIdx)> name_fn;
};
}
#endif // TASKFLOW_TASKCLASS_HPP
