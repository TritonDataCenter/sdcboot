/* reset.c */

/* simple program to reset & clear the screen, useful if aborting the
   install program during testing and need to set colors back. */

/* Jim Hall <jhall@freedos.org> */

/* THIS PROGRAM IS TRIVIAL, AND IS IN THE PUBLIC DOMAIN */

#include <stdio.h>
#include <stdlib.h>

#include <graph.h>

#include "colors.h"

void
main (void)
{
  /* reset colors, and exit */

  _settextcolor (_WHITE_);
  _setbkcolor (_BLACK_);
  _clearscreen (_GCLEARSCREEN);

  exit (0);
}
