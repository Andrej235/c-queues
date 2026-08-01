# c-queues

`c-queues` is a small C queue library with a common API and multiple
implementations:

- `mutex_queue` for a simple mutex-protected ring buffer
- `lf_spsc` for single-producer/single-consumer use
- `lf_spmc` for single-producer/multi-consumer use
- `lf_mpsc` for multi-producer/single-consumer use
- `lf_mpmc` for multi-producer/multi-consumer use

All queues store `int` values and use a fixed capacity. Capacities must be a
power of two.

## Build

The project uses CMake.

```sh
cmake -S . -B build
cmake --build build
```

This builds the library code plus two executables:

- `tests`
- `benchmarks`

## Run Tests

The test binary accepts one or more queue-type flags:

```sh
./build/tests/tests --spsc
./build/tests/tests --spmc
./build/tests/tests --mpsc
./build/tests/tests --mpmc
./build/tests/tests --spsc --spmc --mpsc --mpmc
```

## Run Benchmarks

The benchmark binary accepts one or more benchmark flags:

```sh
./build/benchmarks/benchmarks --mutex
./build/benchmarks/benchmarks --spsc
./build/benchmarks/benchmarks --spmc
./build/benchmarks/benchmarks --mpsc
./build/benchmarks/benchmarks --mpmc
./build/benchmarks/benchmarks --mutex --spsc --spmc --mpsc --mpmc
```

## API

Each queue implementation exposes create, push, pop, and destroy functions.
The generic wrapper in `include/queue.h` provides `queue_enqueue`,
`queue_dequeue`, and `queue_destroy`.

Example:

```c
#include "lf_spsc.h"
#include "queue.h"

int main(void) {
  queue_t *q = lf_spsc_create(1024);
  if (!q) {
    return 1;
  }

  queue_enqueue(q, 42);

  int value = 0;
  if (queue_dequeue(q, &value) == 0) {
    /* value == 42 */
  }

  queue_destroy(q);
  return 0;
}
```

## Notes

- Queue capacity must be non-zero and a power of two.
- The current API stores `int` values.
- `queue_destroy` and the type-specific destroy functions free the queue
  instance and its storage; callers must ensure no other threads are still
  using the queue.
