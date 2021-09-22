global ll_print_a
global malloc_a
global ll_new_a
global ll_append_a
global ll_length_a
global ll_insert_a
global ll_remove_a


extern malloc
extern printf

%define buf_size 0x1000
section .bss
buffer: resb buf_size
.end:

section .data
buffer_size: dq buf_size
buffer_ptr: dq buffer
format_str: db "Node (%p) %d with data %d",10,0

section .text

; ll_node_t* head = rdi
ll_print_a:
    push r12
    mov r12, rdi ; r12 is a calle saved, so we know printf wont kill it
    .loop_top:
    test r12, r12 ; cmp r12, 0
    je .done
    mov rdi, format_str
    mov rsi, r12
    mov edx, dword [r12]
    mov ecx, dword [r12+4]
    xor eax, eax
    call printf ; cant assume any regs stay the same
    mov r12, qword [r12+8]
    jmp .loop_top

    .done:
    pop r12
    ret

; rdi is number of bytes
malloc_a:
    mov rcx, [buffer_ptr]
    add rcx, rdi
    cmp rcx, buffer.end
    jl .after_size_check
    xor eax, eax
    ret

    .after_size_check:
    mov rax, rcx
    sub rax, rdi
    mov [buffer_ptr], rcx

    ret

; int32_t id = rdi
; int32_t data = rsi
ll_new_a:
    push rdi
    push rsi
    mov edi, 16
    call malloc ; rax has ptr
    pop rsi
    pop rdi

    mov dword [rax], edi
    mov dword [rax+4], esi
    mov qword [rax+8], 0
    ret

; ll_node_t** head = rdi
; ll_node_t* newNode = rsi
ll_append_a:

    test rdi, rdi ; cmp rdi, 0
    jne .after_check_head
    xor eax, eax
    ret
    .after_check_head:

    mov rax, [rdi] ; *head
    test rax, rax ; cmp rax, 0
    je .done
    
    .loop_top:
    mov rdi, rax
    mov rax, [rax+8]
    test rax, rax ; cmp rax, 0
    jne .loop_top
    add rdi, 8

    .done:
    mov [rdi], rsi
    mov eax, 1
    ret

; ll_node_t* head = rdi
ll_length_a:

    xor eax, eax
    .loop_top:
    test rdi, rdi ; cmp rdi, 0
    je .done
    add rax, 1
    mov rdi, [rdi+8]
    jmp .loop_top

    .done:
    ret

; ll_node_t** head = rdi
; ll_node_t* newNode = rsi
; size_t idx = rdx
ll_insert_a:
    test rdi, rdi ; cmp rdi, 0
    jne .after_check_head
    xor eax, eax
    ret
    .after_check_head:

    mov rax, [rdi] ; oldHead and insertPnt
    test rdx, rdx ; cmp rdx, 0
    jne .move_to_insert_point
    mov [rdi], rsi ; *head = newNode
    test rax, rax ; cmp rax, 0
    je .insert_at_head
    mov [rsi+8], rax  ; newNode->next = oldHead

    .insert_at_head:
    mov eax, 1
    ret

    .move_to_insert_point:
    mov ecx, 1 ; i
    .loop_top:
    mov r10, [rax+8] ; insertPnt->next
    test r10, r10 ; cmp r10, 0
    je .loop_bottom
    cmp rcx, rdx ; cmp rcx, rdx
    jge .loop_bottom
    mov rax, r10 ; insertPnt = insertPnt->next
    add rcx, 1 ; i++
    jmp .loop_top
    .loop_bottom:
    
    cmp rcx, rdx
    jge .after_idx_check
    xor eax, eax
    ret
    .after_idx_check:
    mov [rsi+8], r10
    mov [rax+8], rsi

    mov eax, 1
    ret

; l_node_t** head = rdi
; size_t idx = rsi
ll_remove_a:

    test rdi, rdi ; cmp rdi, 0
    jne .after_check_head
    xor eax, eax
    ret
    .after_check_head:

    mov rdx, [rdi] ; *head and beforeDelete
    test rsi, rsi ; cmp rsi, 0
    jne .move_to_delete_point
    test rdx, rdx ; cmp rdx, 0
    jne .delete_head
    xor eax, eax
    ret
    .delete_head:
    mov rcx, [rdx+8] ; (*head)->next
    mov [rdi], rcx  ; *head = (*head)->next
    mov eax, 1
    ret

    .move_to_delete_point:
    mov ecx, 1 ; i
    .loop_top:
    mov rax, [rdx+8] ; beforeDelete->next
    test rax, rax ; cmp rax, 0
    je .loop_bottom
    cmp rcx, rsi ; cmp rcx, rdx
    jge .loop_bottom
    mov rdx, rax ; beforeDelete = beforeDelete->next
    add rcx, 1 ; i++
    jmp .loop_top
    .loop_bottom:

    cmp rcx, rsi
    jge .after_idx_check
    xor eax, eax
    ret
    .after_idx_check:
    ; rax = beforeDelete->next
    mov rcx, [rax+8] ; beforeDelete->next->next
    mov [rdx+8], rcx

    mov eax, 1
    ret
