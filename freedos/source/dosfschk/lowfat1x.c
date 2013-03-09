#ifdef __DJGPP__ /* only for DOS */

#include "common.h"
#include "lowfat1x.h"
#include <stdio.h>	/* printf */
#include <string.h>	/* memcpy */
#include <sys/movedata.h>	/* _movedatal, _movedataw */
#include <sys/segments.h>	/* _my_ds */
#include <dpmi.h>

	/*  include file for raw disk I/O for __DJGPP__ DOS  */
	/*    by Eric Auer, inspired by disklib - 11/2002    */
	/* * DOES NOT WRITE YET * FAT32 read added 12/2003 * */
	/* changed to be useable with Imre's volume.c 8/2004 */


/* movedatal(srcSel,srcOfs, dstSel,dstOfs, count): count * dd move */
/* movedatab(...,count): count bytes ... movedataw(...) similar... */

static char xferbuf[512];    /* *** 512 byte per sector, fixed *** */
static __u32 xfersec = 0xffffffffL; /* secnum currently in xferbuf */
static int sel_fat1x = 0;	/* transfer buffer only allocated once */
static int seg_fat1x = 0;	/* transfer buffer only allocated once */

int raw_read_old(int drive, loff_t pos, size_t size, void * data)
{
    __u32 secnum;
    __dpmi_regs regs;
    int i, retVal;

    if ( ((pos >> 9) > 65535) || (size != 512) || (pos & 511))
	return -1;	/* cannot handle with this simplified version */
    retVal = size;
   
    /* dosSeg = __dpmi_allocate_dos_memory(paragraphs, selector); */
    if (!seg_fat1x) {
        seg_fat1x = __dpmi_allocate_dos_memory((512+10+15)>>4, &sel_fat1x);
        if (seg_fat1x == -1)
	    pdie("RAW_READ out of transfer/DCB buffer memory");
        /* one sector plus one data structure (which is used by ..._new) ... */
    }

    secnum = pos >> 9;
    if (secnum != xfersec)	/* not already in buffer? */
    {
	regs.x.ax = drive;           /* 0 = A: */
	regs.x.cx = 1;           /* one sector */
	regs.x.dx = secnum & 0xffff; /* sector */
	regs.x.ds = seg_fat1x;   /* buffer seg */
	regs.x.bx = 0;           /* buffer ofs */
          
	__dpmi_int(0x25, &regs); /* DOSdisk access */
	/* luckily, DPMI knows that this is a farcall! */

	if (regs.x.flags & 1 /* CY */) {
	    retVal = regs.x.ax;
	    printf("DOS 'int' 0x25 (old) sector %u read error: %x\n",
		secnum, retVal);
            retVal = -1; /* must signal error like that! */
	    /* flush buffer (or fill with 0xdeadbeef): */
	    for (i = 0; i < 512; i ++) xferbuf[i] = 0;
	} else {
	    _movedatal(sel_fat1x, 0, _my_ds(), (unsigned int)&xferbuf[0], 512>>2);
	    /* copy from DOS */
	}
	xfersec = secnum;

    } else {
#ifdef _IORDEBUG
	/* otherwise, data is still in cache */
	printf(".");
#endif
    }

    memcpy(data, &xferbuf[0], 512);

#ifdef RAWRDEBUG
    printf("@%l8.8x->s=%5.5x: memcpy(%8.8p, %8.8p, 512)\n",
	(long int)pos, (int)secnum, data, &xferbuf[0]);
#endif

    /* __dpmi_free_dos_memory(sel); -- do not release buffers */
    return retVal;
}

int raw_read_new(int drive, loff_t pos, size_t size, void * data)
{
    __u32 secnum;
    __dpmi_regs regs;
    int i, retVal;
    struct DCB dcb;

    if ((size != 512) || (pos & 511))
	return -1;	/* cannot handle with this simplified version */
    retVal = size;
   
    /* dosSeg = __dpmi_allocate_dos_memory(paragraphs, selector); */
    if (!seg_fat1x) {
        seg_fat1x = __dpmi_allocate_dos_memory((512+10+15)>>4, &sel_fat1x);
        if (seg_fat1x == -1)
	    pdie("RAW_READ out of transfer/DCB buffer memory");
        /* one sector plus one data structure (which is used by ..._new) ... */
    }

    secnum = pos >> 9;
    if (secnum != xfersec)	/* not already in buffer? */
    {
	regs.x.ax = drive;           /* 0 = A: */
	/* BIGDISK protocol */
	dcb.sector = secnum;  /* sector, 32bit */
	dcb.number = 1;          /* one sector */
	dcb.bufptr = seg_fat1x << 16;   /* ... */
	_movedataw(_my_ds(), (unsigned int)&dcb,
	    sel_fat1x, 512, /* DCB at buf+512! */
	(sizeof(struct DCB)+1)>>1);  /* to DOS */
	regs.x.cx = 0xffff;         /* special */
	regs.x.dx = 0xbeef;          /* UNUSED */
	regs.x.ds = seg_fat1x;      /* dcb seg */
	regs.x.bx = 512;   /* points to struct */
          
	__dpmi_int(0x25, &regs); /* DOSdisk access */
	/* luckily, DPMI knows that this is a farcall! */

	if (regs.x.flags & 1 /* CY */) {
	    retVal = regs.x.ax;
	    printf("DOS 'int' 0x25 (new) sector %u read error: %x\n",
		secnum, retVal);
            retVal = -1; /* must signal error like that! */
	    /* flush buffer (or fill with 0xdeadbeef): */
	    for (i = 0; i < 512; i ++) xferbuf[i] = 0;
	} else {
	    _movedatal(sel_fat1x, 0, _my_ds(), (unsigned int)&xferbuf[0], 512>>2);
	    /* copy from DOS */
	}
	xfersec = secnum;

    } else {
#ifdef _IORDEBUG
	/* otherwise, data is still in cache */
	printf(".");
#endif
    }

    memcpy(data, &xferbuf[0], 512);

#ifdef RAWRDEBUG
    printf("@%l8.8x->s=%5.5x: memcpy(%8.8p, %8.8p, 512)\n",
	(long int)pos, (int)secnum, data, &xferbuf[0]);
#endif

    /* __dpmi_free_dos_memory(sel); -- do not release buffers */
    return retVal;
}


