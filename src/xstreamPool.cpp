//
// Created by jiakunyan on 3/12/21.
//
#include <cassert>
#include "tf.hpp"
using namespace std;

namespace tf {
XStreamPool::XStreamPool(Context *context_, int nxstreams_)
    : context(context_),
      nxstreams(nxstreams_) {}

XStreamPool::~XStreamPool() {
  assert(xstreams.empty());
}

void XStreamPool::start() {
  for (int i = 0; i < nxstreams; ++i) {
    thread xstream(xstream_fn, context, i);
    xstreams.push_back(move(xstream));
  }
}

void XStreamPool::join() {
  for (int i = 0; i < nxstreams; ++i) {
    xstreams[i].join();
  }
  xstreams.clear();
}

void xstream_fn(Context *context, int id) {
  context->scheduler.run(id);
}
}