#include <stdio.h>

#include "lf_spsc.h"
#include "queue.h"

int main() {
  queue_t *q = lf_spsc_create(16);
  queue_enqueue(q, 42);
  int value;
  queue_dequeue(q, &value);
  printf("Dequeued value: %d\n", value);
  queue_destroy(q);

  return 0;
}