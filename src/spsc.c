#include "spsc.h"

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

#include "queue.h"

typedef struct mutex_spsc_queue {
  int *buffer;
  size_t capacity; // maximum number of items buffer can hold
  size_t head;     // index to pop from
  size_t tail;     // index to push to
  size_t count;    // current number of items
  pthread_mutex_t lock;
} mutex_spsc_queue_t;

static const queue_vtable_t mutex_spsc_vtable = {.enqueue = mutex_spsc_push, .dequeue = mutex_spsc_pop, .destroy = mutex_spsc_destroy};

// Create a queue with given capacity (must be > 0)
queue_t *mutex_spsc_create(size_t capacity) {
  if (capacity == 0) {
    return NULL;
  }

  queue_t *q = malloc(sizeof(queue_t));
  if (!q) {
    return NULL;
  }

  mutex_spsc_queue_t *imp = malloc(sizeof(*imp));
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
  imp->head = 0;
  imp->tail = 0;
  imp->count = 0;

  if (pthread_mutex_init(&imp->lock, NULL) != 0) {
    free(q);
    free(imp->buffer);
    free(imp);
    return NULL;
  }

  q->impl = imp;
  q->ops = &mutex_spsc_vtable;

  return q;
}

// Destroy queue and free resources. Caller must ensure no threads are using it.
void mutex_spsc_destroy(queue_t *q) {
  mutex_spsc_queue_t *impl = (mutex_spsc_queue_t *)q->impl;
  if (!impl) {
    return;
  }

  pthread_mutex_destroy(&impl->lock);
  free(impl->buffer);
  free(impl);
}

// Push an item. Returns 0 on success, -1 if full.
int mutex_spsc_push(queue_t *q, int item) {
  mutex_spsc_queue_t *impl = (mutex_spsc_queue_t *)q->impl;
  if (!impl) {
    return -1;
  }

  pthread_mutex_lock(&impl->lock);
  if (impl->count == impl->capacity) {
    pthread_mutex_unlock(&impl->lock);
    return -1; // full
  }

  impl->buffer[impl->tail] = item;
  impl->tail = (impl->tail + 1) % impl->capacity;
  impl->count++;

  pthread_mutex_unlock(&impl->lock);
  return 0;
}

// Pop an item. Returns 0 on success and sets *item, -1 if empty.
int mutex_spsc_pop(queue_t *q, int *item) {
  mutex_spsc_queue_t *impl = (mutex_spsc_queue_t *)q->impl;
  if (!impl || !item) {
    return -1;
  }

  pthread_mutex_lock(&impl->lock);
  if (impl->count == 0) {
    pthread_mutex_unlock(&impl->lock);
    return -1; // empty
  }

  *item = impl->buffer[impl->head];
  impl->buffer[impl->head] = 0;
  impl->head = (impl->head + 1) % impl->capacity;
  impl->count--;

  pthread_mutex_unlock(&impl->lock);
  return 0;
}
