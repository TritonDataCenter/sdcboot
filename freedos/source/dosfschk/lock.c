#ifdef __DJGPP__	/* only for DOS */

#include "common.h"
#include "volume.h"
#include <dpmi.h>
#include <stdio.h>	/* printf */


/* Win9x / DOS 7+: LOCK (lock TRUE) / UNLOCK (lock FALSE) drive, lock */
/* can be 2 for the nested second level 0 lock... from FreeDOS FORMAT */
void Lock_Unlock_Drive(int drive, int lock) /* 0 based drive number 0 = A: */
{
    static int lock_nesting = 0;
    __dpmi_regs regs;

    /* As recommended by http://msdn.microsoft.com "Level 0 Lock":          */
    /* get level 0 lock, use filesystem functions to prepare (if needed),   */
    /* get level 0 lock AGAIN, this time with permissions 4, use sector I/O */
    /* and IOCTL functions or int 13 to create filesystem, then release the */
    /* inner level 0 lock, use filesystem functions for cleanup, release    */
    /* the outer level 0 lock, done. You must NOT drop the locks or exit at */
    /* a moment when the filesystem is in inconsistent / nonFAT state, or   */
    /* Win9x will assume that the filesystem is still as before the lock... */

    regs.x.ax = 0x3000; /* get full DOS version and OEM ID */
    __dpmi_int(0x21, &regs);
    if (regs.h.al < 7)	/* Major DOS version number, 7+ is Win9x+ */
	return;		/* ignore the whole locking on older DOS versions */

    if ((lock_nesting==0) && (!lock))	/* extraneous unlock attempt */
	return;

    /* *** try to LOCK the drive (logical volume) *** */
    regs.x.cx = lock ? 0x084a : 0x086a; /* (un)LOCK logical FAT1x volume */
    do
    {
	regs.x.ax = 0x440d; /* IOCTL */
#if 0
	if (/* (!lock) && */ 
	    (param.fat_type == FAT32))
	    regs.h.ch = 0x48; /* we already know that it is FAT32 */
#endif
	regs.h.bh = 0;		/* big fat lock, not one of the "levels" 1..3 */
	regs.h.bl = drive + 1;	/* A: is 1 etc. */
	regs.x.dx = (lock>1) ? 4 : 0;	/* when getting SECOND level 0 lock */
				/* ... then select "format" permissions! */
	regs.x.flags &= ~1;	/* (2 is block new file mappings) */
	__dpmi_int(0x21, &regs); /* call int 0x21 DOS API */
	regs.h.ch += 0x40;    /* try FAT32 (etc.) mode next */
	if (!lock) regs.h.ch = 0x77; /* give up if earlier if unlocking */
    } while ( (regs.h.ch <= 0x48) && (regs.x.flags & 1 /* CY */) );

    if ( (regs.x.flags & 1 /* CY */) && lock) /* ignore unlock errors */
    {
	unsigned int lockerrno = regs.x.ax;

	if (lockerrno == 0x0f) /* invalid drive */
            pdie("Trying to lock invalid drive");

	regs.x.ax = 0x3000; /* get full DOS version and OEM ID */
	__dpmi_int(0x21, &regs);
	if (regs.h.bh == 0xfd)
	    printf("FreeDOS log. drive lock error 0x%x ignored.\n",lockerrno);
	else
	    pdie("Could not lock logical drive (error 0x%x)", lockerrno);
    } /* LOCK error */


#ifdef FLOPPYPHYSLOCKING	/* special floppy handling */
    /* for int 13 writes you have to lock the physical volume, too */
    if ((param.drive_type == FLOPPY) && (lock!=1)) /* 0.91q: not for outer level */
    {
	regs.x.ax = 0x440d;  /* IOCTL */
	regs.x.cx = lock ? 0x084b : 0x086b; /* (un)LOCK physical FAT1x volume */
#if 0
	if (/* (!lock) && */ 
	    (param.fat_type == FAT32))
	    regs.h.ch = 0x48; /* we already know that it is FAT32 */
#endif
	regs.h.bh = 0;       /* lock level (or use 3?) (DX / BL still set) */
	/* or should BL be the BIOS driver number now (MSDN)? RBIL: 01=a: */
	regs.x.flags &= ~1;
	__dpmi_int(0x21, &regs); /* call int 0x21 DOS API */
	if ( (regs.x.flags & 1 /* CY */)  && lock) /* ignore unlock errors */
        {
	    unsigned int lockerrno = regs.x.ax;

	    regs.x.ax = 0x3000; /* get full DOS version and OEM ID */
            __dpmi_int(0x21, &regs);
            if (regs.h.bh == 0xfd)
		printf("FreeDOS phys. drive lock error 0x%x ignored.\n", lockerrno);
	    else
		pdie("Could not lock physical floppy drive for formatting (error 0x%x)!?\n",
		    lockerrno);
		/* could { ... exit(1); } here, but MSDN does not even */
		/* suggest getting a physical drive lock at all..!? */
	} /* LOCK error */
    } /* extra FLOPPY treatment */
#endif	/* floppy handling */

    lock_nesting += (lock) ? 1 : -1;	/* maintain lock nesting count */
}   /* *** drive is now LOCKed if lock, else UNLOCKed *** */


