global my_memmove

section .text

my_memmove:
    mov rax, rdi
    cmp rdi, rsi
    je .done
    jb .forward

    lea rcx, [rsi + rdx]
    cmp rdi, rcx
    jae .forward

.backward:
    test rdx, rdx
    je .done
    lea rsi, [rsi + rdx - 1]
    lea rdi, [rdi + rdx - 1]

.back_loop:
    mov cl, [rsi]
    mov [rdi], cl
    dec rsi
    dec rdi
    dec rdx
    jne .back_loop
    ret

.forward:
    test rdx, rdx
    je .done

.fwd_loop:
    mov cl, [rsi]
    mov [rdi], cl
    inc rsi
    inc rdi
    dec rdx
    jne .fwd_loop

.done:
    ret