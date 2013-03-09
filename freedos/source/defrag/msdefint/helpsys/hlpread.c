/*    
   Hlpread.c - help file reader.

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
   At the moment we don't support compression, a bit in the options byte
   (first byte in the help file indicates wether it is a compressed 
   help file).
   
   These routines are however structured to support per page compression
   as soon as we have a suitable algorithm.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hlpread.h"
#include "hlpparse.h"
#include "uncomp.h"
#include "..\helpkit\checksum.h"
#include "..\logman\logman.h"
#include "..\..\misc\bool.h"

static char HelpFile[512];

static int GetNumberOfHelpPages(void)
{
    FILE* fptr;
    static int NumberOfPages = 0;
    static int before = 0;
    int i;

    if (!before)
    {
       before = TRUE;

       if ((fptr = fopen(HelpFile, "rb")) != NULL)
       {
     fseek(fptr, 0, SEEK_SET);

     for (i=0; i < sizeof(int); i++)
         if ((*(((char*)(&NumberOfPages))+i) = fgetc(fptr)) == EOF)
       NumberOfPages = 0;

     fclose(fptr);
       }
       else
       {
     NumberOfPages = 0;
       }
   }
   return NumberOfPages;
}

static int GetIndexInformation(int index, unsigned long* offset,
                               int* compsize, int* uncompsize)
{
     FILE* fptr;
     unsigned long temp;
     int i;

     if ((fptr = fopen(HelpFile, "rb")) != NULL)
     {
   if (fseek(fptr, index * sizeof(unsigned long)*3+
        sizeof(int), SEEK_SET))
   {
      fclose(fptr);
      return FALSE;
   }

        for (i = 0; i < sizeof(unsigned long); i++)
      if ((*(((char*)(offset))+i) = fgetc(fptr)) == EOF)
      {
         fclose(fptr);
         return FALSE;
      }

        for (i = 0; i < sizeof(unsigned long); i++)
      if ((*(((char*)(&temp))+i) = fgetc(fptr)) == EOF)
      {
         fclose(fptr);
         return FALSE;
      }

        *compsize = (int) (temp - *offset);

        for (i = 0; i < sizeof(unsigned long); i++)
      if ((*(((char*)(&temp))+i)=fgetc(fptr)) == EOF)
      {
         fclose(fptr);
         return FALSE;
      }

        *uncompsize = (int) temp;
     }
     else
     {
   fclose(fptr);
   return FALSE;
     }

     fclose(fptr);
     return TRUE;
}

void CheckHelpFile(char* helpfile)
{
     FILE* fptr;
     int i;
     unsigned sum, csum;
     
     HelpFile[0] = '\0';

     if ((sum = CalculateCheckSum(helpfile)) == 0) return;
     if ((fptr = fopen(helpfile, "rb")) == NULL) return;

     fseek(fptr, -2, SEEK_END);

     for (i = 0; i < sizeof(unsigned); i++)
         if ((*(((char*)(&csum))+i)=fgetc(fptr)) == EOF)
         {
            fclose(fptr);
            return;
    }

     fclose(fptr);

{
char buffer[50];
sprintf(buffer, "csum: %d", csum);     
LogPrint(buffer);

sprintf(buffer, "sum: %d", sum);

LogPrint(buffer);
 }    
     if (csum == sum)
        strcpy(HelpFile, helpfile);
     else
     {
        LogPrint("Invalid check sum");
     }
}

int ReadHelpFile(int index)
{
    FILE* fptr;
    unsigned long offset;
    int complen, uncomplen;
    int c, result, read = 0, numpages;

    unsigned char *CompData, *UncompData, *pos;
      
    /* Free any memory from a previous help file */
    FreeHelpSysData();

    /* Return if the help file was corrupted. */
    if (HelpFile[0] == '\0')
       return HELPREADERROR;

    /* Get the number of pages in the help file, and check wether
       the asked page exists. */
    numpages = GetNumberOfHelpPages();
    if ((numpages == 0) || (index >= numpages))
    {
       LogPrint("Invalid index");
       return HELPREADERROR;
    }

    /* Get the positions in the help file where to read
       the file, the number of bytes to read and the number of
       bytes to reserve as the output. */
    if (!GetIndexInformation(index, &offset, &complen, &uncomplen))
    {
       LogPrint("Invalid index");
       return HELPREADERROR;
    }
       
    /* Try allocating the memory required. */
    CompData   = (unsigned char*) malloc(complen);
    if (!CompData)
       return HELPMEMINSUFFICIENT;

    UncompData = (unsigned char*) malloc(uncomplen);
    if (!UncompData)
    {
       free(CompData);
       return HELPMEMINSUFFICIENT;
    }
 
    /* Read the file. */
    if ((fptr = fopen(HelpFile, "rb")) != NULL)
    {
       fseek(fptr, offset, SEEK_SET);

       pos = CompData;
       while ((c = fgetc(fptr)) != EOF)
       {
             *pos++ = (char) c;
             read++;
        if (read == complen) break;
       }
       fclose(fptr);
    }
    else
    {
       free(CompData);
       free(UncompData);
       return HELPREADERROR;
    }

    if (read != complen)
    {
       free(CompData);
       free(UncompData);
       return HELPREADERROR;
    }
    UncompressBuffer(CompData, UncompData, complen, uncomplen);

    free(CompData);          /* Don't need the compressed data any more. */

    result = ParseHelpFile((char*) UncompData, uncomplen);
    
    free(UncompData);        /* Don't need the raw text data any more.   */

    return result;
}
