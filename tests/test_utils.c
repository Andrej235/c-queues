#define _XOPEN_SOURCE 600

#include "test_utils.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "format.h"
#include "queue.h"
#include "thread_pinning.h"

typedef struct {
  queue_t *q;                      // queue to benchmark
  atomic_int ready_count;          // number of threads that have signaled they are ready
  pthread_barrier_t start_barrier; // used to sync warmup start
  atomic_int phase;                // 0 = waiting for all threads to be ready, 1 = runtime, 2 = stop
} shared_context_t __attribute__((aligned(64)));

typedef struct {
  int cpu;
  uint64_t operations; // local to thread, successful enqueue/dequeue operations during runtime, used to give scale to the
                       // output of the tests
  shared_context_t *shared;
} fifo_thread_context_t __attribute__((aligned(64)));

static void run_fifo_producer(void *arg);
static void run_fifo_consumer(void *arg);

void test_fifo_run(char *name, float run_duration, queue_t *q) {
  if (run_duration <= 0) {
    fprintf(stderr, "Run duration must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (q == NULL) {
    fprintf(stderr, "Queue cannot be NULL\n");
    exit(EXIT_FAILURE);
  }

  printf("Running FIFO test: %s\n", name);
  printf("Time: %.1fs\n", run_duration);

  shared_context_t *shared = malloc(sizeof(shared_context_t));
  if (shared == NULL) {
    fprintf(stderr, "Failed to allocate shared context\n");
    exit(EXIT_FAILURE);
  }

  shared->q = q;
  pthread_barrier_init(&shared->start_barrier, NULL, 2);
  atomic_init(&shared->ready_count, 0);
  atomic_init(&shared->phase, 0);

  fifo_thread_context_t *producer_ctx = malloc(sizeof(fifo_thread_context_t));
  if (producer_ctx == NULL) {
    fprintf(stderr, "Failed to allocate producer context\n");
    exit(EXIT_FAILURE);
  }

  *producer_ctx = (fifo_thread_context_t){
      .cpu = 0,
      .operations = 0,
      .shared = shared,
  };

  fifo_thread_context_t *consumer_ctx = malloc(sizeof(fifo_thread_context_t));
  if (consumer_ctx == NULL) {
    fprintf(stderr, "Failed to allocate consumer context\n");
    exit(EXIT_FAILURE);
  }

  *consumer_ctx = (fifo_thread_context_t){
      .cpu = sysconf(_SC_NPROCESSORS_ONLN) > 1 ? 1 : 0,
      .operations = 0,
      .shared = shared,
  };

  pthread_t producer_thread;
  pthread_t consumer_thread;
  if (pthread_create(&producer_thread, NULL, (void *(*)(void *))run_fifo_producer, producer_ctx) != 0) {
    fprintf(stderr, "Failed to create producer thread\n");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&consumer_thread, NULL, (void *(*)(void *))run_fifo_consumer, consumer_ctx) != 0) {
    fprintf(stderr, "Failed to create consumer thread\n");
    exit(EXIT_FAILURE);
  }

  // wait for threads to be ready
  while (atomic_load(&shared->ready_count) < 2) {
    usleep(1000);
  }

  atomic_store(&shared->phase, 1);
  sleep(run_duration);

  atomic_store(&shared->phase, 2);
  pthread_join(producer_thread, NULL);
  pthread_join(consumer_thread, NULL);

  if (producer_ctx->operations != consumer_ctx->operations) {
    char buffer1[32];
    char buffer2[32];
    fprintf(stderr, "Mismatch in operations count: producer = %s, consumer = %s\n",
            format_number(producer_ctx->operations, buffer1, sizeof(buffer1)),
            format_number(consumer_ctx->operations, buffer2, sizeof(buffer2)));
    exit(EXIT_FAILURE);
  } else {
    char buffer[32];
    printf("Total operations: %s\n", format_number(producer_ctx->operations * 2, buffer, sizeof(buffer)));
  }

  // cleanup
  free(consumer_ctx);
  free(producer_ctx);
  pthread_barrier_destroy(&shared->start_barrier);
  free(shared);
}

static void run_fifo_producer(void *arg) {
  fifo_thread_context_t *ctx = (fifo_thread_context_t *)arg;
  pin_thread(ctx->cpu);

  // signal ready
  atomic_fetch_add(&ctx->shared->ready_count, 1);
  pthread_barrier_wait(&ctx->shared->start_barrier);

  int item = 0;

  // runtime
  while (atomic_load_explicit(&ctx->shared->phase, memory_order_acquire) < 2) {
    if (queue_enqueue(ctx->shared->q, item) == 0) {
      item++;
      ctx->operations++;
    }
  }
}

static void run_fifo_consumer(void *arg) {
  fifo_thread_context_t *ctx = (fifo_thread_context_t *)arg;
  pin_thread(ctx->cpu);

  // signal ready
  atomic_fetch_add(&ctx->shared->ready_count, 1);
  pthread_barrier_wait(&ctx->shared->start_barrier);

  int item = 0;
  int last_item = -1;

  // runtime
  while (atomic_load_explicit(&ctx->shared->phase, memory_order_acquire) < 2) {
    if (queue_dequeue(ctx->shared->q, &item) == 0) {
      if (item != last_item + 1) {
        fprintf(stderr, "FIFO test failed: expected %d but got %d\n", last_item + 1, item);
        exit(EXIT_FAILURE);
      }
      last_item = item;
      ctx->operations++;
    }
  }

  // drain the queue
  // the producer stops generating new items at the end of runtime
  while (queue_dequeue(ctx->shared->q, &item) == 0) {
    if (item != last_item + 1) {
      fprintf(stderr, "FIFO test failed during drain: expected %d but got %d\n", last_item + 1, item);
      exit(EXIT_FAILURE);
    }
    last_item = item;
    ctx->operations++;
  }
}

void test_dupes_run(char *name, int producers, int consumers, size_t items_count, queue_t *q) {}

void test_stress_ops_count_run(char *name, int max_producers, int max_consumers, float run_duration, queue_t *q) {}
