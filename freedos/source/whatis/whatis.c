/* WHATIS (and APROPOS) */

/* Displays and searches one-line descriptions of commands based on the
   information in the Appinfo (.LSM) files */

/* Copyright (C) 2003 Rob Platt, worldwiderob@yahoo.co.uk */

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

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dir.h>

enum SearchType
{ ST_WHATIS, ST_APROPOS };

/* Function Definitions */
FILE *openwhatisfile (const char *source, const char *dest, const char *helppath);
void whatisfilename (const char *helppath, char *filename);
void pagewhatis (const char *pager, const char *whatisfilespec);
void makewhatis (const char *source, const char *dest, const char *helppath);
void usage (void);
int stristr (const char *s1, const char *s2);
void lsmextract (FILE * lsmfile, char *lsmline, int maxline,
		 const char *lsmsection);
void trimspace (char *string);
void WhatIs (FILE * database, const char *searchname,
	     enum SearchType searchtype);
void grabdirname (char *buffer, const char *program, const char *append, const char *def_value);	/* from Jim Hall's fasthelp */

int
main (int argc, char *argv[])
{
  const char *DEFAULT_HELPPATH = "C:\\FDOS\\HELP";
  const char *DEFAULT_APPINFOPATH = "C:\\FDOS\\APPINFO";
  const char *DEFAULT_PAGER = "MORE";

  char whatisfilespec[MAXPATH + 1];
  char helppath[MAXPATH + 1];
  char appinfopath[MAXPATH + 1];
  const char *pager = NULL;
  const char *searchname = NULL;

  enum RunType
  { RT_DEFAULT,
    RT_WHATIS,
    RT_APROPOS,
    RT_MAKEWHATIS
  }
  runtype = RT_DEFAULT;

  int i;

  /* start with default values: usually relative to the location of
     the program whatis.com. But if argv[0] is NULL/empty (an old DOS)
     then use the hard coded defaults instead */
  grabdirname (helppath, argv[0], "..\\help\\", DEFAULT_HELPPATH);
  grabdirname (appinfopath, argv[0], "..\\appinfo\\", DEFAULT_APPINFOPATH);

  /* now test environment variables */

  if (getenv ("HELPPATH") != NULL)
    strcpy (helppath, getenv ("HELPPATH"));

  pager = getenv ("PAGER");
  if (pager == NULL)
    pager = DEFAULT_PAGER;

  whatisfilename (helppath, whatisfilespec);

  /* check out command line switches */

  for (i = 1; i < argc; i++)
    {
      if ((argv[i][0] == '/') || argv[i][0] == '-')
	{
	  switch (argv[i][1])
	    {
	    case 'B':
	    case 'b':
	      if (runtype != RT_DEFAULT)
		{		/* The user has already specified once before */
		  usage ();
		  return 0;
		}
	      runtype = RT_MAKEWHATIS;
	      break;

	    case 'W':
	    case 'w':
	      if (runtype != RT_DEFAULT)
		{		/* The user has already specified once before */
		  usage ();
		  return 0;
		}
	      runtype = RT_WHATIS;
	      break;

	    case 'A':
	    case 'a':
	      if (runtype != RT_DEFAULT)
		{		/* The user has already specified once before */
		  usage ();
		  return 0;
		}
	      runtype = RT_APROPOS;
	      break;

	    case 'S':
	    case 's':
	      strcpy (appinfopath, argv[i] + 2);
	      break;

	    case 'd':
	    case 'D':
	      strcpy (helppath, argv[i] + 2);
	      whatisfilename (helppath, whatisfilespec);
	      break;

	    default:
	      usage ();
	      return 0;
	    }
	}
      else
	{
	  if (searchname == NULL)
	    {
	      searchname = argv[i];
	    }
	  else
	    {
	      usage ();
	      return 0;
	    }
	}
    }

  /* Options have been read in. So lets get to it: */
  switch (runtype)
    {
    case RT_MAKEWHATIS:
      if (searchname)
	{			/* searching for a subject doesn't apply to making WHATIS database */
	  usage ();
	}
      else
	{
	  puts ("Building whatis database...");
	  makewhatis (appinfopath, whatisfilespec, helppath);
	  puts ("...done");
	}
      return 0;

    case RT_APROPOS:
      if (searchname == NULL)
	{
	  usage ();
	}
      else
	{
	  FILE *whatis = openwhatisfile (appinfopath, whatisfilespec, helppath);
	  if (whatis)
	    {
	      WhatIs (whatis, searchname, ST_APROPOS);
	      fclose (whatis);
	    }
	}
      return 0;

    case RT_DEFAULT:
    case RT_WHATIS:
      if (searchname == NULL)
	{
	  /* openwhatisfile ensures the whatis file exists, or
	     else is created */
	  fclose (openwhatisfile (appinfopath, whatisfilespec, helppath));
	  pagewhatis (pager, whatisfilespec);
	}
      else
	{
	  FILE *whatis = openwhatisfile (appinfopath, whatisfilespec, helppath);
	  if (whatis)
	    {
	      WhatIs (whatis, searchname, ST_WHATIS);
	      fclose (whatis);
	    }
	}
      return 0;

    default:
      usage ();
      return 0;
    }
}

