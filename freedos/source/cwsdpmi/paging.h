/* Copyright (C) 1995-1999 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
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

/* active if set */
#define PT_P	0x001	/* present (else not) */
#define PT_W	0x002	/* writable (else read-only) */
#define	PT_U	0x004	/* user mode (else kernel mode) */
#define	PT_T	0x008	/* page write-through (else write-back) */
#define	PT_CD	0x010	/* page caching disabled (else enabled) */
#define PT_A	0x020	/* accessed (else not) */
#define PT_D	0x040	/* dirty (else clean) */
#define PT_PS	0x080	/* page size (4Mb page in PDE) */
#define PT_G	0x100	/* global */
#define PT_I	0x200	/* Initialized (else contents undefined) USERDEF */
#define PT_S	0x400	/* Swappable (else not) USERDEF */
#define	PT_C	0x800	/* Candidate for swapping USERDEF */

/* Page table bit definitions:
    In memory:        P  I  -
    Mapped:           P !I !S
    In swapfile:     !P  I  S
    Not yet touched: !P !I  S
    Uncommitted:     !P  - !S
    Unused:           P !I  S
    If dirty or PT_C save contents, else discard on swapout
*/

void paging_setup(void);
word32 ptr2linear(void far *ptr);

int page_in(void);
va_pn page_out(void);
unsigned page_out_640(void);
int page_is_valid(word32 vaddr);
void physical_map(word32 vaddr, word32 size, word32 where);
int lock_memory(word32 vaddr, word32 size, word8 unlock);
void free_memory(word32 vaddr, word32 vlast);
int free_memory_area(word32 vaddr);
int page_attributes(word8 set, word32 start, word16 count);
void move_pt(word32 vorig, word32 vend, word32 vnew);
int cant_ask_for(int32 amount);

typedef struct AREAS {	/* in linear space, not program space */
  word32 first_addr;
  word32 last_addr;
  struct AREAS *next;
  } AREAS;

extern AREAS *firstarea;
extern word32 far *vcpi_pt;
extern word32 reserved;
