#include <iostream>
#include "tf.hpp"
#include "bench_common.h"

using namespace std;

struct Config {
  bool touch_data = true;
  int min_msg_size = 8;
  int max_msg_size = 64 * 1024;
};

Config parseArgs(int argc, char **argv) {
  Config config;
  int opt;
  opterr = 0;

  struct option long_options[] = {
      {"min-msg-size", required_argument, 0, 'a'},
      {"max-msg-size", required_argument, 0, 'b'},
      {"touch-data",   required_argument, 0, 't'},
  };
  while ((opt = getopt_long(argc, argv, "t:", long_options, NULL)) != -1) {
    switch (opt) {
    case 'a':
      config.min_msg_size = atoi(optarg);
      break;
    case 'b':
      config.max_msg_size = atoi(optarg);
      break;
    case 't':
      config.touch_data = atoi(optarg);
      break;
    default:
      break;
    }
  }
  return config;
}

void run(const Config &config) {
  int rank, nranks;
  tf::Communicator comm;
  rank = comm.rank_me();
  nranks = comm.rank_n();
  MLOG_Assert(nranks == 2, "This benchmark requires exactly two processes\n");
  char value = 'a' + rank;
  char peer_value = 'a' + 1 - rank;
  char *buf;
  const int PAGE_SIZE = sysconf(_SC_PAGESIZE);
  posix_memalign((void **)&buf, PAGE_SIZE, config.max_msg_size);
  if (!buf) {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(EXIT_FAILURE);
  }

  bool recvFlag = false;
  function<void(tf::view<char>&)> fn = [&](tf::view<char>& message) {
    if (config.touch_data) check_buffer(message.data(), message.size(), peer_value);
    recvFlag = true;
  };
  auto am = comm.makeActiveMsg(fn);

  if (rank == 0) {
    RUN_VARY_MSG({config.min_msg_size, config.max_msg_size}, true, [&](int msg_size, int iter) {
      if (config.touch_data) write_buffer((char*) buf, msg_size, value);
      tf::view<char> message(buf, msg_size);
      am.send(1 - rank, message);
      while (!recvFlag) comm.progress();
      recvFlag = false;
    });
  } else {
    RUN_VARY_MSG({config.min_msg_size, config.max_msg_size}, false, [&](int msg_size, int iter) {
      while (!recvFlag) comm.progress();
      recvFlag = false;
      if (config.touch_data) write_buffer((char*) buf, msg_size, value);
      tf::view<char> message(buf, msg_size);
      am.send(1 - rank, message);
    });
  }
  comm.drain();
  comm.barrier();
  free(buf);
}

int main(int argc, char **argv) {
  Config config = parseArgs(argc, argv);
  run(config);
  return 0;
}