#ifndef lf_spsc_H
#define lf_spsc_H

#include <queue.h>

queue_t *lf_spsc_create(size_t capacity);

void lf_spsc_destroy(queue_t *q);
int lf_spsc_push(queue_t *q, int item);
int lf_spsc_pop(queue_t *q, int *item);


#endif // lf_spsc_H