int raw_write_old(int drive, loff_t pos, size_t size, void * data)
{
    __u32 secnum;
    __dpmi_regs regs;
    int retVal;
	/* Must read/modify/write if a sector is not fully   */
	/* overwritten, important! Use xferbuf for this...   */
	/* May be better to have a 2nd xferbuf/xfersec var!  */
	/* CURRENT version only allows full sector writes :) */

    if ((size != 512) || (pos & 511) || ((pos >> 9) > 65535UL))
        return -1;	/* only supports whole sectors yet */

    retVal = size;
   
    /* dosSeg = __dpmi_allocate_dos_memory(paragraphs, selector); */
    if (!seg_fat1x) {
        seg_fat1x = __dpmi_allocate_dos_memory((512+10+15)>>4, &sel_fat1x);
        if (seg_fat1x == -1)
	    pdie("RAW_WRITE out of transfer/DCB buffer memory");
        /* one sector plus one data structure (which is used by ..._new) ... */
    }

    secnum = pos >> 9;
    if (xfersec == secnum)
	xfersec = 0xffffffffUL;	/* invalidate cache if same sector */

    _movedatal(_my_ds(), (unsigned int)data, sel_fat1x, 0, 512>>2);
    /* copy to DOS */

    regs.x.ax = drive;           /* 0 = A: */
    regs.x.cx = 1;           /* one sector */
    regs.x.dx = secnum & 0xffff; /* sector */
    regs.x.ds = seg_fat1x;   /* buffer seg */
    regs.x.bx = 0;           /* buffer ofs */
          
    __dpmi_int(0x26, &regs); /* DOSdisk access */
    /* luckily, DPMI knows that this is a farcall! */

    if (regs.x.flags & 1 /* CY */) {
	retVal = regs.x.ax;
	printf("DOS 'int' 0x26 (old) sector %u write error: %x\n",
	    secnum, retVal);
        retVal = -1; /* must signal error like that! */
    }

    /* __dpmi_free_dos_memory(sel); -- do not release buffers */
    return retVal;
}

int raw_write_new(int drive, loff_t pos, size_t size, void * data)
{
    __u32 secnum;
    __dpmi_regs regs;
    int retVal;
    struct DCB dcb;
	/* Must read/modify/write if a sector is not fully   */
	/* overwritten, important! Use xferbuf for this...   */
	/* May be better to have a 2nd xferbuf/xfersec var!  */
	/* CURRENT version only allows full sector writes :) */

    if ((size != 512) || (pos & 511))
        return -1;	/* only supports whole sectors yet */

    retVal = size;
   
    /* dosSeg = __dpmi_allocate_dos_memory(paragraphs, selector); */
    if (!seg_fat1x) {
        seg_fat1x = __dpmi_allocate_dos_memory((512+10+15)>>4, &sel_fat1x);
        if (seg_fat1x == -1)
	    pdie("RAW_WRITE out of transfer/DCB buffer memory");
        /* one sector plus one data structure (which is used by ..._new) ... */
    }

    secnum = pos >> 9;
    if (xfersec == secnum)
	xfersec = 0xffffffffUL;	/* invalidate cache if same sector */

    _movedatal(_my_ds(), (unsigned int)data, sel_fat1x, 0, 512>>2);
    /* copy to DOS */

    regs.x.ax = drive;           /* 0 = A: */
    /* BIGDISK protocol */
    dcb.sector = secnum;  /* sector, 32bit */
    dcb.number = 1;          /* one sector */
    dcb.bufptr = seg_fat1x << 16;   /* ... */
    _movedataw(_my_ds(), (unsigned int)&dcb,
	sel_fat1x, 512, /* DCB at buf+512! */
	(sizeof(struct DCB)+1)>>1);  /* to DOS */
    regs.x.cx = 0xffff;         /* special */
    regs.x.dx = 0xbeef;          /* UNUSED */
    regs.x.ds = seg_fat1x;      /* dcb seg */
    regs.x.bx = 512;   /* points to struct */

    __dpmi_int(0x26, &regs); /* DOSdisk access */
    /* luckily, DPMI knows that this is a farcall! */

    if (regs.x.flags & 1 /* CY */) {
	retVal = regs.x.ax;
	printf("DOS 'int' 0x26 (new) sector %u write error: %x\n",
	    secnum, retVal);
        retVal = -1; /* must signal error like that! */
    }

    /* __dpmi_free_dos_memory(sel); -- do not release buffers */
    return retVal;
}
#endif
