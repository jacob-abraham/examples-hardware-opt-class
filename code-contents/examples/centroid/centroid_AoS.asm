global centroid_AoS_a

section .data

idx_x: dd 0, 3, 6, 9, 12, 15, 18, 21
idx_y: dd 1, 4, 7, 10, 13, 16, 19, 22
idx_z: dd 2, 5, 8, 11, 14, 17, 20, 23


section .text

; args: xmm to hadd, xmm scratch register, result
%macro hadd_epi32_128 3
vpshufd %2, %1, 01001110b
vpaddd %1, %1, %2
vpshufd %2, %1, 00011011b
vpaddd %1, %1, %2
vmovd %3, %1
%endmacro

; args: ymm# to hadd, xmm scratch register, result
%macro hadd_epi32_256 3
vextracti128 %2, ymm%1, 1
vpaddd xmm%1, xmm%1, %2
hadd_epi32_128 xmm%1, %2, %3
%endmacro


centroid_AoS_a:
    push r12
    push r13
    push r14

    mov rax, [rdi] ; AoS*
    mov rsi, [rdi+8] ; numPoints

    xor rcx, rcx ; i counter

    xor r8, r8 ; scalar sums
    xor r9, r9
    xor r10, r10

    mov rdx, rsi
    and rdx, 1111b ; remainder loop count

    .remainder_loop_top:
    cmp rdx, rcx
    je .remainder_loop_bottom
    add r8d, dword [rax] ; do sums
    add r9d, dword [rax + 4]
    add r10d, dword [rax + 8]
    add rax, 12 ; ptr inc
    lea rcx, [rcx + 1] ; loop inc
    jmp .remainder_loop_top
    .remainder_loop_bottom:

    ; load index accumlator
    vmovdqu ymm6, [rel idx_x]
    vmovdqu ymm7, [rel idx_y]
    vmovdqu ymm8, [rel idx_z]

    ; init accumlators
    vpxor ymm0, ymm0, ymm0 ; load x
    vpxor ymm1, ymm1, ymm1 ; accum x
    vpxor ymm2, ymm2, ymm2 ; load y
    vpxor ymm3, ymm3, ymm3 ; accum y
    vpxor ymm4, ymm4, ymm4 ; load z
    vpxor ymm5, ymm5, ymm5 ; accum z

    vpxor ymm10, ymm10, ymm10 ; load x
    vpxor ymm11, ymm11, ymm11 ; accum x
    vpxor ymm12, ymm12, ymm12 ; load y
    vpxor ymm13, ymm13, ymm13 ; accum y
    vpxor ymm14, ymm14, ymm14 ; load z
    vpxor ymm15, ymm15, ymm15 ; accum z

    ; make ymm9 all 0's
    vpxor ymm9, ymm9, ymm9

    .vector_loop_top:
    cmp rsi, rcx
    je .vector_loop_bottom


    vpcmpeqd ymm9, ymm9, ymm9 ; makes all 1's, has to be done before evey call to gather
    vpgatherdd ymm0, [rax + ymm6*4], ymm9
    vpaddd ymm1, ymm1, ymm0

    vpcmpeqd ymm9, ymm9, ymm9
    vpgatherdd ymm2, [rax + ymm7*4], ymm9
    vpaddd ymm3, ymm3, ymm2

    vpcmpeqd ymm9, ymm9, ymm9
    vpgatherdd ymm4, [rax + ymm8*4], ymm9
    vpaddd ymm5, ymm5, ymm4

    add rax, 96


    vpcmpeqd ymm9, ymm9, ymm9
    vpgatherdd ymm10, [rax + ymm6*4], ymm9
    vpaddd ymm11, ymm11, ymm10

    vpcmpeqd ymm9, ymm9, ymm9
    vpgatherdd ymm12, [rax + ymm7*4], ymm9
    vpaddd ymm13, ymm13, ymm12

    vpcmpeqd ymm9, ymm9, ymm9
    vpgatherdd ymm14, [rax + ymm8*4], ymm9
    vpaddd ymm15, ymm15, ymm14

    add rax, 96


    add rcx, 16
    jmp .vector_loop_top
    .vector_loop_bottom:

    ; combine accumlators
    vpaddd ymm0, ymm1, ymm11
    vpaddd ymm1, ymm3, ymm13
    vpaddd ymm2, ymm5, ymm15

    ; we are done with x,y,z at this point. reuse regs
    hadd_epi32_256 0, xmm4, r12d ; sum x
    hadd_epi32_256 1, xmm5, r13d ; sum y
    hadd_epi32_256 2, xmm6, r14d ; sum z

    ; add to remainder sums
    add r8d, r12d
    add r9d, r13d
    add r10d, r14d

    ; centroid ptr
    mov rcx, [rdi+16]

    ; do div and store to centroid
    mov eax, r8d ; x/n
    cdq 
    idiv esi
    mov dword [rcx], eax

    mov eax, r9d ; y/n
    cdq 
    idiv esi
    mov dword [rcx+4], eax

    mov eax, r10d ; z/n
    cdq 
    idiv esi
    mov dword [rcx+8], eax

    pop r14
    pop r13
    pop r12
    xor rax, rax
    vzeroupper
    ret

