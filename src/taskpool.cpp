#include "tf.hpp"

namespace tf {
Taskpool::Taskpool(Context *context_, int nxstreams_)
    : context(context_),
      nxstreams(nxstreams_) {}

Task* Taskpool::getNextReadyTask() {
  Task *task;
  readyTasksLock.lock();
  if (!readyTasks.empty()) {
    task = readyTasks.top();
    readyTasks.pop();
  } else {
    task = nullptr;
  }
  readyTasksLock.unlock();
  return task;
}

void Taskpool::putReadyTask(Task *task) {
  readyTasksLock.lock();
  readyTasks.push(task);
  readyTasksLock.unlock();
}
}