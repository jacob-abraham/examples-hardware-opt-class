

global packed_bool_bench_set_asm
global packed_bool_bench_clear_asm

packed_bool_bench_set_asm:

    mov r9, [rdi + 8]   ; n_elements
    mov r10, [rdi + 16] ; start, also i

    ; bail out if start = n_elements
    cmp r10, r9
    jae .end_func

    mov r8, [rdi]       ; arr
    mov r11, [rdi + 24] ; stride

.loop_top:


    ; bit location
    mov rdx, r10
    and rdx, 0b111

    ; byte location
    mov rsi, r10
    shr rsi, 3

    ; do bit test and set
    bts byte [r8 + rsi], rdx

    ; loop inc
    add r10, r11
    cmp r10, r9
    jb      .loop_top

.end_func:
    xor eax, eax
    ret



packed_bool_bench_clear_asm:

    mov r9, [rdi + 8]   ; n_elements
    mov r10, [rdi + 16] ; start, also i

    ; bail out if start = n_elements
    cmp r10, r9
    jae .end_func

    mov r8, [rdi]       ; arr
    mov r11, [rdi + 24] ; stride

.loop_top:


    ; bit location
    mov rdx, r10
    and rdx, 0b111

    ; byte location
    mov rsi, r10
    shr rsi, 3

    ; do bit test and clear
    btr byte [r8 + rsi], rdx

    ; loop inc
    add r10, r11
    cmp r10, r9
    jb      .loop_top

.end_func:
    xor eax, eax
    ret
