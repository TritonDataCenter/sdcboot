/*
   ovlimpl.c - overlay implementation for command line interface.
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
   email me at:  imre.leber@vub.ac.be
*/

#include <stdlib.h>
#include <stdio.h>

#include "fte.h"

#include "..\main\chkargs.h"

#include "..\..\modlgate\callback.h"
#include "..\..\modlgate\argvars.h"
#include "..\..\modlgate\expected.h"

static void UpdateInterfaceState(void){}

static void SmallMessage(char* buffer)
{
   if (GiveFullOutput())
   {
      printf(" %s\n", buffer);
   }
}

static void LargeMessage(char* buffer)
{
   printf("%s\n", buffer);     
}

static void DrawDriveMap(CLUSTER maxcluster)
{
   if (GiveFullOutput())
   {
      printf(" The disk has %lu clusters.\n", maxcluster);
   }
}

static void DrawOnDriveMap(CLUSTER cluster, int symbol)
{
#if 0
   if (GiveFullOutput())
      switch (symbol)
      {
         case WRITESYMBOL:
              printf("Write on cluster %d.\n", cluster);  
              break;

         case READSYMBOL:
              printf("Read from cluster %d.\n", cluster);
              break;
       
         case USEDSYMBOL:
              printf("Cluster %d is used.\n", cluster);
              break;   
            
         case UNUSEDSYMBOL:
              printf("Cluster %d is not used.\n", cluster);
              break;

         case BADSYMBOL:
              printf("Bad cluster at %d.\n", cluster);
              break;
        
         case UNMOVABLESYMBOL:
              printf("Cluster %d is not movable.\n", cluster);
              break;

         case OPTIMIZEDSYMBOL:
              printf("Cluster %d is optimized.\n", cluster);
              break;
      }
#else
      cluster = cluster;
      symbol  = symbol;
#endif
}

static void DrawMoreOnDriveMap(CLUSTER cluster, int symbol, unsigned long n)
{
#if 0
     switch (symbol)
     {
       case WRITESYMBOL:
            printf("Writing %lu clusters at %lu\n", n, cluster);
            break;

       case READSYMBOL:
       printf("Reading %lu clusters at %lu\n", n, cluster);            
            break;
       
       case USEDSYMBOL:
       printf("%lu clusters at %lu are used\n", n, cluster);            
            DrawUsedBlocks(cluster, n);
            break;   
            
       case UNUSEDSYMBOL:
       printf("%lu clusters at %lu are unused\n", n, cluster);                        
            break;

       case BADSYMBOL:
       printf("%lu clusters at %lu are bad\n", n, cluster);                                    
            break;
       
       case UNMOVABLESYMBOL:
       printf("%lu clusters at %lu are unmovable\n", n, cluster);                                    
            break;

       case OPTIMIZEDSYMBOL:
       printf("%lu clusters at %lu are optimized\n", n, cluster);                                    
            break;
     }
#else
     
     cluster = cluster;
     symbol  = symbol;
     n       = n;
     
#endif     
}

static void LogMessage(char* message)
{
   if (GiveFullOutput())
   {
      printf("LOG: %s\n", message);
   }
}

static int QuerySaveState(void)
{
   return FALSE;
}

static void IndicatePercentageDone(CLUSTER cluster, CLUSTER totalclusters)
{
   if (cluster);
   if (totalclusters);
}

static unsigned GetMaximumDrvMapBlock(void)
{
     return 0;
}

void CMDEFINT_GetCallbacks(struct CallBackStruct* result)
{
   result->UpdateInterface         = UpdateInterfaceState;
   result->SmallMessage            = SmallMessage;
   result->LargeMessage            = LargeMessage;
   result->DrawDriveMap            = DrawDriveMap;
   result->DrawOnDriveMap          = DrawOnDriveMap;
   result->LogMessage              = LogMessage;
   result->QuerySaveState          = QuerySaveState;
   result->IndicatePercentageDone  = IndicatePercentageDone;
   result->GetMaximumDrvMapBlock   = GetMaximumDrvMapBlock;
   result->DrawMoreOnDriveMap      = DrawMoreOnDriveMap; 
}
