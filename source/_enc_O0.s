	.file	"encoders_for_asm.c"
	.text
	.globl	reed_muller_encode_permnet
	.def	reed_muller_encode_permnet;	.scl	2;	.type	32;	.endef
	.seh_proc	reed_muller_encode_permnet
reed_muller_encode_permnet:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$96, %rsp
	.seh_stackalloc	96
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	movq	24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	movq	%rax, -8(%rbp)
	movq	-8(%rbp), %rax
	andl	$1, %eax
	movq	%rax, -16(%rbp)
	movq	-8(%rbp), %rax
	shrq	%rax
	andl	$1, %eax
	movq	%rax, -24(%rbp)
	movq	-8(%rbp), %rax
	shrq	$2, %rax
	andl	$1, %eax
	movq	%rax, -32(%rbp)
	movq	-8(%rbp), %rax
	shrq	$3, %rax
	andl	$1, %eax
	movq	%rax, -40(%rbp)
	movq	-8(%rbp), %rax
	shrq	$4, %rax
	andl	$1, %eax
	movq	%rax, -48(%rbp)
	movq	-8(%rbp), %rax
	shrq	$5, %rax
	andl	$1, %eax
	movq	%rax, -56(%rbp)
	movq	-8(%rbp), %rax
	shrq	$6, %rax
	andl	$1, %eax
	movq	%rax, -64(%rbp)
	movq	-8(%rbp), %rax
	shrq	$7, %rax
	andl	$1, %eax
	movq	%rax, -72(%rbp)
	movq	-24(%rbp), %rax
	addq	%rax, %rax
	orq	-16(%rbp), %rax
	movq	-32(%rbp), %rdx
	salq	$2, %rdx
	orq	%rax, %rdx
	movq	-40(%rbp), %rax
	salq	$4, %rax
	orq	%rax, %rdx
	movq	-48(%rbp), %rax
	salq	$8, %rax
	orq	%rax, %rdx
	movq	-56(%rbp), %rax
	salq	$16, %rax
	orq	%rax, %rdx
	movq	-64(%rbp), %rax
	salq	$32, %rax
	orq	%rdx, %rax
	movq	%rax, -80(%rbp)
	movq	-72(%rbp), %rax
	movq	%rax, -88(%rbp)
	movq	-80(%rbp), %rax
	leaq	(%rax,%rax), %rdx
	movabsq	$-6148914691236517206, %rax
	andq	%rdx, %rax
	xorq	%rax, -80(%rbp)
	movq	-88(%rbp), %rax
	leaq	(%rax,%rax), %rdx
	movabsq	$-6148914691236517206, %rax
	andq	%rdx, %rax
	xorq	%rax, -88(%rbp)
	movq	-80(%rbp), %rax
	leaq	0(,%rax,4), %rdx
	movabsq	$-3689348814741910324, %rax
	andq	%rdx, %rax
	xorq	%rax, -80(%rbp)
	movq	-88(%rbp), %rax
	leaq	0(,%rax,4), %rdx
	movabsq	$-3689348814741910324, %rax
	andq	%rdx, %rax
	xorq	%rax, -88(%rbp)
	movq	-80(%rbp), %rax
	salq	$4, %rax
	movq	%rax, %rdx
	movabsq	$-1085102592571150096, %rax
	andq	%rdx, %rax
	xorq	%rax, -80(%rbp)
	movq	-88(%rbp), %rax
	salq	$4, %rax
	movq	%rax, %rdx
	movabsq	$-1085102592571150096, %rax
	andq	%rdx, %rax
	xorq	%rax, -88(%rbp)
	movq	-80(%rbp), %rax
	salq	$8, %rax
	movq	%rax, %rdx
	movabsq	$-71777214294589696, %rax
	andq	%rdx, %rax
	xorq	%rax, -80(%rbp)
	movq	-88(%rbp), %rax
	salq	$8, %rax
	movq	%rax, %rdx
	movabsq	$-71777214294589696, %rax
	andq	%rdx, %rax
	xorq	%rax, -88(%rbp)
	movq	-80(%rbp), %rax
	salq	$16, %rax
	movq	%rax, %rdx
	movabsq	$-281470681808896, %rax
	andq	%rdx, %rax
	xorq	%rax, -80(%rbp)
	movq	-88(%rbp), %rax
	salq	$16, %rax
	movq	%rax, %rdx
	movabsq	$-281470681808896, %rax
	andq	%rdx, %rax
	xorq	%rax, -88(%rbp)
	movq	-80(%rbp), %rax
	salq	$32, %rax
	xorq	%rax, -80(%rbp)
	movq	-88(%rbp), %rax
	salq	$32, %rax
	xorq	%rax, -88(%rbp)
	movq	-80(%rbp), %rax
	xorq	%rax, -88(%rbp)
	movq	16(%rbp), %rax
	movq	-80(%rbp), %rdx
	movq	%rdx, (%rax)
	movq	16(%rbp), %rax
	leaq	8(%rax), %rdx
	movq	-88(%rbp), %rax
	movq	%rax, (%rdx)
	nop
	addq	$96, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.section .rdata,"dr"
	.align 32
