/* Copyright (C) 1995-2010 CW Sandmann (cwsdpmi@earthlink.net) 1206 Braelinn, Sugar Land, TX 77479
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
/* PC98 support contributed 6-95 tantan SGL00213@niftyserve.or.jp */

#define _USE_PORT_INTRINSICS

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>

#include "gotypes.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"
#include "utils.h"
#include "valloc.h"
#include "paging.h"
#include "vcpi.h"
#include "dpmisim.h"
#include "dalloc.h"
#include "control.h"
#include "mswitch.h"
#include "exphdlr.h"

/* Notes on HW interrupts (include info on Int 0x1c, 0x23, 0x24 also):
   Since HW interrupts may occur at any time interrupts are disabled, we
   minimize complications by keeping interrupts disabled during all ring 0
   PM code in CWSDPMI and during almost all RM code here too.  Exceptions
   are when we jump to user routines, handle page faults, or handle
   interrupts which may enable interrupts.  An ugly fact is any print
   routines in this code may also enable interrupts while in DOS, so beware.

   HW interrupts which occur in PM go to the IDT and by default are handled
   at Ring 0 (the user stack my be invalid, this is a MUST, the ring change
   switches to the Ring 0 stack which is always valid).  The default handling
   is they are reflected to real mode.  If the user has not hooked a HW int,
   we do not reflect RM ints into PM at all.  Life gets complicated when the
   user hooks a HW interrupt.

   Once hooked, HW interrupts which occur in PM go to the IDT and jump to
   the irq routines in tables.asm which are ring 0 code on a ring 0 stack.
   That code moves to a locked stack and changes to ring 3, jumping to the
   user routine.  Their IRET will jump to user_interrupt_return in dpmisim.asm
   which emulates an IRET, but at the same ring (changing stacks).

   HW interrupts which occur in RM jump to RMCB's which are specially set up
   to call the user's routine (coded as type != 0), then IRET.

   Chaining is ugly.  We return the original PM HW interrupt routine on get
   vector calls, modifying the code selector to be ring 3.  This is OK because
   they should be on the locked stack.  The reflection to real mode would cause
   an infinite loop, since it would trigger a RMCB.  So, before installing
   the RMCB we must save the original RM handler and have special code to
   jump to it if and only if a RMCB is hooked (yuk, this is in mswitch).

   Int 1c doesn't need a PM ring 0 handler, just the RMCB, and is treated as
   IRQ 16.  Int 1c does need to handle chaining.

   Int 23 and Int 24 don't need PM ring 0 handlers, and don't need to handle
   chaining.  But they are non-standard interrupt handling that need to do
   things on the basis of the carry flag/RETF (I23) or AL (I24).  Int 23 is
   restricted to IRET so can be handled in a standard way, but Int 24 requires
   special values to be set up (AX, BP:SI, DI) and a special return with AL=3.
   This is RMCB type 2, handled in a DPMISIM rmcb_task clone (not done).
*/

#define n_user_excep 15
#define n_user_hwint 18				/* extras for 0x1c,0x23 */
#define VADDR_START 0x400000UL

/* If the real mode procedure/interrupt routine uses more than 30 words of
   stack space then you should provide your own real mode stack.
   DPMI 0.9 Specification, Chapter 11, page 53 */

#ifdef STKUSE	
#define STKSIZ 512	/* Words, extra to check usage (bad bios, etc) */
#else
#define STKSIZ 128	/* Words, size = 4 x 0.9 spec */
#endif

static far32 user_exception_handler[n_user_excep];
far32 user_interrupt_handler[n_user_hwint];
far16 saved_interrupt_vector[n_user_hwint];	/* Used by mswitch */
static word8 hw_int_rmcb[n_user_hwint] = {num_rmcb, num_rmcb, num_rmcb, num_rmcb,
  num_rmcb, num_rmcb, num_rmcb, num_rmcb, num_rmcb, num_rmcb, num_rmcb,
  num_rmcb, num_rmcb, num_rmcb, num_rmcb, num_rmcb, num_rmcb, num_rmcb };
dpmisim_rmcb_struct dpmisim_rmcb[num_rmcb];

extern far32 far i30x_jump;
extern far32 far i30x_stack;
extern word8 far i30x_sti;
extern void ivec31x(void);
extern char in_rmcb;

word32 locked_stack[1024];	/* 4K locked stack */
word8 locked_count = 0;

static word8 old_master_lo=0x08;
word8 hard_master_lo=0x08, hard_master_hi=0x0f;
word8 hard_slave_lo=0x70,  hard_slave_hi=0x77;

static char cntrls_initted = 0;
static char far * oldvec;

static int i_21(void), i_2f(void), i_31(void);

#if 0
void segfault(word32 v)
{
  errmsg("Access violation, address = 0x%08lx\n", (v&0xfffffffL));
  do_faulting_finish_message();
}
#endif

void static set_controller(int v)
{
  if (mtype == PC98) {
    int oldmask =inportb(0x02);  /* On PC98 8259 port is 0x00,0x02 */
    outportb(0x00, 0x11);
    outportb(0x02, v);
    outportb(0x02, 0x80);
    outportb(0x02, 0x1d);
    outportb(0x02, oldmask);
  } else {
    int oldmask =inportb(0x21);
    outportb(0x20, 0x11);
    outportb(0x21, v);
    outportb(0x21, 4);
    outportb(0x21, 1);
    outportb(0x21, oldmask);
  }
}

static int find_empty_pic(void)
{
  static word8 try[] = { 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8, 0xf8, 0x68, 0x78 };
  int i, j;
  for (i=0; i<sizeof(try); i++)
  {
    char far * far * vec = MK_FP(0, try[i]*4);
    oldvec = vec[0];
    for (j=1; j<8; j++)
    {
      if (*(++vec) != oldvec)
        goto not_empty;
    }
    return try[i];
    not_empty:;
  }
  return 0x78;
}

/* static word32 saved_interrupt_table[256]; */

void init_controllers(void)
{
  if(cntrls_initted) return;
  cntrls_initted = 1;
  { /* Make sure cleaned up in case previous clients did not */
    int i;
    for(i=0; i < n_user_excep; i++)
      user_exception_handler[i].selector = 0;
    for(i=0; i < n_user_hwint; i++)
      user_interrupt_handler[i].selector = 0;
  }
  locked_count = 0;
  in_rmcb = 0;

/*  movedata(0, 0, _DS, FP_OFF(saved_interrupt_table), 256*4); */

  disable();

  if (vcpi_installed) {
    old_master_lo = vcpi_get_pic();
    hard_slave_lo = vcpi_get_secpic();
  }
  /* else set in initialization; don't turn off VCPI after loading CWSDPMI */

  if (old_master_lo == 0x08) {
    word16 i,offset;
    word16 far *iptr;
    extern void real_i8(void);

    hard_master_lo = find_empty_pic();
    if (vcpi_installed)
      vcpi_set_pics(hard_master_lo, hard_slave_lo);
    set_controller(hard_master_lo);

/*    movedata(0, 0x08*4, 0, hard_master_lo*4, 0x08*4); */
    /* Point new controller block to our space, which we redirect to original
       interrupt area.  Allows nested programs which grab interrupts to work */
    iptr = MK_FP(0, hard_master_lo*4);
    offset = FP_OFF(real_i8);
    for(i=0; i<8; i++) {
      *iptr++ = offset;
      *iptr++ = _CS;
      offset += 3;  /* size of int 0x, iret */
    }
  }
  else
    hard_master_lo = old_master_lo;

  hard_master_hi = hard_master_lo + 7;
  hard_slave_hi = hard_slave_lo + 7;

  enable();
}

