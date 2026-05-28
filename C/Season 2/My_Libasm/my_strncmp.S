global my_strncmp

section .text

my_strncmp:
    test rdx, rdx
    je .equal

.loop:
    mov al, [rdi]
    mov cl, [rsi]
    cmp al, cl
    jne .diff
    test al, al
    je .equal
    inc rdi
    inc rsi
    dec rdx
    je .equal
    jmp .loop

.diff:
    movzx eax, al
    movzx ecx, cl
    sub eax, ecx
    ret

.equal:
    xor eax, eax
    ret