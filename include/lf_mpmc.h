#ifndef lf_mpmc_H
#define lf_mpmc_H

#include <queue.h>

queue_t *lf_mpmc_create(size_t capacity);

void lf_mpmc_destroy(queue_t *q);
int lf_mpmc_push(queue_t *q, int item);
int lf_mpmc_pop(queue_t *q, int *item);
int lf_mpmc_count(queue_t *q);

#endif // lf_mpmc_H