void uninit_controllers(void)
{
  if(!cntrls_initted) return;
  cntrls_initted = 0;
  disable();

/*  movedata(_DS, FP_OFF(saved_interrupt_table), 0, 0, 256*4); */

  {
    int i;
    for(i = 0; i < n_user_hwint; i++)
      if(saved_interrupt_vector[i].segment) {
        int j;
        if(i < 8)
          j = hard_master_lo + i;
        else if(i < 16)
          j = hard_slave_lo + i - 8;
        else if(i == 16)
          j = 0x1c;
        else /* if(i == 17) */
          j = 0x23;
        poke(0, j*4, saved_interrupt_vector[i].offset);
        poke(0, j*4+2, saved_interrupt_vector[i].segment);
        dpmisim_rmcb[hw_int_rmcb[i]].cb_sel = 0;
        hw_int_rmcb[i] = num_rmcb;
        saved_interrupt_vector[i].segment = 0;
      }
  }
  if (old_master_lo == 0x08 )
  {
    if (vcpi_installed)
      vcpi_set_pics(0x08, hard_slave_lo);
    set_controller(0x08);

    { /* Restore the table entries we scrogged */
    int j;
    char far * far * vec = (char far * far *)((word32)(hard_master_lo << 2));
    for (j=0; j<8; j++)
      *(vec++) = oldvec;
    }
  }
  enable();
}

#ifdef RSX
/*
 * try segment translation for valid LDT selectors (Rainer code modified)
 * some clients need this undocumented Windows feature
 */
static word16 sel2segm(word16 sel)
{
  word16 idx = sel/8;

  /* test valid LDT selector, in 1Mb area on paragraph boundary */
  if ((sel & 4) && !(ldt[idx].base1 & 0xf0) && !(ldt[idx].base0 & 0xf)
    && !ldt[idx].base2 && LDT_USED(idx))
      return ((ldt[idx].base0 >> 4) + (ldt[idx].base1 << 12));
  return sel;
}
#endif

extern int generic_handler(void);

/* This routine will send control to the faulting finish cleanup */
static int unsupported_int(void)
{
  return 1;
}

/* The user_exception_handler default == 0 is minor deviation from the DPMI
   spec.  The spec says if a user handler isn't set up, it should be handled
   as an interrupt, eventually passed to real mode.  If they set up a PM
   interrupt handler, we will never get here.  If they didn't, passing it
   to real mode is almost certainly the wrong thing to do (hang) regardless
   of what the spec says.  So, if they chain, we GPF on the null selector
   and abort, just with the wrong register display.  I can live with it. */

static int user_exception(void)
{
  word16 locked_sp;
  word32 *locked_loc;
  int i = tss_ptr->tss_irqn;
#if run_ring != 0
  if(!(tss_ptr->tss_cs & 3))
    return 1;	/* Don't try to handle ring 0 exceptions */
#endif
  if(locked_count > 5)
    return 1;
  if(tss_ptr == &a_tss && user_exception_handler[i].selector) {
    if(locked_count)
      locked_loc = &locked_stack[0];	/* use bottom; copy later */
    else
      locked_loc = &locked_stack[1016];
    *(locked_loc++) = FP_OFF(user_exception_return);
    *(locked_loc++) = GDT_SEL(g_pcode);	/* our local prot CS */
    *(locked_loc++) = tss_ptr->tss_error;
    *(locked_loc++) = tss_ptr->tss_eip;
    *(locked_loc++) = tss_ptr->tss_cs;
    *(locked_loc++) = tss_ptr->tss_eflags;
    *(locked_loc++) = tss_ptr->tss_esp;
    *locked_loc = tss_ptr->tss_ss;
    if(locked_count) {
      tss_ptr->tss_esp -= 32;
      memput(tss_ptr->tss_ss, tss_ptr->tss_esp, &locked_stack[0], 32);
    } else {
      tss_ptr->tss_ss = GDT_SEL(g_pdata);
      tss_ptr->tss_esp = FP_OFF(&locked_stack[1016]);
    }
    tss_ptr->tss_eip = user_exception_handler[i].offset32;
    tss_ptr->tss_cs = user_exception_handler[i].selector;
    tss_ptr->tss_eflags = 0x3002;	/*Interrupts disabled; clear T, etc */
    locked_count++;
    return 0;
  }
  return 1;
}

static int page_in_user(void)
{
  if(in_rmcb)
    return 1;			/* Page fault in interrupted code; save disk! */
  if(page_in())			/* If we can't handle it, give user a shot */
    return user_exception();
  return 0;
}

/* The DPMI spec says we should pass all interrupts through "generic_handler".
   In some cases we choose not to do this for interrupts that most likely would
   cause a wedge, like jumping to data.  All of the unsupported_int calls below
   are deviations from the DPMI spec on purpose.  If any complaints, fix it.  */

#define U unsupported_int
#define UE user_exception
typedef int (*FUNC)(void);
static FUNC exception_handler_list[] = {
  UE, UE, generic_handler, UE, UE, UE, UE, UE,	/* NMI passed - laptops/green */
  /* 08 */ U,			/* Double fault */
  UE, UE, UE, UE, UE,
  /* 0e */ page_in_user,
  UE,
  /* 10 */ generic_handler,	/* Video Interrupt */
  /* 11 */ generic_handler,	/* Equipment detection */
  /* 12 */ generic_handler,	/* Get memory size (outdated) */
  /* 13 */ generic_handler,	/* Disk services */
  /* 14 */ generic_handler,	/* Serial communication */
  /* 15 */ generic_handler,	/* Lots of strange things */
  /* 16 */ generic_handler,	/* Keyboard */
  /* 17 */ generic_handler,	/* Parallel printer */
  U, generic_handler,		/* ROM Basic, Reboot */
  /* 1a */ generic_handler,	/* Get/set system time */
  generic_handler, generic_handler, /* ^Break and User Tic */
  U, U, U, U,			/* Data pointers, "exit" */
  /* 21 */ i_21,		/* DOS Services */
  U, U, U, U, U, U,		/* DOS things which should not be called */
  /* 28 */ generic_handler, 	/* DOS idle */
  /* 29 */ generic_handler,	/* DOS Fast I/O */
  generic_handler, generic_handler, /* Network and unused */
  generic_handler, generic_handler, generic_handler,
  /* 2f */ i_2f, U,		/* Multiplex for PM execution */
  /* 31 */ i_31			/* DPMIsim */
};
#undef U
#undef UE
#define NUM_EXCEPTIONS	(sizeof(exception_handler_list)/sizeof(exception_handler_list[0]))

int exception_handler(void)
{
  int i;
  i = tss_ptr->tss_irqn;
/*  errmsg("i=%#02x\r\n", i); /* */
  if (i < NUM_EXCEPTIONS)
    return (exception_handler_list[i])();
#if 0
  else if(i == 0x4b)
    return i_4b();
#endif
  else
    return generic_handler();
}

