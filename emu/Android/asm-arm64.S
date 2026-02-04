/*
 * ARM64 assembly for Android platform
 * Ported from Linux ARM64 support
 */

	.file	"asm-Android-arm64.S"
	.text

/*
 * umult(ulong m1, ulong m2, ulong *hi)
 * Unsigned 64x64 multiply returning 128-bit result
 */

	.type	umult,@function
	.global	umult
umult:
	/* ARM64 has 64x64->128 multiply in mul and umulh */
	umulh	x3, x0, x1	/* high 64 bits */
	mul	x0, x0, x1	/* low 64 bits */
	str	x3, [x2]	/* store high part */
	ret

	.type	FPsave,@function
	.global	FPsave
FPsave:
	/* Save FP state - ARM64 has 32 S-registers and FPCR/FPSR */
	stp	q0, q1, [x0]
	stp	q2, q3, [x0, #32]
	stp	q4, q5, [x0, #64]
	stp	q6, q7, [x0, #96]
	stp	q8, q9, [x0, #128]
	stp	q10, q11, [x0, #160]
	stp	q12, q13, [x0, #192]
	stp	q14, q15, [x0, #224]
	stp	q16, q17, [x0, #256]
	stp	q18, q19, [x0, #288]
	stp	q20, q21, [x0, #320]
	stp	q22, q23, [x0, #352]
	stp	q24, q25, [x0, #384]
	stp	q26, q27, [x0, #416]
	stp	q28, q29, [x0, #448]
	stp	q30, q31, [x0, #480]
	mrs	x1, fpcr
	mrs	x2, fpsr
	stp	x1, x2, [x0, #512]
	ret

	.type	FPrestore,@function
	.global	FPrestore
FPrestore:
	ldp	q0, q1, [x0]
	ldp	q2, q3, [x0, #32]
	ldp	q4, q5, [x0, #64]
	ldp	q6, q7, [x0, #96]
	ldp	q8, q9, [x0, #128]
	ldp	q10, q11, [x0, #160]
	ldp	q12, q13, [x0, #192]
	ldp	q14, q15, [x0, #224]
	ldp	q16, q17, [x0, #256]
	ldp	q18, q19, [x0, #288]
	ldp	q20, q21, [x0, #320]
	ldp	q22, q23, [x0, #352]
	ldp	q24, q25, [x0, #384]
	ldp	q26, q27, [x0, #416]
	ldp	q28, q29, [x0, #448]
	ldp	q30, q31, [x0, #480]
	ldp	x1, x2, [x0, #512]
	msr	fpcr, x1
	msr	fpsr, x2
	ret

	.type	_tas,@function
	.globl	_tas
_tas:
	/* Test-and-set using ARM64 LL/SC */
	mov	w1, #1
1:	ldaxr	w2, [x0]
	stxr	w3, w1, [x0]
	cbnz	w3, 1b
	and	w0, w2, #1
	ret
