default rel
global my_write
extern __errno_location

section .text

my_write:
    mov rax, 1
    syscall
    cmp rax, 0
    jge .done

    neg rax
    mov r8, rax
    call __errno_location wrt ..plt
    mov dword [rax], r8d
    mov rax, -1

.done:
    ret