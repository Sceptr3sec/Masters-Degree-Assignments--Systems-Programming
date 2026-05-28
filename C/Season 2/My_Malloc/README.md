# Welcome to My Malloc
***

## Task
What is the problem? And where is the challenge? My_malloc is certainly one of the project you will learn the most in term of UNIX / Algorithm / Implementation. It's an endless project, there is no best solution. Dive in with a plan in order to don't be stuck inside this project. Create your own implementation of the malloc families functions in order to allocate memory: my_malloc, my_free, my_calloc, my_realloc

## Description
How have you solved the problem? I solved the problem by implementing a custom dynamic memory allocator that manages its own heap using mmap() and a doubly linked list of blocks.\
Defined a block_header for metadata (size, free flag, next/prev).
Requested large memory chunks with mmap() and stored them in a global doubly linked list.
Implemented my_malloc() using alignment, first-fit search, and block splitting.
Implemented my_free() with block coalescing to reduce fragmentation.
Implemented my_calloc() with overflow checks and zero-initialization.
Implemented my_realloc() with in-place expansion when possible, otherwise allocating/copying/freeing.
Used alignment, splitting, coalescing, and overflow checks to mimic standard allocator behavior.
UPDATE: Broke the monolithic allocator into separate modules by moving the heap/block management logic into heap.c and keeping only the public API in my_malloc.c. The internal data structures and helpers are now exposed through headers instead of being tangled in one giant file, so responsibilities are clearly separated. This makes the allocator easier to maintain and extend while preserving the same allocation strategy.

## Installation
How to install it? Install make, clone the project, and run the program.

## Usage
How does it work? After program is install, run ./my_malloc.