#if 0	/* NOT YET PORTED TO DJGPP */
/* Do not confuse this with DOS 7.0+ drive locking.  */
/* Setting this flag allows to access drives which   */
/* aren't FAT formatted. Is this useful for DOSFSCK? */
void Enable_Disk_Access(int drive) /* DOS 4.0+ drive access flag / locking */
{
    unsigned char category_code;
    unsigned long error_code=0;

    category_code = (param.fat_type == FAT32) ? 0x48 : 0x08;

    /* Get the device parameters for the logical drive */

    regs.h.ah=0x44;                     /* IOCTL Block Device Request      */
    regs.h.al=0x0d;                     /* IOCTL */
    regs.h.bl=drive + 1;
    regs.h.ch=category_code;            /* 0x08 if !FAT32, 0x48 if FAT32   */
    regs.h.cl=0x67;                     /* Get Access Flags                */
    regs.x.dx=FP_OFF(&access_flags);	/* only 2 bytes: 0 and the flag */
    sregs.ds =FP_SEG(&access_flags);
    access_flags.special_function = 0;

    intdosx(&regs, &regs, &sregs);

    error_code = regs.h.al;

    if (regs.x.flags & 1 /* CY */)
    {
	/* BO: if invalid function: try to format anyway maybe access
	 * flags do not work this way in this DOS (e.g. DRDOS 7.03) */
	if (error_code == 0x1 || error_code == 0x16)
	{
	    printf("No disk access flags used (IOCTL int 21.440d.x8.67)\n");
	    return;
	}

	/* Add error trapping here */
	pdie("\nCannot get disk access flags (error %02x). Giving up.\n",
	    error_code);
    }

    if (access_flags.disk_access==0) /* access not yet enabled? */
    {
	access_flags.disk_access++;
	access_flags.special_function = 0;

	regs.h.ah = 0x44;                     /* IOCTL Block Device Request          */
	regs.h.al = 0x0d;                     /* IOCTL */
	regs.h.bl = drive + 1;
	regs.h.ch = category_code;            /* 0x08 if !FAT32, 0x48 if FAT32       */
	regs.h.cl = 0x47;                     /* Set access flags                    */
	regs.x.dx = FP_OFF(&access_flags);
	sregs.ds  = FP_SEG(&access_flags);
	intdosx( &regs, &regs, &sregs);

	error_code = regs.h.al;

	if (regs.x.flags & 1 /* CY */)
	{
	    /* Add error trapping here */
	    pdie("Cannot enable disk access flags (error %02x). Giving up.",
        	error_code);
    	}
    }
} /* Enable_Disk_Access */
#endif

#endif
