/* choice.c */

/* Waits for the user to press a key, which must be in a list of
   choices */

/*
  Copyright (C) 1994--2002 Jim Hall, jhall@freedos.org

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

/* 
   changes 4.0 by tom ehlert, te@drivesnapshot.de
	
   removed getopt (huge filesize overhead)
   removed CATS (huge filesize overhead)
	
   filesize reduced 19K --> 5K
	
   bugs fixed:
   hangs on Ctrl-Break
   check for /C  only (no choices given)
   the TEXT argument doesn't need "'s nay longer
		
	
*/	




#include <stdio.h>
#include <stdlib.h>
#include <string.h>			/* strtok  */
#include <ctype.h>			/* toupper */
#include <dos.h>

#include "kitten.h"			/* catopen/catgets */
  


/* Symbolic constants */

/* #define DEBUG				   debugging   */

#define beep() putch('\a')			/* make a beep */
/* #define beep() printf("**beep**")		   make a beep */


/* Function prototypes */

void usage (nl_catd cat);



#define BIOS_TIME() *(unsigned long far *)MK_FP(0x40,0x6c)

void idle(void);
int direct_console_input(void);

#ifdef __WATCOMC__
#pragma aux direct_console_input =\
    "mov ah, 6"\
    "mov dl,0xff"\
    "int 0x21"\
    "mov ah,0" \
    "jnz charav"\
    "mov ax,-1"\
    "charav:"\
modify [dx] value [ax];

#pragma aux idle = \
    "mov ax,0x1680"\
    "int 0x2f"

#else

void idle(void)
{
  union REGS r;
  r.x.ax = 0x1680;		/* application idle */
  int86(0x2f,&r,&r);			
}

int direct_console_input(void)
{
#define ZERO_FLAG 0x40
  union REGS r;
  r.h.ah = 0x06;	/* direct console input */
  r.h.dl = 0xff;
  intdos(&r,&r);
  /*printf("al %x flags %x\n",r.h.al,r.x.flags);*/
  if (r.x.flags & ZERO_FLAG) /* no character available */
    return -1;
  return (int)r.h.al;
}
#endif

unsigned getch_with_timeout(int timeout_seconds)
{
  int key;
  unsigned long start = BIOS_TIME();


  for (;;)
    {
      key = direct_console_input();
      if (key == -1)
	{
	  if (timeout_seconds && (BIOS_TIME() - start) > (unsigned long)timeout_seconds*18)
	    return 0xffff;

			
	  idle();
	}         
      else {				/* we got a character */
			
	if (key == 0)	/* function key */
	  {
	    return (key << 8) | getch_with_timeout(timeout_seconds);
	  }
	return key;
      }	
    }
}
		
void putch(char c);
#ifdef __WATCOMC__
#pragma aux putch = \
    "mov ah, 2" \
    "int 0x21" \
    parm [dl]
#else
void putch(char c)
{
  union REGS r;
	
  r.h.ah = 0x02;	/* direct character output */
  r.h.dl =    c;
  intdos(&r,&r);  
}
#endif

