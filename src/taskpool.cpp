#include "tf.hpp"

namespace tf {
Taskpool::Taskpool(Context *context_, int nxstreams_)
    : context(context_),
      nxstreams(nxstreams_) {}

void Taskpool::markAsCompleted(Taskflow& taskflow, TaskIdx taskIdx) {
  // For tasktorrent style taskDep, this is a null op
}

bool Taskpool::signal(Taskflow& taskflow, TaskIdx taskIdx) {
  int indegree = taskflow.dependency_fn(taskIdx);
  if (indegree == 1) {
    Task *task = taskflow.makeTask(taskIdx);
    task->status = TaskStatus::READY;
    context->scheduler.putReadyTask(task);
    return true;
  }
  bool ret = false;
  auto key = std::make_pair(taskflow.id, taskIdx);

  depCounterLock.lock();
  auto search = depCounter.find(key);
  if (search == depCounter.end()) {
    assert(indegree > 1);
    auto insert_ret = depCounter.insert(std::make_pair(key, indegree - 1));
    assert(insert_ret.second); // (key,indegree-1) was successfully inserted
  } else {
    int count = --search->second;
    assert(count >= 0);
    if (count == 0) {
      depCounter.erase(key);
      Task *task = taskflow.makeTask(taskIdx);
      task->status = TaskStatus::READY;
      context->scheduler.putReadyTask(task);
      ret = true;
    }
  }
  depCounterLock.unlock();
  return ret;
}
}