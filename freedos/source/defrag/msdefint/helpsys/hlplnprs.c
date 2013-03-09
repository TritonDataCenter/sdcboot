#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "..\..\misc\bool.h"

/*
   Language:
           $(text)[id]:         reference to an other page
           $(-):                full line 
           $\xx:                ascii character xx (hex)
           $[CENTER](text)      Centers the text on the line

*/

int IsHlpReference(char* line)
{
    char *pos, *pos1;

    if (strncmp(line, "$(", 2) != 0)
       return FALSE;

    pos = strchr(line, ')');

    while (pos && (*(pos+1) != '['))
       pos = strchr(pos+1, ')');

    if (!pos)
       return FALSE;

    pos1 = strchr(pos, ']');
    if (!pos1)
       return FALSE;

    pos+=2;
    while ((pos != pos1) && isdigit(*pos)) pos++;

    return (pos == pos1);
}

int IsFullLine(char* line)
{
    return strncmp(line, "$(-)", 4) == 0;
}

int IsHlpAsciiChar(char* line)
{
    if ((strncmp(line, "$\\", 2) == 0) &&
        (isxdigit(line[2]))            &&
        (isxdigit(line[3])))
       return TRUE;
    else
       return FALSE;
}

int IsCenterOnLine(char* linetxt)
{
   char* pos;
   
   if (strncmpi(linetxt, "$[CENTER](", 10) == 0)
   {
        pos = strchr(linetxt, ')');   
        if (!pos) 
           return FALSE;
        else
           return TRUE;     
   }
   
   return FALSE;
}

char* PassHlpReferencePart(char* line)
{
    char* pos = strchr(line, ')');

    while (*(pos+1) != '[')
	  pos = strchr(pos+1, ')');

    return strchr(pos, ']') + 1;
}

char* PassHlpFullLinePart(char* line)
{
    return strchr(line, (int)NULL);
}

char* PassHlpAsciiChar(char* line)
{
    return line+4; /* 4 == strlen("$\41") */  
}

char* PassCenterOnLinePart(char* line)
{
    return strchr(line, (int)NULL);
}

char* PassNextHelpPart(char* line)
{
    char* pos;

    if (line[0] == '$')
       return line+1;

    pos = strchr(line, '$');
    if (pos)
       return pos;
    else
       return strchr(line, (int)NULL); /* Return pointer to end of string */
}

void GetHlpLinkCaption(char* line, char** start, char** end)
{
   *start = line+2;
   *end = strchr(*start, ')');
   while (*((*end)+1) != '[')
	 *end = strchr((*end)+1, ')');
}

int GetHlpLinkIndex(char* line)
{
   char* pos = strchr(line, ')');
   while (*(pos+1) != '[')
         pos = strchr(pos, ')');

   return atoi(pos+2);
}

char GetHlpAsciiChar(char* line)
{
   int  i;
   char num, result=0, radix = 16;

   for (i = 2; i <= 3; i++)
   {
       num = line[i];

       if (isdigit(num))
          result += (num-'0')*radix;
       else if isupper(num)
          result += (num - ('A' - '0') + 10) * radix;
       else
          result += (num - ('a' - '0') + 10) * radix;

       radix = 1;
   }

   return result;
}

void GetHlpTextToCenter(char* line, char** begin, char** end)
{
   char *pos, *chaser;
   
   *begin = line + 10;
   pos = strchr(*begin, ')');
   while (pos)
   {
     chaser = pos;
     pos = strchr(pos+1, ')');
   }
   
   *end = chaser;
}

