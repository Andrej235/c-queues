#ifndef lf_spmc_H
#define lf_spmc_H

#include <queue.h>

queue_t *lf_spmc_create(size_t capacity);

void lf_spmc_destroy(queue_t *q);
int lf_spmc_push(queue_t *q, int item);
int lf_spmc_pop(queue_t *q, int *item);
int lf_spmc_count(queue_t *q);

#endif // lf_spmc_H