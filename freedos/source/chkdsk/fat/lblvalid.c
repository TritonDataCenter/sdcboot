/*
   LblValid.c - label validity checking.
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

#include "stdio.h"

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\struct\FstTrMap.h"
#include "..\errmsgs\errmsgs.h"

static BOOL ValidityChecker(RDWRHandle handle, CLUSTER label,
                            SECTOR datasector, void** structure);

struct Pipe
{
   BOOL invalidfound;     
   BOOL fixit;
};                          

/*=============================== CHECKING ==============================*/

/*************************************************************************
**                           CheckFatLabelValidity
**************************************************************************
** Searches trough the FAT and finds out wether there are any invalid 
** labels.
***************************************************************************/

RETVAL CheckFatLabelValidity(RDWRHandle handle)
{
   struct Pipe pipe, *ppipe = &pipe;
   
   pipe.invalidfound = FALSE;
   pipe.fixit        = FALSE;

   if (!LinearTraverseFat(handle, ValidityChecker, (void**) &ppipe))
      return ERROR;

   return (pipe.invalidfound) ? FAILED : SUCCESS;
}

/*=============================== FIXING ==============================*/

/*************************************************************************
**                           MakeFatLabelsValid
**************************************************************************
** Searches trough the FAT and finds out wether there are any invalid 
** labels, if there are they are changed to last labels.
***************************************************************************/

RETVAL MakeFatLabelsValid(RDWRHandle handle)
{
   struct Pipe pipe, *ppipe = &pipe;
   
   pipe.invalidfound = FALSE;
   pipe.fixit        = TRUE;

   if (!LinearTraverseFat(handle, ValidityChecker, (void**) &ppipe))
      return ERROR;
    
   return SUCCESS;
}

/*================================ COMMON ===============================*/

/*************************************************************************
**                           ValidityChecker
**************************************************************************
** Looks wether the given label is valid.
***************************************************************************/

static BOOL ValidityChecker(RDWRHandle handle, CLUSTER label,
                            SECTOR datasector, void** structure)
{
   char message[80], *p;
   CLUSTER thisCluster;
   struct Pipe* pipe = *((struct Pipe**) structure);
   
   if (!IsLabelValid(handle, label))
   {
      thisCluster = DataSectorToCluster(handle, datasector);
      if (!thisCluster) return FAIL;          
      
      sprintf(message, "Invalid cluster found at: %lu\n", thisCluster);
      ReportFileSysError(message, 0, &p, 0, FALSE);
      
      pipe->invalidfound = TRUE;
      
      if (pipe->fixit)
      {    
         if (!WriteFatLabel(handle, thisCluster, FAT_LAST_LABEL))
            return FAIL;
      }
   }

   return TRUE;
}