#include <stdio.h>

#include "bench.h"
#include "bench_utils.h"
#include "lf_mpsc.h"
#include "queue.h"

void bench_lf_mpsc() {
  queue_t *q = lf_mpsc_create(1024);
  bench_run("Lock-Free MPSC", 4, 1, 2, 5, q);
  queue_destroy(q);
}