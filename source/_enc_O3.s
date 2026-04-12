	.file	"encoders_for_asm.c"
	.text
	.p2align 4
	.globl	reed_muller_encode_permnet
	.def	reed_muller_encode_permnet;	.scl	2;	.type	32;	.endef
	.seh_proc	reed_muller_encode_permnet
reed_muller_encode_permnet:
	.seh_endprologue
	movzbl	(%rdx), %edx
	movq	%rcx, %r8
	movq	%rdx, %r9
	leaq	(%rdx,%rdx), %rax
	salq	$4, %r9
	andl	$256, %r9d
	movq	%rdx, %rcx
	andl	$16, %eax
	orq	%r9, %rax
	andl	$7, %ecx
	orq	%rcx, %rax
	movq	%rdx, %rcx
	salq	$11, %rcx
	andl	$65536, %ecx
	orq	%rcx, %rax
	movq	%rdx, %rcx
	salq	$26, %rcx
	movabsq	$4294967296, %r9
	andq	%r9, %rcx
	orq	%rcx, %rax
	leaq	(%rax,%rax), %rcx
	movabsq	$-6148914691236517206, %r9
	andq	%r9, %rcx
	xorq	%rax, %rcx
	leaq	0(,%rcx,4), %rax
	movabsq	$-3689348814741910324, %r9
	andq	%r9, %rax
	xorq	%rcx, %rax
	movq	%rax, %rcx
	salq	$4, %rcx
	movabsq	$-1085102592571150096, %r9
	andq	%r9, %rcx
	xorq	%rax, %rcx
	movq	%rcx, %rax
	salq	$8, %rax
	movabsq	$-71777214294589696, %r9
	andq	%r9, %rax
	xorq	%rcx, %rax
	movq	%rax, %rcx
	salq	$16, %rcx
	movabsq	$-281470681808896, %r9
	andq	%r9, %rcx
	xorq	%rax, %rcx
	movq	%rcx, %rax
	salq	$32, %rax
	shrq	$7, %rdx
	xorq	%rcx, %rax
	negq	%rdx
	xorq	%rax, %rdx
	vmovq	%rax, %xmm1
	vpinsrq	$1, %rdx, %xmm1, %xmm0
	vmovdqu	%xmm0, (%r8)
	ret
	.seh_endproc
	.p2align 4
	.globl	reed_muller_encode_bitmask
	.def	reed_muller_encode_bitmask;	.scl	2;	.type	32;	.endef
	.seh_proc	reed_muller_encode_bitmask
reed_muller_encode_bitmask:
	.seh_endprologue
	movabsq	$-6148914691236517206, %r8
	movabsq	$-3689348814741910324, %r10
	movzbl	(%rdx), %eax
	movl	%eax, %edx
	sarl	%edx
	andl	$1, %edx
	negq	%rdx
	andq	%r8, %rdx
	movl	%eax, %r8d
	sarl	$2, %r8d
	andl	$1, %r8d
	negq	%r8
	andq	%r10, %r8
	xorq	%r8, %rdx
	movl	%eax, %r9d
	movl	%eax, %r8d
	andl	$1, %r9d
	sarl	$3, %r8d
	negq	%r9
	andl	$1, %r8d
	xorq	%r9, %rdx
	negq	%r8
	movabsq	$-1085102592571150096, %r9
	andq	%r9, %r8
	xorq	%rdx, %r8
	movl	%eax, %edx
	sarl	$4, %edx
	andl	$1, %edx
	negq	%rdx
	movabsq	$-71777214294589696, %r9
	andq	%r9, %rdx
	xorq	%r8, %rdx
	movl	%eax, %r8d
	sarl	$5, %r8d
	andl	$1, %r8d
	negq	%r8
	movabsq	$-281470681808896, %r9
	andq	%r9, %r8
	xorq	%rdx, %r8
	movl	%eax, %edx
	sarl	$6, %edx
	andl	$1, %edx
	negq	%rdx
	movabsq	$-4294967296, %r9
	andq	%r9, %rdx
	xorq	%r8, %rdx
	sarl	$7, %eax
	cltq
	negq	%rax
	xorq	%rdx, %rax
	movq	%rdx, (%rcx)
	movq	%rax, 8(%rcx)
	ret
	.seh_endproc
	.p2align 4
	.globl	reed_muller_encode_branched
	.def	reed_muller_encode_branched;	.scl	2;	.type	32;	.endef
	.seh_proc	reed_muller_encode_branched
reed_muller_encode_branched:
	.seh_endprologue
	vpcmpeqd	%ymm1, %ymm1, %ymm1
	vpsrld	$31, %ymm1, %ymm1
	movzbl	(%rdx), %eax
	vmovd	%eax, %xmm0
	vpbroadcastd	%xmm0, %ymm0
	vpsravd	.LC0(%rip), %ymm0, %ymm0
	vpand	%ymm1, %ymm0, %ymm0
	vpxor	%xmm1, %xmm1, %xmm1
	vpcmpeqd	%ymm1, %ymm0, %ymm0
	vpcmpeqd	%ymm1, %ymm0, %ymm0
	vextracti128	$0x1, %ymm0, %xmm1
	vpmovsxdq	%xmm1, %ymm1
	vpmovsxdq	%xmm0, %ymm0
	vpand	.LC2(%rip), %ymm0, %ymm0
	vpand	.LC3(%rip), %ymm1, %ymm2
	vpand	.LC4(%rip), %ymm1, %ymm1
	vpxor	%ymm0, %ymm2, %ymm2
	vpxor	%ymm0, %ymm1, %ymm0
	vextracti128	$0x1, %ymm0, %xmm1
	vpxor	%xmm0, %xmm1, %xmm0
	vpsrldq	$8, %xmm0, %xmm1
	vpxor	%xmm1, %xmm0, %xmm0
	vextracti128	$0x1, %ymm2, %xmm1
	vpxor	%xmm2, %xmm1, %xmm1
	vpsrldq	$8, %xmm1, %xmm2
	vpxor	%xmm2, %xmm1, %xmm1
	vpunpcklqdq	%xmm1, %xmm0, %xmm0
	vmovdqu	%xmm0, (%rcx)
	vzeroupper
	ret
	.seh_endproc
	.section .rdata,"dr"
	.align 32
.LC0:
	.long	0
	.long	1
	.long	2
	.long	3
	.long	4
	.long	5
	.long	6
	.long	7
	.align 32
.LC2:
	.quad	-1
	.quad	-6148914691236517206
	.quad	-3689348814741910324
	.quad	-1085102592571150096
	.align 32
.LC3:
	.quad	-71777214294589696
	.quad	-281470681808896
	.quad	-4294967296
	.quad	-1
	.align 32
.LC4:
	.quad	-71777214294589696
	.quad	-281470681808896
	.quad	-4294967296
	.quad	0
	.ident	"GCC: (Rev8, Built by MSYS2 project) 15.2.0"
