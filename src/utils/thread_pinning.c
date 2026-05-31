#include "thread_pinning.h"

#include <pthread.h>

void pin_thread(int cpu) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}