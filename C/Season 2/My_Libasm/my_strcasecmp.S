global my_strcasecmp

section .text

my_strcasecmp:
.loop:
    mov al, [rdi]
    mov dl, [rsi]

    mov cl, al
    cmp cl, 'A'
    jb .s1_done
    cmp cl, 'Z'
    ja .s1_done
    add al, 32

.s1_done:
    mov cl, dl
    cmp cl, 'A'
    jb .s2_done
    cmp cl, 'Z'
    ja .s2_done
    add dl, 32

.s2_done:
    cmp al, dl
    jne .diff
    test al, al
    je .equal
    inc rdi
    inc rsi
    jmp .loop

.diff:
    movzx eax, al
    movzx edx, dl
    sub eax, edx
    ret

.equal:
    xor eax, eax
    ret