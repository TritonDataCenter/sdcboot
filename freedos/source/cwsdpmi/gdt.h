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
/* Modified for VCPI Implement by Y.Shibata Aug 5th 1991 */

typedef struct DESC_S {
word16 lim0;
word16 base0;
word8 base1;
word8 stype;	/* type, DT, DPL, present */
word8 lim1;	/* limit, granularity */
word8 base2;
} DESC_S;

#ifndef run_ring
#define run_ring	3	/* set for user app priv level */
#endif

#define g_zero		0
#define g_gdt		1
#define g_idt		2
#define g_rcode		3
#define g_rdata		4
#define g_pcode		5	/* Same as rcode but user ring! */
#define g_pdata		6	/* Same as rdata but user ring! */
#define g_core		7
#define g_BIOSdata	8

#define g_vcpicode	9	/* for VCPI Call Selector in Protect Mode */
#define g_vcpireserve0  10
#define g_vcpireserve1  11

#define g_atss		12	/* set according to tss_ptr in go32() */
#define g_ctss		13
#define g_itss		14
#define g_ldt		15
#define g_iret		16

#define g_num		17

#define l_free		16

#define l_acode 	16
#define l_adata 	17
#define l_apsp		18
#define l_aenv		19

#define l_num		128	/* Should be 8*MAX_AREA in paging.h (balance) */

extern DESC_S gdt[g_num];
extern DESC_S ldt[l_num];

#define LDT_SEL(x)   (((x)*8) | 4 | run_ring)
#define GDT_SEL(x)   (((x)*8) | run_ring)
#define SEL_PRV      (run_ring << 5)
#define LDT_FREE(i)  (!(ldt[i].stype))
#define LDT_USED(i)  (ldt[i].stype)
#define LDT_MARK_FREE(i) free_desc(i)
#define LDT_CODE(i)  ((ldt[i].stype & 8))
#define LDT_DATA(i)  (!(ldt[i].stype & 8))
#define LDT_VALID(i) ((word16)(i) < l_num)
