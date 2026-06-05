#include "lf_spsc.h"

#include <stdalign.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>

#include "queue.h"

typedef struct lf_spsc_queue {
  int *buffer;
  alignas(64) size_t capacity;    // maximum number of items buffer can hold
  alignas(64) atomic_size_t head; // index to pop from
  alignas(64) atomic_size_t tail; // index to push to
} lf_spsc_queue_t;

static const queue_vtable_t lf_spsc_vtable = {
    .enqueue = lf_spsc_push,
    .dequeue = lf_spsc_pop,
    .count = lf_spsc_count,
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

  imp->capacity = capacity;
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

  if (atomic_load_explicit(&impl->tail, memory_order_acquire) + impl->capacity == head) {
    return -1; // full
  }

  impl->buffer[head & (impl->capacity - 1)] = item;
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

  *item = impl->buffer[tail & (impl->capacity - 1)];
  impl->buffer[tail & (impl->capacity - 1)] = 0;
  atomic_store_explicit(&impl->tail, tail + 1, memory_order_release);
  return 0;
}

int lf_spsc_count(queue_t *q) {
  lf_spsc_queue_t *impl = (lf_spsc_queue_t *)q->impl;
  if (!impl) {
    return -1;
  }

  size_t head = atomic_load_explicit(&impl->head, memory_order_acquire);
  size_t tail = atomic_load_explicit(&impl->tail, memory_order_acquire);
  return head - tail;
}