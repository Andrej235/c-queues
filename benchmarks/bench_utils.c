#define _XOPEN_SOURCE 600

#include "bench_utils.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void run_producer(void *arg);
static void run_consumer(void *arg);
static char *format_number(uint64_t number, char *buffer, size_t buffer_size);

typedef struct {
  queue_t *q;                       // shared
  atomic_int *ready_count;          // shared
  pthread_barrier_t *start_barrier; // shared
  atomic_int *phase;                // shared
  uint64_t operations;              // local to thread, successful enqueue/dequeue operations during runtime
} thread_context_t __attribute__((aligned(64)));

void bench_run(char *name, int n_producers, int n_consumers, float warmup_seconds, float runtime_seconds, queue_t *q) {
  if (runtime_seconds <= 0) {
    fprintf(stderr, "Runtime seconds must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (warmup_seconds < 0) {
    fprintf(stderr, "Warmup seconds cannot be negative\n");
    exit(EXIT_FAILURE);
  }

  if (warmup_seconds == 0) {
    printf("Warning: No warmup phase, results may be skewed by startup overhead\n");
  }

  if (n_producers <= 0) {
    fprintf(stderr, "Number of producers must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (n_consumers <= 0) {
    fprintf(stderr, "Number of consumers must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (q == NULL) {
    fprintf(stderr, "Queue cannot be NULL\n");
    exit(EXIT_FAILURE);
  }

  printf("Running benchmark: %s\n", name);
  printf("Producers: %d, Consumers: %d\n", n_producers, n_consumers);
  printf("Time: %.1fs warmup, %.1fs runtime\n", warmup_seconds, runtime_seconds);

  atomic_int ready_count;
  pthread_barrier_t start_barrier; // sync warmup start
  atomic_int phase;                // 0 = not ready, 1 = warmup, 2 = runtime, 3 = done

  pthread_barrier_init(&start_barrier, NULL, n_producers + n_consumers);
  atomic_init(&ready_count, 0);
  atomic_init(&phase, 0);

  thread_context_t *producers = malloc(sizeof(thread_context_t) * n_producers);
  thread_context_t *consumers = malloc(sizeof(thread_context_t) * n_consumers);
  pthread_t *threads = malloc(sizeof(pthread_t) * (n_producers + n_consumers));

  if (producers == NULL || consumers == NULL) {
    fprintf(stderr, "Failed to allocate thread contexts\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < n_producers; i++) {
    producers[i].q = q;
    producers[i].ready_count = &ready_count;
    producers[i].start_barrier = &start_barrier;
    producers[i].phase = &phase;
    producers[i].operations = 0;

    pthread_t thread;
    if (pthread_create(&thread, NULL, (void *(*)(void *))run_producer, &producers[i]) != 0) {
      fprintf(stderr, "Failed to create producer thread\n");
      exit(EXIT_FAILURE);
    }
    threads[i] = thread;
  }

  for (int i = 0; i < n_consumers; i++) {
    consumers[i].q = q;
    consumers[i].ready_count = &ready_count;
    consumers[i].start_barrier = &start_barrier;
    consumers[i].phase = &phase;
    consumers[i].operations = 0;

    pthread_t thread;
    if (pthread_create(&thread, NULL, (void *(*)(void *))run_consumer, &consumers[i]) != 0) {
      fprintf(stderr, "Failed to create consumer thread\n");
      exit(EXIT_FAILURE);
    }
    threads[n_producers + i] = thread;
  }

  // wait for all threads to be ready
  while (atomic_load(&ready_count) < n_producers + n_consumers) {
    usleep(1000);
  }

  // start warmup
  atomic_store(&phase, 1);
  sleep(warmup_seconds);

  // start runtime
  atomic_store(&phase, 2);
  sleep(runtime_seconds);

  // stop all threads
  atomic_store(&phase, 3);
  for (int i = 0; i < n_producers + n_consumers; i++) {
    pthread_join(threads[i], NULL); // wait for thread to finish
  }

  // calculate total operations
  uint64_t ops_count = 0;
  for (int i = 0; i < n_producers; i++) {
    ops_count += producers[i].operations;
  }
  for (int i = 0; i < n_consumers; i++) {
    ops_count += consumers[i].operations;
  }

  char buffer[32];
  printf("Total operations: %s\n", format_number(ops_count, buffer, sizeof(buffer)));
  printf("Throughput: %s ops/sec\n", format_number(ops_count / runtime_seconds, buffer, sizeof(buffer)));

  // cleanup
  free(producers);
  free(consumers);
  free(threads);
  pthread_barrier_destroy(&start_barrier);
}

static void run_producer(void *arg) {
  thread_context_t *ctx = (thread_context_t *)arg;
  // todo: pin thread to a core

  // signal ready
  atomic_fetch_add(ctx->ready_count, 1);
  pthread_barrier_wait(ctx->start_barrier);

  int item = 0;

  // warmup
  while (atomic_load_explicit(ctx->phase, memory_order_acquire) < 2) {
    if (queue_enqueue(ctx->q, item) == 0) {
      item++;
    }
  }

  // runtime
  while (atomic_load_explicit(ctx->phase, memory_order_acquire) < 3) {
    if (queue_enqueue(ctx->q, item) == 0) {
      item++;
      ctx->operations++;
    }
  }
}

static void run_consumer(void *arg) {
  thread_context_t *ctx = (thread_context_t *)arg;
  // todo: pin thread to a core

  // signal ready
  atomic_fetch_add(ctx->ready_count, 1);
  pthread_barrier_wait(ctx->start_barrier);

  int item = 0;

  // warmup
  while (atomic_load_explicit(ctx->phase, memory_order_acquire) < 2) {
    if (queue_dequeue(ctx->q, &item) == 0) {
      item++;
    }
  }

  // runtime
  while (atomic_load_explicit(ctx->phase, memory_order_acquire) < 3) {
    if (queue_dequeue(ctx->q, &item) == 0) {
      item++;
      ctx->operations++;
    }
  }
}

static char *format_number(uint64_t number, char *buffer, size_t buffer_size) {
  char temp[32];
  snprintf(temp, sizeof(temp), "%" PRIu64, number);

  int len = strlen(temp);
  int commas = (len - 1) / 3;
  int new_len = len + commas;

  if ((size_t)(new_len + 1) > buffer_size) {
    return NULL; // buffer too small
  }

  buffer[new_len] = '\0';

  int i = len - 1;
  int j = new_len - 1;
  int digit_count = 0;

  while (i >= 0) {
    buffer[j--] = temp[i--];
    digit_count++;

    if (digit_count == 3 && i >= 0) {
      buffer[j--] = ',';
      digit_count = 0;
    }
  }

  return buffer;
}