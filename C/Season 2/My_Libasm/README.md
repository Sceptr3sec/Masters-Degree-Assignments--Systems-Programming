# Welcome to My Libasm
***

## Task
What is the problem? And where is the challenge? The goal of this project is to reimplement a small subset of standard C library functions in x86_64 assembly. The main challenge is understanding how function arguments, return values, memory access, and system calls work at the register and instruction level instead of relying on C. Another challenge is matching the behavior of the real libc functions closely enough to pass tests, especially for pointer handling, memory movement, and error management in read and write.

## Description
How have you solved the problem? I solved the project by rewriting each required function in assembly while following the Linux x86_64 calling convention. For string and memory functions, I used loops, comparisons, and register-based pointer manipulation to reproduce the expected behavior of the original libc versions. For my_read and my_write, I used raw syscalls and handled errors by updating errno correctly. The project was built and organized with a Makefile to assemble each source file and archive them into a static library.


## Installation
How to install your project? npm install? make? make re? Clone the repository and run: make

## Usage
How does it work? To use the library, compile your program and link it with libasm.a.