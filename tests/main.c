#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tests.h"

typedef enum {
  TEST_SPSC = 1 << 0,
  TEST_SPMC = 1 << 1,
  TEST_MPSC = 1 << 2,
  TEST_MPMC = 1 << 3,
} test_type_t;

static const test_type_t TEST_TYPES[] = {TEST_SPSC, TEST_SPMC, TEST_MPSC, TEST_MPMC};

static void run_test(test_type_t type) {
  switch (type) {
  case TEST_SPSC:
    test_lf_spsc();
    break;

  case TEST_SPMC:
    test_lf_spmc();
    break;

  case TEST_MPSC:
    test_lf_mpsc();
    break;

  case TEST_MPMC:
    test_lf_mpmc();
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

    if (strcmp(flag, "spsc") == 0) {
      test_type |= TEST_SPSC;
    } else if (strcmp(flag, "spmc") == 0) {
      test_type |= TEST_SPMC;
    } else if (strcmp(flag, "mpsc") == 0) {
      test_type |= TEST_MPSC;
    } else if (strcmp(flag, "mpmc") == 0) {
      test_type |= TEST_MPMC;
    } else {
      fprintf(stderr, "Unknown test type: %s\n", flag);
      fprintf(stderr, "Usage: %s [--spsc] [--spmc] [--mpsc] [--mpmc]\n", argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (test_type == 0) {
    fprintf(stderr, "No test type specified\n");
    fprintf(stderr, "Usage: %s [--spsc] [--spmc] [--mpsc] [--mpmc]\n", argv[0]);
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