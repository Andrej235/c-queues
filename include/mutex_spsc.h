#ifndef mutex_spsc_H
#define mutex_spsc_H

#include <queue.h>

queue_t *mutex_spsc_create(size_t capacity);

void mutex_spsc_destroy(queue_t *q);
int mutex_spsc_push(queue_t *q, int item);
int mutex_spsc_pop(queue_t *q, int *item);
int mutex_spsc_count(queue_t *q);

#endif // mutex_spsc_H