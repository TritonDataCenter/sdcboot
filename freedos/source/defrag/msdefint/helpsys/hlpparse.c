/*    
   Helpparse.c - help file parser.

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

#include <stdlib.h>
#include <string.h>

#include "hlpread.h"

static size_t AmofLines;

static char* EmptyString = "";

static char** HelpSysData = NULL;

static size_t CountLines(char* RawData, size_t bufsize)
{
    size_t count = 0, i = 0;

    while (i < bufsize)
    {
       if (RawData[i] == '\r') 
       {
           count++;
           if ((i+1 < bufsize) && (RawData[i+1] == '\n')) i++;
       }
       else if (RawData[i] == '\n')
	  count++;
       
       i++;
    }

    return count + 1;
}   
     
static char* GetNextLine(char* input, char** slot, int restinbuf)
{
    char* p = input;
    int len;
    
    while ((*p != '\r') && (*p != '\n') && restinbuf)
    {
          p++;
          restinbuf--;
    }

    len = (int)(p - input);

    if (len)
    {
       if ((*slot = (char*) malloc(len+1)) == NULL)
          return NULL;

       memcpy(*slot, input, (int)(p-input));
       *((*slot) + len) = '\0';   
    }
    else
       *slot = EmptyString;

    if (*(p+1) == '\n')
       return p+2;
    else
       return p+1;
}

int ParseHelpFile(char* RawData, size_t bufsize)
{
    int i, j;
    char* input = RawData;

    AmofLines = CountLines(RawData, bufsize);

    if ((HelpSysData = (char**) malloc(AmofLines * sizeof(char*))) == NULL)
       return HELPMEMINSUFFICIENT;

    for (i = 0; i < AmofLines; i++)
    {
        input = GetNextLine(input, &HelpSysData[i], (int)(bufsize - (input - RawData)));

        if (!input)
        {
           for (j = 0; j < i; j++)
               free(HelpSysData[j]);
           free(HelpSysData);
           HelpSysData=0;
           return HELPMEMINSUFFICIENT;
        }
    }
   return HELPSUCCESS;
}

size_t GetHelpLineCount()
{
    return AmofLines;
}

char* GetHelpLine(int line)
{
    return HelpSysData[line];
}

void FreeHelpSysData()
{
    int i;

    if (HelpSysData)
    {
       for (i = 0; i < AmofLines; i++)
       {
           if (HelpSysData[i] != EmptyString)
              free(HelpSysData[i]);
       }
       free(HelpSysData);
    }
    HelpSysData = NULL;
}
