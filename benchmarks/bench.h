#ifndef BENCH_H
#define BENCH_H

void bench_mutex_spsc();
void bench_lf_spsc();

void bench_mutex_spmc();
void bench_lf_spmc();

void bench_mutex_mpsc();
void bench_lf_mpsc();

#endif // BENCH_H