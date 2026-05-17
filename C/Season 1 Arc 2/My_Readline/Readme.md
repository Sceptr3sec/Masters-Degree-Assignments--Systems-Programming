# Welcome to My Readline
***

## Task
The goal of this project is to implement a simplified version of the `readline` function in C.  
The challenge lies in:
- Managing dynamic memory manually (`malloc`, `free`).
- Handling partial reads and storing leftover data between calls.
- Splitting incoming data into properly terminated lines.
---

## Description
This project provides a function called `my_readline(int fd)` that:
- Reads data from a file descriptor in chunks (`READLINE_READ_SIZE`).
- Accumulates the data in a global buffer.
- Returns one line at a time (up to `\n`), dynamically allocated.
- Keeps leftover data between calls until a full line can be returned.

---

## Installation
Clone the repository and compile with `make`:
cd my_readline
make