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

#include <stdio.h>
#include <dir.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fte.h"

#include "chkdrvr.h"
#include "checks.h"
#include "version.h"
#include "misc\switchch.h"
#include "summary\summary.h"
#include "struct\FstTrMap.h"
#include "fragfact\defrfact.h"
#include "surface\surface.h"
#include "chkargs.h"

static void Usage(char switchch);
static void OnExit(void);

int main(int argc, char *argv[])
{
    time_t t;
    RDWRHandle handle;
    int retval, i;
    int thesame;
    int optioncounter=0;
    
    BOOL fixerrors = FALSE, checkerrors = TRUE, scansurface = FALSE, calcdefrags = FALSE;
    BOOL ShowAllFiles = FALSE, Interactive = FALSE, PrevD = FALSE;
    BOOL param2switch = FALSE;
    char switchch = SwitchChar();
    char curdrive[3];
    char* curdir, *abspath;

    /* Show copyright message */
    printf("ChkDsk " VERSION "\n"
	   "Copyright 2002, 2003, 2009 Imre Leber under the GNU GPL\n\n");

    /* First check the parameters. */
    if ((argc == 2) &&
        (((argv[1][0] == switchch) || (argv[1][0] == '/'))  &&
	 (argv[1][1] == '?') && (argv[1][2] == '\0')))
    {
       Usage(switchch);
       return 0;
    }

    if (argc > 4)
    {
       printf("Invalid parameters (type %c? for help)\n", switchch);
       return 2;
    }
    
    /*
    Chkdsk [<volume>] [%cf] [%cd <files>] [%cr] [%cs] [%cv]
                      [%ci]
    */
    
    /* Check syntax */
    if (argc > 1)
    {
       param2switch = TRUE;
       
       for (i = 1; i < argc; i++)
       {
           if ((argv[i][0] != switchch) && (argv[i][0] != switchch))   
           {
              if (i == 1)       /* An image file used */
              {
                 param2switch = FALSE;
                 continue;
              }   
              else if (PrevD)
              {
                 continue;
              }
              else
              {
                 printf("Invalid parameters (type %c? for help)\n", switchch);
                 return 2;  
              }
           }
           else if (PrevD)
           {
              printf("Please use a path after /d\n");
              return 2;
           }
           
           PrevD = FALSE;
          
           switch (toupper(argv[i][1]))
           {
                 case 'F':
                      fixerrors = TRUE;
                      optioncounter++;
                      break;
                      
                 case 'D':
                      calcdefrags = TRUE;    
                      PrevD = TRUE;
                      optioncounter++;
                      break;
                      
                 case 'R':
                      scansurface = TRUE;
                      checkerrors = FALSE;                         
                      optioncounter++;
                      break;
                         
                 case 'S':
                      checkerrors = FALSE;
                      optioncounter++;
                      break;
                      
                 case 'V':
                      ShowAllFiles = TRUE;
                      break;
                      
                 case 'I':
                      fixerrors = TRUE;
                      Interactive = TRUE;
                      optioncounter++;
                      
                      printf("%s reserved for future use!", argv[i]);
                      return 2;
                      //break;
                      
                 default:
                      printf("Unknown option %s used!", argv[i]);   
                      return 2;
           }
                      
           if (argv[i][2] != '\0')
           {
              printf("Invalid parameters (type %c? for help)\n", switchch);
              return 2;                
           }  
       }
            
       /* Check semantics */     
       if (optioncounter > 1)
       {
          printf("Some of the options can not be used together!\n");
          return 2;
       }
       
       if (ShowAllFiles && (optioncounter == 1))
       {
          if (!fixerrors)
          {
             printf("Some of the options can not be used together!\n");
             return 2;          
          }
       }
            
       if (ShowAllFiles)
       {
          IndicateAllFileListing();
       }
       if (Interactive)
       {
          SetInteractive();
       }
    }

    atexit(OnExit);

    InitSectorCache();
    StartSectorCache();

    if ((argc == 1) || param2switch)
    {
       curdrive[0] = getdisk() + 'A';
       curdrive[1] = ':';
       curdrive[2] = 0;

       if (!InitReadWriteSectors(curdrive, &handle))
       {
          printf("Cannot access %s\n", curdrive);
          return 2;
       }
    }
    else
    {
       if (!InitReadWriteSectors(argv[1], &handle))
       {
          printf("Cannot access %s\n", argv[1]);
          return 2;
       }

       if (!IsWorkingOnImageFile(handle))
       {
	  curdrive[0] = argv[1][0];
       }
       else
       {
	  curdrive[0] = '?';
       }
    }

    /* FAT32 not currently supported */
    if (GetFatLabelSize(handle) == FAT32)
    {
       printf("FAT32 not currently supported\n");
       return 2;
    }
    
    if (calcdefrags)
    {
        int imagefile, canproceed=TRUE;
          
        abspath = (char*) malloc(MAXPATH);
        curdir  = (char*) malloc(MAXPATH);

        imagefile = IsWorkingOnImageFile(handle);

	if (!imagefile && (argc == 3) && (!getcwd(curdir, MAXPATH)))
	{
           /* No explicit drive was entered and we could not get the current working 
             directory => error */     
	   printf("Cannot get current working dir\n");

	   free(abspath);
	   free(curdir);
	}
	else
	{ 
           if (imagefile ||                     /* An image file */
               (argc == 4) ||                   /* volume explicit entered by user */ 
               ((argv[argc-1][0] != 0) && (argv[argc-1][1] == ':'))) /* drive indicated */
           {
              /* If a drive is entered then it must be equal to the one given by the user */     
	      if ((argv[argc-1][0] != 0) && (argv[argc-1][1] == ':'))
	      {
		 if (imagefile ||
                    (toupper(argv[argc-1][0]) != toupper(curdrive[0])))
		 {
		    printf("Invalid drive specification\n");
		    canproceed = FALSE;
		 }
		 else
		 {
		    strcpy(abspath, argv[argc-1] + 2);
		 }
              }
	      else
		 strcpy(abspath, argv[argc-1]);
           }
	   else                                 /* default drive chosen */
           {
              if ((curdir[0] != 0) && (curdir[1] == ':'))
                  curdir+=2;               /* Take off the drive spec */

              canproceed = MakeAbsolutePath(curdir, argv[argc-1], abspath);
	      if (!canproceed)
	      {
		 printf("Incorrect relative path specification\n");
              }
           }


           if (canproceed)           /* We have the absolute path */
	   {
              printf("%s\n\n", abspath);
                
              free(curdir);
              if (!PrintFileDefragFactors(handle, abspath)) /* Print out the files */
              {
                 printf("Problem printing out defragmentation factors.\n");
	      }
              free(abspath);
           }
           else
           {
	      free(curdir);
              free(abspath);
           }
	}

        CloseReadWriteSectors(&handle);
	return 0;       
    }

    /* Check the BOOT */
   if (!DescriptorCheck(handle))
   {
       printf("Suspicious descriptor in boot\n");
	   
  //     DestroyFastTreeMap();
  //     CloseReadWriteSectors(&handle);
  //     return 2;
    }

    /* See wether there are any differences in the FATs or the BOOTs
       if there are we are not starting the checking in this version. */
    thesame = MultipleFatCheck(handle);
    if ((thesame == TRUE) && (GetFatLabelSize(handle) == FAT32))
    {
       thesame = MultipleBootCheck(handle);
       if (thesame == FALSE)
       {
          printf("BOOTs are different\n");
       }
       if (thesame == FAIL)
       {
          printf("Problem reading BOOT(s)\n");            
       }
    }
    else if (thesame == FALSE)
    {
       printf("FATs are different\n");
    }
    else if (thesame == FAIL)
    {
       printf("Problem reading FAT(s)\n");
    }
    
    /* Create the fast tree map */
    if (thesame && (fixerrors || checkerrors)) /* don't create it if it is not required */
       CreateFastTreeMap(handle);    

    if (thesame == TRUE)
    {
       time(&t);

       /* Scan the data area (if required) */
       if (scansurface)
       {
          if (!ScanSurface(handle))
          {        
             printf("\nProblem scanning surface\n");
             return 1;
          }
          printf("\n");
       }

       /* Now try to check for (and fix) all the defects. */
       if (fixerrors)
       {
          switch (FixVolume(handle, TheChecks, AMOFCHECKS))
          {
              case TRUE:
                   retval = 0;
                   break;
      	      case FAIL:
	           printf("Error accessing the volume\n");
                   DestroyFastTreeMap();
                   CloseReadWriteSectors(&handle);
		   return 2;
          }
       }
       else if (checkerrors)
       {
          switch (CheckVolume(handle, TheChecks, AMOFCHECKS))
          {
              case TRUE:
                   retval = 0;
                   break;
              case FALSE:
 		   printf("\nErrors were found. You did not use the /F switch. "
		          "Errors are not corrected\n\n");
	           retval = 1;
                   break;
	      case FAIL:
	           printf("Error accessing the volume\n");
                   DestroyFastTreeMap();
                   CloseReadWriteSectors(&handle);
                   return 2;
          }
       }

       printf("Elapsed time: %lds\n", time(NULL)-t);
    }
    
    /* Print out the volume summary. */
    if (!ReportVolumeSummary(handle))
    {
       retval = 2;
    }

    if (thesame == FAIL) retval = 2;

    DestroyFastTreeMap();
    CloseReadWriteSectors(&handle);

    return retval;
}

static void Usage(char switchch)
{
    printf("Checks a volume and returns a status report\n"
           "Usage:\n"
           "\tChkdsk [<volume>] [%cf] [%cd <files>] [%cr] [%cs] [%cv]\n"
           "\n"
           "%cf: attempt to fix any errors found.\n"
           "%cd: print out the indicated files and show the fragmentation factor.\n"
           "%cr: scan the data area and try to recover unreadable data.\n"
           "%cs: only show drive summary.\n"
           "%cv: show file name as it is being checked.\n"
           "\n"
           "Note: if volume is ommited, the current drive is assumed.\n",
           switchch,
           switchch,
           switchch,
           switchch,
           switchch,
           switchch,
           switchch,
           switchch,           
           switchch,
           switchch);
}

static void OnExit(void)
{
    CloseSectorCache();
}
