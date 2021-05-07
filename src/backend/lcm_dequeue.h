#ifndef LCM_DEQUEUE_H
#define LCM_DEQUEUE_H

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define LCM_SUCCESS 1
#define LCM_RETRY 0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  size_t top;
  size_t bot;
  size_t size;
  void **container; // a pointer to type void*
} LCM_dequeue_t __attribute__((aligned(64)));

static inline void LCM_dq_init(LCM_dequeue_t *dq, size_t size);
static inline void LCM_dq_finalize(LCM_dequeue_t *dq);
static inline bool LCM_dq_is_full(LCM_dequeue_t *dq);
static inline int LCM_dq_push_top(LCM_dequeue_t *dq, void *p);
static inline int LCM_dq_push_bot(LCM_dequeue_t *dq, void *p);
static inline void *LCM_dq_pop_top(LCM_dequeue_t *dq);
static inline void *LCM_dq_pop_bot(LCM_dequeue_t *dq);

#ifdef __cplusplus
}
#endif

static inline void LCM_dq_init(LCM_dequeue_t* dq, size_t size)
{
  posix_memalign((void**) &(dq->container), 64, size * sizeof(void*));
  dq->top = 0;
  dq->bot = 0;
  dq->size = size;
}

static inline void LCM_dq_finalize(LCM_dequeue_t* dq) {
  free(dq->container);
  dq->container = NULL;
}

static inline bool LCM_dq_is_full(LCM_dequeue_t *dq) {
  size_t new_top = (dq->top + 1) % dq->size;
  return new_top == dq->bot;
}

static inline int LCM_dq_push_top(LCM_dequeue_t* dq, void* p)
{
  size_t new_top = (dq->top + 1) % dq->size;
  if (new_top == dq->bot) {
    return LCM_RETRY;
  }
  dq->container[dq->top] = p;
  dq->top = new_top;
  return LCM_SUCCESS;
};

static inline int LCM_dq_push_bot(LCM_dequeue_t* dq, void* p)
{
  size_t new_bot = (dq->bot + dq->size - 1) % dq->size;
  if (dq->top == new_bot) {
    return LCM_RETRY;
  }
  dq->container[dq->bot] = p;
  dq->bot = new_bot;
  return LCM_SUCCESS;
};

static inline void* LCM_dq_pop_top(LCM_dequeue_t* dq)
{
  void* ret = NULL;
  if (dq->top != dq->bot) {
    dq->top = (dq->top + dq->size - 1) % dq->size;
    ret = dq->container[dq->top];
  }
  return ret;
};

static inline void* LCM_dq_pop_bot(LCM_dequeue_t* dq)
{
  void* ret = NULL;
  if (dq->top != dq->bot) {
    ret = dq->container[dq->bot];
    dq->bot = (dq->bot + 1) % dq->size;
  }
  return ret;
};

#endif
