/*  FreeDOS HTML Help Viewer

    HELP - main module

    Copyright (c) Express Software 1998-2003
    See doc\htmlhelp\copying for licensing terms
    Created by: Joseph Cosentino.

    See doc\htmlhelp\history for a chronology of changes
*/

/* D E F I N E S ***********************************************************/

/* This directive is here so that only this file will declare certain
 variables in help.h. Other files that include help.h will extern those
 variables. Hence this is defined before including help.h */
#define HTML_HELP

/* Something similar with conio.h */
#define NOTEXTERN_IN_CONIO

/* Version: Use this to keep the /? command's display of the version up to
            date. Enclose within double-quotes. */
#define HTML_HELP_VERSION "5.3.3"

/* I N C L U D E S *********************************************************/

/* includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <malloc.h>
#include <dir.h>
#include <dos.h>
#include "catdefs.h"
#include "help_gui.h"
#include "conio.h"
#include "help.h"

/* M A I N   M O D U L E ***************************************************/

int
main (int argc, char *argv[])
{
  /* These have been moved from global to local variables */
  /* this reduces binary size and improves code structure. RP */
  /* Also size of home_page and help_page reduced from 257. RP 11-mar-04 */
  char home_page[20] = "index.htm";
  char help_page[20] = "help.htm";
  char base_dir[257];

  char *showcommand = 0;
  char *oldscreen;
  int oldcursorx = wherex (), oldcursory = wherey ();
  int i;			/* counter for loop */
  int forcemono = 0, fancyscheme = 0;
  int AsciiExtendedChars = 1;
  int codepage = 0;

  cat = catopen("htmlhelp", 0);

  if (getenv ("HELPPATH") == NULL)
    {
      get_base_dir (base_dir, argv[0]);
      strcat (base_dir, "..\\help\\"); /* default location */

      if (lang_add(base_dir, home_page) != 0)
      {
         char testpath[257];
         get_base_dir (base_dir, argv[0]);
         strcat (base_dir, "..\\help\\"); /* bookshelf location */

         strcpy(testpath, base_dir);
         strcat(testpath, home_page);
         if (checkForFile(testpath) != 0)
         {
            get_base_dir (base_dir, argv[0]); /* try same dir as exe */
            strcpy(testpath, base_dir);
            strcat(testpath, home_page);
            if (checkForFile(testpath) != 0)
            {
               *base_dir = '\0'; /* try current dir */
               strcpy(testpath, home_page);
               if (checkForFile(testpath) != 0)
               {
                  get_base_dir (base_dir, argv[0]);
                  strcat (base_dir, "..\\help\\");
               }
            }
         }
      }
    }
  else
    {
      strcpy (base_dir, getenv ("HELPPATH"));
      if (lang_add(base_dir, home_page) != 0)
      {
         strcpy (base_dir, getenv ("HELPPATH"));
         if (base_dir[0] != '\0')
         {
            if (base_dir[strlen(base_dir)-1] != '\\' &&
                base_dir[strlen(base_dir)-1] != '/');
	            strcat (base_dir, "\\");
         }
      }

    }

  if (getenv ("HELPCMD"))
    {
      if (strstr (getenv ("HELPCMD"), "/A"))
	AsciiExtendedChars = 0;
      if (strstr (getenv ("HELPCMD"), "/M"))
	forcemono = 1;
      if (strstr (getenv ("HELPCMD"), "/F1"))
         fancyscheme = 1;
      if (strstr (getenv ("HELPCMD"), "/F2"))
         fancyscheme = 2;
    }

  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '/')
	{
	  switch (argv[i][1])
	    {

	    default:
	      printf ("%s -- %s\n", hcatInvArg, argv[i] + 1);
		   printf ("%s\n", hcatHowGetUsage);
         break;

	    case '?':
	      show_usage ();
	      return 0;

	    case 'a':
	    case 'A':
	      if (argv[i][2] == 0)
		AsciiExtendedChars = 0;
	      break;

       case 'c':
       case 'C':
       codepage = atoi(argv[i]+2);
       if (codepage == 0)
       {
          printf("%s (/Cnnn)\n", hcatCodepagePlease);
          printf("%s:\n%s\n", hcatCodepagesSupported, supportedCodepages);
          return 0;
       }
       break;

	    case 'f':
	    case 'F':
	      fancyscheme = atoi(argv[i] + 2);
         if (fancyscheme < 1 || fancyscheme > 2)
            fancyscheme = 1;
         break;

	    case 'h':
	    case 'H':
	      if (argv[i][2] == 0)	/* Only put /h or /H */
		{
        printf ("%s\n", hcatInvArg);
		  printf ("%s\n", hcatHowGetUsage);
		  return 0;
		}
	      else
		{
           strncpy(help_page, argv[i] + 2, 14);
		}
	      break;

	    case 'l':
	    case 'L':
	      strcat (base_dir, argv[i] + 2);
	      checkForFile (base_dir);
	      get_home_page (home_page, base_dir);
	      get_base_dir (base_dir, base_dir);
	      break;

	    case 'm':
	    case 'M':
	      if (argv[i][2] == '\0')
		forcemono = 1;
	      else
		{
		  printf ("%s -- %s\n", hcatInvArg, argv[i] + 1);
		  printf ("%s\n", hcatHowGetUsage);
		  return 0;
		}
	      break;

	    case 'o':		/* Override index file path/name */
	    case 'O':
	      strcpy (base_dir, argv[i] + 2);
         if (lang_add(base_dir, home_page) != 0)
         {
	         strcpy (base_dir, argv[i] + 2);
	         checkForFile (base_dir);
	         get_home_page (home_page, base_dir);
	         get_base_dir (base_dir, base_dir);
         }
	    }
	}
      else if (showcommand == 0)
	{
	  showcommand = malloc (strlen (argv[i]) + 11);
	  if (!showcommand)
	    {
	      printf ("%s\n", hcatMemErr);
	      return 0;
	    }
	  sprintf (showcommand, "#%s", argv[i]);
	}
      else
	{
	  printf ("%s\n", hcat2ManyTopics);
	  printf ("%s\n", hcatHowGetUsage);
	  return 0;
	}
    }

  if (fancyscheme && forcemono)
  {
     printf ("%s\n", hcatFwithN);
	  printf ("%s\n", hcatHowGetUsage);
     return 0;
  }

  /* detect (or force) the codepage to select UTF-8 and entity
     substition support */
  if (selectCodepage(codepage) != codepage && codepage > 0)
  {
     printf("%s\n", hcatCodepageNotSupported);
     printf("%s:\n%s\n", hcatCodepagesSupported, supportedCodepages);
     return 0;
  }

  /* initialise user interface */
  conio_init (forcemono);

  if (forcemono == 0)
    {
      oldscreen = malloc (W * H * 2);
      if (oldscreen)
	save_window (X, Y, W, H, oldscreen);
    }

  if (MonoOrColor == COLOR_MODE && fancyscheme == 0)
    {
      TEXT_COLOR = C_TEXT_COLOR;
      BOLD_COLOR = C_BOLD_COLOR;
      ITALIC_COLOR = C_ITALIC_COLOR;
      BORDER_BOX_COLOR = C_BORDER_COLOR;
      BORDER_TEXT_COLOR = C_BORDER_TEXT_COLOR;
      LINK_COLOR = C_LINK_COLOR;
      LINK_HIGHLIGHTED_COLOR = C_LINK_HIGHLIGHTED_COLOR;
    }
  else if (MonoOrColor == COLOR_MODE && fancyscheme == 1)
    {
      TEXT_COLOR = F1_TEXT_COLOR;
      BOLD_COLOR = F1_BOLD_COLOR;
      ITALIC_COLOR = F1_ITALIC_COLOR;
      BORDER_BOX_COLOR = F1_BORDER_COLOR;
      BORDER_TEXT_COLOR = F1_BORDER_TEXT_COLOR;
      LINK_COLOR = F1_LINK_COLOR;
      LINK_HIGHLIGHTED_COLOR = F1_LINK_HIGHLIGHTED_COLOR;
    }
  else if (MonoOrColor == COLOR_MODE && fancyscheme == 2)
    {
      TEXT_COLOR = F2_TEXT_COLOR;
      BOLD_COLOR = F2_BOLD_COLOR;
      ITALIC_COLOR = F2_ITALIC_COLOR;
      BORDER_BOX_COLOR = F2_BORDER_COLOR;
      BORDER_TEXT_COLOR = F2_BORDER_TEXT_COLOR;
      LINK_COLOR = F2_LINK_COLOR;
      LINK_HIGHLIGHTED_COLOR = F2_LINK_HIGHLIGHTED_COLOR;
    }
  else
    {
      TEXT_COLOR = M_TEXT_COLOR;
      BOLD_COLOR = M_BOLD_COLOR;
      ITALIC_COLOR = M_ITALIC_COLOR;
      BORDER_BOX_COLOR = M_BORDER_COLOR;
      BORDER_TEXT_COLOR = M_BORDER_TEXT_COLOR;
      LINK_COLOR = M_LINK_COLOR;
      LINK_HIGHLIGHTED_COLOR = M_LINK_HIGHLIGHTED_COLOR;
    }
  if (AsciiExtendedChars == 0)
    {
      strcpy (Border22f, "+-+( )+-+");
      strcpy (Border22if, "+-+( )+-+");
      BarBlock1 = '.';
      BarBlock2 = '#';
    }
  show_mouse ();
  move_mouse (80, 25);
  drawmenu ();
  html_view (showcommand, base_dir, home_page, help_page);
  free (showcommand);
  hide_mouse ();
  if ((oldscreen != 0) && (forcemono == 0))
    {
      load_window (X, Y, W, H, oldscreen);
      free (oldscreen);
    }
  conio_exit ();
  gotoxy (oldcursorx, oldcursory);

  return 0;
}


