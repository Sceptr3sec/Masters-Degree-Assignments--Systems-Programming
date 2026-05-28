#include "heap.h"
#include "util.h"
#include <string.h>
#include <errno.h>

void *my_malloc(size_t size) {
    if (size == 0) size = 1;

    size_t need = align_up(size, ALIGNMENT);

    block_header *b = heap_find_free(need);
    if (!b) b = heap_request_chunk(need);
    if (!b) return NULL;

    block_split(b, need);
    b->free = 0;
    return block_to_payload(b);
}

void my_free(void *ptr) {
    if (!ptr) return;
    block_header *b = payload_to_block(ptr);
    b->free = 1;
    block_coalesce(b);
}

void *my_calloc(size_t nmemb, size_t size) {
    size_t total;
    if (mul_overflow_size_t(nmemb, size, &total)) {
        errno = ENOMEM;
        return NULL;
    }

    void *p = my_malloc(total);
    if (!p) return NULL;
    memset(p, 0, total);
    return p;
}

void *my_realloc(void *ptr, size_t size) {
    if (!ptr) return my_malloc(size);
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    block_header *b = payload_to_block(ptr);
    size_t need = align_up(size, ALIGNMENT);

    if (b->size >= need) {
        block_split(b, need);
        return ptr;
    }

    void *np = my_malloc(size);
    if (!np) return NULL;

    memcpy(np, ptr, min_size(b->size, size));
    my_free(ptr);
    return np;
}