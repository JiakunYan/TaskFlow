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

void Scheduler::run(int id) {
  while (true) {
    Task* task = context->taskpool.getNextReadyTask();
    if (task == nullptr) {
      if (testCompletion())
        break;
      else
        continue;
    }
    ++tasks_in_flight;
    task->run();
    task->status = TaskStatus::DONE;
    switch (task->status) {
    case TaskStatus::DONE:
      task->outdep();
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