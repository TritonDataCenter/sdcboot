/**********************************************
** APM Library                               **
**                                           **
** Shutdown and Reboot the PC                **
** ----------------------------------------- **
** Author: Aitor Santamaria - Cronos project **
** Created: 13th - June - 2000               **
** Distribute: GNU-GPL v. 2.0 or later       **
**             (see GPL.TXT)                 **
***********************************************/


#include <stdio.h>
#include <dos.h>
#include "cext.h"
#include "apmlib.h"

#ifdef __DJGPP__
#include <sys/types.h>
#include <sys/movedata.h>
#include <sys/farptr.h>
#include <dpmi.h>
#include <go32.h>
#endif

BYTE APMVerMax, APMVerMin;


BYTE    apmlib (BYTE funktion)

{
        BYTE max,min;


        if (!APMIsInstalled ())
        {
        return APM_NOT_INSTALLED;
        }


        APMGetVer ( &max, &min);

        if (FlushAllCaches())
            {
            return APM_ERROR_FLUSHING;
            }


        switch (funktion)
          {
          case APM_REBOOT_WARM:
          Reboot (TRUE);
          break;
          case APM_REBOOT_COLD:
          Reboot (FALSE);
          break;          
          case APM_CTRLALTDEL:
          CtrlAltDel ();
          break;
          case APM_SUSPEND:
          Suspend ();
          return APM_SUSPEND_OK;
          case APM_SHUTDOWN:
          ShutDown ();
          break;
          };
    return APM_NOTHING;
}



