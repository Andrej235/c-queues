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

#pragma region FIFO

typedef struct {
  queue_t *q;                      // queue to test
  atomic_int ready_count;          // number of threads that have signaled they are ready
  pthread_barrier_t start_barrier; // used to sync start
  atomic_int phase;                // 0 = waiting for all threads to be ready, 1 = runtime, 2 = stop
} fifo_shared_context_t __attribute__((aligned(64)));

typedef struct {
  int cpu;
  uint64_t operations; // local to thread, successful enqueue/dequeue operations during runtime, used to give scale to the
                       // output of the tests
  fifo_shared_context_t *shared;
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

  fifo_shared_context_t *shared = malloc(sizeof(fifo_shared_context_t));
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

#pragma endregion

#pragma region Dupes

typedef struct {
  queue_t *q;       // queue to test
  atomic_int phase; // 0 = running, 1 = stop
  atomic_size_t
      *dequeued_items_bitmap; // bitmap to track which items have been dequeued, used to detect duplicates and missing items
  size_t items_count;         // total number of items to be enqueued, used to determine the size of the bitmap
} dupes_shared_context_t __attribute__((aligned(64)));

typedef struct {
  int cpu;
  size_t start_index;      // starting index for enques
  size_t items_to_enqueue; // number of items to enqueue
  dupes_shared_context_t *shared;
} dupes_producer_thread_context_t __attribute__((aligned(64)));

typedef struct {
  int cpu;
  dupes_shared_context_t *shared;
} dupes_consumer_thread_context_t __attribute__((aligned(64)));

static void run_dupes_producer(void *arg);
static void run_dupes_consumer(void *arg);
static void add_item_to_bitmap(dupes_shared_context_t *shared, size_t item);

void test_dupes_run(char *name, int producers, int consumers, size_t items_count, queue_t *q) {
  if (producers <= 0) {
    fprintf(stderr, "Number of producers must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (consumers <= 0) {
    fprintf(stderr, "Number of consumers must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (items_count <= 0) {
    fprintf(stderr, "Items count must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (q == NULL) {
    fprintf(stderr, "Queue cannot be NULL\n");
    exit(EXIT_FAILURE);
  }

  printf("Running dupes test: %s\n", name);
  char buf[32];
  printf("Operations: %s spread across %d producers and %d consumers\n", format_number(items_count, buf, sizeof(buf)),
         producers, consumers);

  dupes_shared_context_t *shared = malloc(sizeof(dupes_shared_context_t));
  if (shared == NULL) {
    fprintf(stderr, "Failed to allocate shared context\n");
    exit(EXIT_FAILURE);
  }

  shared->q = q;
  atomic_init(&shared->phase, 0);
  shared->items_count = items_count;
  size_t bitmap_size = (items_count + 63) / 64; // number of uint64_t needed to track items_count bits
  shared->dequeued_items_bitmap = malloc(sizeof(atomic_size_t) * bitmap_size);

  if (shared->dequeued_items_bitmap == NULL) {
    fprintf(stderr, "Failed to allocate dequeued items bitmap\n");
    exit(EXIT_FAILURE);
  }

  for (size_t i = 0; i < bitmap_size; i++) {
    atomic_init(&shared->dequeued_items_bitmap[i], 0);
  }

  dupes_producer_thread_context_t *producer_ctxs = malloc(sizeof(dupes_producer_thread_context_t) * producers);
  if (producer_ctxs == NULL) {
    fprintf(stderr, "Failed to allocate producer contexts\n");
    exit(EXIT_FAILURE);
  }

  dupes_consumer_thread_context_t *consumer_ctxs = malloc(sizeof(dupes_consumer_thread_context_t) * consumers);
  if (consumer_ctxs == NULL) {
    fprintf(stderr, "Failed to allocate consumer contexts\n");
    exit(EXIT_FAILURE);
  }

  pthread_t *producer_threads = malloc(sizeof(pthread_t) * producers);
  if (producer_threads == NULL) {
    fprintf(stderr, "Failed to allocate producer threads\n");
    exit(EXIT_FAILURE);
  }

  pthread_t *consumer_threads = malloc(sizeof(pthread_t) * consumers);
  if (consumer_threads == NULL) {
    fprintf(stderr, "Failed to allocate consumer threads\n");
    exit(EXIT_FAILURE);
  }

  long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  int items_per_producer = items_count / producers;

  for (int i = 0; i < producers; i++) {
    producer_ctxs[i] = (dupes_producer_thread_context_t){
        .cpu = i % cpu_count,
        .start_index = items_per_producer * i,
        .items_to_enqueue =
            items_per_producer + (i == producers - 1 ? items_count % producers : 0), // last producer enqueues the remainder
        .shared = shared,
    };

    if (pthread_create(&producer_threads[i], NULL, (void *(*)(void *))run_dupes_producer, &producer_ctxs[i]) != 0) {
      fprintf(stderr, "Failed to create producer thread %d\n", i);
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < consumers; i++) {
    consumer_ctxs[i] = (dupes_consumer_thread_context_t){
        .cpu = (producers + i) % cpu_count,
        .shared = shared,
    };

    if (pthread_create(&consumer_threads[i], NULL, (void *(*)(void *))run_dupes_consumer, &consumer_ctxs[i]) != 0) {
      fprintf(stderr, "Failed to create consumer thread %d\n", i);
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < producers; i++) {
    pthread_join(producer_threads[i], NULL);
  }

  printf("All producers finished enqueuing\n");

  atomic_store(&shared->phase, 1); // signal consumers to stop after producers are done

  for (int i = 0; i < consumers; i++) {
    pthread_join(consumer_threads[i], NULL);
  }

  // verify all items were dequeued
  for (size_t i = 0; i < items_count; i++) {
    size_t bitmap_index = i / 64;
    size_t bit_offset = i % 64;
    uint64_t bit_mask = (uint64_t)1 << bit_offset;

    if ((atomic_load_explicit(&shared->dequeued_items_bitmap[bitmap_index], memory_order_relaxed) & bit_mask) == 0) {
      fprintf(stderr, "Dupe test failed: item %zu was not dequeued\n", i);
      exit(EXIT_FAILURE);
    }
  }

  // cleanup
  free(consumer_threads);
  free(producer_threads);
  free(consumer_ctxs);
  free(producer_ctxs);
  free(shared->dequeued_items_bitmap);
  free(shared);

  printf("Dupe test passed: all %s items were enqueued and dequeued exactly once\n",
         format_number(items_count, buf, sizeof(buf)));
}

static void run_dupes_producer(void *arg) {
  dupes_producer_thread_context_t *ctx = (dupes_producer_thread_context_t *)arg;
  pin_thread(ctx->cpu);

  size_t end_index = ctx->start_index + ctx->items_to_enqueue;
  for (size_t i = ctx->start_index; i < end_index; i++) {
    while (queue_enqueue(ctx->shared->q, i) != 0) {
      // retry until the item is enqueued to not skip any items
    }
  }
  
  printf("Producer on CPU %d finished enqueuing items [%zu, %zu)\n", ctx->cpu, ctx->start_index, end_index);
}

static void run_dupes_consumer(void *arg) {
  dupes_consumer_thread_context_t *ctx = (dupes_consumer_thread_context_t *)arg;
  pin_thread(4);

  int item = 0;
  while (atomic_load_explicit(&ctx->shared->phase, memory_order_acquire) < 1) {
    if (queue_dequeue(ctx->shared->q, &item) == 0) {
      add_item_to_bitmap(ctx->shared, (size_t)item);
    }
  }

  // drain the queue after producers are done
  while (queue_dequeue(ctx->shared->q, &item) == 0) {
    add_item_to_bitmap(ctx->shared, (size_t)item);
  }
}

static void add_item_to_bitmap(dupes_shared_context_t *shared, size_t item) {
  if (item < 0 || (size_t)item >= shared->items_count) {
    fprintf(stderr, "Dupe test failed: dequeued item %d is out of valid range [0, " PRIu64 ")\n", item, shared->items_count);
    exit(EXIT_FAILURE);
  }

  size_t bitmap_index = item / 64;
  size_t bit_offset = item % 64;
  uint64_t bit_mask = (uint64_t)1 << bit_offset;

  // check if the bit is already set
  uint64_t old_value = atomic_fetch_or(&shared->dequeued_items_bitmap[bitmap_index], bit_mask);
  if ((old_value & bit_mask) != 0) {
    fprintf(stderr, "Dupe test failed: item %d was dequeued more than once\n", item);
    exit(EXIT_FAILURE);
  }
}

#pragma endregion

#pragma region Stress Ops Count

typedef struct {
  queue_t *q;       // queue to test
  atomic_int phase; // 0 = runtime, 1 = stop
} stress_shared_context_t __attribute__((aligned(64)));

typedef struct {
  int cpu;
  uint64_t operations; // local to thread, successful enqueue/dequeue operations during runtime, used to compare total
                       // enqueues and dequeues at the end of the test
  uint64_t rng_seed;   // random seed for the thread
  stress_shared_context_t *shared;
} stress_thread_context_t __attribute__((aligned(64)));

static inline uint64_t xorshift64(uint64_t *state);
void run_stress_producer(void *arg);
void run_stress_consumer(void *arg);

static volatile int stress_sink; // used to prevent compiler optimizing away the dequeue operations in the consumer

void test_stress_ops_count_run(char *name, int producers, int consumers, float run_duration, queue_t *q) {
  if (producers <= 0) {
    fprintf(stderr, "Number of producers must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (consumers <= 0) {
    fprintf(stderr, "Number of consumers must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (run_duration <= 0) {
    fprintf(stderr, "Run duration must be greater than 0\n");
    exit(EXIT_FAILURE);
  }

  if (q == NULL) {
    fprintf(stderr, "Queue cannot be NULL\n");
    exit(EXIT_FAILURE);
  }

  printf("Running stress ops count test: %s\n", name);
  printf("Time: %.1fs, Producers: %d, Consumers: %d\n", run_duration, producers, consumers);

  stress_shared_context_t *shared = malloc(sizeof(stress_shared_context_t));
  if (shared == NULL) {
    fprintf(stderr, "Failed to allocate shared context\n");
    exit(EXIT_FAILURE);
  }

  shared->q = q;
  atomic_init(&shared->phase, 0);

  stress_thread_context_t *producer_ctxs = malloc(sizeof(stress_thread_context_t) * producers);
  if (producer_ctxs == NULL) {
    fprintf(stderr, "Failed to allocate producer contexts\n");
    exit(EXIT_FAILURE);
  }

  stress_thread_context_t *consumer_ctxs = malloc(sizeof(stress_thread_context_t) * consumers);
  if (consumer_ctxs == NULL) {
    fprintf(stderr, "Failed to allocate consumer contexts\n");
    exit(EXIT_FAILURE);
  }

  pthread_t *producer_threads = malloc(sizeof(pthread_t) * producers);
  if (producer_threads == NULL) {
    fprintf(stderr, "Failed to allocate producer threads\n");
    exit(EXIT_FAILURE);
  }

  pthread_t *consumer_threads = malloc(sizeof(pthread_t) * consumers);
  if (consumer_threads == NULL) {
    fprintf(stderr, "Failed to allocate consumer threads\n");
    exit(EXIT_FAILURE);
  }

  long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);

  for (int i = 0; i < producers; i++) {
    producer_ctxs[i] = (stress_thread_context_t){
        .cpu = i % cpu_count,
        .operations = 0,
        .rng_seed = (i + 1) * 0x123456789ABCDEFULL,
        .shared = shared,
    };

    if (pthread_create(&producer_threads[i], NULL, (void *(*)(void *))run_stress_producer, &producer_ctxs[i]) != 0) {
      fprintf(stderr, "Failed to create producer thread %d\n", i);
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < consumers; i++) {
    consumer_ctxs[i] = (stress_thread_context_t){
        .cpu = (producers + i) % cpu_count,
        .operations = 0,
        .rng_seed = (producers + i + 1) * 0x123456789ABCDEFULL,
        .shared = shared,
    };

    if (pthread_create(&consumer_threads[i], NULL, (void *(*)(void *))run_stress_consumer, &consumer_ctxs[i]) != 0) {
      fprintf(stderr, "Failed to create consumer thread %d\n", i);
      exit(EXIT_FAILURE);
    }
  }

  // runtime
  sleep(run_duration);

  // signal threads to stop
  atomic_store(&shared->phase, 1);

  for (int i = 0; i < producers; i++) {
    pthread_join(producer_threads[i], NULL);
  }

  for (int i = 0; i < consumers; i++) {
    pthread_join(consumer_threads[i], NULL);
  }

  uint64_t total_enqueues = 0;
  for (int i = 0; i < producers; i++) {
    total_enqueues += producer_ctxs[i].operations;
  }
  uint64_t total_dequeues = 0;
  for (int i = 0; i < consumers; i++) {
    total_dequeues += consumer_ctxs[i].operations;
  }

  if (total_enqueues == total_dequeues) {
    char buf[32];
    printf("Total number of enqueues and dequeues match: %s\n",
           format_number(total_enqueues + total_dequeues, buf, sizeof(buf)));
  } else {
    char buf1[32];
    char buf2[32];
    fprintf(stderr, "Mismatch in total operations: enqueues = %s, dequeues = %s\n",
            format_number(total_enqueues, buf1, sizeof(buf1)), format_number(total_dequeues, buf2, sizeof(buf2)));
    exit(EXIT_FAILURE);
  }
}

void run_stress_producer(void *arg) {
  stress_thread_context_t *ctx = (stress_thread_context_t *)arg;
  pin_thread(ctx->cpu);

  uint64_t rng_state = ctx->rng_seed;

  // runtime
  while (atomic_load_explicit(&ctx->shared->phase, memory_order_acquire) < 1) {
    int item = (int)xorshift64(&rng_state); // generate a random item to enqueue
    if (queue_enqueue(ctx->shared->q, item) == 0) {
      ctx->operations++;
    }

    // randomly yield or sleep to increase the likelihood of exposing concurrency issues
    uint64_t rand = xorshift64(&rng_state);

    if ((rand & 0xFF) == 0) { // ~ 1 in 256
      sched_yield();
    }

    if ((rand & 0xFFF) == 0) { // ~ 1 in 4096
      usleep(1);
    }
  }
}

void run_stress_consumer(void *arg) {
  stress_thread_context_t *ctx = (stress_thread_context_t *)arg;
  pin_thread(ctx->cpu);

  uint64_t rng_state = ctx->rng_seed;
  int item;

  // runtime
  while (atomic_load_explicit(&ctx->shared->phase, memory_order_acquire) < 1) {
    if (queue_dequeue(ctx->shared->q, &item) == 0) {
      ctx->operations++;
      stress_sink += item; // prevent optimizing away the dequeue
    }

    // randomly yield or sleep to increase the likelihood of exposing concurrency issues
    uint64_t rand = xorshift64(&rng_state);

    if ((rand & 0xFF) == 0) { // ~ 1 in 256
      sched_yield();
    }

    if ((rand & 0xFFF) == 0) { // ~ 1 in 4096
      usleep(1);
    }
  }

  // drain the queue
  while (queue_dequeue(ctx->shared->q, &item) == 0) {
    ctx->operations++;
    stress_sink += item; // prevent optimizing away the dequeue
  }
}

static inline uint64_t xorshift64(uint64_t *state) {
  uint64_t x = *state;

  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;

  *state = x;
  return x;
}

#pragma endregion
