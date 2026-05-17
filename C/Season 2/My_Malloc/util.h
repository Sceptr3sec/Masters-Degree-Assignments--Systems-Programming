#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#define ALIGNMENT 16
#define CHUNK_MIN_BYTES (64 * 1024)
#define MIN_SPLIT_REMAINDER 32

static inline size_t align_up(size_t x, size_t a) {
    return (x + (a - 1)) & ~(a - 1);
}

static inline size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

static inline int mul_overflow_size_t(size_t a, size_t b, size_t *out) {
    if (a == 0 || b == 0) { *out = 0; return 0; }
    if (a > SIZE_MAX / b) return 1;
    *out = a * b;
    return 0;
}

#endif