#ifdef __DJGPP__
WORD FlushAllCaches ( void )
{
    /* Declarations */
    __dpmi_regs regs;
    WORD Status = 0;
    FILE *FF;
    BYTE SmartDrvBuf;

    /* Begin program */


    /* 1.- CDBLITZ  */
     regs.x.ax = 0x1500;
     regs.x.bx = 0x1234;
     regs.x.cx = 0x9000;
     __dpmi_int (0x2F, &regs);  /* call DOS  */     

    if (regs.x.cx == 0x1234)
    {
	  printf ("Flushing CDBLITZ v. %u.%u...\n", HI(regs.x.dx), LO(regs.x.dx));

	    regs.x.ax = 0x1500;
            regs.x.bx = 0x1234;
            regs.x.cx = 0x9600;
            __dpmi_int (0x2F, &regs);  /* call DOS  */

          if (regs.x.flags & FCarry)
             Status = Status + 1;  /*  ERROR!  */
	  else
	     puts ("Success");
    }


    /* 2.- PC-Cache */
     regs.x.ax = 0xFFA5;
     regs.x.bx = 0x1111;
     __dpmi_int (0x16, &regs);  /* call DOS  */

    if (regs.x.cx == 0)
    {
	puts ("Flushing PC-Cache...");
	regs.x.ax = 0xFFA5;
	regs.x.cx = 0xFFFF;
	__dpmi_int (0x16, &regs);
	/* In practice, never reports error = 2 */
	puts ("Success");
    }


    /* 3.- Quick Cache  */
     regs.x.ax = 0x2700;
     regs.x.bx = 0;
     __dpmi_int (0x13, &regs);  /* call DOS  */

    if ( (! regs.x.ax) && regs.x.bx)
    {
	  printf ("Flushing QuickCache v. %u.%u...\n", HI(regs.x.bx), LO(regs.x.bx));

	  regs.x.ax = 0x21;
	  __dpmi_int (0x13, &regs);
	  if (! regs.x.ax)
	     Status = Status + 4;  /*  ERROR!  */
	  else
	     puts ("Success");
    }


    /* 4.- PC - Kwik: PC-Tools, Qualitas Cache...  */
     regs.x.ax = 0x2B00;
     regs.x.cx = 0x4358;
     __dpmi_int (0x21, &regs);  /* call DOS  */
    
    if ( ! HI(regs.x.ax))
    {
          printf ("Flushing QuickCache v. %u.%u...\n", HI(regs.x.dx), LO(regs.x.dx));
          regs.x.ax = 0xA1;
          regs.x.si = 0x4358;
          __dpmi_int(0x21, &regs);
          if (regs.x.flags & FCarry)
	     Status = Status + 8;  /*  ERROR!  */
	  else
	     puts ("Success");
    }


    /* 5.- Microsoft SmartDrv.EXE  */
     regs.x.ax = 0x4A10;
     regs.x.bx = 0x0000;
     regs.x.cx = 0xEBAB;
     __dpmi_int (0x2F, &regs);  /* call DOS  */


    if (regs.x.ax == 0xBABE)
    {
	printf ("Flushing Microsoft SmartDrv.EXE v. %u.%u...\n", HI(regs.x.bp), LO(regs.x.bp));
     regs.x.ax = 0x4A01;
     regs.x.bx = 0x0001;
     __dpmi_int (0x2F, &regs);  /* call DOS  */

	/* In practice, never reports error = 16 */
	puts ("Success");
    }


    /* 6.- Microsoft SmartDrv.SYS  */
    FF = fopen ("SMARTAAR", "r");
    if (FF != NULL)
    {
	puts ("Flushing Microsoft SmartDrv.SYS");

	SmartDrvBuf = 2;
	regs.x.ax = 0x4403;
	regs.x.bx = fileno (FF);
	regs.x.cx = 0x0000;
	regs.x.ds = __tb >> 4;     /* transfer buffer address in DS:DX  */
	regs.x.dx = __tb & 0x0f;
	__dpmi_int (0x21, &regs);  /* call DOS  */


        dosmemget (__tb, 1, &SmartDrvBuf);


        if ((regs.x.flags & FCarry) || ! regs.x.ax)
	   Status = Status + 32;  /*  ERROR!  */
	else
	   puts ("Success");
    }

    puts ("Reseting disks system...");

    /* B1.- Disks reset */
    regs.x.ax = 0x0D00;
    __dpmi_int (0x21, &regs);  /* call DOS  */



    /* B2.- Reset Disk system */
    regs.x.ax = 0x0000;
    regs.x.dx = (1 << 7);
    __dpmi_int (0x13, &regs);  /* call DOS  */

    if (regs.x.flags & FCarry)
	Status = Status || ( 1 << 15 );
    else
	puts ("Success");

    return Status;
}
#else
WORD FlushAllCaches ( void )
{
    /* Declarations */
    struct REGPACK Regs;
    WORD Status = 0;
    FILE *FF;
    BYTE SmartDrvBuf;


    /* Begin program */


    /* 1.- CDBLITZ  */
    Regs.r_ax = 0x1500;
    Regs.r_bx = 0x1234;
    Regs.r_cx = 0x9000;
    intr (0x2F, &Regs);
    if (Regs.r_cx == 0x1234)
    {
	  printf ("Flushing CDBLITZ v. %u.%u...\n", HI(Regs.r_dx), LO(Regs.r_dx));
          Regs.r_ax = 0x1500;
          Regs.r_bx = 0x1234;
          Regs.r_cx = 0x9600;
          intr (0x2F, &Regs);
          if (Regs.r_flags & FCarry)
	     Status = Status + 1;  /*  ERROR!  */
	  else
	     puts ("Success");
    }


    /* 2.- PC-Cache */
    Regs.r_ax = 0xFFA5;
    Regs.r_cx = 0x1111;
    intr (0x16, &Regs);
    if (Regs.r_cx == 0)
    {
        puts ("Flushing PC-Cache...");
          Regs.r_ax = 0xFFA5;
          Regs.r_cx = 0xFFFF;
          intr (0x16, &Regs);
        /* In practice, never reports error = 2 */
        puts ("Success");
    }


    /* 3.- Quick Cache  */
    Regs.r_ax = 0x2700;
    Regs.r_bx = 0;
    intr (0x13, &Regs);
    if ( (! Regs.r_ax) && Regs.r_bx)
    {
          printf ("Flushing QuickCache v. %u.%u...\n", HI(Regs.r_bx), LO(Regs.r_bx));
          Regs.r_ax = 0x21;
          intr (0x13, &Regs);
          if (! Regs.r_ax)
             Status = Status + 4;  /*  ERROR!  */
          else
             puts ("Success");
    }


    /* 4.- PC - Kwik: PC-Tools, Qualitas Cache...  */
    Regs.r_ax = 0x2B00;
    Regs.r_cx = 0x4358;
    intr (0x21, &Regs);
    if ( ! HI(Regs.r_ax))
    {
          printf ("Flushing QuickCache v. %u.%u...\n", HI(Regs.r_dx), LO(Regs.r_dx));
          Regs.r_ax = 0xA1;
          Regs.r_si = 0x4358;
          intr (0x13, &Regs);
          if (Regs.r_flags & FCarry)
             Status = Status + 8;  /*  ERROR!  */
          else
             puts ("Success");
    }


    /* 5.- Microsoft SmartDrv.EXE  */
    Regs.r_ax = 0x4A10;
    Regs.r_bx = 0;
    Regs.r_cx = 0xEBAB;
    intr (0x2F, &Regs);
    if (Regs.r_ax == 0xBABE)
    {
        printf ("Flushing Microsoft SmartDrv.EXE v. %u.%u...\n", HI(Regs.r_bp), LO(Regs.r_bp));
        Regs.r_ax = 0x4A01;
        Regs.r_bx = 0x0001;
        intr (0x2F, &Regs);
        /* In practice, never reports error = 16 */
        puts ("Success");
    }


    /* 6.- Microsoft SmartDrv.SYS  */
    FF = fopen ("SMARTAAR", "r");
    if (FF != NULL)
    {
        puts ("Flushing Microsoft SmartDrv.SYS");
        SmartDrvBuf = 2;
          Regs.r_ax = 0x4403;
          Regs.r_bx = fileno (FF);
          Regs.r_cx = 0;
          Regs.r_ds = FP_SEG (&SmartDrvBuf);
          Regs.r_dx = FP_OFF (&SmartDrvBuf);
          intr (0x21, &Regs);


        if ((Regs.r_flags & FCarry) || ! Regs.r_ax)
           Status = Status + 32;  /*  ERROR!  */
        else
           puts ("Success");
    }


    puts ("Reseting disks system...");


    /* B1.- Disks reset */
    Regs.r_ax = 0x0D00;
    intr (0x21, &Regs);


    /* B2.- Reset Disk system */
    Regs.r_ax = 0;
    Regs.r_dx = (1 << 7);
    intr (0x13, &Regs);
    if (Regs.r_flags & FCarry)
        Status = Status || ( 1 << 15 );
    else
        puts ("Success");



    return Status;
}