int
main (int argc, char **argv)
{
  char *s;
  char *choices;			/* the list of choices       */

  int i, opt;
  unsigned key;				/* the key that was pressed */
  int def_key;				/* default key after wait   */
  int def_wait = 0;			/* default time to wait     */

  int opt_beep = 0;			/* do we make a beep?       */
  int show_choices = 1;			/* do we show choices?      */
  int case_sensitive = 0;		/* are we case sensitive?   */

  nl_catd cat;				/* language catalog         */

#ifdef DEBUG /* debugging */
  printf ("Begin\n");
#endif /* debugging */

  /* Open the message catalog */

  cat = catopen ("choice", 0);

  /* Scan command line */


  choices = catgets (cat, 3, 0, "yn");		/* set default choices */
  
  for (opt = 1; opt < argc; opt++)
    {
      char *argptr = argv[opt];
  	
      if (*argptr != '/' && *argptr != '-')
	{
	  break;
	}

      argptr++;
	  
      switch (toupper(*argptr))
	{
	case 'B': /* Beep */
	  opt_beep = 1;
	  break;

	case 'C': /* allowable chars */
	  choices = argptr+1;			/* pointer assignment */
	  if (choices[0] == ':')
	    {
	      choices++;
	    }
	  
	  if (*choices == 0)
	    {
	      printf("a valid choice must be given\n");
	      exit(0);
	    }  
	    
	  break;

	case 'N': /* do not display choices at end */
	  show_choices = 0;
	  break;

	case 'S': /* be case sensitive */
	  case_sensitive = 1;
	  break;

	case 'T': /* time to wait */
	  s = argptr+1;
	  if (*s == ':')
	    s++;

      if (s[1] != ',' || !isdigit(s[2]))
      	{
		  printf ("CHOICE:%s\n",catgets(cat,0,9,"Incorrect timeout syntax.  Expected form Tc,nn or T:c,nn"));
		  exit (0);
      	}
	  
	  def_key = s[0];
	  
	  s+=2;

	  /* Check that time to wait contains only numbers */

	  for (i = 0; s[i] != '\0'; i++)
	    {
	      if (!isdigit (s[i]))
		{
		  printf ("Time to wait is not a number [%s]\n",s+i);
		  exit (0);
		}
	    }

	  def_wait = atoi (s);

#ifdef DEBUG /* debugging */
	  printf ("/T triggered:\n");
	  printf ("def_wait=%d\n", def_wait);
	  printf ("def_key=%c\n", def_key);
#endif /* debugging */
	  break;

	default:
	  /* show usage, and quit with error */
	  
	  printf("unknown argument [%s]\n",argptr);
	
	case 'H':
	case '?':  
	  usage(cat);

	  exit (0);
	  break;
	} /* switch */
    } /* while */

  /* Make sure def_key is valid */
  /* Set def_key if not given */

  if (def_wait > 0)
    {
      if (!strchr (choices, def_key))
	{
		  printf ("CHOICE:%s\n",catgets(cat,0,10,"Timeout default not in specified (or default) choices."));
		  exit (0);
	}
    }
  else
    {
      def_key = choices[0];
    }

#ifdef DEBUG /* debugging */
  printf ("After fixup:\n");
  printf ("def_wait=%d\n", def_wait);
  printf ("def_key=%c\n", def_key);
#endif /* debugging */

  /* Display text */
  
  for (;opt < argc; opt++)
    {
      printf("%s%s",argv[opt], opt < argc-1 ? " " : "");
    }
  
  
  
  


  if (show_choices)
    {
      if (!case_sensitive)
	{
	  for (i = 0; choices[i]; i++)
	    {
	      /* Uppercase the choices */
	      choices[i] = toupper(choices[i]);
	    }
	}

      /* print the choices */
      
      printf("[");
      
      for (s = choices;;)
        {
      	printf("%c",*s);
      	s++;
      	if (*s == 0)
      		break;
      	printf(",");
      	}	
      
      printf("]?");

    }

  /* Make a beep */

  if (opt_beep)
    {
      beep();
    }

  /* Grab the key.  Don't exit until we found it. */

  for (; ; )
    { 
      key = getch_with_timeout(def_wait);
    
      if (key == 0xffff) /* no key detected after timeout */
    	{
	  key = def_key;
    	}
    
      for (i = 0; choices[i]; i++)
	{
	  if (key == choices[i])
	    {
	      printf ("%c\n",key);		/* force a new line */
	      exit (i+1);		/* exit starts counting at 1 */
	    }

	  if ((!case_sensitive) && (toupper(key) == toupper(choices[i])))
	    {
	      printf ("%c\n",toupper(key));		/* force a new line */
	      exit (i+1);		/* exit starts counting at 1 */
	    }
	} /* for */

      beep();
    } /* while */
  return 0;
}

void
usage (nl_catd cat)
{
  char *s1, *s2;
  
  cat = cat;

  s1 = catgets (cat, 0, 0, "Waits for the user to press a key, from a list of choices");
  printf ("CHOICE version 4.4, Copyright (C) 1994--2003 Jim Hall, jhall@freedos.org\n");
  printf ("%s\n", s1);

  s1 = catgets (cat, 1, 0, "Usage:");
  printf ("%s ", s1);

  s1 = catgets (cat, 0, 1, "choices");
  s2 = catgets (cat, 0, 2, "text");
  printf ("CHOICE [ /B ] [ /C[:]%s ] [ /N ] [ /S ] [ /T[:]c,nn ] [ %s ]\n", s1, s2);

  s2 = catgets (cat, 0, 8, "Sound an alert (beep) at prompt");
  printf ("  /B  -  %s\n", s2);

  s2 = catgets (cat, 0, 3, "Specifies allowable keys.  Default is:");
  printf ("  /C[:]%s  -  %s ", s1, s2);

  s2 = catgets (cat, 3, 0, "yn");
  printf ("%s\n", s2);

  s1 = catgets (cat, 0, 4, "Do not display the choices at end of prompt");
  printf ("  /N  -  %s\n", s1);

  s1 = catgets (cat, 0, 5, "Be case-sensitive");
  printf ("  /S  -  %s\n", s1);

  s1 = catgets (cat, 0, 6, "Automatically choose key c after nn seconds");
  printf ("  /T[:]c,nn  -  %s\n", s1);

  s1 = catgets (cat, 0, 7, "The text to display as a prompt");
  s2 = catgets (cat, 0, 2, "text");
  printf ("  %s  -  %s\n", s2, s1);
}
