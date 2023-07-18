.global memcpy_gobble_mem_fwd_fix

.text

memcpy_gobble_mem_fwd_fix:
  /* rdi(dst) rsi(src) rdx(size) */


	sub	$0x80, %rdx
gobble_ll_loop_fwd:
	/*prefetchnta 0x1c0(%rsi)
	prefetchnta 0x280(%rsi)
	prefetchnta 0x1c0(%rdi)
	prefetchnta 0x280(%rdi)*/
  /*prefetchw 0x1c0(%rdi) 
  prefetchw 0x280(%rdi)*/
	sub	$0x80, %rdx
	movdqu	(%rsi), %xmm0
	movdqu	0x10(%rsi), %xmm1
	movdqu	0x20(%rsi), %xmm2
	movdqu	0x30(%rsi), %xmm3
	movdqu	0x40(%rsi), %xmm4
	movdqu	0x50(%rsi), %xmm5
	movdqu	0x60(%rsi), %xmm6
	movdqu	0x70(%rsi), %xmm7
	movdqa	%xmm0, (%rdi)
	movdqa	%xmm1, 0x10(%rdi)
	movdqa	%xmm2, 0x20(%rdi)
	movdqa	%xmm3, 0x30(%rdi)
	movdqa	%xmm4, 0x40(%rdi)
	movdqa	%xmm5, 0x50(%rdi)
	movdqa	%xmm6, 0x60(%rdi)
	movdqa	%xmm7, 0x70(%rdi)
	lea	0x80(%rsi), %rsi
	lea	0x80(%rdi), %rdi
	jae	gobble_ll_loop_fwd
  ret

/*
	sub	$0x80, %rdx
shl_0_loop:
	movdqa	(%rsi), %xmm1
	movdqa	%xmm1, (%rdi)
	movaps	0x10(%rsi), %xmm2
	movaps	%xmm2, 0x10(%rdi)
	movaps	0x20(%rsi), %xmm3
	movaps	%xmm3, 0x20(%rdi)
	movaps	0x30(%rsi), %xmm4
	movaps	%xmm4, 0x30(%rdi)
	movaps	0x40(%rsi), %xmm1
	movaps	%xmm1, 0x40(%rdi)
	movaps	0x50(%rsi), %xmm2
	movaps	%xmm2, 0x50(%rdi)
	movaps	0x60(%rsi), %xmm3
	movaps	%xmm3, 0x60(%rdi)
	movaps	0x70(%rsi), %xmm4
	movaps	%xmm4, 0x70(%rdi)
	sub	$0x80, %rdx
	lea	0x80(%rsi), %rsi
	lea	0x80(%rdi), %rdi
	jae	shl_0_loop
  ret
*/

/*
memcpy_erms:
    movq %rdi, %rax
    movq %rdx, %rcx
    rep movsb
    ret
*/
