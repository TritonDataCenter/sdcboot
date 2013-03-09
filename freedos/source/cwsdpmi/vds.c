/* Copyright (C) 1997 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
** VDS support contributed by Chris Matrakidis October 1997
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

#include "gotypes.h"
#include "tss.h"
#include "gdt.h"
#include "xms.h"
#include "utils.h"
#include "valloc.h"
#include "paging.h"

static word32 vds_buf;
static word16 vds_size;
static int vds_used;
static word16 vds_count[8]; /* 8 DMA channels?  Windows and emm386 say so.*/
word8 vds_flag;

void memcpy32(word16 tsel, word32 taddr, word16 ssel, word32 saddr, word16 len)
{
  TSS *old_ptr;
  utils_tss->tss_eip = (word16)_do_memmov32;
  utils_tss->tss_ecx = len;
  utils_tss->tss_es = tsel;
  utils_tss->tss_edi = taddr;
  utils_tss->tss_ds = ssel;
  utils_tss->tss_esi = saddr;
  old_ptr = tss_ptr;
  tss_ptr = utils_tss;
  go_til_stop(1);
  tss_ptr = old_ptr;
}

void sort(va_pn *x, int n)
{
  int ndelta,i;
  char inorder;
  va_pn t;
  
  ndelta = n;
  while(ndelta > 1) {
    ndelta /= 2;
    do {
      inorder = 1;
      for(i=0;i<n-ndelta;i++)
        if(x[i] > x[i+ndelta]) {
          t = x[i];
          x[i] = x[i+ndelta];
          x[i+ndelta] = t;
          inorder = 0;
        }
    } while (!inorder);
  }
}

#define TRYMAX 16	/* About 2X times size wanted */
int init_vds(void)	/* allocate a contig buffer not crossing 64K boundary */
{
  va_pn pn[TRYMAX];
  int i,start,startt,size,sizet;

  for(i=0;i<TRYMAX;i++)
    pn[i] = valloc();

  sort(pn,TRYMAX);
  
  startt = 0;
  sizet = 1;
  size = 0;

  for(i=1;i<TRYMAX;i++) {
    if(pn[i-1]+1 == pn[i] && (pn[i]&15) ) {
      sizet++;
    } else {
      if(sizet > size) {
        size = sizet;
        start = startt;
      }
      sizet = 1;
      startt = i;
    }
  }

  if(sizet > size) {
    size = sizet;
    start = startt;
  }

  /* Want size either 4 (16K) or 8 (32K), could do larger ... */
  if(size >=8)
    size = 8;
  else if(size >=4)
    size = 4;
  else
    size = 0;

  for(i=0;i<TRYMAX;i++)
    if(i < start || i >= start+size)
      vfree(pn[i]);

  if(size == 0)
    return 1;

  vds_size = size << 12;
  vds_buf = (word32)pn[start] << 12;
  if(vds_buf > 0x100000)
    physical_map(vds_buf,size,vds_buf);		/* Maps and locks the memory */

  vds_flag=0x30;
  vds_used=0;
  for(i=0; i<8; i++)
    vds_count[i]=0;
  return 0;
}

#define SET_CARRY (word8)tss_ptr->tss_eflags |= 1

typedef struct {
  word32 size;
  word32 offset;
  word16 seg;
  word16 buf;
  word32 addr;
} vds_dds;


static void vds_cp_in_buf(vds_dds* dds, word32 offset)
{
  word16 sel;
  if(dds->buf != 1 || !vds_used)
  {
    (word8)tss_ptr->tss_eax=0xa;              /* invalid buffer */
    SET_CARRY;
  }
  else if(offset + dds->size > vds_size)
  {
    (word8)tss_ptr->tss_eax=0xb;              /* boundary violation */
    SET_CARRY;
  }
  else
  {
    if(dds->seg)
      sel=dds->seg;
    else
      sel=g_core*8;
    memcpy32(g_core*8, vds_buf+offset, sel,dds->offset, dds->size);
  }
}