G:
	.quad	-1
	.quad	-1
	.quad	-6148914691236517206
	.quad	-6148914691236517206
	.quad	-3689348814741910324
	.quad	-3689348814741910324
	.quad	-1085102592571150096
	.quad	-1085102592571150096
	.quad	-71777214294589696
	.quad	-71777214294589696
	.quad	-281470681808896
	.quad	-281470681808896
	.quad	-4294967296
	.quad	-4294967296
	.quad	0
	.quad	-1
	.text
	.globl	reed_muller_encode_bitmask
	.def	reed_muller_encode_bitmask;	.scl	2;	.type	32;	.endef
	.seh_proc	reed_muller_encode_bitmask
reed_muller_encode_bitmask:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$32, %rsp
	.seh_stackalloc	32
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	movq	$0, -8(%rbp)
	movq	$0, -16(%rbp)
	movl	$0, -20(%rbp)
	jmp	.L3
.L4:
	movq	24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %edx
	movl	-20(%rbp), %eax
	movl	%eax, %ecx
	sarl	%cl, %edx
	movl	%edx, %eax
	cltq
	andl	$1, %eax
	negq	%rax
	movq	%rax, -32(%rbp)
	movl	-20(%rbp), %eax
	cltq
	salq	$4, %rax
	movq	%rax, %rdx
	leaq	G(%rip), %rax
	movq	(%rdx,%rax), %rax
	andq	-32(%rbp), %rax
	xorq	%rax, -8(%rbp)
	movl	-20(%rbp), %eax
	cltq
	salq	$4, %rax
	movq	%rax, %rdx
	leaq	8+G(%rip), %rax
	movq	(%rdx,%rax), %rax
	andq	-32(%rbp), %rax
	xorq	%rax, -16(%rbp)
	addl	$1, -20(%rbp)
.L3:
	cmpl	$7, -20(%rbp)
	jle	.L4
	movq	16(%rbp), %rax
	movq	-8(%rbp), %rdx
	movq	%rdx, (%rax)
	movq	16(%rbp), %rax
	leaq	8(%rax), %rdx
	movq	-16(%rbp), %rax
	movq	%rax, (%rdx)
	nop
	addq	$32, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.globl	reed_muller_encode_branched
	.def	reed_muller_encode_branched;	.scl	2;	.type	32;	.endef
	.seh_proc	reed_muller_encode_branched
reed_muller_encode_branched:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$32, %rsp
	.seh_stackalloc	32
	.seh_endprologue
	movq	%rcx, 16(%rbp)
	movq	%rdx, 24(%rbp)
	movq	$0, -8(%rbp)
	movq	$0, -16(%rbp)
	movl	$0, -20(%rbp)
	jmp	.L6
.L8:
	movq	24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %edx
	movl	-20(%rbp), %eax
	movl	%eax, %ecx
	sarl	%cl, %edx
	movl	%edx, %eax
	andl	$1, %eax
	testl	%eax, %eax
	je	.L7
	movl	-20(%rbp), %eax
	cltq
	salq	$4, %rax
	movq	%rax, %rdx
	leaq	G(%rip), %rax
	movq	(%rdx,%rax), %rax
	xorq	%rax, -8(%rbp)
	movl	-20(%rbp), %eax
	cltq
	salq	$4, %rax
	movq	%rax, %rdx
	leaq	8+G(%rip), %rax
	movq	(%rdx,%rax), %rax
	xorq	%rax, -16(%rbp)
.L7:
	addl	$1, -20(%rbp)
.L6:
	cmpl	$7, -20(%rbp)
	jle	.L8
	movq	16(%rbp), %rax
	movq	-8(%rbp), %rdx
	movq	%rdx, (%rax)
	movq	16(%rbp), %rax
	leaq	8(%rax), %rdx
	movq	-16(%rbp), %rax
	movq	%rax, (%rdx)
	nop
	addq	$32, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.ident	"GCC: (Rev8, Built by MSYS2 project) 15.2.0"
