#include <conio.h>
#include <stdlib.h>
#include <string.h>

#include "..\screen\screen.h"
#include "hlpfncs.h"
#include "hlpread.h"
#include "hlpparse.h"
#include "hlplnprs.h"
#include "hlpsysid.h"
#include "hlplnprs.h"

#include "..\..\misc\bool.h"
#include "..\dialog\dialog.h"

#define HILINKFORCOLOR  WHITE
#define HILINKBACKCOLOR GREEN

#define LOLINKFORCOLOR  WHITE
#define LOLINKBACKCOLOR DIALOGBACKCOLOR

#define LOGGING

#ifdef LOGGING
#include <stdio.h>
#include "..\logman\logman.h"
#endif

static int FileReadSuccess;

struct LinkPosition CurrentLink = {0, 0};

static int GetLinkIndex(struct LinkPosition* pos)
{
   int linkcntr=0;
   char* linetxt = GetHelpLine(pos->line);

   while (*linetxt)
   {
      if (IsHlpReference(linetxt))
      {
         linkcntr++;
         if (linkcntr == pos->nr)
            return GetHlpLinkIndex(linetxt);
         else
            linetxt = PassHlpReferencePart(linetxt);
      }
      else if (IsFullLine(linetxt))
         linetxt = PassHlpFullLinePart(linetxt);
      else if (IsHlpAsciiChar(linetxt))
         linetxt = PassHlpAsciiChar(linetxt);
      else if (IsCenterOnLine(linetxt))
         linetxt = PassCenterOnLinePart(linetxt);
      else
         linetxt = PassNextHelpPart(linetxt);
   }

   return -1;
}

static int NextLinkPos(int line, int nr)
{
   char* linetxt;
   int   linkcntr = 0;

   linetxt = GetHelpLine(line);

   while (*linetxt)
   {
         if (IsHlpReference(linetxt))
         {
            if (linkcntr++ == nr)
               return linkcntr;
            else
               linetxt = PassHlpReferencePart(linetxt);
         }
         else if (IsFullLine(linetxt))
            linetxt = PassHlpFullLinePart(linetxt);
         else if (IsHlpAsciiChar(linetxt))
            linetxt = PassHlpAsciiChar(linetxt);
         else if (IsCenterOnLine(linetxt))
            linetxt = PassCenterOnLinePart(linetxt);
         else
            linetxt = PassNextHelpPart(linetxt);
   }

   return 0;
}

static int SeekLinkNr(int line, int charpos)
{
   int pos = -1;
   int linkcntr = 0;
   char* linetxt = GetHelpLine(line);

   while (*linetxt && (pos < charpos))
   {
       if (IsHlpReference(linetxt))
       {
          char *start, *end;
           
          GetHlpLinkCaption(linetxt, &start, &end);

          linkcntr++;
          pos += (int)(end - start);
          if (pos >= charpos)
             return linkcntr;

          linetxt = PassHlpReferencePart(linetxt);   
       }
       else if (IsFullLine(linetxt))
          linetxt = PassHlpFullLinePart(linetxt);
       else if (IsCenterOnLine(linetxt))
          linetxt = PassCenterOnLinePart(linetxt);
       else if (IsHlpAsciiChar(linetxt))
       {
          pos++;
          linetxt = PassHlpAsciiChar(linetxt);
       }
       else
       {
          char* next = PassNextHelpPart(linetxt);

          pos += (int)(next - linetxt);
          linetxt = next;
       }
   }
   return 0;
}

