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
    TFC_error_t ret;
    do {
      ret = TFC_stream_push(device, rank, s.header.address, s.header.size, id);
    } while (ret != TFC_SUCCESS);
    for (auto &chunk : s.chunks) {
      do {
        ret = TFC_stream_push(device, rank, chunk.address, chunk.size, id);
      } while (ret != TFC_SUCCESS);
    }
  }
};

struct MatchEntry {
  int am_id;
  int chunk_num = -1;
  buffer_t header;
  std::vector<buffer_t> chunks;
};

class Communicator {
private:
  TFC_device_t device;
  int rank;
  int nranks;
  bool isDrained;
  std::vector<ActiveMsg> activeMsgs;
  std::vector<MatchEntry> matchArray;
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
