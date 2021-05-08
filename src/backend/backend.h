#ifndef TASKFLOW_BACKEND_H
#define TASKFLOW_BACKEND_H

// 4K
#define TFC_MAX_PENDING_MSG (1 << 12)

typedef enum {
  TFC_SUCCESS = 0,
  TFC_RETRY,
  TFC_ERROR
} TFC_error_t;

typedef uint32_t imm_data_t;

typedef struct {
  void *buffer;         // 8 bytes
  int64_t size;         // 8 bytes
  int rank;             // 4 bytes
  imm_data_t imm_data;  // 4 bytes
} TFC_entry_t;

struct TFC_device_opaque_t;
typedef struct TFC_device_opaque_t* TFC_device_t;

static void TFC_init();
static void TFC_finalize();
static void TFC_init_device(TFC_device_t *device);
static void TFC_finalize_device(TFC_device_t *device);
static inline int TFC_rank_me(TFC_device_t device);
static inline int TFC_rank_n(TFC_device_t device);
static inline void* TFC_malloc(TFC_device_t device, int64_t size);
static inline void TFC_free(TFC_device_t device, void *buffer);
static inline TFC_error_t TFC_stream_push(TFC_device_t device, int rank, void *buffer, int64_t size, imm_data_t imm_data);
static inline bool TFC_progress_push(TFC_device_t device);
static inline TFC_error_t TFC_stream_pop(TFC_device_t device, TFC_entry_t *entry);
static void TFC_barrier(TFC_device_t device);

#ifdef TFC_USE_MPI
#include "backend_mpi.h"
#endif
#endif // TASKFLOW_BACKEND_H
