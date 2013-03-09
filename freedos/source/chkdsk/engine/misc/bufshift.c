/*    
   Bufshift.c - routines to manipulate buffers.

   Copyright (C) 2000, 2002 Imre Leber

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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void RotateBufRight(char* begin, char* end, int n)
{
   int i;     
   char temp;
   char* buf;
    
   assert(begin && end);
   
   if (!n) return;
   
   buf = (n == 1) ? 0:(char*) malloc(n);
   if (buf)
   {
      memcpy(buf,end-n,n);
      memmove(begin+n, begin, (size_t) (end-begin)-n);
      memcpy(begin,buf,n);
      free(buf);
   }        
   else
   {
      for (i = 0; i < n; i++)
      {
          temp = *(end-1);
	  memmove(begin+1, begin, (size_t) (end-begin)-1);
          *begin = temp;
      }
   }
}

void RotateBufLeft(char* begin, char* end, int n)
{
   int i;     
   char temp;
   char* buf;     
   
   assert(begin && end);
    
   if (!n) return;
   buf = (n == 1)? 0:(char*) malloc(n);
   if (buf)
   {
      memmove(buf,begin,n);
      memmove(begin, begin+n, (size_t)(end-begin)-n);
      memmove(end-n,buf,n);
      free(buf);
   }        
   else
   {
      for (i = 0; i < n; i++)
      {
          temp = *begin;
	  memmove(begin, begin+1, (size_t) (end-begin)-1);
          *(end-1) = temp;    
      }
   }
}

void SwapBuffer(char* p1, char* p2, unsigned len)
{
   char temp;
   char* buf;
   unsigned i;

   assert(p1 && p2); 
    
   if (!len) return;
   
   buf = (len == 1) ? 0 : (char*) malloc(len);
   if (buf)
   {
      memcpy(buf, p1, len);
      memmove(p1, p2, len);
      memcpy(p2, buf, len);
      free(buf);
   }
   else 
   {
      for (i=0; i < len; i++)
      {
         temp  = p1[i];   
         p1[i] = p2[i];
         p2[i] = temp;
     } 
  }
}

static void SlowSwapBufferParts(char* begin1, char* end1, 
                                char* begin2, char* end2)
{
   unsigned len1, len2, difference;
   
   assert(begin1 && end1 && begin2 && end2); 
    
   len1 = (unsigned)(end1-begin1);
   len2 = (unsigned)(end2-begin2);
      
   if (len1 < len2)
   {
      difference = len2-len1;
      RotateBufRight(end1, begin2+difference, difference);
      SwapBuffer(begin1, begin2+difference, len1);
      RotateBufRight(begin1, end1+difference, difference); 
   }
   else if (len1 > len2)
   {
      difference = len1-len2;
      RotateBufLeft(end1-difference, begin2, difference);
      SwapBuffer(begin1, begin2, len2);
      RotateBufLeft(begin2-difference, end2, difference); 
   }
   else /* len1 == len2 */
   {
      SwapBuffer(begin1, begin2, len1);     
   }
}

void SwapBufferParts(char* begin1, char* end1, char* begin2, char* end2)
{
   char* buf = (char*) malloc((size_t)(end2-begin1)), *buf1 = buf;
    
   assert(begin1 && end1 && begin2 && end2);    
    
   if (buf)
   {
      memcpy(buf, begin2, (size_t)(end2-begin2));
      buf += (size_t)(end2-begin2);
      memcpy(buf, end1, (size_t)(begin2-end1));
      buf += (size_t)(begin2-end1);
      memcpy(buf, begin1, (size_t)(end1-begin1));
      memcpy(begin1, buf1, (size_t) (end2-begin1));
      free(buf1);
   }
   else
   {
      SlowSwapBufferParts(begin1, end1, begin2, end2);
   }
}                               
      
#ifdef TEST_SHIFTBUF

#include <time.h>

int main()
{
    int i, j;
    long k;
    char buffer[] = {'0', '1','2','3','4','5','6','7','8','9'};

    time_t t1, t2;

    clrscr();

    time (&t1);

    for (i = 0; i < 10; i++)
	printf("%c", buffer[i]);
    puts("");

    for (j = 0; j < 10; j++)
    {
	RotateBufRight(buffer, &buffer[10], j);
	for (i = 0; i < 10; i++)
	    printf("%c", buffer[i]);
	puts("");
    }

    
    time(&t2);
    printf("\n%f\n", difftime(t2, t1));

    getch();puts("\n\n");
    
    clrscr();

    
    for (j = 0; j < 10; j++)
    {
	RotateBufLeft(buffer, &buffer[10], j);
        for (i = 0; i < 10; i++)
            printf("%c", buffer[i]);
        puts("");
    }
  
    puts("\n\n");
  
    SwapBuffer(&buffer[0], &buffer[5], 5);
    for (i = 0; i < 10; i++)
            printf("%c", buffer[i]);
        puts("");
   puts("\n\n");
   
   SwapBuffer(&buffer[0], &buffer[5], 5);
   for (i = 0; i < 10; i++)
            printf("%c", buffer[i]);
        puts("");
        puts("\n\n");
  
   RotateBufLeft(&buffer[0], &buffer[10], 2);

 
   
   SlowSwapBufferParts(&buffer[1],  &buffer[5], &buffer[7], &buffer[9]);
   for (i = 0; i < 10; i++)
            printf("%c", buffer[i]);
        puts("");
      
   SwapBufferParts(&buffer[1],  &buffer[5], &buffer[7], &buffer[9]);
   for (i = 0; i < 10; i++)
            printf("%c", buffer[i]);
        puts("");

    
}

#endif
