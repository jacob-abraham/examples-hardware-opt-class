


global cntsetbits_asm


section .text

; rdi=x
cntsetbits_asm:

    xor eax, eax     ; we will keep our count here
    
    xor cl, cl

cntsetbits_loop:

    inc cl

    mov rdx, rdi      ; copy value so we dont overwrite it
    and rdx, 1        ; perform calculation
    add rax, rdx

    shr rdi, 1        ; move to next bit

    cmp cl, 64
    jl cntsetbits_loop

    ret
