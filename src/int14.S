/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2014, Joyent, Inc.
 */

/*
 * DOS "COM" style TSR
 *
 * Installs alternate handler for int 14 (BIOS serial I/O) and exits.
 * Note that the PSP is from CS:0000 to CS:0100.  We cannot put anything
 * there, and must save it when we terminate.
 */

#define	IVT_INT14_IP	(0x14 * 4)
#define	IVT_INT14_CS	(0x14 * 4 + 2)

#define	DOS_SYSCALL_TSR	0x31
#define	DOS_SYSCALL	0x21

#define	ERROR_RETURN	0x8000

#define	BDA_SEG		0x40

#define	R_8250_DLL	0x0
#define	R_8250_DLH	0x1
#define	R_8250_LCR	0x3
#define	R_8250_MCR	0x4
#define	R_8250_LSR	0x5
#define	R_8250_MSR	0x6

#define	RF_8250_LCR_DLAB	0x80
#define	RF_8250_LSR_RDY		0x01
#define	RF_8250_LSR_THRE	0x20

#define	RI_8250_DLL	0x01	/* 115200 */
#define	RI_8250_DLH	0x00
#define	RI_8250_LCR	0x03	/* 8,n,1 */
#define	RI_8250_MCR	0x03	/* assert DSR, RTS */

#define	TIMEOUT	0xffff

.text
.code16

.globl _start
.type _start, "function"
_start:
	xor	%si, %si		/* si = port addr offset in BDA */
1:
	cmp	$0x8, %si		/* max 4 ports */
	jae	2f
	push	%ds
	push	$BDA_SEG
	pop	%ds
	mov	(%si), %dx		/* get port address */
	pop	%ds
	test	%dx, %dx		/* no more ports */
	jz	2f
	call	uart_init
	add	$0x2, %si		/* next port index */
	jmp	1b

2:
	push	%cs
	pop	%ax			/* ax = TSR CS */
	mov	$int14_entry, %di	/* di = entry point */
	xor	%bx, %bx		/* bx = IVT DS, IVT base */
	push	%ds
	push	%bx
	pop	%ds			/* ds -> IVT */
	mov	%di, IVT_INT14_IP(%bx)	/* destination address */
	mov	%ax, IVT_INT14_CS(%bx)	/* destination CS */
	pop	%ds
	mov	$_end, %dx		/* PSP + all code; linker will adjust */
	shr	$0x4, %dx
	xor	%ax, %ax
	mov	$DOS_SYSCALL_TSR, %ah
	int	$DOS_SYSCALL
.size _start, (. - _start)

/*
 * Inputs:
 *
 * %dx = UART base address
 */
.type uart_init, "function"
uart_init:
	push	%di
	mov	%dx, %di

	add	$R_8250_LCR, %dx	/* set DLAB */
	in	%dx, %al
	or	$RF_8250_LCR_DLAB, %al
	out	%al, %dx
	mov	%di, %dx

	mov	$RI_8250_DLL, %al
	out	%al, %dx		/* set DLL */
	mov	$RI_8250_DLH, %al
	inc	%dx
	out	%al, %dx		/* set DLH */
	mov	%di, %dx
	mov	$RI_8250_LCR, %al
	add	$R_8250_LCR, %dx
	out	%al, %dx		/* set LCR */
	mov	%di, %dx
	add	$R_8250_MCR, %dx
	mov	$RI_8250_MCR, %al
	out	%al, %dx		/* set MCR */

	pop	%di
	ret
.size uart_init, (. - uart_init)

/*
 * Replacement for BIOS int 14 handler
 *
 * %ah = command
 *	00 - init port (always pretend to succeed)
 *	01 - send byte in %al to port index %dx, return 0 or 1<lsr>
 *	02 - receive into %al (clear %ah or 1<lsr> on error)
 *	03 - put LSR:MSR into %ax for port index %dx
 * %dx = port index [0,3]
 *
 * %ax is always trashed.  All other registers must be preserved!
 */
