#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "queue.h"

/**
 * @brief Tests first in first out behavior of the queue. Runs with a single producer and a single consumer. The producer
 * enqueues items with increasing values, and the consumer checks that the values are dequeued in the same order. The test
 * runs for a specified duration, and the producer and consumer run in separate threads.
 *
 * @param name Test name, used only for logging
 * @param run_duration Test duration in seconds
 * @param q Queue to test
 */
void test_fifo_run(char *name, float run_duration, queue_t *q);

/**
 * @brief Tests the queue to ensure that no items are lost or duplicated when multiple producers and consumers are running
 * concurrently. Each producer enqueues a specified number of items with unique values, and each consumer dequeues items and
 * checks that they are unique and that no items are lost. The test runs until all items have been produced and consumed.
 * Logs any duplicates or lost items.
 *
 * @param name Test name, used only for logging
 * @param producers Number of producer threads
 * @param consumers Number of consumer threads
 * @param items_count Number of items *each* producer will enqueue
 * @param q Queue to test
 */
void test_dupes_run(char *name, int producers, int consumers, size_t items_count, queue_t *q);

/**
 * @brief Tests the queue under stress conditions with a specified number of producers and consumers. The test runs for a
 * specified duration, counting the number of enqueues and dequeues. The producers and consumers run in separate threads,
 * with random yields and sleeps to increase the likelihood of exposing concurrency issues. Logs the total number of
 * operations performed by producers and consumers at the end of the test. Test is considered successful if the total number
 * of enqueues matches the total number of dequeues, indicating a likelihood that no items were lost or duplicated during the
 * test.
 *
 * @param name Test name, used only for logging
 * @param producers Number of producer threads
 * @param consumers Number of consumer threads
 * @param run_duration Test duration in seconds
 * @param q Queue to test
 */
void test_stress_ops_count_run(char *name, int producers, int consumers, float run_duration, queue_t *q);

#endif // TEST_UTILS_H