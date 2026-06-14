#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"

typedef enum {
  BENCH_MUTEX = 1 << 0,
  BENCH_SPSC = 1 << 1,
  BENCH_SPMC = 1 << 2,
  BENCH_MPSC = 1 << 3,
  BENCH_MPMC = 1 << 4,
} benchmark_type_t;

static const benchmark_type_t BENCHMARK_TYPES[] = {BENCH_MUTEX, BENCH_SPSC, BENCH_SPMC, BENCH_MPSC, BENCH_MPMC};

static void run_benchmark(benchmark_type_t type) {
  switch (type) {
  case BENCH_MUTEX:
    bench_mutex();
    break;

  case BENCH_SPSC:
    bench_lf_spsc();
    break;

  case BENCH_SPMC:
    bench_lf_spmc();
    break;

  case BENCH_MPSC:
    bench_lf_mpsc();
    break;

  case BENCH_MPMC:
    bench_lf_mpmc();
    break;

  default:
    break;
  }
}

int main(int argc, char *argv[]) {
  benchmark_type_t benchmark_type = 0;

  for (int i = 1; i < argc; i++) {
    char *flag = argv[i];
    if (flag[0] != '-' || flag[1] != '-') {
      continue;
    }

    flag += 2; // skip the "--" prefix

    if (strcmp(flag, "mutex") == 0) {
      benchmark_type |= BENCH_MUTEX;
    } else if (strcmp(flag, "spsc") == 0) {
      benchmark_type |= BENCH_SPSC;
    } else if (strcmp(flag, "spmc") == 0) {
      benchmark_type |= BENCH_SPMC;
    } else if (strcmp(flag, "mpsc") == 0) {
      benchmark_type |= BENCH_MPSC;
    } else if (strcmp(flag, "mpmc") == 0) {
      benchmark_type |= BENCH_MPMC;
    } else {
      fprintf(stderr, "Unknown benchmark type: %s\n", flag);
      fprintf(stderr, "Usage: %s [--mutex] [--spsc] [--spmc] [--mpsc] [--mpmc]\n", argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (benchmark_type == 0) {
    fprintf(stderr, "No benchmark type specified\n");
    fprintf(stderr, "Usage: %s [--mutex] [--spsc] [--spmc] [--mpsc] [--mpmc]\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (benchmark_type & (benchmark_type - 1)) {
    printf("------------------------------------\n");
    printf("--- Running multiple benchmarks: ---\n");
    printf("------------------------------------\n");

    for (size_t i = 0; i < sizeof(BENCHMARK_TYPES) / sizeof(BENCHMARK_TYPES[0]); i++) {
      if (benchmark_type & BENCHMARK_TYPES[i]) {
        printf("\n");
        run_benchmark(BENCHMARK_TYPES[i]);
        printf("\n------------------------------------\n");
      }
    }

    printf("----- All benchmarks completed -----\n");
    printf("------------------------------------\n");
  } else {
    run_benchmark(benchmark_type);
  }

  return 0;
}