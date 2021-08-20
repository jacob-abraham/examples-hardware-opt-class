global _start

section .data
alen: db 10
array: db 10,5,9,13,63,76,20,1,42,87

section .text
_start:

    xor rax, rax
    mov al, [alen]                  ; load the array length
    xor eax, eax                      ; init counter
    loop_top: 
        cmp rax, rcx                ; ZF will equal 1 when counter reaches the length
        je loop_end

        test byte [array + rcx], 1  ; bitwise AND to check if even, ZF=1 when even
        je return_top               ; done with this loop iteration if even
        add byte [array + rcx], 1   ; make the data even

        return_top:
        inc rcx
        jmp loop_top
    loop_end:

    mov rax, 60                      ; exit cleanly
    xor rdi, rdi                     ; return code of zero
    syscall