static int i_21(void)
{
  word8 ah;
  ah = (word8)((word16)(tss_ptr->tss_eax) >> 8);
  if(ah == 0x4c)
      cleanup((word16)(tss_ptr->tss_eax));
  return generic_handler();
}

static int i_2f(void)
{
  if((word16)(tss_ptr->tss_eax) == 0x1686) {
    (word16)(tss_ptr->tss_eax) = 0;
    return 0;
  }
  return generic_handler();
}

static int8 hwirq;

static int reg2gate(word8 r)
{
  if(r >= 0x08 && r <= 0x0f) {
    hwirq = r - 8;
    r = hwirq + hard_master_lo;
  } else if(r >= hard_slave_lo && r <= hard_slave_hi)
    hwirq = r - hard_slave_lo + 8;
  else if(r == 0x1c)
    hwirq = 16;
  else if(r == 0x23)
    hwirq = 17;
  else
    hwirq = -1;
  return r;
}

#if 0

#define DESC_BASE(d)  (((((word32)d.base2<<8)|(word32)d.base1)<<16)|(word32)d.base0)

static void check_pointer(word16 sel, word32 off)
{
  /* Checking limit ugly (direction, G bit) so skip. */
  int i = sel/8;
  word32 base;
  if(sel&4)
    base = DESC_BASE(ldt[i]);
  else
    base = DESC_BASE(gdt[i]);
  base += off;
  if(base < 0xfffff || page_is_valid(base))	/* 640K area or our area */
    return;
  segfault(off);
}

#define CHECK_POINTER(sel,off) check_pointer((word16)sel,off)
#else
#define CHECK_POINTER(sel,off)
#endif

#define SET_CARRY (word8)tss_ptr->tss_eflags |= 1
#define EXIT_OK goto i31_exit
#define EXIT_ERROR goto i31_exit_error

int alloc_ldt(int ns)
{
  word16 i, j;

  for(i=l_free; (i+ns)<=l_num; i++) {
    for(j=0; j<ns && LDT_FREE(i+j); j++);
    if(j>=ns)break;
  }
  if((i+ns)<=l_num){
    for(j=0;j<ns;j++,i++) {
      ldt[i].lim0 = ldt[i].base0 = 0;
      ldt[i].base1 = ldt[i].base2 = 0;
      ldt[i].lim1 = 0x40;		/* 32-bit, byte granularity, used.  */
      ldt[i].stype = 0x92 | SEL_PRV;	/* Present, data, r/w. */
    }
    return i-ns;	/* Allocation succeeded.  */
  }
  SET_CARRY;
  return 0;		/* Allocation not possible.  */
}

static word16 sel_to_ldt_index(word16 reg)
{
  word16 sel = reg;
  if(sel & 0x4) {
    sel = sel / 8;
    if(sel < l_num && LDT_USED(sel))
      return sel;
  }
  return 0xffff;
}

/* This routine exists to compress code space.  Address calculation is
   expensive; since ebx is used many times, we hardcode it once. */
static word16 sel_to_ldt_index0(void)
{
  return sel_to_ldt_index((word16)tss_ptr->tss_ebx);
}

#define ASSIGN_LDT_INDEX(VAR,REG) if((VAR = sel_to_ldt_index0()) == 0xffff) EXIT_ERROR
#define ASSIGN_LDT_INDEX1(VAR,REG) if((VAR = sel_to_ldt_index((word16)tss_ptr->REG)) == 0xffff) EXIT_ERROR

static void free_desc(word16 i)
{
/* DPMI 1.0 free descriptor clears any sregs containing value.  Don't bother
   with CS, SS since illegal and will be caught in task switch */
  word16 tval = LDT_SEL(i);
  ldt[i].stype = 0;
  if(tss_ptr->tss_ds == tval) tss_ptr->tss_ds = 0;
  if(tss_ptr->tss_es == tval) tss_ptr->tss_es = 0;
  if(tss_ptr->tss_fs == tval) tss_ptr->tss_fs = 0;
  if(tss_ptr->tss_gs == tval) tss_ptr->tss_gs = 0;
}

/* The DPMI 1.0 specification says 32-bit clients only get one descriptor on
   a DOS memory allocation over 64K.  The 0.9 specification does not agree.
   Windows 3.1 (which is 0.9) only allocates a single descriptor.  Go figure.
   Since we are currently 32-bit only, disable to decrease footprint.  Code
   must be enabled to pass rigorous tests; adds 600 bytes to resident size.
   Since Win 3.1 does not support it, I am sure it is not used by anyone.
   It will be needed when 16-bit support is added.  */

#ifdef DOSMEM_MULTIPLE
static void set_limits(word16 segm, word16 para, word16 sel0, int sels)
{
  word32 lim,base,limit;
  int first;

  base = (word32)segm << 4;
  limit = para ? ((word32)para << 4) - 1L : 0L;
  first = 1;		/* Only set this if 32-bit client */
  while (sels--) {
    if (first || (word16)(limit >> 16) == 0)
      lim = limit;
    else
      lim = 0xffffL;
    ldt[sel0].base0 = (word16)base;
    ldt[sel0].base1 = (word8)(base >> 16);
    ldt[sel0].base2 = 0;
    ldt[sel0].lim0 = (word16)lim;
    ldt[sel0].lim1 = (word8)(lim >> 16);
    /* Resize does not use alloc_ldt, so we must set the type */
    ldt[sel0++].stype = 0x92 | SEL_PRV;  /* Present, data, r/w. */

    limit -= 0x10000L;
    base += 0x10000L;
    first = 0;
  }
}
#else

static void set_para_limit(word16 i, word16 npara)
{
  word32 limit = npara;
  if(npara)
    limit = (limit << 4) - 1L;
  ldt[i].lim0 = (word16)limit;
  ldt[i].lim1 = limit >> 16;
}
#endif

/* Note: mswitch leaves interrupts disabled, so this entire routine runs with
   interrupts disabled.  Debugging note:  printf() statements may re-enable
   interrupts temporarily and cause unexpected behavior, especially with
   HW interrupts or mouse callbacks. */

