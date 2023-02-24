
.global memcpy_erms

.text

memcpy_erms:
    movq %rdi, %rax
    movq %rdx, %rcx
    rep movsb
    ret
