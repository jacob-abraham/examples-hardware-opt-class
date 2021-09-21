
global centroid_SoA_a

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


centroid_SoA_a:

    push r12
    push r13
    push r14

    mov rax, [rdi] ; SoA*
    mov r12, [rax] ; x
    mov r13, [rax+8] ; y
    mov r14, [rax+16] ; z
    mov rsi, [rax+24] ; numPoints


    xor rcx, rcx ; i counter

    xor r8, r8 ; scalar sums
    xor r9, r9
    xor r10, r10

    mov rdx, rsi
    and rdx, 11111b ; remainder loop count

    .remainder_loop_top:
    cmp rdx, rcx
    je .remainder_loop_bottom
    add r8d, dword [r12 + 4*rcx] ; do sums
    add r9d, dword [r13 + 4*rcx]
    add r10d, dword [r14 + 4*rcx]
    lea rcx, [rcx + 1] ; loop inc
    jmp .remainder_loop_top
    .remainder_loop_bottom:

    ; init accumlators
    vpxor ymm0, ymm0, ymm0 ; sum_x_vec1
    vpxor ymm1, ymm1, ymm1 ; sum_x_vec2
    vpxor ymm2, ymm2, ymm2 ; sum_y_vec1
    vpxor ymm3, ymm3, ymm3 ; sum_y_vec2
    vpxor ymm4, ymm4, ymm4 ; sum_z_vec1
    vpxor ymm5, ymm5, ymm5 ; sum_z_vec2

    .vector_loop_top:
    cmp rsi, rcx
    je .vector_loop_bottom

    vpaddd ymm0, ymm0, [r12 + 4*rcx] 
    vpaddd ymm1, ymm1, [r12 + 4*rcx + 32] 
    vpaddd ymm2, ymm2, [r13 + 4*rcx] 
    vpaddd ymm3, ymm3, [r13 + 4*rcx + 32] 
    vpaddd ymm4, ymm4, [r14 + 4*rcx] 
    vpaddd ymm5, ymm5, [r14 + 4*rcx + 32] 

    vpaddd ymm0, ymm0, [r12 + 4*rcx + 64] 
    vpaddd ymm1, ymm1, [r12 + 4*rcx + 96] 
    vpaddd ymm2, ymm2, [r13 + 4*rcx + 64] 
    vpaddd ymm3, ymm3, [r13 + 4*rcx + 96] 
    vpaddd ymm4, ymm4, [r14 + 4*rcx + 64] 
    vpaddd ymm5, ymm5, [r14 + 4*rcx + 96] 

    add rcx, 32
    jmp .vector_loop_top
    .vector_loop_bottom:

    ; combine accumlators
    vpaddd ymm0, ymm1, ymm0
    vpaddd ymm1, ymm3, ymm2
    vpaddd ymm2, ymm5, ymm4

    ; we are done with x,y,z at this point. reuse regs
    hadd_epi32_256 0, xmm4, r12d ; sum x
    hadd_epi32_256 1, xmm5, r13d ; sum y
    hadd_epi32_256 2, xmm6, r14d ; sum z

    ; add to remainder sums
    add r8d, r12d
    add r9d, r13d
    add r10d, r14d

    ; centroid ptr
    mov rcx, [rdi+8]

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



