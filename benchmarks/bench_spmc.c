#include <stdio.h>

#include "bench.h"
#include "bench_utils.h"
#include "lf_spmc.h"
#include "mutex_q.h"
#include "queue.h"

void bench_mutex_spmc() {
  queue_t *q = mutex_queue_create(1024);
  bench_run("Mutex SPMC", 1, 4, 2, 5, q);
  queue_destroy(q);
}

void bench_lf_spmc() {
  queue_t *q = lf_spmc_create(1024);
  bench_run("Lock-Free SPMC", 1, 4, 2, 5, q);
  queue_destroy(q);
}