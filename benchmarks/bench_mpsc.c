#include <stdio.h>

#include "bench.h"
#include "bench_utils.h"
#include "lf_mpsc.h"
#include "mutex_q.h"
#include "queue.h"

void bench_mutex_mpsc() {
  queue_t *q = mutex_queue_create(1024);
  bench_run("Mutex MPSC", 4, 1, 2, 5, q);
  queue_destroy(q);
}

void bench_lf_mpsc() {
  queue_t *q = lf_mpsc_create(1024);
  bench_run("Lock-Free MPSC", 4, 1, 2, 5, q);
  queue_destroy(q);
}