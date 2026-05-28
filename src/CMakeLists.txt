add_executable(queues)

target_sources(queues
    PRIVATE main.c

    PRIVATE queue.c

    PRIVATE mutex_spsc.c
    PRIVATE lf_spsc.c
)

target_include_directories(queues PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
