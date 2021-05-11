#include "tf.hpp"


namespace tf {
Communicator::Communicator() : p_thread(nullptr), toStop(false) {
  MLOG_Init();
  TFC_init();
  TFC_init_device(&device);
  rank = TFC_rank_me(device);
  nranks = TFC_rank_n(device);
}
Communicator::~Communicator() {
  MLOG_Assert(p_thread == nullptr, "Progress thread haven't been joined!\n");
  TFC_finalize_device(&device);
  TFC_finalize();
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

  // we get an AM
  MLOG_DBG_Log(MLOG_LOG_TRACE, "recv header size: %ld B\n", entry.header.size);
  MLOG_DBG_Log(MLOG_LOG_TRACE, "recv chunk num: %d\n", entry.chunk_num);
  for (int i = 0; i < entry.chunk_num; ++i)
    MLOG_DBG_Log(MLOG_LOG_TRACE, "recv chunk[%d] size: %ld B\n", i, entry.chunks[i].size);
  // process new active message
  assert(entry.imm_data < activeMsgs.size());
  ActiveMsg &activeMsg = activeMsgs[entry.imm_data];
  buffer_t header;
  header.address = entry.header.address;
  header.size = entry.header.size;
  std::vector<buffer_t> chunks;
  for (int i = 0; i < entry.chunk_num; ++i) {
    chunks.push_back({entry.chunks[i].address, entry.chunks[i].size});
  }
  if (entry.chunk_num > 0)
    free(entry.chunks);
  activeMsg.run(header, chunks);

  // release resource
  TFC_free(device, header.address);
  for (auto &chunk : chunks) {
    TFC_free(device, chunk.address);
  }
}

void Communicator::drain() {
  bool isSendDone = false;
  do {
    isSendDone = TFC_progress_push(device);
  } while (!isSendDone);
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
