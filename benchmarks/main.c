#include <stdio.h>
#include <string.h>

#include "bench.h"

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    char *flag = argv[i];
    if (flag[0] != '-' || flag[1] != '-') {
      continue;
    }

    flag += 2; // skip the "--" prefix

    if (strcmp(flag, "mutex-spsc") == 0) {
      mutex_spsc_bench();
    } else if (strcmp(flag, "lf-spsc") == 0) {
      lf_spsc_bench();
    }
  }

  return 0;
}