int lang_add(char *base_dir, const char *home_page)
{
   char testpath[257];

   if (base_dir[0] != '\0')
   {
      if (base_dir[strlen(base_dir)-1] != '\\' &&
          base_dir[strlen(base_dir)-1] != '/')
          strcat(base_dir, "\\");
   }

   if (getenv ("LANG"))
   {
      struct ffblk dummy_ffblk;
      strcpy (testpath, base_dir);
      strcat (testpath, getenv("LANG"));

      if (findfirst(testpath, &dummy_ffblk, FA_DIREC) == 0)
      {
         /* found */
         strcpy (base_dir, testpath);
         strcat (base_dir, "\\");
      }
      else
         strcat (base_dir, "en\\");
   }
   else
   {
      strcat (base_dir, "en\\");
   }

   strcpy(testpath, base_dir);
   strcat(testpath, home_page);
   return checkForFile(testpath);
}


int
checkForFile (char *givenname)
{
  char backupname[257];
  strcpy (backupname, givenname);

  if (fclose (fopen (givenname, "r")) == -1)
    {
      /* try adding .htm */
      strcat (givenname, ".htm");
      if (fclose (fopen (givenname, "r")) == -1)
	{
	  /* try adding .zip */
	  strcpy (givenname, backupname);
	  strcat (givenname, ".zip");
	  if (fclose (fopen (givenname, "r")) == -1)
	    {
	      strcpy (givenname, backupname);
	      /* This simply can't be opened as a file */
	      /* so treat as directory instead with maybe index.htm */
	      strcat (givenname, "\\index.htm");
	      if (fclose (fopen (givenname, "r")) == -1)
		{
		  /* Last chance: may be a directory with a zip in it */
		  char *zipdir;
		  strcpy (givenname, backupname);
		  if ((zipdir = strrchr (backupname, '\\')) != NULL)
		    {
		      strcat (givenname, "\\");
		      strcat (givenname, zipdir + 1);
		      strcat (givenname, ".zip");
		      if (fclose (fopen (givenname, "r")) == -1)
			{
			  strcpy (givenname, backupname);
			  return 1;
			}
		    }
		}
	    }
	}
    }
  return 0;
}

