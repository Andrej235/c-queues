#include "lf_spsc.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>

#include "queue.h"

typedef struct lf_spsc_queue {
  char pad0[64];
  int *buffer;
  size_t buffer_mask; // maximum number of items buffer can hold - 1, used for wrapping indices
  char pad1[64];

  atomic_size_t head; // index to pop from
  char pad2[64];

  atomic_size_t tail; // index to push to
  char pad3[64];
} lf_spsc_queue_t;

static const queue_vtable_t lf_spsc_vtable = {
    .enqueue = lf_spsc_push,
    .dequeue = lf_spsc_pop,
    .destroy = lf_spsc_destroy,
};

queue_t *lf_spsc_create(size_t capacity) {
  if (capacity == 0) {
    return NULL;
  }

  if ((capacity & (capacity - 1)) != 0) {
    return NULL; // not a power of two
  }

  queue_t *q = malloc(sizeof(queue_t));
  if (!q) {
    return NULL;
  }

  lf_spsc_queue_t *imp = malloc(sizeof(*imp));
  if (!imp) {
    free(q);
    return NULL;
  }

  imp->buffer = calloc(capacity, sizeof(int));
  if (!imp->buffer) {
    free(q);
    free(imp);
    return NULL;
  }

  imp->buffer_mask = capacity - 1;
  atomic_init(&imp->head, 0);
  atomic_init(&imp->tail, 0);

  q->impl = imp;
  q->ops = &lf_spsc_vtable;

  return q;
}

void lf_spsc_destroy(queue_t *q) {
  if (!q) {
    return;
  }

  lf_spsc_queue_t *imp = (lf_spsc_queue_t *)q->impl;
  free(imp->buffer);
  free(imp);
  free(q);
}

int lf_spsc_push(queue_t *q, int item) {
  lf_spsc_queue_t *impl = (lf_spsc_queue_t *)q->impl;
  if (!impl) {
    return -1;
  }

  size_t head = atomic_load_explicit(&impl->head, memory_order_relaxed);

  if (atomic_load_explicit(&impl->tail, memory_order_acquire) + impl->buffer_mask + 1 == head) {
    return -1; // full
  }

  impl->buffer[head & impl->buffer_mask] = item;
  atomic_store_explicit(&impl->head, head + 1, memory_order_release);
  return 0;
}

int lf_spsc_pop(queue_t *q, int *item) {
  lf_spsc_queue_t *impl = (lf_spsc_queue_t *)q->impl;
  if (!impl) {
    return -1;
  }

  size_t tail = atomic_load_explicit(&impl->tail, memory_order_relaxed);

  if (atomic_load_explicit(&impl->head, memory_order_acquire) == tail) {
    return -1; // empty
  }

  *item = impl->buffer[tail & impl->buffer_mask];
  impl->buffer[tail & impl->buffer_mask] = 0;
  atomic_store_explicit(&impl->tail, tail + 1, memory_order_release);
  return 0;
}
