#ifndef mutex_H
#define mutex_H

#include <queue.h>

queue_t *mutex_queue_create(size_t capacity);

void mutex_queue_destroy(queue_t *q);
int mutex_queue_push(queue_t *q, int item);
int mutex_queue_pop(queue_t *q, int *item);
int mutex_queue_count(queue_t *q);

#endif // mutex_H