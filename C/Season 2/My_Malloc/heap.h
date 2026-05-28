#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include "util.h"

typedef struct block_header {
    size_t size;
    int free;
    struct block_header *next;
    struct block_header *prev;
} block_header;

/* ensure payload alignment even though header isn't multiple of ALIGNMENT */
#define HDR_SIZE (align_up(sizeof(block_header), ALIGNMENT))

block_header *heap_find_free(size_t need);
block_header *heap_request_chunk(size_t need);

void block_split(block_header *b, size_t need);
block_header *block_coalesce(block_header *b);

void *block_to_payload(block_header *b);
block_header *payload_to_block(void *p);

#endif