static void DrawHelpLineParts(int line, int x, int y, int xlen)
{
     int linkcntr = 0;
     char* linetxt = GetHelpLine(line);

     while (*linetxt)
     {
           if (IsHlpReference(linetxt))
           {
              char *start, *end, sentinel;
           
              GetHlpLinkCaption(linetxt, &start, &end);

              sentinel = *end;
              *end = 0;

              linkcntr++;
              if ((line == CurrentLink.line) &&
                  (CurrentLink.nr == linkcntr))
                 DrawText(x, y, start, HILINKFORCOLOR, HILINKBACKCOLOR);
              else
                 DrawText(x, y, start, LOLINKFORCOLOR, LOLINKBACKCOLOR);

              x += strlen(start);
              
              *end = sentinel;

              linetxt = PassHlpReferencePart(linetxt);
           }
           else if (IsFullLine(linetxt))
           {
              DrawSequence(x, y, xlen-1, 'Ä',
                           DIALOGFORCOLOR, DIALOGBACKCOLOR);

              linetxt = PassHlpFullLinePart(linetxt);
           }
           else if (IsHlpAsciiChar(linetxt))
           {
              DrawSequence(x, y, 1, GetHlpAsciiChar(linetxt),
                           DIALOGFORCOLOR, DIALOGBACKCOLOR);

              linetxt = PassHlpAsciiChar(linetxt);
              x++;
           }
           else if (IsCenterOnLine(linetxt))
           {
              int centerx;
              char *begin, *end, sentinel;
              
              GetHlpTextToCenter(linetxt, &begin, &end);
              
              centerx = x + ((xlen / 2) - ((int)(end - begin) / 2));
              sentinel = *end;
              *end = 0;  
                
              if (centerx > x)  
                 DrawText(centerx, y, begin, DIALOGFORCOLOR, DIALOGBACKCOLOR);
              else
                 DrawText(x, y, begin, DIALOGFORCOLOR, DIALOGBACKCOLOR);
              
              *end = sentinel;
              linetxt = PassCenterOnLinePart(linetxt);
           }
           else
           {
              char* next = PassNextHelpPart(linetxt);
              char sentinel = *next;

              *next = 0;

              DrawText(x, y, linetxt, DIALOGFORCOLOR, DIALOGBACKCOLOR);
              
              x += strlen(linetxt);
              
              *next = sentinel;
              linetxt = next;
           }
     }
}

void DrawHelpLine(int line, int x, int y, int xlen)
{
     DrawSequence(x, y, xlen, ' ',
                  DIALOGFORCOLOR, DIALOGBACKCOLOR);

     if (FileReadSuccess == HELPMEMINSUFFICIENT)
     {
        if (line == 0)
           DrawText(x, y, "Not enough memory to load memory page",
                    DIALOGFORCOLOR, DIALOGBACKCOLOR);
     }
     else if (FileReadSuccess == HELPREADERROR)
     {
        if (line == 0)
           DrawText(x, y, "Cannot read help page",
                    DIALOGFORCOLOR, DIALOGBACKCOLOR);
     }
     else
     {
        if (line < GetHelpLineCount())
           DrawHelpLineParts(line, x, y, xlen);
     }
}

int CheckHelpClick(int cx, int cy, int cxlen, int cylen, int msx, int msy,
                   int top)
{
    struct LinkPosition pos;

    if ((msx < cx)         ||
        (msy < cy)         ||
        (msx > cx + cxlen) ||
        (msy > cy + cylen))
      return -1;

    pos.line = top + msy - cy;
    pos.nr   = SeekLinkNr(pos.line, msx - cx);

    if (pos.nr)
    {
       CurrentLink.nr = 0;
       return GetLinkIndex(&pos);
    }
    else
       return -1;
}

int CheckHelpTab(int top, int ylen)
{
    int i, pos, start;

    if ((CurrentLink.nr == 0)    ||
        (CurrentLink.line < top) ||
        (CurrentLink.line >= top+ylen))
    {
       pos = 0;
       start = top;
    }
    else
    {
       pos = CurrentLink.nr;
       start = CurrentLink.line;
    }

    for (i = start; i < top+ylen; i++)
    {
        pos = NextLinkPos(i, pos);
        if (pos) break;
    }

    CurrentLink.line = i;
    CurrentLink.nr   = pos;

    if (pos)
       return CurrentLink.line;
    else
       return -1;
}

int CheckHelpEnter(int top, int ylen)
{
    if ((CurrentLink.nr == 0)          ||
        (CurrentLink.line < top)       ||
        (CurrentLink.line >= top+ylen))
       return -1;
    else
       return GetLinkIndex(&CurrentLink);
}

void SelectHelpPage(int index)
{
#ifdef LOGGING
    char indextxt[33];
    sprintf(indextxt, "Help page %d selected.", index);
    LogPrint(indextxt);
#endif

    RememberHelpIndex(index);
    FileReadSuccess = ReadHelpFile(GetHelpIndex());
    CurrentLink.nr = 0;
}

int PastEndOfHelp(int top, int ylen)
{
    if (top > GetLastHelpTop(ylen))
       return TRUE;
    else
       return FALSE;
}

int GetLastHelpTop(int ylen)
{
    int count;
    
    if (FileReadSuccess == HELPSUCCESS)
    {
       count = (int) GetHelpLineCount();
       if (count <= ylen)
          return 0;
       else
          return count - ylen;
    }
    else
       return 0;
}
