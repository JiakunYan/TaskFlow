#ifndef TASKFLOW_COMMUNICATION_HPP
#define TASKFLOW_COMMUNICATION_HPP

namespace tf {

struct ActiveMsg {
  std::function<void(buffer_t header, const std::vector<buffer_t> &chunks)> run;
};

struct ActiveMsgIdx {
  TFC_device_t device;
  int id;
  template <typename... Args>
  void send(int rank, Args&&... args) {
    Serializer<Args...> s;
    s.set_size(std::forward<Args>(args)...);
    s.header.address = TFC_malloc(device, s.header.size);
    for (auto & chunk : s.chunks) {
      chunk.address = TFC_malloc(device, chunk.size);
    }
    s.write(std::forward<Args>(args)...);
    MLOG_Log(MLOG_LOG_TRACE, "send header size: %ld B\n", s.header.size);
    MLOG_Log(MLOG_LOG_TRACE, "send chunk num: %lu\n", s.chunks.size());
    for (int i = 0; i < s.chunks.size(); ++i)
      MLOG_Log(MLOG_LOG_TRACE, "send chunk[%d] size: %ld B\n", i, s.chunks[i].size);
    TFC_error_t ret;
    TFC_entry_t entry;
    entry.rank = rank;
    entry.imm_data = id;
    entry.header.address = s.header.address;
    entry.header.size = s.header.size;
    entry.chunk_num = s.chunks.size();
    if (entry.chunk_num > 0) {
      entry.chunks =
          (TFC_buffer_t *)malloc(sizeof(TFC_buffer_t) * entry.chunk_num);
      for (int i = 0; i < s.chunks.size(); ++i) {
        entry.chunks[i].address = s.chunks[i].address;
        entry.chunks[i].size = s.chunks[i].size;
      }
    }
    do {
      ret = TFC_stream_push(device, entry);
    } while (ret != TFC_SUCCESS);
  }
};

class Communicator {
private:
  TFC_device_t device;
  int rank;
  int nranks;
  bool isDrained;
  std::vector<ActiveMsg> activeMsgs;
  volatile bool toStop;
  std::thread *p_thread;
public:
  explicit Communicator();
  ~Communicator();

  [[nodiscard]] int rank_me() const {
    return rank;
  }

  [[nodiscard]] int rank_n() const {
    return nranks;
  }

  template <typename... Args>
  ActiveMsgIdx makeActiveMsg(std::function<void(Args...)>& fn) {
    int id = activeMsgs.size();
    auto run_fn = [&](buffer_t header, const std::vector<buffer_t> &chunks) {
      Serializer<std::remove_reference_t<Args>...> s;
      s.header = header;
      s.chunks = chunks;
      auto tup = s.read();
      std::apply(fn, tup);
    };
    activeMsgs.push_back({run_fn});
    return ActiveMsgIdx{device, id};
  }

  void barrier();

  void progress();
  void drain();

  void startPrgThread();
  void joinPrgThread();
};
} // namespace tf

#endif // TASKFLOW_COMMUNICATION_HPP
