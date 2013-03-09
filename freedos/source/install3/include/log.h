/*
  LOG functionality - Written by Jeremy Davis <jeremyd@computer.org>
  and released to public domain.  
*/

#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif


/* attempts to open the specified logfile, in the specified destination directory
   returns 0 if successfully opened log, 
   1 on any error opening, 
   2 if filenames are bad (NULL or =='\0'),
   3 if failed to alloc memory constructing full path,
   and -1 if logfile already opened 
*/
int openlog(const char *destdir, const char *logfile);


/* has same format as printf, returns 0 if log file not open, 
   otherwise returns result of vfprintf */
int log(const char *msg, ...);


/* closes the log if open */
void closelog(void);


#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
