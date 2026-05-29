add_executable(tests)

target_sources(tests
    PRIVATE main.c

    PRIVATE test_utils.c
    PRIVATE test_spsc.c
)

target_include_directories(tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(tests PRIVATE queues)
