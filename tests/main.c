#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tests.h"

typedef enum {
  TEST_MUTEX_SPSC = 1 << 0,
  TEST_LF_SPSC = 1 << 1,
  TEST_MUTEX_SPMC = 1 << 2,
  TEST_LF_SPMC = 1 << 3,
  TEST_MUTEX_MPSC = 1 << 4,
  TEST_LF_MPSC = 1 << 5,
  TEST_MUTEX_MPMC = 1 << 6,
  TEST_LF_MPMC = 1 << 7,

  TEST_SPSC = TEST_MUTEX_SPSC | TEST_LF_SPSC,
  TEST_SPMC = TEST_MUTEX_SPMC | TEST_LF_SPMC,
  TEST_MPSC = TEST_MUTEX_MPSC | TEST_LF_MPSC,
  TEST_MPMC = TEST_MUTEX_MPMC | TEST_LF_MPMC,
} test_type_t;

static const test_type_t TEST_TYPES[] = {
    TEST_MUTEX_SPSC, TEST_LF_SPSC, TEST_MUTEX_SPMC, TEST_LF_SPMC,
    TEST_MUTEX_MPSC, TEST_LF_MPSC, TEST_MUTEX_MPMC, TEST_LF_MPMC,
};

static void run_test(test_type_t type) {
  switch (type) {
  case TEST_MUTEX_SPSC:
    test_mutex_spsc();
    break;

  case TEST_LF_SPSC:
    test_lf_spsc();
    break;

  case TEST_MUTEX_SPMC:
    break;

  case TEST_LF_SPMC:
    break;

  case TEST_MUTEX_MPSC:
    break;

  case TEST_LF_MPSC:
    break;

  case TEST_MUTEX_MPMC:
    break;

  case TEST_LF_MPMC:
    break;

  default:
    break;
  }
}

int main(int argc, char *argv[]) {
  test_type_t test_type = 0;

  for (int i = 1; i < argc; i++) {
    char *flag = argv[i];
    if (flag[0] != '-' || flag[1] != '-') {
      continue;
    }

    flag += 2; // skip the "--" prefix

    if (strcmp(flag, "mutex-spsc") == 0) {
      test_type |= TEST_MUTEX_SPSC;
    } else if (strcmp(flag, "lf-spsc") == 0) {
      test_type |= TEST_LF_SPSC;
    } else if (strcmp(flag, "mutex-spmc") == 0) {
      test_type |= TEST_MUTEX_SPMC;
    } else if (strcmp(flag, "lf-spmc") == 0) {
      test_type |= TEST_LF_SPMC;
    } else if (strcmp(flag, "mutex-mpsc") == 0) {
      test_type |= TEST_MUTEX_MPSC;
    } else if (strcmp(flag, "lf-mpsc") == 0) {
      test_type |= TEST_LF_MPSC;
    } else if (strcmp(flag, "mutex-mpmc") == 0) {
      test_type |= TEST_MUTEX_MPMC;
    } else if (strcmp(flag, "lf-mpmc") == 0) {
      test_type |= TEST_LF_MPMC;
    } else if (strcmp(flag, "spsc") == 0) {
      test_type |= TEST_SPSC;
    } else if (strcmp(flag, "spmc") == 0) {
      test_type |= TEST_SPMC;
    } else if (strcmp(flag, "mpsc") == 0) {
      test_type |= TEST_MPSC;
    } else if (strcmp(flag, "mpmc") == 0) {
      test_type |= TEST_MPMC;
    } else {
      fprintf(stderr, "Unknown test type: %s\n", flag);
      fprintf(stderr,
              "Usage: %s [--mutex-spsc] [--lf-spsc] [--mutex-spmc] [--lf-spmc] [--mutex-mpsc] [--lf-mpsc] [--mutex-mpmc] "
              "[--lf-mpmc] [--spsc] [--spmc] [--mpsc] [--mpmc]\n",
              argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (test_type == 0) {
    fprintf(stderr, "No test type specified\n");
    fprintf(stderr,
            "Usage: %s [--mutex-spsc] [--lf-spsc] [--mutex-spmc] [--lf-spmc] [--mutex-mpsc] [--lf-mpsc] [--mutex-mpmc] "
            "[--lf-mpmc] [--spsc] [--spmc] [--mpsc] [--mpmc]\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  if (test_type & (test_type - 1)) {
    printf("------------------------------------\n");
    printf("----- Running multiple tests: ------\n");
    printf("------------------------------------\n");

    for (size_t i = 0; i < sizeof(TEST_TYPES) / sizeof(TEST_TYPES[0]); i++) {
      if (test_type & TEST_TYPES[i]) {
        printf("\n");
        run_test(TEST_TYPES[i]);
        printf("\n------------------------------------\n");
      }
    }

    printf("------- All tests completed --------\n");
    printf("------------------------------------\n");
  } else {
    run_test(test_type);
  }

  return 0;
}