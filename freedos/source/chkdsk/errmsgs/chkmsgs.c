/*
  chkmsgs.c - Show messages to the user
  Copyright (C) 2002 Imre Leber
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
  If you have any questions, comments, suggestions, or fixes please
  email me at:  imre.leber@worldonline.be  
  
*/

#include <stdio.h>

#include "..\misc\bool.h"

#include "errmsgs.h"

#define UNUSED(x) if (x)

/*************************************************************************
**                          ReportFileSysError
**************************************************************************
** Show a message to the user indicating a faulty file system.
**************************************************************************/

int ReportFileSysError(char* message,
                       unsigned numfixes,
                       char** fixmessages,
                       unsigned defaultfix,
                       BOOL allowignore)
{
    UNUSED(numfixes);
    UNUSED(fixmessages);
    UNUSED(allowignore);
   
    printf("%s\n", message);
    return defaultfix;
}

/*************************************************************************
**                        ReportFileSysWarning
**************************************************************************
** Show a message to the user indicating that something in the file system
** is not optimal.
**************************************************************************/

int ReportFileSysWarning(char* message,
                         unsigned numfixes,
                         char** fixmessages,
                         unsigned defaultfix,
                         BOOL allowignore)
{
    UNUSED(numfixes);
    UNUSED(fixmessages);
    UNUSED(allowignore);

    printf("%s\n", message);

    return defaultfix;
}

/*************************************************************************
**                        ReportGeneralRemark
**************************************************************************
** Show a general remark to the user.
**************************************************************************/

void ReportGeneralRemark(char* message)
{       
    printf("%s\n", message);
}

/*************************************************************************
**                        ReportFileName
**************************************************************************
** Reports a filename.
**************************************************************************/

void ReportFileName(char* file)
{
    printf("%s\n", file);
}