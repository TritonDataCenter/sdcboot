/* Simple counter by Eric Auer - public domain. 11/2002 */
/* COUNT3 version modifies environment of caller of caller of caller */
/* COUNT2 version modifies environment of caller of caller */
/* COUNT version modifies environment of caller */

#include <stdlib.h> /* getenv, putenv, exit */
#include <string.h> /* strlen */
#include <stdio.h>  /* sprintf, printf */
#include <dos.h>    /* MK_FP, _psp */

/* compile with a memory model like Huge that uses FAR data */
#define DEBUG 1

int main(int argc, char ** argv)
/* sigh, no warning for swapped args of main() by Turbo C 2... */
{
  long int count;
  int varlen;
  char buffer[32];
  const char far * var;
  const char far * envp;
  char format[32];
  long int max[8] = { 10, 100, 1000, 10000,
		      100000L, 1000000L, 10000000L, 100000000L };
  int retval;
  typedef unsigned int word;
  word far * wordp; /* sigh, forgot that this must be far */

  wordp = (word far *)MK_FP(_psp, 0x16);
  /* points to a word that points to parentpsp now */
#ifdef DEBUG
  printf("  [COUNT3: %Fp->", wordp);
#endif
  wordp = (word far *)MK_FP(wordp[0], 0x16);
#ifdef DEBUG
  printf("%Fp->", wordp);
#endif
  wordp = (word far *)MK_FP(wordp[0], 0x16);
#ifdef DEBUG
  printf("%Fp->ppPSP at %4.4x:0]\n\n", wordp, wordp[0]);
#endif
  wordp = (word far *)MK_FP(wordp[0], 0x2c);
  envp = (const char far *)MK_FP(wordp[0], 0);
  /* environ would be array of strings, wrong type, sigh... */
  /* so we cannot make putenv() write to the parent environment */

  retval = 0;
  do
  {
    var = strstr(envp, "COUNT=");
    if (var == NULL)
    {
      envp = &envp[ (int)strlen(envp) + 1 ]; /* skip to next string */
      if (envp[0] == 0)
      {
	printf("COUNT environment variable not found, EOF at %Fp\n",
	       envp);
	var = NULL;
	retval = 2;
      }
    }
  } while ((var == NULL) && (retval == 0));

  if (var != NULL)
  {
    var += strlen("COUNT=");
    varlen = (int)strlen(var);
    if ((varlen < 1) || (varlen > 8))
    {
      printf("COUNT environment variable must be 1..8 characters long\n");
      printf("found variable of length %d only (at %Fp)\n", varlen, var);
      retval = 2;
    }
  }

  if ((var == NULL) || (retval != 0) || (argc != 1))
  {

    if (argc != 1) retval = 1;
    printf("Usage: %s\nIncrements environment variable COUNT, which must\n",
	   (argc > 0) ? argv[0] : "count");
    printf("exist and is not changed in length by this tool.\n\n");
    printf("This tool is public domain software,\n");
    printf("written by Eric Auer, eric@coli.uni-sb.de in November 2002\n");
    printf("Please keep it bundled with its C source code!\n\n");
    printf("Example use:\nset COUNT=0000\n:loop\n");
    printf("if not exist file%%COUNT%%.txt ");
    printf("copy template.txt file%%COUNT%%.txt\ncount > nul\n");
    printf("if not x%%COUNT%%==x0042 goto loop\n");
    exit(retval);
  }

  format[0] = 0;
  sprintf(format,"%%0%d.%dld",varlen,varlen);
  /* the above means 0-padded, varlen..varlen chars precision long int */

  /* printf("Formatting: %s\n",format); */

  count = atol(var); /* 0 on error, but we do not care */
  count = (count < 0) ? 1 : count + 1; /* increment */
  if (count == max[varlen-1])
  {
    printf("Warning: COUNT wrapped to 0, avoiding %ld\n", max[varlen-1]);
    count = 0;
    retval = 2;
  }

  printf("COUNT (at %Fp in *parent* env.) was %s ", var, var);
  buffer[0] = 0; /* start with 0 length */
  if (sprintf(buffer,format,count) == varlen)
  {
    strncpy((char far *)var, buffer, varlen);
    printf("and is now %s\n", var);

  } else {
     printf("invalid value %s not stored\n", buffer);
     exit(1);
  }

  exit(retval);
}
