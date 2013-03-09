#ifndef CHK_DRIVER_H_
#define CHK_DRIVER_H_

/* Return value for the checks and fixes. */
#define RETVAL int

#define ERROR  -1  /* There is an internal error, eg. out of memory.  */

/* Returned by the check function. */
#define FAILED  0  /* The check has found a problem in the file system
                      structure. */
#define SUCCESS 1  /* The check has not found a problem. */

/* Returned by the fix function. */
#define RESCAN  2  /* A fix was made that requires all the test to be
                      performed again. */

struct CheckAndFix
{
   RETVAL (*check)(RDWRHandle handle);
   RETVAL (*fix)  (RDWRHandle handle);
   
#ifdef ENABLE_LOGGING
   char*  CheckName; 
#endif   
};

BOOL CheckVolume(RDWRHandle handle,
                 struct CheckAndFix* TheChecks,
                 unsigned numberofchecks);
                       
BOOL FixVolume(RDWRHandle handle,
               struct CheckAndFix* TheChecks,
               unsigned numberofchecks);

#endif