.type int14_entry, "function"
int14_entry:
	push	%bx
	push	%cx
	push	%dx
	push	%si
	push	%di
	push	%ds
	push	%cs
	pop	%ds
	mov	$cmdtbl, %bx
	mov	$cmdmax, %cx
	mov	%ax, %si
	shr	$0x7, %si
	and	$0xfe, %si	/* si = cmd << 1 */
	add	%si, %bx	/* bx = address of cmd vector */
	cmp	%bx, %cx	/* check for valid command */
	jb	1f

	push	%ds
	push	$BDA_SEG
	pop	%ds		/* ds = BDA */
	shl	$0x1, %dx
	mov	%dx, %si
	mov	(%si), %dx	/* dx = UART base address */
	pop	%ds
	test	%dx, %dx	/* zf = port does not exist */
	jz	1f

	mov	(%bx), %si	/* si = cmd vector */
	callw	*%si		/* execute command */
	jmp	2f
1:
	mov	$ERROR_RETURN, %ax	/* bad command */
2:
	pop	%ds
	pop	%di
	pop	%si
	pop	%dx
	pop	%cx
	pop	%bx
	iret
.size int14_entry, (. - int14_entry)

cmdtbl:
	.word	int14_stat	/* init is undesirable */
	.word	int14_xmit
	.word	int14_recv
	.word	int14_stat
cmdmax:
	.word	0

/*
 * Inputs:
 *
 * %ah = 0
 * %dx = port address
 *
 * Outputs:
 *
 * %ah = LSR
 * %al = MSR
 */
.type int14_stat, "function"
int14_stat:
	push	%dx
	add	$R_8250_LCR, %dx
	in	%dx, %al	/* fetch LCR */
	and	$0x7f, %al	/* clear DLAB */
	out	%al, %dx
	add	$0x2, %dx	/* R_8250_LSR - R_8250_LCR */
	xor	%ax, %ax
	in	%dx, %al	/* fetch LSR */
	shl	$0x8, %ax
	inc	%dx
	in	%dx, %al	/* fetch MSR */
	pop	%dx
	ret			/* ax = LSR:MSR */
.size int14_stat, (. - int14_stat)

/*
 * Inputs:
 *
 * %ah = 1
 * %al = byte to transmit
 * %dx = port address
 *
 * Outputs:
 *
 * %ah[7] = timeout
 * %ah[6:0] = LSR[6:0]
 * %al = byte transmitted
 */
.type int14_xmit, "function"
int14_xmit:
	push	%bx
	mov	%ax, %bx
	push	%cx

	mov	$TIMEOUT, %cx
1:
	call	int14_stat
	test	$RF_8250_LSR_THRE, %ah	/* wait for LSR:THRE */
	jnz	2f
	loop	1b
	or	$0x80, %ah	/* error bit */
	jmp	3f
2:
	mov	%bl, %al
	out	%al, %dx	/* transmit */
	call	int14_stat	/* get post-transmit status */
	mov	%bl, %al
	and	$0x7f, %ah	/* clear error bit */
3:
	pop	%cx
	pop	%bx
	ret
.size int14_xmit, (. - int14_xmit)

/*
 * Inputs:
 *
 * %ah = 2
 * %dx = port address
 *
 * Outputs:
 *
 * %ah[7] = timeout
 * %ah[6:0] = LSR[6:0]
 * %al = byte received
 */
.type int14_recv, "function"
int14_recv:
	push	%bx
	push	%cx

	mov	$TIMEOUT, %cx	/* timeout counter */
1:
	call	int14_stat
	test	$RF_8250_LSR_RDY, %ah
	jnz	2f
	loop	1b		/* check timeout */
	or	$0x80, %ah	/* set error bit */
	mov	$0x0, %al
	jmp	3f

2:
	in	%dx, %al
	mov	%ax, %bx
	call	int14_stat
	and	$0x7f, %ah	/* clear error bit */
	mov	%bl, %al
3:
	pop	%cx
	pop	%bx
	ret
.size int14_recv, (. - int14_recv)

.align 16
_end:
