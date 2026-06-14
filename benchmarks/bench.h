#ifndef BENCH_H
#define BENCH_H

void bench_mutex();
void bench_lf_spsc();
void bench_lf_spmc();
void bench_lf_mpsc();
void bench_lf_mpmc();

#endif // BENCH_H