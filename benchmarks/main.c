#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"

typedef enum {
  BENCH_MUTEX_SPSC = 1 << 0,
  BENCH_LF_SPSC = 1 << 1,
  BENCH_MUTEX_SPMC = 1 << 2,
  BENCH_LF_SPMC = 1 << 3,
  BENCH_MUTEX_MPSC = 1 << 4,
  BENCH_LF_MPSC = 1 << 5,
  BENCH_MUTEX_MPMC = 1 << 6,
  BENCH_LF_MPMC = 1 << 7,

  BENCH_SPSC = BENCH_MUTEX_SPSC | BENCH_LF_SPSC,
  BENCH_SPMC = BENCH_MUTEX_SPMC | BENCH_LF_SPMC,
  BENCH_MPSC = BENCH_MUTEX_MPSC | BENCH_LF_MPSC,
  BENCH_MPMC = BENCH_MUTEX_MPMC | BENCH_LF_MPMC,
} benchmark_type_t;

static const benchmark_type_t BENCHMARK_TYPES[] = {
    BENCH_MUTEX_SPSC, BENCH_LF_SPSC, BENCH_MUTEX_SPMC, BENCH_LF_SPMC,
    BENCH_MUTEX_MPSC, BENCH_LF_MPSC, BENCH_MUTEX_MPMC, BENCH_LF_MPMC,
};

static void run_benchmark(benchmark_type_t type) {
  switch (type) {
  case BENCH_MUTEX_SPSC:
    bench_mutex_spsc();
    break;

  case BENCH_LF_SPSC:
    bench_lf_spsc();
    break;

  case BENCH_MUTEX_SPMC:
    bench_mutex_spmc();
    break;

  case BENCH_LF_SPMC:
    bench_lf_spmc();
    break;

  case BENCH_MUTEX_MPSC:
    bench_mutex_mpsc();
    break;

  case BENCH_LF_MPSC:
    bench_lf_mpsc();
    break;

  case BENCH_MUTEX_MPMC:
    break;

  case BENCH_LF_MPMC:
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

    if (strcmp(flag, "mutex-spsc") == 0) {
      benchmark_type |= BENCH_MUTEX_SPSC;
    } else if (strcmp(flag, "lf-spsc") == 0) {
      benchmark_type |= BENCH_LF_SPSC;
    } else if (strcmp(flag, "mutex-spmc") == 0) {
      benchmark_type |= BENCH_MUTEX_SPMC;
    } else if (strcmp(flag, "lf-spmc") == 0) {
      benchmark_type |= BENCH_LF_SPMC;
    } else if (strcmp(flag, "mutex-mpsc") == 0) {
      benchmark_type |= BENCH_MUTEX_MPSC;
    } else if (strcmp(flag, "lf-mpsc") == 0) {
      benchmark_type |= BENCH_LF_MPSC;
    } else if (strcmp(flag, "mutex-mpmc") == 0) {
      benchmark_type |= BENCH_MUTEX_MPMC;
    } else if (strcmp(flag, "lf-mpmc") == 0) {
      benchmark_type |= BENCH_LF_MPMC;
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
      fprintf(stderr,
              "Usage: %s [--mutex-spsc] [--lf-spsc] [--mutex-spmc] [--lf-spmc] [--mutex-mpsc] [--lf-mpsc] [--mutex-mpmc] "
              "[--lf-mpmc] [--spsc] [--spmc] [--mpsc] [--mpmc]\n",
              argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (benchmark_type == 0) {
    fprintf(stderr, "No benchmark type specified\n");
    fprintf(stderr,
            "Usage: %s [--mutex-spsc] [--lf-spsc] [--mutex-spmc] [--lf-spmc] [--mutex-mpsc] [--lf-mpsc] [--mutex-mpmc] "
            "[--lf-mpmc] [--spsc] [--spmc] [--mpsc] [--mpmc]\n",
            argv[0]);
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