static int i_31(void)
{
/*  errmsg("DPMI request 0x%x 0x%x 0x%x 0x%lx\r\n",(word16)(tss_ptr->tss_eax),
    (word16)(tss_ptr->tss_ebx),(word16)(tss_ptr->tss_ecx),(tss_ptr->tss_edx));   /* */
  (word8)tss_ptr->tss_eflags &= ~1;	/* Clear carry, only set on failure */
#if 0	/* Testing code for 16-bit */
  tss_ptr->tss_edx = (word16)tss_ptr->tss_edx;
  tss_ptr->tss_esi = (word16)tss_ptr->tss_esi;
  tss_ptr->tss_edi = (word16)tss_ptr->tss_edi;
#endif
  switch ((word16)(tss_ptr->tss_eax))
  {
    case 0x0000: /* Allocate LDT descriptors.  */
      {
      word16 i = alloc_ldt((word16)tss_ptr->tss_ecx);
      if (i)
        (word16)tss_ptr->tss_eax = LDT_SEL(i);
      EXIT_OK;
      }

    case 0x0001: /* Free LDT descriptor.  */
      {
      word16 i;
      ASSIGN_LDT_INDEX(i, ebx);
      LDT_MARK_FREE(i);
      EXIT_OK;
      }

    case 0x0002: /* Segment to descriptor.  */
      {
      word16 i;
      word16 base0 = (word16)tss_ptr->tss_ebx << 4;
      word16 base1 = (word16)tss_ptr->tss_ebx >> 12;

      /* Search for same descriptor */
      for(i=l_free; i<l_num; i++)
        if(!ldt[i].lim1 && !ldt[i].base2 && ldt[i].lim0 == 0xffff &&
            LDT_USED(i) && ldt[i].base0 == base0 && ldt[i].base1 == base1) {
          (word16)tss_ptr->tss_eax = LDT_SEL(i);
          EXIT_OK;
        }

      i = alloc_ldt(1);
      if(i) {
        (word16)tss_ptr->tss_eax = LDT_SEL(i);
        ldt[i].lim0 = 0xffff;
        ldt[i].base0 = base0;
        ldt[i].base1 = base1;
        ldt[i].lim1 = 0;        /* 16-bit, not 32 */
      }
      EXIT_OK;
      }

    case 0x0003: /* Get Selector Increment.  */
      (word16)tss_ptr->tss_eax = 8;
      EXIT_OK;

    case 0x0006: /* Get Selector Base Address.  */
      {
      word16 i;
      ASSIGN_LDT_INDEX(i, ebx);
      (word16)tss_ptr->tss_edx = ldt[i].base0;
      (word16)tss_ptr->tss_ecx = ldt[i].base1 | (ldt[i].base2 << 8);
      EXIT_OK;
      }

    case 0x0007: /* Set Selector Base Address.  */
      {
      word16 i;
      ASSIGN_LDT_INDEX(i, ebx);
      ldt[i].base0 = (word16)tss_ptr->tss_edx;
      ldt[i].base1 = (word8)tss_ptr->tss_ecx;
      ldt[i].base2 = ((word16)tss_ptr->tss_ecx >> 8) & 0xff;
      EXIT_OK;
      }

    case 0x0008: /* Set Selector Limit.  */
      {
      word16 i, j, k;
/*    word32 *show32; show32 = ldt + i; errmsg("Descriptor(%d,8): 0x%lx 0x%lx\n",i,show32[0],show32[1]); */
      ASSIGN_LDT_INDEX(i, ebx);
      j = (word16)tss_ptr->tss_edx;
      k = (word16)tss_ptr->tss_ecx;
      if(k > 0xf) {
        j = (j >> 12) | (k << 4);
        k = 0x80 | (k >> 12);
      }
      ldt[i].lim0 = j;
      ldt[i].lim1 &= 0x70;
      ldt[i].lim1 |= k;
      EXIT_OK;
      }

    case 0x0009: /* Set Selector Access Rights.  */
      {
      word16 i;
      ASSIGN_LDT_INDEX(i, ebx);
      ldt[i].stype = 0x10 | (word8)tss_ptr->tss_ecx;
      ldt[i].lim1 = (0x0f & ldt[i].lim1) | (((word16)tss_ptr->tss_ecx >> 8) & 0xd0);
      EXIT_OK;
      }

    case 0x000a: /* Create Code Selector Alias.  */
      {
      word16 i,j;
      ASSIGN_LDT_INDEX(i, ebx);
      j = alloc_ldt(1);
      if(j) {
        (word16)tss_ptr->tss_eax = LDT_SEL(j);
        ldt[j] = ldt[i];
        ldt[j].stype = 2 | (ldt[j].stype & 0xf0);	/* Data, exp-up, wrt */
      }
      EXIT_OK;
      }

    case 0x000b: /* Get Descriptor.  */
      {
      word16 i;
      CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi);
      ASSIGN_LDT_INDEX(i, ebx);
      memput(tss_ptr->tss_es, tss_ptr->tss_edi, &ldt[i], 8);
      EXIT_OK;
      }

    case 0x000c: /* Set Descriptor.  */
      {
      word16 i;
      CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi);
      ASSIGN_LDT_INDEX(i, ebx);
      memget(tss_ptr->tss_es, tss_ptr->tss_edi, &ldt[i], 8);
      ldt[i].stype |= 0x10;		/* Make sure non-system, non-zero */
      EXIT_OK;
      }

    case 0x000d: /* Allocate Specific LDT Selector.  */
      {
      word16 j = (word16)tss_ptr->tss_ebx;
      /* Using ASSIGN_LDT_INDEX here doesn't work since the selector is
         supposed to be free, not used, and they may want "0" */
      word16 i = j/8;
      if(i<16 && (j&4) && LDT_FREE(i)) {
        ldt[i].lim0 = ldt[i].base0 = 0;
        ldt[i].base1 = ldt[i].base2 = 0;
        ldt[i].lim1 = 0x40; /* 32-bit, byte granularity, used.  */
        ldt[i].stype = 0x92 | SEL_PRV; /* Present, data, r/w.  */
        EXIT_OK;
      } else
        EXIT_ERROR;
      }

#ifdef DOSMEM_MULTIPLE
    case 0x0100: /* Allocate Dos Memory Block with multiple desciptors.  */
      {
      word16 i, sel0, max, rmseg, ax, bx;
      int sels, bad;

      rmseg = bad = 0;
      max = (word16)tss_ptr->tss_ebx;

      /* First make sure we have the right number of selectors.  */
      sels = max ? ((max - 1) >> 12) + 1 : 1;
      if ((sel0 = alloc_ldt (sels)) == 0) {
        bad = 8;  /* Insufficient memory.  */
        while (sels > 1 && sel0 == 0)
          sel0 = alloc_ldt (--sels);
        if (sel0)
          max = sels << 12;
        else
          max = 0;
      }

      /* Now check for available dos memory.  */
      _BX = max;
      _AH = 0x48;
      geninterrupt(0x21);
      ax = _AX;
      bx = _BX;
      if(_FLAGS & 1) {
        bad = ax;
        max = bx;
      } else
        rmseg = ax;

      if (bad) {
        (word16)tss_ptr->tss_eax = bad;
        (word16)tss_ptr->tss_ebx = max;
        if (rmseg) {
          _ES = rmseg;
          _AH = 0x49;  /* Release memory.  */
          geninterrupt(0x21);
        }
        if (sel0)
          for (i = 0; i < sels; i++)
            LDT_MARK_FREE (sel0 + i);
        EXIT_ERROR;
      } else {
        (word16)tss_ptr->tss_eax = rmseg;
        (word16)tss_ptr->tss_edx = LDT_SEL(sel0);
        set_limits(rmseg, max, sel0, sels);
        EXIT_OK;
      }
    }
#else

    case 0x0100: /* Allocate Dos Memory Block.  */
      {
      word16 i,ax,bx;
      _BX = (word16)tss_ptr->tss_ebx;	/* number of paragr */
      _AH = 0x48;	/* Allocate memory block */
      geninterrupt(0x21);
      ax = _AX;
      bx = _BX;
      (word16)tss_ptr->tss_eax = ax;
      if(_FLAGS & 1) {
        (word16)tss_ptr->tss_ebx = bx;
        EXIT_ERROR;
      } else {
        i = alloc_ldt(1);
        if(i) {
          (word16)tss_ptr->tss_edx = LDT_SEL(i);
          set_para_limit(i, bx);
          ldt[i].base0 = ax << 4;
          ldt[i].base1 = ax >> 12;
        }
      }
      EXIT_OK;
      }
