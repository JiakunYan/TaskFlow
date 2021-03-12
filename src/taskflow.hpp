#ifndef TASKFRAMEWORK_TASKFLOW_HPP
#define TASKFRAMEWORK_TASKFLOW_HPP

namespace tf {
enum TaskStatus {
  PENDING,
  READY,
  RUNNING,
  COMPLETED,
};

struct Task {
  TaskStatus status;
  std::function<void()> run;
  std::function<void()> fulfill;
  double priority;
  std::string name;

  struct OpLess {
    bool operator()(const Task *lhs, const Task *rhs) {
      return lhs->priority < rhs->priority;
    }
  };
};

class Taskflow {
public:
  Taskflow() : id(total_taskflow_num++) {
    task_fn = [](TaskIdx taskIdx) {
      printf("Taskflow: undefined task function\n");
    };
    dependency_fn = [](TaskIdx taskIdx) {
      printf("Taskflow: undefined indegree function\n");
      return 1;
    };
    fulfill_fn = [](TaskIdx taskIdx) { (void)taskIdx; };
    priority_fn = [](TaskIdx taskIdx) { return 0.0; };
    affinity_fn = [](TaskIdx taskIdx) { return 0; };
    binding_fn = [](TaskIdx taskIdx) { return false; };
    name_fn = [](TaskIdx taskIdx) { return std::to_string(taskIdx); };
  }

  Taskflow& set_task(std::function<void(TaskIdx)> f) {
    task_fn = std::move(f);
    return *this;
  }
  Taskflow& set_fulfill(std::function<void(TaskIdx)> f) {
    fulfill_fn = std::move(f);
    return *this;
  }
  Taskflow& set_dependency(std::function<int(TaskIdx)> f) {
    dependency_fn = std::move(f);
    return *this;
  }
  Taskflow& set_priority(std::function<double(TaskIdx)> f) {
    priority_fn = std::move(f);
    return *this;
  }
  Taskflow& set_affinity(std::function<int(TaskIdx)> f) {
    affinity_fn = std::move(f);
    return *this;
  }
  Taskflow& set_binding(std::function<bool(TaskIdx)> f) {
    binding_fn = std::move(f);
    return *this;
  }
  Taskflow& set_name(std::function<std::string(TaskIdx)> f) {
    name_fn = std::move(f);
    return *this;
  }

  Task* makeTask(TaskIdx taskIdx) {
    Task *t = new Task();
    t->run = [this, taskIdx]() { task_fn(taskIdx); };
    t->fulfill = [this, taskIdx]() { fulfill_fn(taskIdx); };
    t->name = name_fn(taskIdx);
    t->priority = priority_fn(taskIdx);
    // We have not implemented the distributed memory verison yet.
    assert(affinity_fn(taskIdx) == 0);
    // Current version makes all tasks unbound.
    assert(binding_fn(taskIdx) == false);
    return t;
  }
  int id;
  std::function<void(TaskIdx)> task_fn;
  std::function<void(TaskIdx)> fulfill_fn;
  std::function<double(TaskIdx)> priority_fn;
  std::function<int(TaskIdx)> dependency_fn;
  std::function<int(TaskIdx)> affinity_fn;
  std::function<bool(TaskIdx)> binding_fn;
  std::function<std::string(TaskIdx)> name_fn;
private:
  static std::atomic<int> total_taskflow_num;
};
}
#endif // TASKFRAMEWORK_TASKFLOW_HPP
