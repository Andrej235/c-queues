#include <stdio.h>

#include "bench.h"
#include "bench_utils.h"
#include "lf_spsc.h"
#include "mutex_spsc.h"
#include "queue.h"

void bench_mutex_spsc() {
  queue_t *q = mutex_spsc_create(1024);
  bench_run("Mutex SPSC", 1, 1, 3, 5, q);
  queue_destroy(q);
}

void bench_lf_spsc() {
  queue_t *q = lf_spsc_create(1024);
  bench_run("Lock-Free SPSC", 1, 1, 3, 5, q);
  queue_destroy(q);
}