static int vds_req_buf(vds_dds* dds)
{
  if(vds_used)
  {
    (word8)tss_ptr->tss_eax=0x6;              /* bufer in use */
    SET_CARRY;
    return 0;
  }
  if(dds->size > vds_size)
  {
    (word8)tss_ptr->tss_eax=0x5;              /* region too large */
    SET_CARRY;
    return 0;
  }
  vds_used=1;
  dds->buf=1;
  dds->addr=vds_buf;
  if((word16)tss_ptr->tss_edx & 2)
  {
   vds_cp_in_buf(dds,0);
  }
  return 1;
}

static void vds_cp_out_buf(vds_dds* dds, word32 offset)
{
  word16 sel;
  if(dds->buf != 1 || !vds_used)
  {
    (word8)tss_ptr->tss_eax=0xa;              /* invalid buffer */
    SET_CARRY;
  }
  else if(offset + dds->size > vds_size)
  {
    (word8)tss_ptr->tss_eax=0xb;              /* boundary violation */
    SET_CARRY;
  }
  else
  {
    if(dds->seg)
      sel=dds->seg;
    else
      sel=g_core*8;
    memcpy32(sel,dds->offset, g_core*8, vds_buf+offset, dds->size);
  }
}

static void vds_free_buf(vds_dds* dds)
{
  if(dds->buf != 1 || !vds_used)
  {
    (word8)tss_ptr->tss_eax=0xa;              /* invalid buffer */
    SET_CARRY;
  }
  if((word16)tss_ptr->tss_edx & 2)
  {
   vds_cp_out_buf(dds,0);
  }
  vds_used=0;
}


static int vds_test_flags(word16 flags)
{
  if((word16)tss_ptr->tss_edx & ~flags)
  {
    (word8)tss_ptr->tss_eax=0x10;             /* invalid flags */
    SET_CARRY;
    return 0;
  }
  return 1;
}

