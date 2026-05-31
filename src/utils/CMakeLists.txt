add_library(utils)

target_sources(utils
    PRIVATE format.c
    PRIVATE thread_pinning.c
)

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
