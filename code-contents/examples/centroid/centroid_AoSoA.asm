global centroid_AoSoA_a


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


%define AoSoA_TileSize 32
%define AoSoA_sizeof_struct (AoSoA_TileSize*4*3 + 8)

%if AoSoA_TileSize == 8
%define AoSoA_ShiftAmt 3
%elif AoSoA_TileSize == 16
%define AoSoA_ShiftAmt 4
%elif AoSoA_TileSize == 32
%define AoSoA_ShiftAmt 5
%endif

centroid_AoSoA_a:
    push r12
    push r13
    push r14

    mov rax, [rdi] ; AoSoA*
    mov rsi, [rdi+8] ; numPoints
    add rsi, (AoSoA_TileSize - 1)
    shr rsi, (AoSoA_ShiftAmt) ; numStructs

    xor rcx, rcx ; i counter

    ; init accumlators
    vpxor ymm0, ymm0, ymm0 ; sum_x_vec1
    vpxor ymm1, ymm1, ymm1 ; sum_y_vec1
    vpxor ymm2, ymm2, ymm2 ; sum_z_vec1

    %if AoSoA_TileSize == 16 || AoSoA_TileSize == 32
    vpxor ymm3, ymm3, ymm3 ; sum_x_vec2
    vpxor ymm4, ymm4, ymm4 ; sum_z_vec2
    vpxor ymm5, ymm5, ymm5 ; sum_y_vec2
    %endif

    lea rdx, [rsi - 1] ; numStructs -1

    .vector_loop_top:
    cmp rdx, rcx
    je .vector_loop_bottom
    %if AoSoA_TileSize == 8
    vpaddd ymm0, ymm0, [rax]
    vpaddd ymm1, ymm1, [rax + (AoSoA_TileSize*4)]
    vpaddd ymm2, ymm2, [rax + (AoSoA_TileSize*4*2)]
    %endif
    %if AoSoA_TileSize >= 16
    vpaddd ymm0, ymm0, [rax]
    vpaddd ymm3, ymm3, [rax + 32]
    vpaddd ymm1, ymm1, [rax + (AoSoA_TileSize*4)]
    vpaddd ymm4, ymm4, [rax + 32 + (AoSoA_TileSize*4)]
    vpaddd ymm2, ymm2, [rax + (AoSoA_TileSize*4*2)]
    vpaddd ymm5, ymm5, [rax + 32 + (AoSoA_TileSize*4*2)]
    %endif
    %if AoSoA_TileSize == 32
    vpaddd ymm0, ymm0, [rax + 64]
    vpaddd ymm3, ymm3, [rax + 96]
    vpaddd ymm1, ymm1, [rax + 64 +(AoSoA_TileSize*4)]
    vpaddd ymm4, ymm4, [rax + 96 + (AoSoA_TileSize*4)]
    vpaddd ymm2, ymm2, [rax + 64 + (AoSoA_TileSize*4*2)]
    vpaddd ymm5, ymm5, [rax + 96 + (AoSoA_TileSize*4*2)]
    %endif

    add rax, AoSoA_sizeof_struct ; ptr inc
    lea rcx, [rcx + 1] ; loop inc
    jmp .vector_loop_top
    .vector_loop_bottom:

    %if AoSoA_TileSize == 16 || AoSoA_TileSize == 32
    ; combine accumlators
    vpaddd ymm0, ymm3, ymm0
    vpaddd ymm1, ymm4, ymm1
    vpaddd ymm2, ymm5, ymm2
    %endif

    hadd_epi32_256 0, xmm4, r8d ; sum x
    hadd_epi32_256 1, xmm5, r9d ; sum y
    hadd_epi32_256 2, xmm6, r10d ; sum z

    .remainder_loop_top:
    cmp rsi, rcx
    je .remainder_loop_bottom

    xor r12, r12 ; j counter
    mov r13, [rax + (AoSoA_sizeof_struct-8)]; numPoints Used
    .remainder_inner_loop_top:
    cmp r12, r13
    je .remainder_inner_loop_bottom ; when inner loop done, go back to top
    add r8d, dword [rax + r12*4] ; do sums
    add r9d, dword [rax + r12*4 + (AoSoA_TileSize*4)]
    add r10d, dword [rax + r12*4 + (AoSoA_TileSize*4*2)]
    lea r12, [r12+1]
    jmp .remainder_inner_loop_top
    .remainder_inner_loop_bottom:

    add rax, AoSoA_sizeof_struct ; ptr inc
    lea rcx, [rcx + 1] ; loop inc
    jmp .remainder_loop_top
    .remainder_loop_bottom:

    ; centroid ptr
    mov rcx, [rdi+16]
    mov rsi, [rdi+8] ; numPoints

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