#endif

#ifdef __DJGPP__
void Reboot (BOOL Warm)
{
   /* Declarations */
   __dpmi_regs regs;
//   void ((far *PProcedure)()) = MK_FP(0xFFFF,0);

   /* Begin program */


   /* 1.- Nice reboot: using AT Keyboard */
   if ( ! Warm)
   {
        regs.x.ax = 0x1200;
        __dpmi_int(0x16, &regs);
        if (regs.x.ax != 0x1200)  /* AT MF-II Keyboard present */
                outportb (0x64, 0xFE);
    }


    /* No AT Keyboard present */

    _farpokew(_dos_ds, 0x0040*16+0x0072, 0x1234 * Warm);
//  same as  *(int far *) MK_FP(0x0040, 0x0072) = 0x1234 * Warm; in Borlandc

    regs.x.cs = 0xffff;
    regs.x.ip = 0x0000;
    regs.x.ss = regs.x.sp = 0;
    __dpmi_simulate_real_mode_procedure_retf (&regs);
    //  same as  (*PProcedure) (); in borlandc

}
    
#else

void Reboot (BOOL Warm)
{
   /* Declarations */
   void ((far *PProcedure)(void)) = (void (far *)(void))MK_FP(0xFFFF,0);
   struct REGPACK Regs;


   /* Begin program */


   /* 1.- Nice reboot: using AT Keyboard */
   if ( ! Warm)
   {
        Regs.r_ax = 0x1200;
        intr (0x16, &Regs);
        if (Regs.r_ax != 0x1200)  /* AT MF-II Keyboard present */
                outportb (0x64, 0xFE);
    }


    /* No AT Keyboard present */
    *(int far *) MK_FP(0x0040, 0x0072) = 0x1234 * Warm;
    (*PProcedure) ();
}
#endif

