/**********************************************
** APM Library                               **
**                                           **
** Shutdown and Reboot the PC                **
** ----------------------------------------- **
** Author: Aitor Santamaria - Cronos project **
** Created:  8th - August - 2000             **
** Distribute: GNU-GPL v. 2.0 or later       **
**             (see GPL.TXT)                 **
***********************************************/


#ifndef _Cr_APM
#define _Cr_APM


#include "CExt.h"


#define APM_REBOOT_WARM     1
#define APM_REBOOT_COLD     2
#define APM_SUSPEND         3
#define APM_CTRLALTDEL      4
#define APM_SHUTDOWN        5

#define APM_NOTHING         0
#define APM_NOT_INSTALLED   1
#define APM_ERROR_FLUSHING  2
#define APM_SUSPEND_OK      3



BYTE    apmlib (BYTE funktion);
/* Manages the PC power
   Use the constants defined above */


WORD FlushAllCaches ( void );
/* Flushes the existing caches.
   RETURN:  0:    ok
                else: bipmap of caches that couldn't be flushed:
                         bit0: CDBlitz
                         bit1: PC-Cache (this never fails ;-))
                         bit2: Quick Cache
                         bit3: Super PC-Kwik  (PC-Tools, Qualitas QCache, ...)
                         bit4: MS-SmartDrv.EXE (this never fails ;-))
                         bit5: MS-SmartDrv.SYS */

void Reboot (BOOL warm);
/* Reboots the system.
   INPUT:  Warm: indicates if a cold boot or warm boot will be performed
   NOTE:   Caches MUST be flushed before calling this procedure */


void ShutDown (void);
/* Turns the system off
   NOTES:  - Caches MUST be flushed before calling this procedure
             - This will only work under BIOSes supporting APM v.1.1 or later */


void CtrlAltDel (void);
/* Performs a Ctrl+Alt+Del
   NOTE:   - Caches MUST be flushed before calling this procedure */


void Suspend (void);
/* Suspends the system to a status of minimum power until a key is pressed
   NOTE:   - This will only work under BIOSes supporting APM v.1.1 or later */


BOOL APMIsInstalled (void);
/* Checks if APMIsInstalled
   RETURN:  TRUE if APM is installed*/


void APMGetVer (BYTE *Max, BYTE *Min);
/* Returns the version of the APM, if found
   NOTE:  APMIsInstalled MUST be called first, otherwise it won't return
            a correct version number*/


#endif
