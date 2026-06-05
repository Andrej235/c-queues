#include "lf_spmc.h"

#include <inttypes.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>

#include "queue.h"

typedef struct {
  int value;
  alignas(64) atomic_size_t seq; // used to determine if node is ready for push or pop
} lf_spmc_queue_node_t;

typedef struct lf_spmc_queue {
  lf_spmc_queue_node_t *buffer;
  alignas(64) size_t capacity;    // maximum number of items buffer can hold
  alignas(64) atomic_size_t head; // index to push to
  alignas(64) atomic_size_t tail; // index to pop from
} lf_spmc_queue_t;

static const queue_vtable_t lf_spmc_vtable = {
    .enqueue = lf_spmc_push,
    .dequeue = lf_spmc_pop,
    .count = lf_spmc_count,
    .destroy = lf_spmc_destroy,
};

queue_t *lf_spmc_create(size_t capacity) {
  if (capacity <= 1) {
    return NULL; // vyukov seq approach doesn't work for capacity of 1
  }

  if ((capacity & (capacity - 1)) != 0) {
    return NULL; // not a power of two
  }

  queue_t *q = malloc(sizeof(queue_t));
  if (!q) {
    return NULL;
  }

  lf_spmc_queue_t *imp = malloc(sizeof(*imp));
  if (!imp) {
    free(q);
    return NULL;
  }

  imp->buffer = calloc(capacity, sizeof(lf_spmc_queue_node_t));
  if (!imp->buffer) {
    free(q);
    free(imp);
    return NULL;
  }

  imp->capacity = capacity;
  atomic_init(&imp->head, 0);
  atomic_init(&imp->tail, 0);

  for (size_t i = 0; i < capacity; i++) { // init all seqs
    atomic_init(&imp->buffer[i].seq, i);
  }

  q->impl = imp;
  q->ops = &lf_spmc_vtable;

  return q;
}

void lf_spmc_destroy(queue_t *q) {
  if (!q) {
    return;
  }

  lf_spmc_queue_t *imp = (lf_spmc_queue_t *)q->impl;
  free(imp->buffer);
  free(imp);
  free(q);
}

int lf_spmc_push(queue_t *q, int item) {
  lf_spmc_queue_t *impl = (lf_spmc_queue_t *)q->impl;
  if (!impl) {
    return -1;
  }

  size_t head = atomic_load_explicit(&impl->head, memory_order_relaxed);
  lf_spmc_queue_node_t *node = &impl->buffer[head & (impl->capacity - 1)];

  size_t seq = atomic_load_explicit(&node->seq, memory_order_acquire);
  intptr_t diff = (intptr_t)seq - (intptr_t)head;

  if (diff != 0) {
    return -1; // full
  }

  node->value = item;
  atomic_store_explicit(&node->seq, head + 1, memory_order_release);
  atomic_store_explicit(&impl->head, head + 1, memory_order_relaxed);
  return 0;
}

int lf_spmc_pop(queue_t *q, int *item) {
  lf_spmc_queue_t *impl = (lf_spmc_queue_t *)q->impl;
  if (!impl) {
    return -1;
  }

  while (1) { // loop until empty or successful pop
    size_t tail = atomic_load_explicit(&impl->tail, memory_order_acquire);
    lf_spmc_queue_node_t *node = &impl->buffer[tail & (impl->capacity - 1)];

    size_t seq = atomic_load_explicit(&node->seq, memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)tail - 1;

    if (diff < 0) { // empty
      return -1;
    }

    if (diff > 0) { // another consumer popped before this one
      continue;
    }

    // try to claim the index
    if (atomic_compare_exchange_weak_explicit(&impl->tail, &tail, tail + 1, memory_order_acq_rel, memory_order_acquire)) {
      *item = node->value;

      atomic_store_explicit(&node->seq, tail + impl->capacity, memory_order_release);

      return 0;
    }
  }
}

int lf_spmc_count(queue_t *q) {
  lf_spmc_queue_t *impl = (lf_spmc_queue_t *)q->impl;
  if (!impl) {
    return -1;
  }

  size_t head = atomic_load_explicit(&impl->head, memory_order_acquire);
  size_t tail = atomic_load_explicit(&impl->tail, memory_order_acquire);
  return head - tail;
}