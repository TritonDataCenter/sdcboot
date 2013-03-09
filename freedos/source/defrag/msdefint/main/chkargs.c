/*    
   Chkargs.c - command line argument parsing.

   Copyright (C) 2000 Imre Leber

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

/*
   Remember:

            defrag [<drive>:] [{/F|/U}] [/Sorder[-]] [/B] [/X]

            with:
                 order = {N|       # name
                          D|       # date & time
                          E|       # extension
                          S}       # size
                 
                 - suffix to sort in descending order

                 F:  full optimization.
		 U:  unfragment files only.
		 FD: Directories together with files.
		 FF: Files first.
		 DF: Directories first.
                 Q:  Quick try.
                 CQ: complete quick try.

                 B: reboot after optimization.
                 X: automatically exit.

            No-Ops: 
            
                 /SKIPHIGH 
                 /LCD      
                 /BW       
                 /G0       
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "..\..\modlgate\defrpars.h"
#include "..\..\modlgate\argvars.h"
#include "..\..\misc\bool.h"

void BadOption(char* option);
void WarnOnNoOp(char* option);
void ShowError(char* msg);

void ParseInteractiveArguments (int argc, char** argv, char optchar)
{
     int  SortOrder = ASCENDING, SortCriterium = UNSORTED;
     int  Method = FULL_OPTIMIZATION;
     int  index = 1;

     if (argc > 1)           /* If there where any arguments. */
     {
        /* See wether argv[1] is a drive specification. */
        if ((strlen(argv[1]) == 2) && (argv[1][1] == ':'))
        {
           SetParsedDrive(argv[1][0]);
           index++;
        }

        for (; index < argc; index++)
        {
            if ((argv[index][0] != optchar) && (argv[index][0] != '/'))
               ShowError("Invalid input");
        
            switch(toupper(argv[index][1]))
            {
               case '\0': 
			  BadOption(argv[index]);
			  break;

	       case 'D':
			 if ((toupper(argv[index][2]) == 'F') &&
			     (argv[index][3] == '\0'))
			 {
			    Method = DIRECTORIES_FIRST;
			    MethodIsEntered();
			 }
			 else
			 {
			    BadOption(argv[index]);
			 }
			 break;

	       case 'F':
			 switch (toupper(argv[index][2]))
			 {
			   case '\0':
				Method = FULL_OPTIMIZATION;
				break;
			   case 'F':
				if (argv[index][3] != '\0')
				   BadOption(argv[index]);

				Method = FILES_FIRST;
				break;
			   case 'D':
				if (argv[index][3] != '\0')
				   BadOption(argv[index]);

				Method = DIRECTORIES_FILES;
				break;
			   default:
				BadOption(argv[index]);
			 }
			 MethodIsEntered();
			 break;

               case 'U':
                         if (argv[index][2] == '\0')
                         {
                            Method = UNFRAGMENT_FILES;
                            MethodIsEntered();
                         }
                         else
			    BadOption(argv[index]);
                         break;

               case 'S':
                         switch(toupper(argv[index][2]))
                         {
                           case '\0':
                                ShowError("Invalid sort criterium");
                                break;

                           case 'N':
                                SortCriterium = NAMESORTED;
                                break; 

                           case 'D':
                                SortCriterium = DATETIMESORTED;
                                break; 

                           case 'E':
                                SortCriterium = EXTENSIONSORTED;
                                break; 

                           case 'S':
                                SortCriterium = SIZESORTED;
                                break; 
                           
                           default: 
                                ShowError("Invalid sort criterium");
                         }
            
                         switch(argv[index][3])
                         {
                           case '\0':
                                SortOrder = ASCENDING;
                                break;

                           case '-':
                                if (argv[index][4] == '\0')
                                   SortOrder = DESCENDING;
                                else
				   BadOption(argv[index]);
                                break;
                           
                           default:
                                ShowError("Invalid sort criterium");
                         }
                         break;

               case 'C':
                        switch(toupper(argv[index][2]))
                        {
                        case 'Q':
                            switch(toupper(argv[index][3]))
                            {
                            case '\0':
                                Method = COMPLETE_QUICK_TRY;
                                MethodIsEntered();
                                break;

                            default:
                                BadOption(argv[index]);
                            }
                            break;
                        default:
                            BadOption(argv[index]);
                        }
                        break;
                         
               case 'B':
                         if (argv[index][2] == 0)
                            RequestReboot();
                         else
			    BadOption(argv[index]);
                         break;

               case 'X':
                         if (argv[index][2] == 0)
                            AutomaticallyExit();
                         else
			    BadOption(argv[index]);
                         break;
               
               case 'Q':
                        switch(toupper(argv[index][2]))
                        {
                        case '\0':
                            Method = QUICK_TRY;
                            MethodIsEntered();
                            break;
                        default:
                            BadOption(argv[index]);
                        }
                        break;

               default: 
                        if ((stricmp(&argv[index][1], "SKIPHIGH") == 0) ||
                            (stricmp(&argv[index][1], "LCD") == 0)      ||
                            (stricmp(&argv[index][1], "BW") == 0)       ||    
                            (stricmp(&argv[index][1], "G0") == 0))
                           WarnOnNoOp(argv[index]);
                        else
			   BadOption(argv[index]);
            }
        }
     }

     SetOptimizationMethod(Method);
     SetSortOptions(SortCriterium, SortOrder);
}