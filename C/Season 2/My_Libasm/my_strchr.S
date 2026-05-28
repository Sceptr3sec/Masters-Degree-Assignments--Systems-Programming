global my_strchr

section .text

my_strchr:
    mov dl, sil

.loop:
    mov al, [rdi]
    cmp al, dl
    je .found
    cmp al, 0
    je .not_found
    inc rdi
    jmp .loop

.found:
    mov rax, rdi
    ret

.not_found:
    xor rax, rax
    ret