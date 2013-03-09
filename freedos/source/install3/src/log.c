/*
  LOG functionality - Written by Jeremy Davis <jeremyd@computer.org>
  and released to public domain.  
*/

#include "log.h"

#include <stdlib.h>  /* malloc */
#include <stdio.h>   /* vfprintf */
#include <stdarg.h>  /* va_list, va_start, va_end */
#include <string.h>  /* strlen, strcat */
#include "dir.h"     /* for DIR_CHAR */

/* The logfile */
FILE *logf = NULL;


/* attempts to open the specified logfile, in the specified destination directory
   returns 0 if successfully opened log, 
   1 on any error opening, 
   2 if filenames are bad (NULL or =='\0'),
   3 if failed to alloc memory constructing full path,
   and -1 if logfile already opened 
*/
int openlog(const char *destdir, const char *logfile)
{
  char *buffer;
  register int destdirlen = 0;

  /* if already open, return error, we only support a single log file open at one time */
  if (logf != NULL) return -1;

  /* if bad arguments given return 2, bad filenames */
  if ((destdir == NULL) || !*destdir || (logfile == NULL) || !*logfile) return 2;

  /* alloc buffer for full path, if fail return 3 */
  destdirlen = strlen(destdir);
  if ((buffer = (char *)malloc((destdirlen+strlen(logfile)+2)*sizeof(char))) == NULL)
    return 3;

  /* actually construct full path, adding DIR_CHAR if needed */
  strcpy(buffer, destdir);
  if (buffer[destdirlen-1] != DIR_CHAR)   /* under DOS this only allows \, no / */
  {
    buffer[destdirlen] = DIR_CHAR;
    buffer[destdirlen+1] = '\0';
  }
  strcat(buffer, logfile);

  /* actually open the log file, return 1 on an error (after freeing mem) */
  if ((logf = fopen(buffer, "wt")) == NULL)
  {
    free(buffer);
    return 1;
  }
  
  /* successfully opened log */
  free(buffer);
  return 0;
}


/* has same format as printf, returns 0 if log file not open, 
   otherwise returns result of vfprintf */
int log(const char *msg, ...)
{
  va_list argptr;
  int cnt;

  if (logf == NULL) return 0;

  va_start(argptr, msg);
  cnt = vfprintf(logf, msg, argptr);
  va_end(argptr);
  return cnt;
}


/* closes the log if open */
void closelog(void)
{
  if (logf != NULL)
  {
    fclose(logf);
    logf = NULL;
  }
}
