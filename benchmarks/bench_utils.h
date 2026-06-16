#ifndef BENCH_UTILS_H
#define BENCH_UTILS_H

#include "queue.h"

/**
 * @brief Meassures the throughput of a queue, runs for a specified duration, and logs the average number of operations
 * performed per second.
 *
 * @param name Benchmark name, used only for logging
 * @param n_producers Number of producer threads
 * @param n_consumers Number of consumer threads
 * @param warmup_seconds Duration of the warmup phase in seconds, during which the benchmark runs but does not record any
 * measurements. This allows the system to reach a steady state before measurements are taken
 * @param runtime_seconds Duration of the runtime phase in seconds, during which the benchmark records measurements
 * @param q Queue to benchmark
 */
void bench_throughput_run(char *name, int n_producers, int n_consumers, float warmup_seconds, float runtime_seconds,
                          queue_t *q);

/**
 * @brief Meassures the time it takes to perform a specified number of operations on a queue, logs total time taken.
 *
 * @param name Benchmark name, used only for logging
 * @param n_producers Number of producer threads
 * @param n_consumers Number of consumer threads
 * @param warmup_seconds Duration of the warmup phase in seconds, during which the benchmark runs but does not record any
 * measurements. This allows the system to reach a steady state before measurements are taken
 * @param runtime_operations Number of operations to perform during the runtime phase
 * @param q Queue to benchmark
 */
void bench_nops_run(char *name, int n_producers, int n_consumers, float warmup_seconds, int runtime_operations, queue_t *q);

#endif // BENCH_UTILS_H
