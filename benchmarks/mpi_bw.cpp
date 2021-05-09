#include <mpi.h>
#include <unistd.h>
#include "bench_common.h"
#include "mlog.h"

#define MPI_CHECK(stmt)                                          \
do {                                                             \
   int mpi_errno = (stmt);                                       \
   if (MPI_SUCCESS != mpi_errno) {                               \
       fprintf(stderr, "[%s:%d] MPI call failed with %d \n",     \
        __FILE__, __LINE__,mpi_errno);                           \
       exit(EXIT_FAILURE);                                       \
   }                                                             \
} while (0)

struct Config {
    bool touch_data = true;
    int min_msg_size = 8;
    int max_msg_size = 64 * 1024;
    int window_size = 64;
};

Config parseArgs(int argc, char **argv) {
    Config config;
    int opt;
    opterr = 0;

    struct option long_options[] = {
            {"min-msg-size", required_argument, 0, 'a'},
            {"max-msg-size", required_argument, 0, 'b'},
            {"touch-data",   required_argument, 0, 't'},
            {"window-size",   required_argument, 0, 'w'},
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
            case 'w':
                config.window_size = atoi(optarg);
                break;
            default:
                break;
        }
    }
    return config;
}

void run(const Config &config) {
    int rank, nranks;
    MPI_CHECK(MPI_Init(0, 0));
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &nranks));
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
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

    MPI_Request requests[128];
    MLOG_Assert(config.window_size <= 128, "Window is too large!\n");
    if (rank == 0) {
        RUN_VARY_MSG({config.min_msg_size, config.max_msg_size}, true, [&](int msg_size, int iter) {
          for (int i = 0; i < config.window_size; ++i) {
              if (config.touch_data) write_buffer((char *)buf, msg_size, value);
              MPI_CHECK(MPI_Isend(buf, msg_size, MPI_CHAR, 1 - rank, 1, MPI_COMM_WORLD, requests + i));
            }
          MPI_CHECK(MPI_Waitall(config.window_size, requests, MPI_STATUS_IGNORE));
          MPI_CHECK(MPI_Recv(buf, msg_size, MPI_CHAR, 1 - rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
          if (config.touch_data) check_buffer((char*) buf, msg_size, peer_value);
        }, {0, config.window_size});
    } else {
        RUN_VARY_MSG({config.min_msg_size, config.max_msg_size}, false, [&](int msg_size, int iter) {
          for (int i = 0; i < config.window_size; ++i) {
            MPI_CHECK(MPI_Irecv(buf, msg_size, MPI_CHAR, 1 - rank, 1, MPI_COMM_WORLD, requests + i));
          }
          MPI_CHECK(MPI_Waitall(config.window_size, requests, MPI_STATUS_IGNORE));
          for (int i = 0; i < config.window_size; ++i) {
            if (config.touch_data) check_buffer((char*) buf, msg_size, peer_value);
          }
          if (config.touch_data) write_buffer((char *)buf, msg_size, value);
          MPI_CHECK(MPI_Send(buf, msg_size, MPI_CHAR, 1 - rank, 1, MPI_COMM_WORLD));
        }, {0, config.window_size});
    }
    free(buf);
    MPI_CHECK(MPI_Finalize());
}

int main(int argc, char **argv) {
    Config config = parseArgs(argc, argv);
    run(config);
    return 0;
}