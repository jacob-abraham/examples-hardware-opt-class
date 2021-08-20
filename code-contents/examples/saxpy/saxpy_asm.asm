
global saxpy_asm
global saxpy_bad_asm
global saxpy_asm_sse


section .text

; rdi=*a, rsi=*x, rdx=*y, rcx=*r, r8=n
saxpy_asm:

    movss xmm0, [rdi]     ;load scalar
    xor eax, eax     ;load loop counter
    
loop_saxpy:
    cmp rax, r8      ;cmp to size
    jae done_saxpy

    %macro loop_unroll 2
    movss %1, [rsi + 4*rax + %2] ; copy x
    mulss %1, xmm0  ; multiply by scalar
    addss %1, [rdx + 4*rax + %2] ; add y
    movss [rcx + 4*rax + %2], %1 ; copy to r
    %endmacro

    loop_unroll xmm1, 0
    loop_unroll xmm2, 4

    %unmacro loop_unroll 2

    add rax, 2
    jmp loop_saxpy
done_saxpy:
    ret

; rdi=*a, rsi=*x, rdx=*y, rcx=*r, r8=n
saxpy_bad_asm:

    movss xmm0, [rdi]     ;load scalar
    xor eax, eax     ;load loop counter
    
loop_saxpy_bad:
    cmp rax, r8      ;cmp to size
    jae done_saxpy_bad

    %macro loop_unroll 2
    movss %1, xmm0 ; move scalar
    mulss %1, [rsi + 4*rax + %2]  ; multiply by x
    addss %1, [rdx + 4*rax + %2] ; add y
    movss [rcx + 4*rax + %2], %1 ; copy to r
    %endmacro

    loop_unroll xmm1, 0
    loop_unroll xmm2, 4

    %unmacro loop_unroll 2

    add rax, 2
    jmp loop_saxpy_bad
done_saxpy_bad:
    ret


; rdi=*a, rsi=*x, rdx=*y, rcx=*r, r8=n
saxpy_asm_sse:

    movss xmm0, [rdi]     ;load scalar
    shufps xmm0, xmm0, 0 ; broadcast from index 0 to all location
    xor eax, eax    ;load loop counter
    
loop_saxpy_sse:
    cmp rax, r8      ;cmp to size
    jae done_saxpy_sse

    ; %macro loop_unroll_sse 2
    ; movaps %1, [rsi + 4*rax + %2] ; copy x
    ; mulps %1, xmm0  ; multiply by scalar
    ; addps %1, [rdx + 4*rax + %2] ; add y
    ; movaps [rcx + 4*rax + %2], %1 ; copy to r
    ; %endmacro

    ; loop_unroll_sse xmm1, 0 ; 4 floats
    ; loop_unroll_sse xmm2, 16 ; 4 floats

    movaps xmm1, [rsi + 4*rax + 0] ; copy x
    movaps xmm2, [rsi + 4*rax + 16] ; copy x
    mulps xmm1, xmm0  ; multiply by scalar
    mulps xmm2, xmm0  ; multiply by scalar
    addps xmm1, [rdx + 4*rax + 0] ; add y
    addps xmm2, [rdx + 4*rax + 16] ; add y
    movaps [rcx + 4*rax + 0], xmm1 ; copy to r
    movaps [rcx + 4*rax + 16], xmm2 ; copy to r

    add rax, 8 ; total of 16 floats handled
    jmp loop_saxpy_sse
done_saxpy_sse:
    ret
