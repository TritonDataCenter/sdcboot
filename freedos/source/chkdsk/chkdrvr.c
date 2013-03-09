/*    
   Chkdsk.c - check disk utility.

   Copyright (C) 2002 Imre Leber

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be
*/

#ifdef ENABLE_LOGGING
#include <time.h>
#include <stdio.h>
#endif

#include "fte.h"
#include "chkdrvr.h"

/*
   Returns:
           FAIL   if there was an internal error
           TRUE   if all the checks got performed with success
           FALSE  if there was a check that indicated a faulty file system
*/

BOOL CheckVolume(RDWRHandle handle,
                 struct CheckAndFix* TheChecks,
                 unsigned numberofchecks)
{
   BOOL retval = TRUE;
   
   unsigned i = 0;

   for (i = 0; i < numberofchecks; i++)
   {
#ifdef ENABLE_LOGGING
      clock_t start, end;
      
      printf("checking %s\n", TheChecks[i].CheckName); 
      start = clock();      
#endif                 
       switch (TheChecks[i].check(handle))
       {
          case FAILED:        /* There was an error in the file system. */
               retval = FALSE;
          case SUCCESS:       /* No error found in the file system. */
               break;
          case ERROR:         /* Media error. */
               return FAIL;
       }
#ifdef ENABLE_LOGGING
      end = clock();  
      printf("Ticks elapsed: %ld\n",(long)(end - start));
#endif                
   }

   return retval;
}

BOOL FixVolume(RDWRHandle handle,
               struct CheckAndFix* TheChecks,
               unsigned numberofchecks)
{
   RETVAL result;
   unsigned performing = 0;

   while (performing < numberofchecks)
   {
#ifdef ENABLE_LOGGING
      clock_t start, end;

      printf("fixing %s\n", TheChecks[performing].CheckName); 
      start = clock();
#endif                 
           
      result = TheChecks[performing].fix(handle);

      if (!SynchronizeFATs(handle))
         return FAIL;
      
      switch (result)
      {
            case SUCCESS:       /* Test was successfull. */
                 performing++;
                 break;

            case RESCAN:        /* Rescan the file system. */
                 performing = 0;
                 break;
                   
            case ERROR:         /* Internal error. */
                 return FAIL;
      }
      
#ifdef ENABLE_LOGGING
      end = clock();  
      printf("Ticks elapsed: %ld\n", (long)(end - start));
#endif      
   }

   return TRUE; /* All checks performed. */
}
