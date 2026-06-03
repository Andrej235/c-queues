#include <stdio.h>
#include <stdlib.h>

#include "lf_spmc.h"
#include "mutex_q.h"
#include "queue.h"
#include "test_utils.h"
#include "tests.h"

void test_lf_spmc() {
  queue_t *q_2 = lf_spmc_create(2);
  queue_t *q_16 = lf_spmc_create(16);
  queue_t *q_1024_1024 = lf_spmc_create(1024 * 1024);

  // fifo
  test_fifo_run("Lock Free (LF) SPMC (buffer = 2)", 5, q_2);
  test_fifo_run("Lock Free (LF) SPMC (buffer = 16)", 5, q_16);
  test_fifo_run("Lock Free (LF) SPMC (buffer = 1024 * 1024)", 5, q_1024_1024);

  // dupes, 10M ops each
  test_dupes_run("Lock Free (LF) SPMC (buffer = 2)", 1, 1, 10000000, q_2);
  test_dupes_run("Lock Free (LF) SPMC (buffer = 2)", 1, 16, 10000000, q_2);
  test_dupes_run("Lock Free (LF) SPMC (buffer = 2)", 1, 64, 10000000, q_2);
  test_dupes_run("Lock Free (LF) SPMC (buffer = 16)", 1, 1, 10000000, q_16);
  test_dupes_run("Lock Free (LF) SPMC (buffer = 16)", 1, 16, 10000000, q_16);
  test_dupes_run("Lock Free (LF) SPMC (buffer = 16)", 1, 64, 10000000, q_16);
  test_dupes_run("Lock Free (LF) SPMC (buffer = 1024 * 1024)", 1, 1, 10000000, q_1024_1024);
  test_dupes_run("Lock Free (LF) SPMC (buffer = 1024 * 1024)", 1, 16, 10000000, q_1024_1024);
  test_dupes_run("Lock Free (LF) SPMC (buffer = 1024 * 1024)", 1, 64, 10000000, q_1024_1024);

  queue_destroy(q_2);
  queue_destroy(q_16);
  queue_destroy(q_1024_1024);
}