FILE *
openwhatisfile (const char *source, const char *dest, const char *helppath)
{
  FILE *whatis;

  whatis = fopen (dest, "rt");

  if (whatis)
    {
      return whatis;
    }
  else
    {
      puts ("Creating whatis database for first use...");
      makewhatis (source, dest, helppath);

      whatis = fopen (dest, "rt");
      return whatis;
    }
}


void
whatisfilename (const char *helppath, char *filename)
{
  strncpy (filename, helppath, MAXPATH - 7);

  if (strlen (filename))
    {
      if (filename[strlen (filename) - 1] != '\\')
	strcat (filename, "\\");
    }

  strcat (filename, "whatis");
}


void
pagewhatis (const char *pager, const char *whatisfilespec)
{
  char *commandlinetxt;

  commandlinetxt = malloc (strlen (pager) + strlen (whatisfilespec) + 2);
  sprintf (commandlinetxt, "%s %s", pager, whatisfilespec);
  system (commandlinetxt);
  free (commandlinetxt);
}


void
makewhatis (const char *source, const char *dest, const char *helppath)
{
	char sourcefiles[MAXPATH + 1];
	char helppathtmp[MAXPATH + 1];
	char *tempfilename;
	FILE *tempfile, *whatisfile;
	struct ffblk fileinfo;
	int done;

	strncpy (sourcefiles, source, MAXPATH - 6);	/* ensure enough room for "\*.LSM" */
	if (strlen (sourcefiles))
		{
			if (sourcefiles[strlen (sourcefiles) - 1] != '\\')
	strcat (sourcefiles, "\\");
		}
	strcat (sourcefiles, "*.LSM");

	done = findfirst (sourcefiles, &fileinfo, 0);

	if (done)
		{
			fprintf (stderr, "No LSM files found in %s!\n"
				 "Could not create whatis database.\n", source);
			exit (0);
		}

	/* Suspiciously, tempnam does not take const char as the dir argument.
		 So copy to a temporary buffer (helppathtmp) to make sure the original
		 isn't modified.
		 The temporary file is unsorted. It is used to create a sorted WHATIS
		 database */
	strcpy(helppathtmp, helppath);
	tempfilename = tempnam(helppathtmp, "WHAT");
	if (tempfilename == NULL)
		{
			fprintf (stderr, "Could not create a temporary file.\n"
				 "Could not create whatis database.\n");
			exit (0);
		}
	tempfile = fopen (tempfilename, "w+t");
	if (!tempfile)
		{
			fprintf (stderr, "Could not create a temporary file.\n"
				 "Could not create whatis database.\n");
			exit (0);
    }

  while (!done)
		{
      FILE *lsmfile;
      char lsmfilename[MAXPATH + 1];

			strncpy (lsmfilename, source, MAXPATH - 14);
			if (strlen (lsmfilename))
	{
	  if (lsmfilename[strlen (lsmfilename) - 1] != '\\')
	    strcat (lsmfilename, "\\");
	}
      strcat (lsmfilename, fileinfo.ff_name);

      lsmfile = fopen (lsmfilename, "rt");
			if (lsmfile)
	{
	  int maxTitleLength = 80;
	  char title[81];
		int maxDescriptionLength = 80;
		char description[81];

	  lsmextract (lsmfile, title, maxTitleLength, "Title:");

	  lsmextract (lsmfile, description, maxDescriptionLength,
		      "Description:");

	  /* test to see if the lsm extract function blanked out either
			 the title or description (which it would do if those sections
			 weren't found */
		if (strlen (title) && strlen (description))
			{
				/* truncate strings so that the title and description will
					 all fit on one line */
				title[19] = '\0';
				description[57] = '\0';
				fprintf (tempfile, "%-19s- %s\n", title, description);
			}
	}
			fclose (lsmfile);

			done = findnext (&fileinfo);
		}

	whatisfile = fopen (dest, "wt");
	if (!whatisfile)
		{
			fprintf (stderr, "Could not open destination file: %s\n"
				 "Could not create whatis database.\n", dest);
			fclose(tempfile);
			if (remove(tempfilename) == -1)
				 fprintf(stderr, "Warning: could not delete temporary file: %s\n", tempfilename);
			free(tempfilename);
			exit(0);
		}

	/* Sort temporary file into the WHATIS database file: */
	do
	{
		const int maxLineLength = 80; /* room for full line */
		char line1[81];
		char line2[81];
		long posLine1, posLine2;
		done = 1;
		fseek(tempfile, 0L, SEEK_SET);
		line1[0] = '\0';
		line2[0] = '\0';
		posLine1 = EOF;
		posLine2 = EOF;
		do
		{
			 posLine2 = ftell(tempfile);
			 if (fgets(line2, maxLineLength, tempfile) != NULL)
			 {
				 if (strchr(line2, '-')) /* needs one of these to be a valid line. */
				 {
						if (done)
						{ /* First string, and we're not done! */
							done = 0;
							posLine1 = posLine2;
							strcpy(line1, line2);
						}
						else
						{
							 if (stricmp(line2, line1) < 0)
							 {  /* We've found a lesser string. */
									posLine1 = posLine2;
									strcpy(line1, line2);
							 }
						}
				 }
			 }
		}
		while (!feof(tempfile));

		/* Put this string in WHATIS */
		if (!done)
		{
			if (strchr(line1, '-'))
				fputs(line1, whatisfile);

			if (strlen(line1))
			{
				strset(line1, ' '); /* strike the line from tempfile, set it to all space */
				line1[strlen(line1)-1] = '\n';
				fseek(tempfile, posLine1, SEEK_SET);
				fputs(line1, tempfile);
			}
		}
	}
	while (!done);

  fclose (tempfile);
	if (remove(tempfilename) == -1)
     fprintf(stderr, "Warning: could not delete temporary file: %s\n", tempfilename);
	free(tempfilename);
  fclose (whatisfile);
}


