#include <stdio.h>

#include "queue.h"
#include "spsc.h"

int main() {
  queue_t *q = mutex_spsc_create(16);
  queue_enqueue(q, 42);
  int value;
  queue_dequeue(q, &value);
  printf("Dequeued value: %d\n", value);
  queue_destroy(q);

  return 0;
}