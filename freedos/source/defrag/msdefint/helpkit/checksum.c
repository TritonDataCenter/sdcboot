#include <stdio.h>

#include "checksum.h"

static unsigned CalculateCS(int c, unsigned previous)
{
    int i;
    unsigned sum;

    sum = (previous ^ c) << 8;

    for (i = 0; i < 8; i++)
    {
	if (sum & 0x8000)
	   sum = (sum << 1) ^ 0x1021;
	else
	   sum = sum << 1;
    }

    return sum;
}

#ifdef CALCULATION_PROG

/*
** Calculate checksum for entire file.
*/

unsigned CalculateCheckSum(char* file)
{
    FILE* fptr;
    int sum=0, c;

    fptr = fopen(file, "rb");
    if (!fptr)
       return 0;

    while ((c = fgetc(fptr)) != EOF)
	  sum = CalculateCS(c, sum);

    fclose(fptr);

    if (sum == 0)
       return 1;
    else
       return sum;
}

#endif

/*
** Calculate checksum for all but the two last characters.
*/

#ifdef RECALCULATION_PROG

unsigned CalculateCheckSum(char* file)
{
    FILE* fptr;
    int sum=0, back1=0, back2, c;

    fptr = fopen(file, "rb");
    if (!fptr)
       return 0;

    while ((c = fgetc(fptr)) != EOF)
    {
	  back2 = back1;
	  back1 = sum;
	  sum = CalculateCS(c, sum);
    }

    fclose(fptr);

    if (back2 == 0)
       return 1;
    else
       return back2;
}

#endif