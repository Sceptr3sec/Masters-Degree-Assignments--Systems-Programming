global my_strcmp

section .text

my_strcmp:
.loop:
    movzx eax, byte [rdi]
    movzx edx, byte [rsi]
    cmp eax, edx
    jne .diff
    test eax, eax
    je .equal
    inc rdi
    inc rsi
    jmp .loop

.diff:
    sub eax, edx
    ret

.equal:
    xor eax, eax
    ret