#endif

    case 0x0101: /* Free Dos Memory Block.  */
      {
      word16 i,ax;
      ASSIGN_LDT_INDEX1(i, tss_edx);
      _ES = (ldt[i].base0 >> 4) + (ldt[i].base1 << 12);
      _AH = 0x49;		/* Release memory */
      geninterrupt(0x21);
      ax = _AX;
      if(_FLAGS & 1) {
        (word16)tss_ptr->tss_eax = ax;
        EXIT_ERROR;
      }
#ifdef DOSMEM_MULTIPLE
      {
        unsigned sels = ldt[i].lim1 & 0xf;
        if (ldt[i].lim0 > 0 || sels == 0)
          sels++;
        while (sels--)
          LDT_MARK_FREE(i++);
      }
#else
      LDT_MARK_FREE(i);
#endif
      EXIT_OK;
      }

#ifdef DOSMEM_MULTIPLE
    case 0x0102: /* Resize Dos Memory Block with multiple descriptors.  */
      {
      word16 i, j, pars, oldpars, ax, bx, carry;
      int oldsels, newsels, maxsels;

      ASSIGN_LDT_INDEX1(i, tss_edx);
      pars = (word16)tss_ptr->tss_ebx;
      oldpars = ((ldt[i].lim1 & 0xf) << 12) + (ldt[i].lim0 >> 4)
        + (ldt[i].lim0 > 0);

      oldsels = oldpars ? ((oldpars - 1) >> 12) + 1 : 1;
      newsels = pars ? ((pars - 1) >> 12) + 1 : 1;

      if (newsels > oldsels) {
        for (j = i + oldsels, maxsels = oldsels;
             maxsels < newsels && LDT_VALID (j) && LDT_FREE (j);
             j++, maxsels++)
          /* Nothing.  */;
        if (maxsels != newsels)
          pars = maxsels << 12;
      } else
        maxsels = newsels;

      _ES = (ldt[i].base0 >> 4) + (ldt[i].base1 << 12);
      _BX = pars;
      _AH = 0x4a;  /* Resize memory.  */
      geninterrupt(0x21);
      ax = _AX;
      bx = _BX;
      carry = _FLAGS & 1;
      if (carry || (newsels != maxsels)) {
        if (carry) {
          (word16)tss_ptr->tss_eax = ax;
          (word16)tss_ptr->tss_ebx = bx;
        } else {
          (word16)tss_ptr->tss_eax = 8;  /* Insufficient memory.  */
          (word16)tss_ptr->tss_ebx = pars;
          _BX = oldpars;
          _AH = 0x4a;  /* Resize memory.  */
          geninterrupt(0x21);
        }
        EXIT_ERROR;
      } else {
        set_limits(_ES, pars, i, newsels);
        for (j = i + newsels; newsels < oldsels; j++, newsels++)
          LDT_MARK_FREE (j);
        EXIT_OK;
      }
      }
#else

    case 0x0102: /* Resize Dos Memory Block.  */
      {
      word16 i,ax,bx;
      ASSIGN_LDT_INDEX1(i, tss_edx);
      _ES = (ldt[i].base0 >> 4) + (ldt[i].base1 << 12);
      _BX = (word16)tss_ptr->tss_ebx;
      _AH = 0x4a;		/* Resize memory */
      geninterrupt(0x21);
      ax = _AX;
      bx = _BX;
      if(!(_FLAGS & 1)) {
        set_para_limit(i, bx);
        EXIT_OK;
      } else {
        (word16)tss_ptr->tss_eax = ax;
        (word16)tss_ptr->tss_ebx = bx;
        EXIT_ERROR;
      }
      }
#endif

    case 0x0200: /* Get Real Mode Interrupt Vector.  */
      {
      word16 i = reg2gate((word8)tss_ptr->tss_ebx);
      (word16)tss_ptr->tss_ecx = peek(0, i*4+2);
      (word16)tss_ptr->tss_edx = peek(0, i*4);
      EXIT_OK;
      }

    case 0x0201: /* Set Real Mode Interrupt Vector.  */
      {
      word16 i = reg2gate((word8)tss_ptr->tss_ebx);
      disable();
      poke(0, i*4+2, (word16)tss_ptr->tss_ecx);
      poke(0, i*4, (word16)tss_ptr->tss_edx);
/*      enable();  Should be disabled already and stay that way */
      EXIT_OK;
      }

    case 0x0202: /* Get Protected Mode Exception Vector.  */
      {
      word16 i = (word8)tss_ptr->tss_ebx;
      if(i < n_user_excep) {
        (word16)tss_ptr->tss_ecx = user_exception_handler[i].selector;
        tss_ptr->tss_edx = user_exception_handler[i].offset32;
        EXIT_OK;
      } else
        EXIT_ERROR;
      }

    case 0x0203: /* Set Protected Mode Exception Vector.  */
      {
      word16 i = (word8)tss_ptr->tss_ebx;
      if(i < n_user_excep) {
        user_exception_handler[i].selector = (word16)tss_ptr->tss_ecx;
        user_exception_handler[i].offset32 = tss_ptr->tss_edx;
        EXIT_OK;
      } else
        EXIT_ERROR;
      }

    case 0x0204: /* Get Protected Mode Interrupt Vector.  */
      {
      word16 i = reg2gate((word8)tss_ptr->tss_ebx);
      if(hwirq != -1 && user_interrupt_handler[hwirq].selector) {
        (word16)tss_ptr->tss_ecx = user_interrupt_handler[hwirq].selector;
        tss_ptr->tss_edx = user_interrupt_handler[hwirq].offset32;
        EXIT_OK;
      }
      if(idt[i].selector == g_rcode*8)
        (word16)tss_ptr->tss_ecx = GDT_SEL(g_pcode);	/* Allow chaining */
      else
        (word16)tss_ptr->tss_ecx = idt[i].selector;
      tss_ptr->tss_edx = idt[i].offset0 | ((word32)idt[i].offset1 << 16);
      EXIT_OK;
      }

    case 0x0205: /* Set Protected Mode Interrupt Vector.  */
      {
      word16 i = reg2gate((word8)tss_ptr->tss_ebx);
/*      if(hwirq != -1)
      errmsg("205: %x (%x, %d) 0x%x 0x%lx\r\n",(word8)(tss_ptr->tss_ebx),i, hwirq,
         (word16)(tss_ptr->tss_ecx),(tss_ptr->tss_edx));   /* */
      /* Must use ring 0 handler for i < 8 (bug) and hwirq */
/*      if(i < 8) {
        errmsg("Warning: setting PM Int %d not supported\n",i);
        EXIT_ERROR;
      } */
      if(hwirq != -1) {
        word16 iseg, ioff, rmcb_num;
        iseg = peek(0, i*4+2);
        ioff = peek(0, i*4);
        rmcb_num = hw_int_rmcb[hwirq];
        if((word16)(tss_ptr->tss_ecx) == GDT_SEL(g_pcode)) { /* Restore */
          if(iseg == _CS && ioff == (word16)dpmisim_rmcb0 + rmcb_num * ((word16)dpmisim_rmcb1 - (word16)dpmisim_rmcb0)) {
            poke(0, i*4, saved_interrupt_vector[hwirq].offset);
            poke(0, i*4+2, saved_interrupt_vector[hwirq].segment);
            dpmisim_rmcb[rmcb_num].cb_sel = 0;
            rmcb_num = num_rmcb;
            saved_interrupt_vector[hwirq].segment = 0;
          }
        } else { /* Reflect RM HW ints to PM */
          if(rmcb_num == num_rmcb)	/* RMCB not yet allocated */
            for (rmcb_num=0; rmcb_num<num_rmcb; rmcb_num++)
              if (dpmisim_rmcb[rmcb_num].cb_sel == 0)
                break;
          if (rmcb_num >= num_rmcb)
            EXIT_ERROR;
          dpmisim_rmcb[rmcb_num].cb_address = tss_ptr->tss_edx;
          dpmisim_rmcb[rmcb_num].cb_sel = (word16)tss_ptr->tss_ecx;
          dpmisim_rmcb[rmcb_num].cb_type = 1;
          poke(0, i*4, (word16)dpmisim_rmcb0 + rmcb_num * ((word16)dpmisim_rmcb1 - (word16)dpmisim_rmcb0) );
          poke(0, i*4+2, _CS);
          if(saved_interrupt_vector[hwirq].segment == 0) {
            saved_interrupt_vector[hwirq].segment = iseg;
            saved_interrupt_vector[hwirq].offset = ioff;
          }
        }
        hw_int_rmcb[hwirq] = rmcb_num;
#if run_ring != 0
        if(hwirq < 16) {	/* Only for PM ring 0 wrappers */
          /* Note IDT selector always rcode, offset1 always 0, type always OK */
          if((word16)tss_ptr->tss_ecx == GDT_SEL(g_pcode)) { /* Restore */
            idt[i].offset0 = (word16)tss_ptr->tss_edx;
            user_interrupt_handler[hwirq].selector = 0;
            EXIT_OK;
          } else {
            extern void irq0(void);
            idt[i].offset0 = (int)FP_OFF(irq0) + 4*hwirq; /*debug comment*/
            user_interrupt_handler[hwirq].selector = (word16)tss_ptr->tss_ecx;
            user_interrupt_handler[hwirq].offset32 = tss_ptr->tss_edx;
            EXIT_OK;
          }
        }
#endif
      } else if(i == 7 || i >= hard_master_lo && i <= hard_master_hi)
        EXIT_OK; /* Must ignore it or DOS/4G apps fail */
      idt[i].selector = (word16)(tss_ptr->tss_ecx);
      idt[i].offset0 = (word16)(tss_ptr->tss_edx);
      idt[i].offset1 = (word16)(tss_ptr->tss_edx >> 16);
      EXIT_OK;
      }

    case 0x0300: /* Simulate Real Mode Interrupt.  */
    case 0x0301: /* Call Real Mode Procedure with Far Return Frame.  */
    case 0x0302: /* Call Real Mode Procedure with Iret Frame.  */
      {
      word16 i;
      word16 dpmisim_spare_stack[STKSIZ];
      word16 far *fptr;

#ifndef I31PROT
      /* The copy of the reg structure to our space is done in ivec31 in
         tables.asm since this saves an extra round trip mode swap. */
      CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi);
      memget(tss_ptr->tss_es, tss_ptr->tss_edi, dpmisim_regs, 50);