#ifdef __DJGPP__
void ShutDown (void)
{
   /* Declarations */
    __dpmi_regs regs;


   /* Begin program */


   /*RealMode Interface connect*/
     regs.x.ax = 0x5301;
     regs.x.bx = 0;
     __dpmi_int (0x15, &regs);  /* call DOS  */


   /*Engage power management*/
     regs.x.ax = 0x530F;
     regs.x.bx = 0x0001;
     regs.x.cx = 0x0001;
     __dpmi_int (0x15, &regs);  /* call DOS  */



   /*Enable APM for all devices*/
     regs.x.ax = 0x5308;
     regs.x.bx = 0x0001;
     regs.x.cx = 0x0001;
     __dpmi_int (0x15, &regs);  /* call DOS  */


   /*Force version 1.1 compatibility*/
     regs.x.ax = 0x530E;
     regs.x.bx = 0x0000;
     regs.x.cx = 0x0101;
     __dpmi_int (0x15, &regs);  /* call DOS  */



   /*1.- First attempt: using APM 1.1 or later*/
   /*Shutdown all the devices supported by AMP*/
     regs.x.ax = 0x5307;
     regs.x.bx = 0x0001;
     regs.x.cx = 0x0003;
     __dpmi_int (0x15, &regs);  /* call DOS  */



   /*2.- Second attempt: using APM 1.0*/
   /*Shutdown only the system BIOS*/
     regs.x.ax = 0x5307;
     regs.x.bx = 0x0000;
     regs.x.cx = 0x0003;
     __dpmi_int (0x15, &regs);  /* call DOS  */



}
#else
void ShutDown (void)
{
   /* Declarations */
   struct REGPACK Regs;


   /* Begin program */


   /*RealMode Interface connect*/
   Regs.r_ax = 0x5301;
   Regs.r_bx = 0;
   intr (0x15, &Regs);


   /*Engage power management*/
   Regs.r_ax = 0x530F;
   Regs.r_bx = 0x0001;
   Regs.r_cx = 0x0001;
   intr (0x15, &Regs);


   /*Enable APM for all devices*/
   Regs.r_ax = 0x5308;
   Regs.r_bx = 0x0001;
   Regs.r_cx = 0x0001;
   intr (0x15, &Regs);


   /*Force version 1.1 compatibility*/
   Regs.r_ax = 0x530E;
   Regs.r_bx = 0x0000;
   Regs.r_cx = 0x0101;
   intr (0x15, &Regs);


   /*1.- First attempt: using APM 1.1 or later*/
   /*Shutdown all the devices supported by AMP*/
   Regs.r_ax = 0x5307;
   Regs.r_bx = 0x0001;
   Regs.r_cx = 0x0003;
   intr (0x15, &Regs);


   /*2.- Second attempt: using APM 1.0*/
   /*Shutdown only the system BIOS*/
   Regs.r_ax = 0x5307;
   Regs.r_bx = 0x0000;
   Regs.r_cx = 0x0003;
   intr (0x15, &Regs);


}

#endif


