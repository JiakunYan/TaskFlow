//
// Created by jiakunyan on 3/12/21.
//
#include <cassert>
#include "tf.hpp"
using namespace std;

namespace tf {

Scheduler::Scheduler(Context *context_, int nxstreams_)
    : context(context_),
      nxstreams(nxstreams_),
      tasks_in_flight(0) {}

Task* Scheduler::getNextReadyTask() {
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

void Scheduler::putReadyTask(Task *task) {
  readyTasksLock.lock();
  readyTasks.push(task);
  readyTasksLock.unlock();
}

void Scheduler::run(int id) {
  while (true) {
    Task* task = getNextReadyTask();
    if (task == nullptr) {
      if (testCompletion())
        break;
      else
        continue;
    }
    ++tasks_in_flight;
    task->run();
    task->status = TaskStatus::COMPLETED;
    switch (task->status) {
    case TaskStatus::COMPLETED:
      task->fulfill();
      delete task;
      break;
    default:
      throw runtime_error("invalid task status!");
    }
    --tasks_in_flight;
  }
}

/**
 * @todo this implementation might be wrong
 */
bool Scheduler::testCompletion() {
  return tasks_in_flight == 0;
}
}