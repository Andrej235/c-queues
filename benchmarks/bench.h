#ifndef BENCH_H
#define BENCH_H

void bench_mutex_spsc();
void bench_lf_spsc();

void bench_mutex_spmc();
void bench_lf_spmc();

#endif // BENCH_H