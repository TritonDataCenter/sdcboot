/* Copyright (C) 1995-1997 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
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

#include <dos.h>
#include "gotypes.h"
#include "tss.h"
#include "gdt.h"
#include "utils.h"
#include "mswitch.h"
#include "exphdlr.h"
#include "control.h"
#include "dpmisim.h"

TSS *utils_tss = &o_tss;

void go_til_stop(int allow_ret)
{
  while (1)
  {
    go32();
    if (!was_exception){
      if(allow_ret && !(word16)tss_ptr->tss_ebx)	/* BX is SP for RAW */
        return;
      do_raw_switch();				/* jmpt ctss with BX non-zero */
    } else if (exception_handler())
      do_faulting_finish_message();
  }
}

void memput(word16 umemsel, word32 vaddr, void far *ptr, word16 len)
{
  TSS *old_ptr;
  utils_tss->tss_eip = (word16)_do_memmov32;
  utils_tss->tss_ecx = len;
  utils_tss->tss_es = umemsel;
  utils_tss->tss_edi = vaddr;
  utils_tss->tss_ds = g_core*8;
  utils_tss->tss_esi = (word32)FP_SEG(ptr)*16 + (word32)FP_OFF(ptr);
  old_ptr = tss_ptr;
  tss_ptr = utils_tss;
  go_til_stop(1);
  tss_ptr = old_ptr;
}

void memget(word16 umemsel, word32 vaddr, void far *ptr, word16 len)
{
  TSS *old_ptr;
  utils_tss->tss_eip = (word16)_do_memmov32;
  utils_tss->tss_ecx = len;
  utils_tss->tss_es = g_core*8;
  utils_tss->tss_edi = (word32)FP_SEG(ptr)*16 + (word32)FP_OFF(ptr);
  utils_tss->tss_ds = umemsel;
  utils_tss->tss_esi = vaddr;
  old_ptr = tss_ptr;
  tss_ptr = utils_tss;
  go_til_stop(1);
  tss_ptr = old_ptr;
}