#endif

      if (dpmisim_regs[24] == 0) {
        dpmisim_regs[24] = _SS;
        dpmisim_regs[23] = (word16)(dpmisim_spare_stack) + sizeof(dpmisim_spare_stack);
#ifdef STKUSE
        for(i=0;i<sizeof(dpmisim_spare_stack)/2;i++)
          dpmisim_spare_stack[i] = 0xeeee;
#endif
      }

      fptr = MK_FP(dpmisim_regs[24], dpmisim_regs[23]); /* Stack pointer */
      if ((word16)tss_ptr->tss_ecx) {
        fptr -= (word16)tss_ptr->tss_ecx;
        memget(tss_ptr->tss_ss, tss_ptr->tss_esp, fptr, 2 * (word16)tss_ptr->tss_ecx);
      }

      dpmisim_regs[16] &= 0x3ed7;	/* Clear bits we don't allow */
      dpmisim_regs[16] |= 0x3002;	/* Set bits we require */

      if ((word8)tss_ptr->tss_eax != 0x01) {
        fptr--;
        *fptr = dpmisim_regs[16];       /* fake pushing flags on stack */
        dpmisim_regs[16] &= 0xfcff;     /* Clear IF and TF for entry */
      }
      dpmisim_regs[23] = FP_OFF (fptr);	/* Restore stack pointer */

      if ((word8)tss_ptr->tss_eax == 0x00)
      {
        dpmisim_regs[21] = peek(0, (word8)tss_ptr->tss_ebx * 4);
        dpmisim_regs[22] = peek(0, (word8)tss_ptr->tss_ebx * 4 + 2);
      }

      dpmisim();

#ifdef STKUSE
      {
        extern word16 simstacku;
        for(i=0;dpmisim_spare_stack[i] == 0xeeee;i++);
        if(simstacku < (sizeof(dpmisim_spare_stack)/2 - i))
          simstacku = (sizeof(dpmisim_spare_stack)/2 - i);
      }
#endif
#ifndef I31PROT
      /* Only restore 42 bytes; don't modify CS:IP or SS:SP */
      /* The copy of the reg structure from our space is done in ivec31 in
         tables.asm since this saves an extra round trip mode swap. */
      memput(tss_ptr->tss_es, tss_ptr->tss_edi, dpmisim_regs, 42);
#else
      i30x_jump.offset32 = tss_ptr->tss_eip;
      tss_ptr->tss_eip = (word32)FP_OFF(ivec31x);
      i30x_jump.selector = tss_ptr->tss_cs;
      tss_ptr->tss_cs = GDT_SEL(g_pcode);
      i30x_stack.offset32 = tss_ptr->tss_esp;
      tss_ptr->tss_esp = (word32)FP_OFF(&locked_stack[16]);	/* temp */
      i30x_stack.selector = tss_ptr->tss_ss;
      tss_ptr->tss_ss = GDT_SEL(g_pdata);
      if((word16)tss_ptr->tss_eflags & 0x200) {	/* Interrupt enabled? */
        (word16)tss_ptr->tss_eflags &= ~0x200;	/* Clear interrupt */
        i30x_sti = 0xfb;	/* STI */
      } else
        i30x_sti = 0x90;	/* NOP */
