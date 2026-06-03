#include <stdio.h>
#include <stdlib.h>

#include "lf_spsc.h"
#include "mutex_q.h"
#include "queue.h"
#include "test_utils.h"
#include "tests.h"

void test_lf_spsc() {
  queue_t *q_1 = lf_spsc_create(1);
  test_fifo_run("Lock Free (LF) SPSC (buffer = 1)", 5, q_1);
  queue_destroy(q_1);

  queue_t *q_16 = lf_spsc_create(16);
  test_fifo_run("Lock Free (LF) SPSC (buffer = 16)", 5, q_16);
  queue_destroy(q_16);

  queue_t *q_1024_1024 = lf_spsc_create(1024 * 1024);
  test_fifo_run("Lock Free (LF) SPSC (buffer = 1024 * 1024)", 5, q_1024_1024);
  queue_destroy(q_1024_1024);
}