/* unzip.c */

/* fake Unzip - using Info-Zip's UzpMain API */

/* Jim Hall <jhall@freedos.org> */

/* THIS PROGRAM IS TRIVIAL, AND IS IN THE PUBLIC DOMAIN */

#include <stdio.h>
#include <stdlib.h>

#include <env.h>

int UzpMain ( int argc, char **argv );		/* from Info-Zip's Unzip */

int
main (int argc, char **argv)
{
  int ret;

  /* wrapper to UzpMain */

  /* set TZ env variable, if not already set */

  if (getenv("TZ") == NULL)
    {
      setenv ("TZ", "PST8PDT", 1);		/* env.h */
    }

  /* printf ("DEBUG: calling UzpMain() . . .\n"); */

  ret = UzpMain (argc, argv);
  exit (ret);
}
