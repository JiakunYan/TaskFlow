#include "tf.hpp"


namespace tf {
Communicator::Communicator() : p_thread(nullptr), toStop(false), isDrained(false) {
  TFC_init_device(&device);
  rank = TFC_rank_me(device);
  nranks = TFC_rank_n(device);
  matchArray.resize(nranks);
}
Communicator::~Communicator() {
  MLOG_Assert(p_thread == nullptr, "Progress thread haven't been joined!\n");
  if (!isDrained)
    MLOG_Log(MLOG_LOG_WARN, "You must drain the communicator send queue before destroying it!\n");
  TFC_finalize_device(&device);
}

void Communicator::barrier() {
  TFC_barrier(device);
}

/**
 * @note this is not thread-safe
 */
void Communicator::progress() {
  TFC_entry_t entry;
  TFC_error_t ret = TFC_stream_poll(device, &entry);
  if (ret != TFC_SUCCESS)
    return;

  // process new message
  MatchEntry &matchEntry = matchArray[entry.rank];
  if (matchEntry.chunk_num == -1) {
    // this is a header message
    matchEntry.header.address = entry.buffer;
    matchEntry.header.size = entry.size;
    matchEntry.chunk_num = *(int*)entry.buffer;
    matchEntry.am_id = entry.imm_data;
  } else {
    // this is a chunk message
    matchEntry.chunks.push_back({entry.buffer, entry.size});
  }
  if (matchEntry.chunks.size() < matchEntry.chunk_num) {
    // wait for more chunk messages
    return;
  }

  // process new active message
  assert(matchEntry.am_id < activeMsgs.size());
  ActiveMsg &activeMsg = activeMsgs[matchEntry.am_id];
  activeMsg.run(matchEntry.header, matchEntry.chunks);

  // release resource
  TFC_free(device, matchEntry.header.address);
  for (auto &chunk : matchEntry.chunks) {
    TFC_free(device, chunk.address);
  }
  matchEntry.chunk_num = -1;
  matchEntry.chunks.clear();
}

void Communicator::drain() {
  bool isSendDone = false;
  do {
    isSendDone = TFC_progress_push(device);
  } while (!isSendDone);
  isDrained = true;
}

void Communicator::startPrgThread() {
  MLOG_Assert(p_thread == nullptr, "Already have a progress thread!\n");
  toStop = false;
  p_thread = new std::thread([&]() {
    while (!toStop) {
      progress();
    }
  });
}

void Communicator::joinPrgThread() {
  MLOG_Assert(p_thread != nullptr, "Do not have a progress thread\n");
  toStop = true;
  p_thread->join();
  delete p_thread;
  p_thread = nullptr;
}

} // namespace tf
