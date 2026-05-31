#include "format.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char *format_number(uint64_t number, char *buffer, size_t buffer_size) {
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
