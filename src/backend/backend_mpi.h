#ifndef TASKFLOW_BACKEND_MPI_H
#define TASKFLOW_BACKEND_MPI_H

#include "lcm_dequeue.h"
#include <mpi.h>

typedef TFC_entry_t TFCI_send_request_t;

typedef struct {
  void *buffer;
  MPI_Request request;
} TFCI_pending_request_t;

struct TFCI_device_t {
  pthread_spinlock_t request_sq_lock;
  LCM_dequeue_t request_sq;
  LCM_dequeue_t pending_sq;
  LCM_dequeue_t pending_rq;
};

static void TFC_init() {
  int flag;
  MPI_Initialized(&flag);
  if (flag == 0) {
//    MPI_Init(nullptr, nullptr);
    int provided;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_FUNNELED, &provided);
    MLOG_Assert(provided == MPI_THREAD_FUNNELED,
                "Cannot get MPI_THREAD_FUNNELED!\n");
  }
}

static void TFC_finalize() {
  MPI_Finalize();
}

static void TFC_init_device(TFC_device_t *device) {
  TFCI_device_t *p_device;
  posix_memalign((void **) &p_device, 64, sizeof(TFCI_device_t));
  pthread_spin_init(&p_device->request_sq_lock, PTHREAD_PROCESS_PRIVATE);
  LCM_dq_init(&p_device->request_sq, TFC_MAX_PENDING_MSG);
  LCM_dq_init(&p_device->pending_sq, TFC_MAX_PENDING_MSG);
  LCM_dq_init(&p_device->pending_rq, TFC_MAX_PENDING_MSG);
  *device = (TFC_device_t) p_device;
}
static void TFC_finalize_device(TFC_device_t *device) {
  TFCI_device_t *p_device = (TFCI_device_t *)*device;
  LCM_dq_finalize(&p_device->pending_rq);
  LCM_dq_finalize(&p_device->pending_sq);
  LCM_dq_finalize(&p_device->request_sq);
  pthread_spin_destroy(&p_device->request_sq_lock);
  free(p_device);
  *device = NULL;
}
static inline int TFC_rank_me(TFC_device_t device) {
  int rank_me;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_me);
  return rank_me;
}
static inline int TFC_rank_n(TFC_device_t device) {
  int rank_n;
  MPI_Comm_size(MPI_COMM_WORLD, &rank_n);
  return rank_n;
}
static inline void* TFC_malloc(TFC_device_t device, int64_t size) {
  void* buffer = malloc(size);
  MLOG_DBG_Assert(buffer != NULL, "Cannot malloc memory!");
  return buffer;
}
static inline void TFC_free(TFC_device_t device, void *buffer) {
  free(buffer);
}
static inline TFC_error_t TFC_stream_push(TFC_device_t device, int rank, void *buffer, int64_t size, imm_data_t imm_data) {
  TFCI_device_t *p_device = (TFCI_device_t *)device;
  TFCI_send_request_t *p_request = (TFCI_send_request_t *) malloc(sizeof(TFCI_send_request_t));
  p_request->rank = rank;
  p_request->buffer = buffer;
  p_request->size = size;
  p_request->imm_data = imm_data;
  pthread_spin_lock(&p_device->request_sq_lock);
  int ret = LCM_dq_push_top(&p_device->request_sq, (void*) p_request);
  pthread_spin_unlock(&p_device->request_sq_lock);
  if (ret == LCM_SUCCESS)
    return TFC_SUCCESS;
  else
    return TFC_RETRY;
}
static inline bool TFC_progress_push(TFC_device_t device) {
  bool isRequestSQEmpty = false;
  TFCI_device_t *p_device = (TFCI_device_t *)device;
  while (!LCM_dq_is_full(&p_device->pending_sq)) {
    pthread_spin_lock(&p_device->request_sq_lock);
    TFCI_send_request_t *p_request = (TFCI_send_request_t *) LCM_dq_pop_bot(&p_device->request_sq);
    pthread_spin_unlock(&p_device->request_sq_lock);
    if (p_request == NULL) {
      isRequestSQEmpty = true;
      break;
    }
    // send the message
    TFCI_pending_request_t *p_pending = (TFCI_pending_request_t*) malloc(sizeof(TFCI_pending_request_t));
    p_pending->buffer = p_request->buffer;
    MPI_Isend(p_request->buffer, p_request->size, MPI_BYTE,
              p_request->rank, p_request->imm_data, MPI_COMM_WORLD, &p_pending->request);
    int ret = LCM_dq_push_top(&p_device->pending_sq, (void*) p_pending);
    assert(ret == LCM_SUCCESS);
    // clean up resource
    free(p_request);
  }

  // check for completed Isend
  bool isPendingSQEmpty = false;
  while (true) {
    TFCI_pending_request_t *p_pending = (TFCI_pending_request_t *) LCM_dq_pop_bot(&p_device->pending_sq);
    if (p_pending == NULL) {
      isPendingSQEmpty = true;
      break;
    }
    int flag;
    MPI_Test(&p_pending->request, &flag, MPI_STATUS_IGNORE);
    if (!flag) {
      int ret = LCM_dq_push_bot(&p_device->pending_sq, p_pending);
      assert(ret == LCM_SUCCESS);
      break;
    }
    TFC_free(device, p_pending->buffer);
    free(p_pending);
  }
  return isRequestSQEmpty && isPendingSQEmpty;
}
static inline TFC_error_t TFC_stream_poll(TFC_device_t device, TFC_entry_t *entry) {
  assert(entry != NULL);
  // make progress on send queue
  TFCI_device_t *p_device = (TFCI_device_t *)device;
  TFC_progress_push(device);

  // probe and post Irecv
  while (!LCM_dq_is_full(&p_device->pending_rq)) {
    MPI_Status status;
    int flag;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
    if (!flag) {
      // nothing to receive
      break;
    }
    // has something to receive
    int size;
    MPI_Get_count(&status, MPI_BYTE, &size);
    // this can be slow, try Irecv
    TFCI_pending_request_t *p_pending = (TFCI_pending_request_t*) malloc(sizeof(TFCI_pending_request_t));
    void* buffer = TFC_malloc(device, size);
    p_pending->buffer = buffer;
    MPI_Irecv(buffer, size, MPI_BYTE, status.MPI_SOURCE,
              status.MPI_TAG, MPI_COMM_WORLD, &p_pending->request);
    int ret = LCM_dq_push_top(&p_device->pending_rq, (void*) p_pending);
    assert(ret == LCM_SUCCESS);
  }
  
  // check for Irecv
  TFCI_pending_request_t *p_pending = (TFCI_pending_request_t *) LCM_dq_pop_bot(&p_device->pending_rq);
  if (p_pending == NULL) {
    return TFC_RETRY;
  }
  MPI_Status status;
  int flag;
  MPI_Test(&p_pending->request, &flag, &status);
  if (!flag) {
    int ret = LCM_dq_push_bot(&p_device->pending_rq, p_pending);
    assert(ret == LCM_SUCCESS);
    return TFC_RETRY;
  } else {
    entry->imm_data = status.MPI_TAG;
    entry->rank = status.MPI_SOURCE;
    int size;
    MPI_Get_count(&status, MPI_BYTE, &size);
    entry->size = size;
    entry->buffer = p_pending->buffer;
    free(p_pending);
    return TFC_SUCCESS;
  }
}
static void TFC_barrier(TFC_device_t device) {
  MPI_Barrier(MPI_COMM_WORLD);
}


#endif // TASKFLOW_BACKEND_MPI_H
