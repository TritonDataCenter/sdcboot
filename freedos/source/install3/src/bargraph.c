/* $Id: bargraph.c,v 1.2 2001/01/04 05:00:48 jhall1 Exp $ */

/* Copyright (C) 2000, Jim Hall <jhall@freedos.org> */

/*
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
*/


#include <stdio.h>
#ifdef __WATCOMC__
#include <screen.h>
#else
#include <conio.h>
#endif

#include "bargraph.h"

/* bargraph() - function that uses conio to create a bar of 'width'
   width to show progress of num/den. Returns 0 (false) if failure, or
   non-zero (true) if success */

int
bargraph (int num, int den, int width)
{
  int i;
  int n;

  /* compute how many 'filled' boxes to display */

  num = (num < 0 ? -num : num);
  den = (den < 0 ? -den : den);

  if (den == 0)
    {
      return (0);
    }

  n = (int) ((float) width * (float) num / (float) den);

  /* Print n many filled boxes */

  for (i = 0; i < n; i++)
    {
      putch (GRAPH_BOX_FILLED);
    }

  /* Now print width-n many empty boxes */

  for ( ; i < width; i++)
    {
      putch (GRAPH_BOX_EMPTY);
    }

  return (n);
}