int i_4b(void)
{
  vds_dds dds;
  word8 ah;

  ah = (word8)((word16)(tss_ptr->tss_eax) >> 8);

  if(ah != 0x81)
    return generic_handler();

  if(!vds_flag)				/* VDS is disabled, we ignore call */
    if(init_vds())
      return 0;

  (word8)tss_ptr->tss_eflags &= ~1;	/* Clear carry, only set on failure */

  switch ((word8)tss_ptr->tss_eax)
  {
    case 0x2: /* Get Version */
      if(vds_test_flags(0))
      {
        (word16)tss_ptr->tss_eax=0x100;			/* spec version */ 
        (word16)tss_ptr->tss_ebx=0;			/* product number */
        (word16)tss_ptr->tss_ecx=0;			/* revison number */
        (word16)tss_ptr->tss_esi=0;			/* buffer size high */
        (word16)tss_ptr->tss_edi=vds_size;		/* buffer size low */
      }
      break;

    case 0x3: /* Lock DMA Region */
      if(vds_test_flags(0x3e))
      {
        if((word16)tss_ptr->tss_edx & 0x4)
        {
          (word8)tss_ptr->tss_eax=0x03;               /* unable to lock */
          SET_CARRY;
        }
        else
        {
          memget(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
          if(vds_req_buf(&dds))
          {
            memput(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
          }
        }
      }
      break;

    case 0x4: /* Unlock DMA Region */
      if(vds_test_flags(2))
      {
        memget(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
        if(dds.buf)
        {
          vds_free_buf(&dds);
        }
        else
        {
          (word8)tss_ptr->tss_eax=0x08;               /* wasn't locked */
          SET_CARRY;
        }
      }
      break;

    case 0x5: /* Scatter/Gather Lock Region */
      if(!vds_test_flags(0xc0))
      {
        (word8)tss_ptr->tss_eax=0x03;         /* unable to lock */
        SET_CARRY;
      }
      memget(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
      dds.size=0;                             /* can't lock any memory */
      memput(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
      break;

    case 0x6: /* Scatter/Gather Unlock Region */
      if(!vds_test_flags(0xc0))
      {
        (word8)tss_ptr->tss_eax=0x08;         /* wasn't locked */
        SET_CARRY;
      }
      break;

    case 0x7: /* Request DMA Buffer */
      if(vds_test_flags(2))
      {
        memget(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
        if(vds_req_buf(&dds))
        {
          dds.size=vds_size;
          memput(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
        }
      }
      break;
 
    case 0x8: /* Release DMA Buffer */
      if(vds_test_flags(2))
      {
        memget(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
        vds_free_buf(&dds);
      }
      break;

    case 0x9: /* Copy Into DMA Buffer */
      if(vds_test_flags(0))
      {
        memget(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
        vds_cp_in_buf(&dds, tss_ptr->tss_ebx<<16 | (word16)tss_ptr->tss_ecx);
      }
      break;

    case 0xa: /* Copy Out Of DMA Buffer */
      if(vds_test_flags(0))
      {
        memget(tss_ptr->tss_es,tss_ptr->tss_edi,&dds,sizeof(dds));
        vds_cp_out_buf(&dds, tss_ptr->tss_ebx<<16 | (word16)tss_ptr->tss_ecx);
      }
      break;

    case 0xb: /* Disable DMA Translation */
      if(vds_test_flags(0))
      {
        if((word16)tss_ptr->tss_ebx > 7)
        {
          (word8)tss_ptr->tss_eax=0xc;                /* invalid dma channel */
          SET_CARRY;
        }
        else
        {
          vds_count[(word16)tss_ptr->tss_ebx]++;
          if(!vds_count[(word16)tss_ptr->tss_ebx])
          {
            (word8)tss_ptr->tss_eax=0xd;              /* count overflow */
            SET_CARRY;
          }
        }
      }
      break;

    case 0xc: /* Enable DMA Translation */
      if(vds_test_flags(0))
      {
        if((word16)tss_ptr->tss_ebx > 7)
        {
          (word8)tss_ptr->tss_eax=0xc;                /* invalid dma channel */
          SET_CARRY;
        }
        else 
        {
          if(!vds_count[(word16)tss_ptr->tss_ebx])
          {
            (word8)tss_ptr->tss_eax=0xe;              /* count underflow */
            SET_CARRY;
          }
          else
          {
            vds_count[(word16)tss_ptr->tss_ebx]--;
            if(!vds_count[(word16)tss_ptr->tss_ebx])
            {
              (word8)tss_ptr->tss_eflags |= 0x40;     /* set zero flag */
            }
          }
        }
      }

      break;

   default:
      (word8)tss_ptr->tss_eax=0x0f;
      SET_CARRY;
  }
  return 0;
}

/* This code was originally in valloc.  It doesn't compile here without
   exporting some variables/routines from there.  But I want to use higher
   level routines instead *

static int16 vds_emb_handle = -1;
static va_pn vds_pn = 0;

word32 alloc_vds_buffer(int size)
{
  int c;
  word32 vds_buf = 0;

  if (use_vcpi)               
  {
    vds_emb_handle = xms_emb_allocate(size*4);
  }

  if (vds_emb_handle == -1)
  {
    for (vds_pn = pn_hi_next; vds_pn<pn_hi_last-size; vds_pn++)
    { 
      for (c=0; c<size; c++)
        if(vtest(vds_pn+c))
        {
          vds_pn += c;
          continue;
        }

      for (c=0; c<size; c++)
         vset(vds_pn+c, 1);

      vds_buf = (word32)vds_pn*4096;
      mem_used += size;
    }
  }
  else
  {
    vds_buf = xms_lock_emb(vds_emb_handle);
  }

  return vds_buf;
}


void free_vds_buffer(int size)
{
  int c;

  if (vds_emb_handle == -1)
  {
    if (vds_pn)
    {
      for (c=0; c<size; c++)
      {
        vset(vds_pn+c, 0);
      }
      if(vds_pn < pn_hi_next)
        pn_hi_next = vds_pn;
      mem_used -= size;
    }
  }
  else
  {
    xms_unlock_emb(vds_emb_handle);
    xms_emb_free(vds_emb_handle);
  }
  vds_emb_handle = -1;
  vds_pn=0;
}

void init_vds(void)
{
  int c;

  vds_buf= alloc_vds_buffer(8);
  if(vds_buf)
  {
    vds_size=0x8000; 
    if((vds_buf & 0xffff)>0x8000)
    {
      if((vds_buf & 0xffff)>0xc000)
      {
        vds_buf+=0x4000;
      }
      vds_size=0x4000;
    }
    vds_flag=0x30;
    vds_used=0;
    for(c=0; c<8; c++)
      vds_count[c]=0;
  }
}

void uninit_vds(void)
{
  free_vds_buffer(8);
  vds_flag=0;
}

*/
