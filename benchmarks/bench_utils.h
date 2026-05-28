#ifndef BENCH_UTILS_H
#define BENCH_UTILS_H

#include "queue.h"

void bench_run(char *name, int n_producers, int n_consumers, int warmup_seconds, int runtime_seconds, queue_t *q);

#endif // BENCH_UTILS_H
