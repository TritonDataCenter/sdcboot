/*  FreeDOS HTML Help Viewer

    READFILE - opens and reads in a text file

    Copyright (c) Express Software 2002-3
    See doc\htmlhelp\copying for licensing terms
    Created by: Joe Cosentino/Rob Platt
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <dir.h>
#include "help_gui.h"
#include "readfile.h"
#include "unz\unzip.h"
#include "catdefs.h"


/* min macro isn't implemented by some compilers */
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

char *readUncompressedFile (const char *filename);
char *readZipFile (unzFile * handle);
void readFileError (const char *filename);
char *tryasuncompressed (const char *filename);


char *
readUncompressedFile (const char *filename)
{
  FILE *f;
  long l;
  char *text_buf;

  f = fopen (filename, "rb");

  if (!f)
      return 0;

  /* determine the length of file using standard file i/o. -RP
   */
  fseek (f, 0L, SEEK_END);
  l = min (ftell (f), 0xFFFEU);
  fseek (f, 0L, SEEK_SET);

  /* Allocate memory for file text buffer */
  while ((text_buf = (char *) malloc (l + 1)) == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	return 0;
      }
  /* Read file into text buffer */
  if (!fread (text_buf, 1, l, f))
    {
      free (text_buf);
      return 0;
    }

  /* Clean up and 0-terminate the text buffer */
  fclose (f);
  text_buf[l] = 0;

  return text_buf;
}


char *
readZipFile (unzFile * handle)
{
  long l, result;
  char *text_buf;
  unz_file_info info;


  /* determine the length of file */
  unzGetCurrentFileInfo (handle, &info, NULL, 0, NULL, 0, NULL, 0);
  l = min (info.uncompressed_size, 65530L);
  if (l == 0)
    return NULL;

  /* Allocate memory for file text buffer */
  while ((text_buf = (char *) malloc (l + 1)) == NULL)
    if (pesListDeleteOldest ())
      {
	show_error (hcatMemErr);
	return NULL;
      }
  /* Read file into text buffer */
  result = unzReadCurrentFile (handle, text_buf, l);
  if (result < 0)
    {
      show_error (hcatReadCompressFail);
      free (text_buf);
      return NULL;
    }

  /* 0-terminate the text buffer */
  text_buf[l] = '\0';

  return text_buf;
}


void
readFileError (const char *filename)
{
  char *err_msg;
  int i;

  err_msg = (char *) malloc (strlen (filename) + strlen(hcatOpenErr) + 3);
  if (err_msg == 0)
    {
      show_error (hcatMemErr);
      return;
    }

  sprintf (err_msg, "%s %s", hcatOpenErr, filename);

  /* Beautify the err_msg by replacing / with \ */
  for (i = 15; i < strlen (err_msg); i++)
    if (err_msg[i] == '/')
      err_msg[i] = '\\';

  show_error (err_msg);
  free (err_msg);
  return;
}


char *
readfile (const char *filename, const char *homepage)
{
  char filetoopen[MAXPATH] = "";
  char fileinzip[MAXPATH] = "";
  char dir[MAXDIR];
  char name[MAXFILE];
  char extension[MAXEXT];
  char *attempt = NULL;
  unzFile *handle = NULL;

  fnsplit (filename, NULL, dir, name, extension);

  /* If extension is ".zip" */
  if (strcmpi (extension, ".zip") == 0)
    {
      /* try to load this zip file. */
      strcpy (filetoopen, filename);
      strcpy (fileinzip, "index.htm");
      handle = unzOpen (filetoopen);
    }
  else
    {
      char temp1[MAXPATH];
      char temp2[MAXPATH];
      char *trydir1=NULL, *trydir2=NULL;
      int i;
      /* other/no extension:
      /* Look at the directory and try to open the associated zip file */
      strcpy(temp1, filename);
      for (i = 0; i < strlen(temp1); i++)
          if (temp1[i] == '/')
             temp1[i] = '\\';

      while((trydir1=strrchr(temp1, '\\')) != NULL)
      {
         *trydir1 = '\0';

         trydir2 = strrchr(temp1, '\\');
         if (trydir2 == NULL)
            trydir2 = temp1;

         strcpy(temp2, trydir2);
         if (*temp1 != '\0')
            if ((temp1[strlen(temp1)-1] != '\\') &&
                 temp2[0] != '\\')
               strcat(temp1, "\\");
         strcat(temp1, temp2);
         strcat(temp1, ".zip");
         if ((handle = unzOpen(temp1)) != 0)
         {
            strcpy(filetoopen, temp1);
            trydir1 = strrchr(temp1, '\\');
            strcpy(fileinzip, filename+(trydir1-temp1+1));
            break;
         }

         *trydir1 = '\0';
      }

      /* if that didn't work or wasn't possible, but the homepage is a
         zip file (happens if a zip is opened directly) then look in
         the homepage zipfile instead. Added for help 5.3.2 */
      if (homepage != NULL && handle == NULL)
      {
         if (strcmpi(homepage+strlen(homepage)-4, ".zip") == 0)
         {
            strcpy (fileinzip, filename);
            strcpy (filetoopen, homepage);
            handle = unzOpen (filetoopen);
         }
      }
  }

  if (handle)
	{
      int i;
      for (i = 0; i < strlen(fileinzip); i++)
          if (fileinzip[i] == '\\')
             fileinzip[i] = '/';

	  if (unzLocateFile (handle, fileinzip, 0) != UNZ_OK)
	    {
	      /* Could not find this file in the zip file */

	      /* Look for as uncompressed file instead: */
	      if ((attempt = tryasuncompressed (filename)) != NULL)
		return attempt;

	      show_error
		(hcatFirstFile);
	      /* Else look for first html file in zip: */
	      if (unzGoToFirstFile (handle) != UNZ_OK)
		{
		  show_error (hcatZipEmpty);
		  return NULL;
		}
	      unzGetCurrentFileInfo (handle, NULL, fileinzip, MAXPATH,
				     NULL, 0, NULL, 0);
	      fnsplit (fileinzip, NULL, NULL, NULL, extension);
	      while (stricmp (extension, ".htm") != 0)
		{
		  if (unzGoToNextFile (handle) != NULL)
		    {
		      show_error (hcatHTMLinZipErr);
		      return NULL;
		    }
		  unzGetCurrentFileInfo (handle, NULL, fileinzip, MAXPATH,
					 NULL, 0, NULL, 0);
		  fnsplit (fileinzip, NULL, NULL, NULL, extension);
		}
	    }

	  if (unzOpenCurrentFile (handle) == UNZ_OK)
	    {
	      char *text_buf = readZipFile (handle);
         if (text_buf == NULL)
            show_error(hcatReadCompressFail);
	      unzCloseCurrentFile (handle);
	      unzClose (handle);
	      return text_buf;
	    }
	  unzClose (handle);
	}

  /* Look for as uncompressed file instead: */
  if ((attempt = tryasuncompressed (filename)) != NULL)
    return attempt;

  return NULL;
}


char *
tryasuncompressed (const char *filename)
{
  char filetoopen[257];
  /* try as uncompressed .htm file */

  strcpy (filetoopen, filename);
  if (fclose (fopen (filetoopen, "r")) == -1)
    {
      /* Could not open as is */
      /* Try with .htm on end */
      strcat (filetoopen, ".htm");
      if (fclose (fopen (filetoopen, "r")) == -1)
	return NULL;
    }

  return readUncompressedFile (filetoopen);
}
