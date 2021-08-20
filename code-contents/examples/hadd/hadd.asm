


global hardware_hadd
global emulate_hadd_1
global emulate_hadd_2

%define iterations 10000

section .text

; cycle counts
; https://uops.info/table.html
; for skylake

; rdi=struct
hardware_hadd:

    mov rax, [rdi] ; data
    mov rdx, [rdi+8] ; result


    %ifndef BENCHMARK
    movaps xmm0, [rax] ; load
    %else

    mov rcx, iterations
    movaps xmm2, [rax] ; load
    
    .looptop:
    ; for speed this should be removed
    ; because of loop dependencies it is needed for benchmark
    movaps xmm0, xmm2
    %endif

    haddps xmm0, xmm0 ; 6, 2
    haddps xmm0, xmm0 ; 6, 2
                      ; 12
    
    %ifdef BENCHMARK
    dec rcx
    jnz hardware_hadd.looptop
    %endif

    ; store
    movss [rdx], xmm0

    xor eax, eax
    ret


; rdi=struct
emulate_hadd_1:

    mov rax, [rdi] ; data
    mov rdx, [rdi+8] ; result
    
    %ifndef BENCHMARK
    movaps xmm0, [rax] ; load
    %else

    mov rcx, iterations
    movaps xmm2, [rax] ; load
    
    .looptop:
    ; for speed this should be removed
    ; because of loop dependencies it is needed for benchmark
    movaps xmm0, xmm2
    %endif

    ; A  B  C  D
    ;     +
    ; C  D  A  B
    ;     =
    ; AC BD CA DB
    ;     +
    ; DB CA BD AC
    ;     =
    ; ACDB BDCA CABD DBAC

    movaps xmm1, xmm0            ; 0/1, 0.25
    shufps xmm1, xmm0, 01001110b ; 1  , 1
    addps xmm0, xmm1             ; 4  , 0.5
    movaps xmm1, xmm0            ; 0/1, 0.25
    shufps xmm1, xmm0, 00011011b ; 1  , 1
    addss xmm0, xmm1             ; 4  , 0.5
                                 ; 10/12

    %ifdef BENCHMARK
    dec rcx
    jnz emulate_hadd_1.looptop
    %endif
    ; store
    movss [rdx], xmm0

    xor eax, eax
    ret

emulate_hadd_2:

    mov rax, [rdi] ; data
    mov rdx, [rdi+8] ; result

    %ifndef BENCHMARK
    movaps xmm0, [rax] ; load
    %else

    mov rcx, iterations
    movaps xmm2, [rax] ; load
    
    .looptop:
    ; for speed this should be removed
    ; because of loop dependencies it is needed for benchmark
    movaps xmm0, xmm2
    %endif

    ; A  B  C  D
    ;     +
    ; B  B  D  D
    ;     =
    ; AB BB CD DD
    ;     +
    ; CD DD AB BB 
    ;     =
    ; ABCD BBDD CDAB DDBB

    movshdup xmm1,xmm0 ; 1, 1
    addps xmm0, xmm1   ; 4, 0.5
    movhlps xmm1, xmm0 ; 1, 1
    addss xmm0, xmm1   ; 4, 0.5
                       ; 10

    %ifdef BENCHMARK
    dec rcx
    jnz emulate_hadd_2.looptop
    %endif

    ; store
    movss [rdx], xmm0

    xor eax, eax
    ret

%undef iterations
