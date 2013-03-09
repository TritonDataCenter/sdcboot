/* Copyright (C) 1995,1996 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugarland, TX 77479
** Copyright (C) 1993 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
**
** This file is distributed under the terms listed in the document
** "copying.cws", available from CW Sandmann at the address above.
** A copy of "copying.cws" should accompany this file; if not, a copy
** should be available from where this file was obtained.  This file
** may not be distributed without a verbatim copy of "copying.cws".
**
** This file is distributed WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/* Hack note: esp1, esp2 not used, so replaced with backlink info */

typedef struct TSS {
	unsigned short tss_back_link;
	unsigned short res0;
	unsigned long  tss_esp0;
	unsigned short tss_ss0;
	unsigned short res1;
/*	unsigned long  tss_esp1; */
	unsigned short tss_cur_es;
	unsigned short tss_lastused;
	unsigned short tss_ss1;
	unsigned short res2;
/*	unsigned long  tss_esp2; */
	unsigned short tss_old_env;
	unsigned short tss_cur_psp;
	unsigned short tss_ss2;
	unsigned short res3;
	unsigned long  tss_cr3;
	unsigned long  tss_eip;
	unsigned long  tss_eflags;
	unsigned long  tss_eax;
	unsigned long  tss_ecx;
	unsigned long  tss_edx;
	unsigned long  tss_ebx;
	unsigned long  tss_esp;
	unsigned long  tss_ebp;
	unsigned long  tss_esi;
	unsigned long  tss_edi;
	unsigned short tss_es;
	unsigned short res4;
	unsigned short tss_cs;
	unsigned short res5;
	unsigned short tss_ss;
	unsigned short res6;
	unsigned short tss_ds;
	unsigned short res7;
	unsigned short tss_fs;
	unsigned short res8;
	unsigned short tss_gs;
	unsigned short res9;
	unsigned short tss_ldt;
	unsigned short res10;
	unsigned short tss_trap;
	unsigned char  tss_iomap;	/* Not used, ring 0 or IOPL = 3 */
	unsigned char  tss_irqn;
	unsigned long  tss_cr2;
	unsigned long  tss_error;
	unsigned short stack0[64];	/* 15 max used, 30 worst case */
	unsigned short tss_stack[1];
} TSS;

extern TSS c_tss, a_tss, o_tss, i_tss, f_tss;
extern TSS *tss_ptr;