#endif      
      EXIT_OK;
      }

    case 0x0303: /* Allocate Real Mode Call-Back Address */
      {
      word16 i;
      CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi);
      CHECK_POINTER(tss_ptr->tss_ds, tss_ptr->tss_esi);
      for (i=0; i<num_rmcb; i++)
        if (dpmisim_rmcb[i].cb_sel == 0)
          break;
      if (i == num_rmcb)
        EXIT_ERROR;
      dpmisim_rmcb[i].cb_address = tss_ptr->tss_esi;
      dpmisim_rmcb[i].cb_sel = tss_ptr->tss_ds;
      dpmisim_rmcb[i].cb_type = 0;
      dpmisim_rmcb[i].reg_ptr = tss_ptr->tss_edi;
      dpmisim_rmcb[i].reg_sel = tss_ptr->tss_es;
      (word16)tss_ptr->tss_ecx = _CS;
      (word16)tss_ptr->tss_edx = (word16)dpmisim_rmcb0 + i * ((word16)dpmisim_rmcb1 - (word16)dpmisim_rmcb0);
      EXIT_OK;
      }

    case 0x0304: /* Free Real Mode Call-Back Address */
      {
      word16 i;
      if ((word16)tss_ptr->tss_ecx == _CS)
        for (i=0; i<num_rmcb; i++)
          if ((word16)tss_ptr->tss_edx == (word16)dpmisim_rmcb0 + i * ((word16)dpmisim_rmcb1 - (word16)dpmisim_rmcb0))
          {
            dpmisim_rmcb[i].cb_sel = 0;
            EXIT_OK;
          }
      EXIT_ERROR;
      }

    case 0x0305: /* Get State Save/Restore Adresses.  */
      (word16)tss_ptr->tss_eax = 0;
      (word16)tss_ptr->tss_ebx = _CS;
      (word16)tss_ptr->tss_ecx = (word16)savestate_real;
      (word16)tss_ptr->tss_esi = GDT_SEL(g_pcode);
      tss_ptr->tss_edi = (word16)savestate_prot;
      EXIT_OK;

    case 0x0306: /* Get Raw Mode Switch Addresses.  */
      (word16)tss_ptr->tss_ebx = _CS;
      (word16)tss_ptr->tss_ecx = (word16)do_raw_switch_ret;
      (word16)tss_ptr->tss_esi = g_ctss*8;
      tss_ptr->tss_edi = 0; /* doesn't matter, above is a task */
      EXIT_OK;

    case 0x0400: /* Get Version.  */
      (word16)tss_ptr->tss_eax = 0x005a;
      (word16)tss_ptr->tss_ebx = dalloc_max_size() ? 5 : 1;	/* Lie, always V86 mode (bit 1) */
      (word8)tss_ptr->tss_ecx = cpu_family;
      (word16)tss_ptr->tss_edx = (old_master_lo << 8) | hard_slave_lo;
      EXIT_OK;

#if 1
    case 0x0401: /* 1.0 Get DPMI Capabilities.  */
      (word16)tss_ptr->tss_eax = 0x2d; /* No restart, zero-fill, host writeprot */
      (word16)tss_ptr->tss_ebx = 0;
      (word16)tss_ptr->tss_ecx = 0;
      memput(tss_ptr->tss_es, tss_ptr->tss_edi, "\7\0CWSDPMI", 10);
      EXIT_OK;
#endif

    case 0x0500: /* Get Free Memory Information.  */
      {
      word32 tmp_buffer[12];
      CHECK_POINTER(tss_ptr->tss_es, tss_ptr->tss_edi);
      memset(tmp_buffer, 0xff, sizeof(tmp_buffer));
      tmp_buffer[4] = tmp_buffer[6] = valloc_max_size();
      tmp_buffer[2] = tmp_buffer[5] = valloc_max_size() - valloc_used();
      tmp_buffer[8] = dalloc_max_size();
      tmp_buffer[1] = tmp_buffer[8] - reserved + valloc_max_size();
      tmp_buffer[0] = tmp_buffer[1] << 12;
      memput(tss_ptr->tss_es, tss_ptr->tss_edi, tmp_buffer, sizeof(tmp_buffer));
      EXIT_OK;
      }

    case 0x0501: /* Allocate Memory Block.  */
      {
      word32 rsize,newbase;
      AREAS *area = firstarea;
      AREAS **lasta = &firstarea;
      newbase = VADDR_START - 1;
      while(area) {
        newbase = area->last_addr;
        lasta = &area->next;
        area = area->next;
      }
      if(!(area = (AREAS *)malloc(sizeof(AREAS))))
        EXIT_ERROR;
#ifdef OLDWAY
      newbase |= 0x0fffffff;	/* Leave room for resize */
#endif
      rsize = (((word16)tss_ptr->tss_ecx + 0xfffL) & ~0xfffL) + (tss_ptr->tss_ebx << 16);
      area->first_addr = newbase + 1;
      area->last_addr = area->first_addr + rsize - 1;
      area->next = NULL;
      if((area->last_addr < area->first_addr) || cant_ask_for(rsize)) {
        free(area);
        EXIT_ERROR;
      }
      *lasta = area;
#if run_ring == 0
      lock_memory(area->first_addr, rsize, 0);
#endif
      (word16)tss_ptr->tss_edi =
      (word16)tss_ptr->tss_ecx = (word16)area->first_addr;
      (word16)tss_ptr->tss_esi =
      (word16)tss_ptr->tss_ebx = area->first_addr >> 16;
      EXIT_OK;
      }

    case 0x0502: /* Free Memory Block.  */
      if(free_memory_area((word16)tss_ptr->tss_edi + (tss_ptr->tss_esi << 16)))
        EXIT_OK;
      else
        EXIT_ERROR;

    case 0x0503: /* Resize Memory Block.  */
      {
      AREAS *area = firstarea;
      AREAS **lasta = &firstarea;
      word32 vaddr = (word16)tss_ptr->tss_edi + (tss_ptr->tss_esi << 16);
      while(area) {
        if(vaddr == area->first_addr) {
          word32 rsize,asize;
          rsize = (((word16)tss_ptr->tss_ecx + 0xfffL) & ~0xfffL) + (tss_ptr->tss_ebx << 16);
          asize = rsize-(area->last_addr-area->first_addr+1);
          if(cant_ask_for(asize))
            EXIT_ERROR;
          area->last_addr += asize;
          if(area->next && area->last_addr >= (area->next)->first_addr) {
            /* Resize needs to move the zone - groan */
            word32 newbase;
            *lasta = area->next;	/* Remove from chain */
            while((*lasta)->next)
              lasta = &((*lasta)->next);
            (*lasta)->next = area;
            newbase = (*lasta)->last_addr;
            move_pt(area->first_addr,area->last_addr-asize,++newbase);
            area->first_addr = newbase;
            area->last_addr = area->first_addr + rsize - 1;
            area->next = NULL;
            (word16)tss_ptr->tss_edi = (word16)area->first_addr;
            (word16)tss_ptr->tss_esi = area->first_addr >> 16;
          }
#if run_ring == 0
          lock_memory(area->last_addr+1-asize, asize, 0);
#endif
          (word16)tss_ptr->tss_ecx = (word16)tss_ptr->tss_edi;
          (word16)tss_ptr->tss_ebx = (word16)tss_ptr->tss_esi;
          EXIT_OK;
        }
        lasta = &area->next;
        area = area->next;
      }
      EXIT_ERROR;
      }

    case 0x0506: /* V1.0 Get Page Attributes.  */
    case 0x0507: /* V1.0 Set Page Attributes.  */
      {
      AREAS *area = firstarea;
      word32 start = tss_ptr->tss_esi;
      if(CWSFLAG_NOEXT) EXIT_ERROR;
      while(area) {
        if(start == area->first_addr) {
          start += tss_ptr->tss_ebx;
          (word16)start &= ~0xfff;			/* Page align */
          page_attributes((word8)tss_ptr->tss_eax == 7, start,
            (word16)tss_ptr->tss_ecx);	/* ES:EDX known by routine */
          EXIT_OK;
        }
        area = area->next;
      }
      (word16)tss_ptr->tss_eax = 0x8023;	/* Bad handle */
      EXIT_ERROR;
      }

    case 0x0508: /* V1.0 Map Device in Memory Block.  */
    case 0x0509: /* V1.0 Map Conventional Memory in Memory Block.  */
      {
      AREAS *area = firstarea;
      word32 start = tss_ptr->tss_esi;
      if(CWSFLAG_NOEXT) EXIT_ERROR;
      while(area) {
        if(start == area->first_addr) {
          word32 size = tss_ptr->tss_ecx << 12;
          start += tss_ptr->tss_ebx;
          if(((word16)start & 0xfff) ||
             ((word16)tss_ptr->tss_edx & 0xfff) ||
             (start + size - 1 > area->last_addr)) {
              (word16)tss_ptr->tss_eax = 0x8025;	/* Invalid address */
              EXIT_ERROR;
           }
          physical_map(tss_ptr->tss_edx,size,start);	/* maps to "start" */
          EXIT_OK;
        }
        area = area->next;
      }
      (word16)tss_ptr->tss_eax = 0x8023;	/* Bad handle */
      EXIT_ERROR;
      }

    case 0x0600: /* Lock Linear Region.  */
    case 0x0601: /* Unlock Linear Region.  */