void
get_home_page (char *home_page, char *path)
{
  char *p;

  if (path != 0)
    if (path[0] != 0)
      {
	p = path + strlen (path) - 1;

	while (p != path && *p != '\\' && *p != '/')
	  --p;

	if (p[0] == '\\' || p[0] == '/')	/* Went a bit too far back */
	  ++p;

	strncpy (home_page, p, 12);
	home_page[13] = '\0';
      }
}

void
get_base_dir (char *base_dir, char *path)
{
  char *p;

  if (path != 0)
    if (path[0] != 0)
      {
	p = path + strlen (path) - 1;
	while (p != path && *p != '\\' && *p != '/')
	  p--;

	if (*p == '\\' || *p == '/')
	  {
	    strncpy (base_dir, path, p - path + 1);
	    base_dir[p - path + 1] = 0;
	  }
	else
	  base_dir[0] = 0;
      }
}

void
show_usage (void)
{
  printf ("%s%s"
          "%s"
          "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
          "%s"
          "%s%s%s%s%s%s%s%s%s%s%s"
          "%s"
          "%s%s%s%s%s",
     hcatProgramName, " " HTML_HELP_VERSION "\n\n"
	  "HELP [options] [topic]\n\n",
	  hcatBasicOptions, ":\n"
	  "  topic            ", hcatUsageTopic, "\n"
	  "  /?               ", hcatUsageQMark, "\n"
	  "  /M               ", hcatUsageMono,  "\n"
     "  /F1              ", hcatUsageFancy, " 1\n"
     "  /F2              ", hcatUsageFancy, " 2\n"
	  "  /A               ", hcatUsageASCII, "\n"
     "  /Cnnn            ", hcatUsageCP, "\n\n",
	  hcatAdvancedOptions, ":\n"
	  "  /O[path[file]]   ", hcatUsageOverride1, "\n"
	  "                    ", hcatUsageOverride2, "\n"
	  "  /Lfile           ",  hcatUsageLoad, "\n"
	  "  /Htopic          ", hcatUsageHelp1, "\n"
	  "                   ", hcatUsageHelp2, "\n\n",
	  hcatEnvironVar, ":\n"
	  "  HELPPATH         ", hcatUsageHelpPath, ".\n"
	  "  HELPCMD          ", hcatUsageHelpCmd, "\n");
}
