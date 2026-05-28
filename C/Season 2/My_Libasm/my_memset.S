global my_memset

section .text

my_memset:
    mov rax, rdi
    mov cl, sil
    test rdx, rdx
    je .done

.loop:
    mov [rdi], cl
    inc rdi
    dec rdx
    jne .loop

.done:
    ret