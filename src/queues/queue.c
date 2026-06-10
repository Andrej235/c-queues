#include "queue.h"

int queue_enqueue(queue_t *q, int value) { return q->ops->enqueue(q, value); }

int queue_dequeue(queue_t *q, int *out) { return q->ops->dequeue(q, out); }

void queue_destroy(queue_t *q) { q->ops->destroy(q); }
