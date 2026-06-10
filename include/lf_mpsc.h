#ifndef lf_mpsc_H
#define lf_mpsc_H

#include <queue.h>

queue_t *lf_mpsc_create(size_t capacity);

void lf_mpsc_destroy(queue_t *q);
int lf_mpsc_push(queue_t *q, int item);
int lf_mpsc_pop(queue_t *q, int *item);

#endif // lf_mpsc_H