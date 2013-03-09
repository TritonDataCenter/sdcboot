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

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <io.h>
#include <string.h>

#include "gotypes.h"
#include "dalloc.h"
#include "control.h"

#define DA_FREE	0
#define DA_USED	1

word8 far *dmap;
#if run_ring != 0
static da_pn first_avail;
static int dfile = -1;
static da_pn disk_used,disk_max;
#define max_dblock (da_pn)CWSpar.maxdblock
static word16 paging_pid;

static void dset(da_pn i, int b)
{
  unsigned o;
  o = (unsigned)(i>>3);
  _AL = 1<<((unsigned)i&7);
  if (b)
    dmap[o] |= _AL;
  else
    dmap[o] &= ~_AL;
}

static word8 dtest(da_pn i)
{
  return dmap[(unsigned)(i>>3)] & (1<<((unsigned)i&7));
}

static char* dfilename;

void dalloc_file(char *swapname)
{
  dfilename = swapname;
}

void dalloc_init(void)
{
  disk_used = disk_max = 0;
  first_avail = MAX_DBLOCK;		/* If no paging */
  if(!dfilename || !dfilename[0]) {
    SHOW_MEM_INFO("  No Paging.\n", 0);
  } else if (0 > (dfile = _creat(dfilename, 0))) {
    errmsg("Warning: cannot open swap file %s\n", dfilename);
  } else {
    paging_pid = get_pid();
    first_avail = 0;
  }
  max_dblock = dalloc_max_size();
  /* Actual allocation and zeroing of dmap happens elsewhere (valloc) */
  SHOW_MEM_INFO("  Swap space: %ld Kb\n", (max_dblock * 4L));
}

void dalloc_uninit(void)
{
  word16 save_pid = get_pid();
  if (dfile < 0)
    return;
  set_pid(paging_pid);
  _close(dfile);
  set_pid(save_pid);
  dfile = -1;
  unlink(dfilename);
}

da_pn dalloc(void)
{
  da_pn pn;
  for (pn=first_avail; pn<max_dblock; pn++)
    if (dtest(pn) == DA_FREE) {
      dset(pn, DA_USED);
      first_avail = pn+1;
      if(first_avail > disk_max)
        disk_max = first_avail;		/* pn+1 since zero based */
      disk_used++;
      return pn;
    }
  errmsg("No swap space!\n");
  cleanup(1);
  return 0;
}

void dfree(da_pn pn)
{
  dset(pn, DA_FREE);
  if (pn < first_avail)
    first_avail = pn;
  disk_used--;
}

da_pn dalloc_max_size(void)
{
  word32 fr;
  word16 ax,bx,cx;
  if (dfile < 0)
    return 0;
  _DL = dfilename[0] & 0x1f;
  _AH = 0x36;
  geninterrupt(0x21);
  ax = _AX;		/* Sectors per cluster */
  bx = _BX;		/* Number of available clusters */
  cx = _CX;		/* bytes per sector */
  if (ax == 0xffff)
    return 0;
  fr = (word32)(ax * cx) * (word32)bx;
  fr >>= 12;
  fr += (unsigned long)disk_max;
  if (fr > max_dblock)
    fr = max_dblock;
  return (da_pn)fr;
}

/* da_pn dalloc_used(void)
{
  return (da_pn)disk_used;
} */

void dwrite(word8 *buf, da_pn block)
{
  int c;
  word16 save_pid = get_pid();
  set_pid(paging_pid);
  lseek(dfile, (long)block << 12, 0);
  c = _write(dfile, buf, 4096);
  set_pid(save_pid);
  if (c < 4096) {
    errmsg("Swap disk full!\n");
    cleanup(1);
  }
}

void dread(word8 *buf, da_pn block)
{
  word16 save_pid = get_pid();
  set_pid(paging_pid);
  lseek(dfile, (long)block << 12, 0);
  _read(dfile, buf, 4096);
  set_pid(save_pid);
}
#endif
