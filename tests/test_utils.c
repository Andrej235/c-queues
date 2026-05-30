#include "test_utils.h"

#include <stdatomic.h>
#include <stdio.h>

#include "queue.h"

void test_fifo_run(char *name, float run_duration, queue_t *q) {}

void test_dupes_run(char *name, int producers, int consumers, size_t items_count, queue_t *q) {}

void test_stress_ops_count_run(char *name, int max_producers, int max_consumers, float run_duration, queue_t *q) {}