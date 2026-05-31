#ifndef FORMAT_H
#define FORMAT_H

#include <stddef.h>
#include <stdint.h>

char *format_number(uint64_t number, char *buffer, size_t buffer_size);

#endif // FORMAT_H