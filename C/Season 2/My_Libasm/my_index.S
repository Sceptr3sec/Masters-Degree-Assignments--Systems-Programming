global my_index

section .text

my_index:
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