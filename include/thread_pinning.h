#ifndef THREAD_PINNING_H
#define THREAD_PINNING_H

#define _GNU_SOURCE

void pin_thread(int cpu);

#endif // THREAD_PINNING_H