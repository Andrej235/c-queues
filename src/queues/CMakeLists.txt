add_library(queues)

target_sources(queues
    PRIVATE queue.c

    PRIVATE mutex_q.c
    PRIVATE lf_spsc.c
    PRIVATE lf_spmc.c
    PRIVATE lf_mpsc.c
)

target_include_directories(queues PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