#ifdef __DJGPP__
void CtrlAltDel (void)
{
   /* Declarations */
    __dpmi_regs regs;

   /* Begin program */
    __dpmi_int (0x19, &regs);  /* call DOS  */

}
#else
void CtrlAltDel (void)
{
   /* Declarations */
   struct REGPACK Regs;


   /* Begin program */
   intr (0x19, &Regs);
}
#endif

#ifdef __DJGPP__
void Suspend (void)
{
   /* Declarations */
    __dpmi_regs regs;

   /* Begin program */


   /*RealMode Interface connect*/
     regs.x.ax = 0x5301;
     regs.x.bx = 0x0000;
     __dpmi_int (0x15, &regs);  /* call DOS  */



   /*Engage power management*/

     regs.x.ax = 0x530F;
     regs.x.bx = 0x0001;
     regs.x.cx = 0x0001;
     __dpmi_int (0x15, &regs);  /* call DOS  */



   /*Enable APM for all devices*/
     regs.x.ax = 0x5308;
     regs.x.bx = 0x0001;
     regs.x.cx = 0x0001;
     __dpmi_int (0x15, &regs);  /* call DOS  */



   /*Force version 1.1 compatibility*/

     regs.x.ax = 0x530E;
     regs.x.bx = 0x0000;
     regs.x.cx = 0x0101;
     __dpmi_int (0x15, &regs);  /* call DOS  */



    /*Suspend*/

     regs.x.ax = 0x5307;
     regs.x.bx = 0x0001;
     regs.x.cx = 0x0002;
     __dpmi_int (0x15, &regs);  /* call DOS  */



}
#else
void Suspend (void)
{
   /* Declarations */
   struct REGPACK Regs;


   /* Begin program */


   /*RealMode Interface connect*/
   Regs.r_ax = 0x5301;
   Regs.r_bx = 0;
   intr (0x15, &Regs);


   /*Engage power management*/
   Regs.r_ax = 0x530F;
   Regs.r_bx = 0x0001;
   Regs.r_cx = 0x0001;
   intr (0x15, &Regs);


   /*Enable APM for all devices*/
   Regs.r_ax = 0x5308;
   Regs.r_bx = 0x0001;
   Regs.r_cx = 0x0001;
   intr (0x15, &Regs);


   /*Force version 1.1 compatibility*/
   Regs.r_ax = 0x530E;
   Regs.r_bx = 0x0000;
   Regs.r_cx = 0x0101;
   intr (0x15, &Regs);


    /*Suspend*/
   Regs.r_ax = 0x5307;
   Regs.r_bx = 0x0001;
   Regs.r_cx = 0x0002;
   intr (0x15, &Regs);


}
#endif

#ifdef __DJGPP__
BOOL APMIsInstalled (void)
{
   /* Declarations */

    __dpmi_regs regs;


   /* Begin program */




     regs.x.ax = 0x5300;
     regs.x.bx = 0x0000;
     __dpmi_int (0x15, &regs);  /* call DOS  */



   if (! (regs.x.flags & FCarry))
   {
                APMVerMax = LO(regs.x.ax);
                APMVerMin = LO(regs.x.ax);
                return ( TRUE );
    }
    else
                return ( FALSE );
}
#else
BOOL APMIsInstalled (void)
{
   /* Declarations */
   struct REGPACK Regs;


   /* Begin program */


   Regs.r_ax = 0x5300;
   Regs.r_bx = 0x0000;
   intr (0x15, &Regs);


   if (! (Regs.r_flags & FCarry))
   {
                APMVerMax = LO(Regs.r_ax);
                APMVerMin = LO(Regs.r_ax);
                return ( TRUE );
    }
    else
                return ( FALSE );
}
#endif


#ifdef __DJGPP__
void APMGetVer (BYTE *Max, BYTE *Min)
{
   /* Begin program */
     *Max = APMVerMax;
     *Min = APMVerMin;
}
#else
void APMGetVer (BYTE *Max, BYTE *Min)
{
   /* Begin program */
     *Max = APMVerMax;
     *Min = APMVerMin;
}
#endif
