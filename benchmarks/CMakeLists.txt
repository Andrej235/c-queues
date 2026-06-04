add_executable(benchmarks)

target_sources(benchmarks
    PRIVATE main.c

    PRIVATE bench_utils.c
    PRIVATE bench_spsc.c
    PRIVATE bench_spmc.c
    PRIVATE bench_mpsc.c
)

target_include_directories(benchmarks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(benchmarks PRIVATE queues)
target_link_libraries(benchmarks PRIVATE utils)