void
lsmextract (FILE * lsmfile, char *lsmline, int maxline,
	    const char *lsmsection)
{
  int sectionlength = strlen (lsmsection);
  char *temporary;
	int offset;

  do
    {
			lsmline[0] = '\0';
      fgets (lsmline, maxline, lsmfile);
      trimspace (lsmline);
    }
  while (strnicmp (lsmline, lsmsection, sectionlength) && !feof (lsmfile));

  if (feof (lsmfile))
    {
      lsmline[0] = '\0';
			return;
    }

  /* Find beginning of title text, after section e.g. "Title:" */
	offset = sectionlength;
  while (isspace (lsmline[offset]))
    offset++;

  temporary = malloc (maxline);
  strcpy (temporary, lsmline + offset);
  strcpy (lsmline, temporary);
  free (temporary);
}


void
trimspace (char *string)
{
  /* trim whitespace. */
  while (isspace (string[strlen (string) - 1]))
    string[strlen (string) - 1] = '\0';
}

void
WhatIs (FILE * database, const char *searchname, enum SearchType searchtype)
{
  int maxline = 80;
  char lsmline[81];
  int matchfound = 0;

  while (!feof (database))
    {
      lsmline[0] = '\0';
      fgets (lsmline, maxline, database);

      if (searchtype == ST_WHATIS)
	{
	  if (strnicmp (lsmline, searchname, strlen (searchname)) == 0)
	    {
	      printf (lsmline);
	      matchfound = 1;
	    }
	}
      else
	{
	  if (stristr (lsmline, searchname))
	    {
	      printf (lsmline);
	      matchfound = 1;
	    }
	}
    }

  if (!matchfound)
    puts ("No matches found.");
}

int
stristr (const char *s1, const char *s2)
{
  int retvalue;
  char *slwr1, *slwr2;

  slwr1 = malloc (strlen (s1));
  strcpy (slwr1, s1);
  strlwr (slwr1);

  slwr2 = malloc (strlen (s2));
  strcpy (slwr2, s2);
  strlwr (slwr2);

  if (strstr (slwr1, slwr2))
    retvalue = 1;
  else
    retvalue = 0;

  free (slwr1);
  free (slwr2);

  return retvalue;
}


/* The following is based on the helpdirname function from Jim Hall's
   FastHelp program. Jim Hall's email is: jhall@freedos.org. */
void
grabdirname (char *buffer,
	     const char *program, const char *append, const char *def_value)
{
  char drive[MAXDRIVE], dir[MAXPATH], name[MAXFILE], ext[MAXEXT];

  /* If program path not given (NULL or empty string) use hard
     coded value */
  if (program == NULL)
    {
      strcpy (buffer, def_value);
    }
  else
    {
      if (program[0] == '\0')
	{
	  strcpy (buffer, def_value);
	}
      else
	{
	  /* Split path up into components, then remerge */
	  /* Technically, you shouldn't give a path in the 'name' field,
	     but in this case, Borland C / Turbo C just passed it thru */
	  fnsplit (program, drive, dir, name, ext);
	  fnmerge (buffer, drive, dir, append, "");
	}
    }
}


void
usage ()
{
  fprintf (stderr, "\n"
	   "WHATIS and APROPOS\n"
	   "Displays descriptions of programs based on the Appinfo (.LSM) files.\n"
	   "Copyright (C) 2003 Robert Platt\n"
	   "This program is free software (GNU General Public License)\n"
	   "Version 1.0\n\n"
	   "  WHATIS [options] [command]\n"
	   "          Displays a one-line description of the given command.\n"
	   "          If no command is specfied, all commands and their\n"
	   "          one-line descriptions are listed\n\n"
	   "  WHATIS  /B [options]\n"
	   "          Rebuilds the whatis database for use with WHATIS and APROPOS\n\n"
	   "  APROPOS [options] subject\n"
	   "          Find commands related to the given subject\n\n"
	   "Options:\n"
	   "    /S[path]    non-default path to the lsm appinfo files\n"
	   "    /D[path]    non-default path for whatis database location\n");
}
