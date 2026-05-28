#ifndef BENCH_UTILS_H
#define BENCH_UTILS_H

#include "queue.h"

void bench_run(char *name, int n_producers, int n_consumers, float warmup_seconds, float runtime_seconds, queue_t *q);

#endif // BENCH_UTILS_H
