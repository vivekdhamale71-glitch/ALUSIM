; Sum, maximum, linear search, bubble sort and binary search on an array.

default rel

section .text
global sum_array, find_max, linear_search, bubble_sort, binary_search

; sum_array(rdi = array of 64-bit numbers, rsi = count) -> rax : the sum of all elements
sum_array:
    xor eax, eax                    ; total = 0
    xor ecx, ecx                    ; index = 0
.next:
    cmp rcx, rsi
    jge .done
    add rax, [rdi + rcx * 8]
    inc rcx
    jmp .next
.done:
    ret

; find_max(rdi = array, rsi = count >= 1) -> rax : the largest element
find_max:
    mov rax, [rdi]                  ; best = first element
    mov ecx, 1
.next:
    cmp rcx, rsi
    jge .done
    mov rdx, [rdi + rcx * 8]
    cmp rdx, rax
    jle .skip
    mov rax, rdx                    ; a larger one was found
.skip:
    inc rcx
    jmp .next
.done:
    ret

; linear_search(rdi = array, rsi = count, rdx = key) -> rax : index of the key or -1
linear_search:
    xor ecx, ecx
.next:
    cmp rcx, rsi
    jge .missing
    cmp [rdi + rcx * 8], rdx
    je .found
    inc rcx
    jmp .next
.found:
    mov rax, rcx
    ret
.missing:
    mov rax, -1
    ret

; bubble_sort(rdi = array, rsi = count) : sorts ascending in place, stops when a pass swaps nothing
bubble_sort:
    mov r8, rsi
    dec r8                          ; last pair to compare in this pass
    jle .done
.pass:
    xor r9d, r9d                    ; swapped = false
    xor ecx, ecx
.compare:
    cmp rcx, r8
    jge .end_of_pass
    mov rax, [rdi + rcx * 8]
    mov rdx, [rdi + rcx * 8 + 8]
    cmp rax, rdx
    jle .in_order
    mov [rdi + rcx * 8], rdx        ; swap the two neighbours
    mov [rdi + rcx * 8 + 8], rax
    mov r9d, 1
.in_order:
    inc rcx
    jmp .compare
.end_of_pass:
    test r9d, r9d
    jz .done                        ; nothing was swapped: sorted
    dec r8
    jg .pass
.done:
    ret

; binary_search(rdi = SORTED array, rsi = count, rdx = key) -> rax : index of the key or -1
binary_search:
    xor r8d, r8d                    ; low
    lea r9, [rsi - 1]               ; high
.again:
    cmp r8, r9
    jg .missing
    lea rax, [r8 + r9]
    sar rax, 1                      ; middle = (low + high) / 2
    mov rcx, [rdi + rax * 8]
    cmp rcx, rdx
    je .found
    jl .go_right
    lea r9, [rax - 1]               ; key is smaller: high = middle - 1
    jmp .again
.go_right:
    lea r8, [rax + 1]               ; key is larger: low = middle + 1
    jmp .again
.found:
    ret
.missing:
    mov rax, -1
    ret
