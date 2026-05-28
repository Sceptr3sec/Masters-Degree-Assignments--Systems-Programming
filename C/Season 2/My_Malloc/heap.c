#define _GNU_SOURCE
#include "heap.h"
#include "util.h"
#include <sys/mman.h>
#include <unistd.h>

static block_header *g_head = NULL;

void *block_to_payload(block_header *b) {
    return (char *)b + HDR_SIZE;
}

block_header *payload_to_block(void *p) {
    return (block_header *)((char *)p - HDR_SIZE);
}

static block_header *list_tail(void) {
    block_header *t = g_head;
    while (t && t->next) t = t->next;
    return t;
}

block_header *heap_find_free(size_t need) {
    for (block_header *cur = g_head; cur; cur = cur->next) {
        if (cur->free && cur->size >= need) return cur;
    }
    return NULL;
}

block_header *heap_request_chunk(size_t need) {
    size_t page = getpagesize();
    size_t total = HDR_SIZE + need;

    size_t chunk = CHUNK_MIN_BYTES;
    if (chunk < total) chunk = total;
    chunk = align_up(chunk, page);

    void *mem = mmap(NULL, chunk,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1, 0);

    if (mem == MAP_FAILED) return NULL;

    block_header *b = mem;
    b->size = chunk - HDR_SIZE;
    b->free = 1;
    b->next = NULL;
    b->prev = NULL;

    if (!g_head) g_head = b;
    else {
        block_header *t = list_tail();
        t->next = b;
        b->prev = t;
    }

    return b;
}

void block_split(block_header *b, size_t need) {
    size_t rem = b->size - need;
    if (rem < HDR_SIZE + MIN_SPLIT_REMAINDER) return;

    block_header *nb = (block_header *)
        ((char *)block_to_payload(b) + need);

    nb->size = rem - HDR_SIZE;
    nb->free = 1;
    nb->next = b->next;
    nb->prev = b;

    if (b->next) b->next->prev = nb;
    b->next = nb;

    b->size = need;
}

static int adjacent(block_header *a, block_header *b) {
    char *end = (char *)block_to_payload(a) + a->size;
    return end == (char *)b;
}

block_header *block_coalesce(block_header *b) {
    if (b->next && b->next->free && adjacent(b, b->next)) {
        block_header *n = b->next;
        b->size += HDR_SIZE + n->size;
        b->next = n->next;
        if (n->next) n->next->prev = b;
    }

    if (b->prev && b->prev->free && adjacent(b->prev, b)) {
        block_header *p = b->prev;
        p->size += HDR_SIZE + b->size;
        p->next = b->next;
        if (b->next) b->next->prev = p;
        b = p;
    }

    return b;
}