global my_memcpy

section .text

my_memcpy:
    mov rax, rdi
    test rdx, rdx
    je .done

.loop:
    mov cl, [rsi]
    mov [rdi], cl
    inc rsi
    inc rdi
    dec rdx
    jne .loop

.done:
    ret