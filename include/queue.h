// TODO: make queues work with size_t instead of int

#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

typedef struct queue queue_t;

typedef struct {
  int (*enqueue)(queue_t *q, int value);
  int (*dequeue)(queue_t *q, int *out);
  int (*count)(queue_t *q);
  void (*destroy)(queue_t *q);
} queue_vtable_t;

struct queue {
  void *impl;
  const queue_vtable_t *ops;
};

// Generic interface
int queue_enqueue(queue_t *q, int value);
int queue_dequeue(queue_t *q, int *out);
int queue_count(queue_t *q);
void queue_destroy(queue_t *q);

#endif