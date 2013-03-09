/***********************************************************/
/* UTF-8 to DOS Codepage Converter                         */
/* Version 1.0                                             */
/***********************************************************/
/* Copyright (C) 2004 Robert Platt <worldwiderob@yahoo.co.uk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <string.h> /* strnicmp */
#include <dos.h>   /* int86 */
#include "cpcodes.h"
#include "utfcp.h"

struct codepageEntryStruct *cpTable1 = 0;
struct codepageEntryStruct *cpTable2 = 0;

int selectCodepage(int codepage)
{
   union REGS regs;

   if (codepage == 0) /* detect */
   {
      codepage = 437; /* if display not set up, assume US English 437 */

      regs.x.ax = 0xAD00;
      int86(0x2F, &regs, &regs);
      if (regs.h.al == 0xFF)
      {
         regs.x.ax = 0xAD02;
         regs.x.bx = 0xFFFF;
         int86(0x2F, &regs, &regs);
         if (!regs.x.cflag)
            codepage = regs.x.bx;
      }
   }

   switch (codepage)
   {
      case 437: /* English (US) */
      cpTable1 = Unicode_CP437;
      break;

      case 737: /* Greek-2 */
      cpTable1 = Unicode_CP737;
      break;

      case 808: /* Cyrillic-2, euro instead of curren */
      cpTable1 = Unicode_CP866;
      cpTable2 = Unicode_CP808x;
      break;

      case 850: /* Latin-1, dotless i */
      cpTable1 = Unicode_CP850;
      cpTable2 = Unicode_CP850x;
      break;

      case 851: /* Greek */
      cpTable1 = Unicode_CP851;
      cpTable2 = Unicode_CP851x;
      break;

      case 852: /* Latin-2 */
      cpTable1 = Unicode_CP852;
      break;

      case 853: /* Latin-3 */
      cpTable1 = Unicode_CP853;
      break;

      case 855: /* Cyrillic-1 */
      cpTable1 = Unicode_CP855;
      cpTable2 = Unicode_CP855x;
      break;

      case 857: /* Latin-5 */
      cpTable1 = Unicode_CP857;
      break;

      case 858: /* Latin-1, euro instead of dotless i */
      cpTable1 = Unicode_CP850;
      cpTable2 = Unicode_CP858x;
      break;

      case 860: /* Portuguese */
      cpTable1 = Unicode_CP860;
      break;

      case 861: /* Icelandic */
      cpTable1 = Unicode_CP861;
      break;

      case 863: /* Canadian and French */
      cpTable1 = Unicode_CP863;
      break;

      case 865: /* Nordic */
      cpTable1 = Unicode_CP865;
      break;

      case 866: /* Cyrillic-2 */
      cpTable1 = Unicode_CP866;
      cpTable2 = Unicode_CP866x;
      break;

      case 869: /* Greek, with Euro */
      cpTable1 = Unicode_CP851;
      cpTable2 = Unicode_CP869x;
      break;

      case 872: /* Cyrillic-1, euro instead of curren */
      cpTable1 = Unicode_CP855;
      cpTable2 = Unicode_CP872x;
      break;

      default: /* not supported */
      codepage = 0;
      break;
   }

   return codepage;
}


int UTF8ToCodepage (unsigned char *string)
{
   unsigned char *r, *w;
   unsigned long unicode;

   r = string;
   w = string;

   while (*r != '\0')
   {
      if (*r > 0x7F)
      {
         if ((*r >= 0xC0) && (*r <= 0xDF)) /* 2 octets */
         {
            unicode = (*r & 0x1F) * 0x40UL;

            r++;
            if ((*r >= 0x80) && (*r <= 0xBF))
               unicode += (*r & 0x3F);
            else
               return 0;

            if ((unicode < 0x80) || (unicode > 0x07FF))
               return 0;

            *w = unicodeToCodepage(unicode);
         }
         else if ((*r >= 0xE0) && (*r <= 0xEF)) /* 3 octets */
         {
            unicode = (*r & 0xF) * 0x1000UL;

            r++;
            if ((*r >= 0x80) && (*r <= 0xBF))
               unicode += (*r & 0x3F) * 0x40UL;
            else
               return 0;

            r++;
            if ((*r >= 0x80) && (*r <= 0xBF))
               unicode += (*r & 0x3F);
            else
               return 0;

            if ((unicode < 0x0800) || (unicode > 0xFFFF))
               return 0;

            *w = unicodeToCodepage(unicode);
         }
         else if ((*r >= 0xF0) && (*r <= 0xF7)) /* 4 octets */
         {
            unicode = (*r & 0x7) * 0x40000UL;

            r++;
            if ((*r >= 0x80) && (*r <= 0xBF))
               unicode += (*r & 0x3F) * 0x1000UL;
            else
               return 0;

            r++;
            if ((*r >= 0x80) && (*r <= 0xBF))
               unicode += (*r & 0x3F) * 0x40UL;
            else
               return 0;

            r++;
            if ((*r >= 0x80) && (*r <= 0xBF))
               unicode += (*r & 0x3F);
            else
               return 0;

            if ((unicode < 0x10000) || (unicode > 0x10FFFF))
               return 0;

            *w = unicodeToCodepage(unicode);
         }
         else
            return 0;
      }
      else
         *w = *r;

      r++;
      w++;
   }

   *w = '\0';

   return 1;
}

unsigned char unicodeToCodepage(unsigned long unicode)
{
   int i;

   if (cpTable1)
   {
      for (i = 0; cpTable1[i].cp != 0; i++)
      {
         if (cpTable1[i].unicode == unicode)
            return cpTable1[i].cp;
      }
   }

   if (cpTable2)
   {
      for (i = 0; cpTable2[i].cp != 0; i++)
      {
         if (cpTable2[i].unicode == unicode)
            return cpTable2[i].cp;
      }
   }

   return '?';
}