#if run_ring != 0
    {
      word32 vaddr, size;
      vaddr = (word16)tss_ptr->tss_ecx + (tss_ptr->tss_ebx << 16);
      size = (word16)tss_ptr->tss_edi + (tss_ptr->tss_esi << 16);
#if 0
      /* Should we allow locking DOS memory?  The 0.9 spec doesn't say much
         other than it's not needed.  The 1.0 spec says its OK.  DJGPP 
         programmers sometimes miss the selector base so failure helps them
         catch errors.  */
      if (vaddr < 0x100000L)
        EXIT_OK;
#endif
      if (vaddr < VADDR_START)
        EXIT_ERROR;
      /* bugs here - our lock/unlock does not keep lock count */
      if(lock_memory(vaddr,size,(word8)tss_ptr->tss_eax))
        EXIT_ERROR;
      EXIT_OK;
    }
#endif

    case 0x0602: /* Mark Real Mode Region as Pageable.  */
    case 0x0603: /* Relock Real Mode Region.  */
      EXIT_OK;		/* We don't page real mode region so return success */

    case 0x0604: /* Get Page Size.  */
      (word16)tss_ptr->tss_ebx = 0;
      (word16)tss_ptr->tss_ecx = 4096;
      EXIT_OK;

    case 0x0702: /* Mark Page as Demand Paging Candidate.  */
      EXIT_OK;		/* We choose to ignore hints and return success */

    case 0x0703: /* Discard Page Contents.  */
    {
      word32 firsta, lasta;
      firsta = (word16)tss_ptr->tss_ecx + (tss_ptr->tss_ebx << 16);
      lasta = firsta + (word16)tss_ptr->tss_edi + (tss_ptr->tss_esi << 16);
      firsta += 0xfffL;
      (word16)firsta &= ~0xfff;		/* Round up - partial pages not discared */
      if (firsta < VADDR_START)
        EXIT_ERROR;
      (word16)lasta &= ~0xfff;		/* Round down */
      free_memory(firsta,lasta);
      EXIT_OK;
    }

    case 0x0800: /* Physical Address Mapping.  */
    {
      word32 phys, vaddr, size;
      size = (word16)tss_ptr->tss_edi + (tss_ptr->tss_esi << 16);
      vaddr = phys = (word16)tss_ptr->tss_ecx + (tss_ptr->tss_ebx << 16);
      if((word16)tss_ptr->tss_ebx < 0x10) EXIT_ERROR;	/* Don't map 1M area */
      if((word8)((word16)tss_ptr->tss_ebx >> 12) <= 1) {	/* Conflict with our default mapping */
        vaddr += 0xe0000000;
        (word16)tss_ptr->tss_ebx += 0xe000;
      }
      physical_map(phys,size,vaddr);  /* Does 1:1 mapping, we return BX:CX */
      EXIT_OK;
    }

    case 0x0900: /* Get and Disable Virtual Interrupt State.  */
    case 0x0901: /* Get and Enable Virtual Interrupt State.  */
    case 0x0902: /* Get Virtual Interrupt State.  */
      if((word16)tss_ptr->tss_eflags & 0x200) {
        if((word8)tss_ptr->tss_eax == 0)
          (word16)tss_ptr->tss_eflags &= ~0x200;
        (word8)tss_ptr->tss_eax = 1;
      } else {
        if((word8)tss_ptr->tss_eax == 1)
          (word16)tss_ptr->tss_eflags |= 0x200;
        (word8)tss_ptr->tss_eax = 0;
      }
      EXIT_OK;

#if 0
    case 0x0A00: /* Get Vendor Specific API Entry Point.  */
#endif

    case 0x0B00: /* Set Debug Watchpoint.  */
      {
      int enabled = (int)dr[7];
      word16 i;
      for(i=0;i<4;i++)
        if(! (enabled >> (i*2))&3 ) {
          unsigned mask;
          dr[i] = (tss_ptr->tss_ebx << 16) | (word16)tss_ptr->tss_ecx;
          (word16)dr[7] |= (3 << (i*2));
          mask = ((word16)tss_ptr->tss_edx >> 8) & 3;   /* dh = type */
          if(mask == 2) mask++;         /* Change dpmi 0,1,2 to 0,1,3 */
          mask |= (((word16)tss_ptr->tss_edx-1) << 2) & 0x0c; /* dl = size */
          dr[7] |= (word32)mask << (i*4 + 16);
          (word16)tss_ptr->tss_ebx = i;
          EXIT_OK;
        }
      EXIT_ERROR;		/* None available */
      }
    case 0x0B01: /* Clear Debug Watchpoint.  */
      {
      word16 i = (word16)tss_ptr->tss_ebx & 3;
      (word16)dr[6] &= ~(1 << i);
      (word16)dr[7] &= ~(3 << (i*2));
      dr[7] &= ~(15L << (i*4+16));
      EXIT_OK;
      }
    case 0x0B02: /* Get State of Debug Watchpoint.  */
      {
      word16 i = (word16)tss_ptr->tss_ebx & 3;
      (word16)tss_ptr->tss_eax = (word16)dr[6] >> i;
      EXIT_OK;
      }
    case 0x0B03: /* Reset Debug Watchpoint.  */
      {
      word16 i = (word16)tss_ptr->tss_ebx & 3;
      (word16)dr[6] &= ~(1 << i);
      EXIT_OK;
      }

#if 0
    case 0x0E00: /* DPMI 1.0: Get Coprocessor Status.  */
#endif
    case 0x0E01: /* DPMI 1.0: Set Coprocessor Emulation.  */
      /* Ignore MP bit, since clearing it can cause problems.  We don't
         actually set the EM bit here, since a task swap will set TS and
         cause the same effect (by not using our custom TS clear handler). */
      if((word8)tss_ptr->tss_ebx & 2) {		/* Client supplies emulation */
        idt[7].offset0 = (int)FP_OFF(ivec0) + 7*((int)ivec1-(int)ivec0);
      } else {
        idt[7].offset0 = (int)ivec7;		/* To handle TS bit set faults */
      }
      EXIT_OK;

#ifdef DIAGNOSE
    default: /* Mark As Unsupported.  */
      errmsg("DPMI request 0x%x unsupported\n",(word16)(tss_ptr->tss_eax));
/*      EXIT_ERROR; */
#endif
  }

i31_exit_error:
  SET_CARRY;
i31_exit:
  return 0;
}
