#include <stdio.h>

#include "bench.h"
#include "bench_utils.h"
#include "lf_mpmc.h"
#include "mutex_q.h"
#include "queue.h"

void bench_mutex_mpmc() {
  queue_t *q = mutex_queue_create(1024);
  bench_run("Mutex MPMC", 4, 1, 2, 5, q);
  bench_run("Mutex MPMC", 1, 4, 2, 5, q);
  bench_run("Mutex MPMC", 4, 4, 2, 5, q);
  queue_destroy(q);
}

void bench_lf_mpmc() { 
  queue_t *q = lf_mpmc_create(1024);
  bench_run("Lock-Free MPMC", 4, 1, 2, 5, q);
  bench_run("Lock-Free MPMC",  1, 4, 2, 5, q);
  bench_run("Lock-Free MPMC", 4, 4, 2, 5, q);
  queue_destroy(q);
}