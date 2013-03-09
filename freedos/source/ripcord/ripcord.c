/* This is a simple little program that users can use to 
   easily get the version of the distribution installed,
   and if wanted the version of the specific program
   that CAME WITH the distribution this program is 
   distributed with.  This program is updated with each
   release, and simply displays hard coded information.
   Only covers boot and base.
*/

#define DIST_VERSION "FreeDOS 1.0 Final Distribution"

#define NA 1                /* not currently available                          */
#define INTERNAL 32768      /* command is internal to FreeCom                   */
#define UPDATED 2           /* updated in this ripcord release                  */
#define RIPCORD 4           /* updated in a ripcord, but not the current one    */
#define BETA6 8             /* indicates available in Beta6 or earlier          */
#define BETA7 16            /* indicates available released in Beta7            */
#define BETA8 32            /* indicates available released in Beta8            */
#define BETA9 64            /* indicates available released in Beta9            */
#define FINAL10 128         /* indicates available released in 1.0 Final        */
#define ALL (NA | FINAL10 | BETA9 | BETA8 | BETA7 | BETA6 | RIPCORD | UPDATED | INTERNAL)
#define BETA (BETA9 | BETA8 | BETA7 | BETA6)

#define NUM_PROGRAMS 102
typedef struct PROGINFO
{
  char * name;              /* The name of the program                    */
  char * version;           /* String representing the version            */
  unsigned int  updated;    /* indicates if updated this release or not   */
} PROGINFO;

PROGINFO programs[NUM_PROGRAMS] = 
{
  { "Kernel",   "Build 2.0.36cvs                ", FINAL10 },
  { "Install",  "3.78                           ", FINAL10 },
  { "alias",    "internal                       ", INTERNAL },
  { "Append",   "5.0-0.6                        ", FINAL10 },
  { "Assign",   "1.4                            ", BETA6 },
  { "Attrib",   "2.1                            ", FINAL10 },
  { "Backup",   "Not Available                  ", NA },
  { "beep",     "internal                       ", INTERNAL },
  { "break",    "internal                       ", INTERNAL },
  { "call",     "internal                       ", INTERNAL },
  { "cd",       "internal (aka chdir, cdd)      ", INTERNAL },
  { "chcp",     "internal                       ", INTERNAL },
  { "Chkdsk",   "0.9                            ", FINAL10 },
  { "cls",      "internal                       ", INTERNAL },
  { "Choice",   "4.3a                           ", BETA9 },
  { "CtMouse",  "1.9.1 alpha 1, 2.0 alpha 4     ", BETA9 },
  { "FreeCom",  "0.84 pre2                      ", FINAL10 },
  { "Comp",     "1.03                           ", BETA9 },
  { "copy",     "internal                       ", INTERNAL },
  { "ctty",     "internal                       ", INTERNAL },
  { "Debug",    "0.98                           ", BETA9 },
  { "Defrag",   "1.0                            ", FINAL10 },
  { "date",     "internal                       ", INTERNAL },
  { "del",      "internal (aka erase)           ", INTERNAL },
  { "Deltree",  "1.02g                          ", FINAL10 },
  { "dir",      "internal                       ", INTERNAL },
  { "dirs",     "internal                       ", INTERNAL },
  { "Diskcomp", "0.74                           ", BETA9 },
  { "Diskcopy", "beta 0.94                      ", BETA9 },
  { "Display",  "0.14                           ", FINAL10 },
  { "doskey",   "internal                       ", INTERNAL },
  { "echo",     "internal                       ", INTERNAL },
  { "Edit",     "0.7d                           ", FINAL10 },
  { "Edlin",    "2.8                            ", FINAL10 },
  { "Emm386",   "2.26                           ", FINAL10 },
  { "Exe2bin",  "1.5                            ", FINAL10 },
  { "exit",     "internal                       ", INTERNAL },
  { "Fc",       "3.03                           ", BETA9 },
  { "Fasthelp", "3.5                            ", BETA9 },
  { "FDAPM",    "23May2005                      ", BETA9 },
  { "Fdisk",    "1.2.1-k                        ", FINAL10 },
  { "FDShield", "26Mar2005                      ", FINAL10 },
  { "FDXMS",    "0.94.Bananas                   ", BETA9 },
  { "FDXMS286", "0.03.Temperaments              ", BETA9 },
  { "Find",     "2.9                            ", BETA9 },
  { "for",      "internal                       ", INTERNAL },
  { "Format",   "0.91v                          ", FINAL10 },
  { "goto",     "internal                       ", INTERNAL },
  { "Graphics", "2003Jul12                      ", BETA9 },
  { "Himem",    "3.26                           ", BETA9 },
  { "history",  "internal                       ", INTERNAL },
  { "Htmlhelp", "1.05a                          ", FINAL10 },
  { "if",       "internal                       ", INTERNAL },
  { "Keyb",     "2.0                            ", FINAL10 },
  { "Label",    "1.4                            ", BETA9 },
  { "LBACache", "2005-06-19                     ", BETA9 },
  { "lfnfor",   "internal                       ", INTERNAL },
  { "loadfix",  "internal                       ", INTERNAL },
  { "loadhigh", "internal                       ", INTERNAL },
  { "Mem",      "1.11                           ", FINAL10 },
  { "memory",   "internal                       ", INTERNAL },
  { "md",       "internal (aka mkdir)           ", INTERNAL },
  { "Mirror",   "0.2                            ", BETA6 },
  { "Mode",     "12May2005                      ", FINAL10 },
  { "More",     "4.0b                           ", BETA9 },
  { "Move",     "3.3                            ", BETA9 },
  { "Nansi",    "4.0b                           ", FINAL10 },
  { "Nlsfunc",  "0.4                            ", FINAL10 },
  { "path",     "internal                       ", INTERNAL },
  { "pause",    "internal                       ", INTERNAL },
  { "Print",    "1.0                            ", BETA6 },
  { "PrintQ",   "05-Jul-1997                    ", BETA6 },
  { "prompt",   "internal                       ", INTERNAL },
  { "pushd",    "internal                       ", INTERNAL },
  { "popd",     "internal                       ", INTERNAL },
  { "Recover",  "0.1beta                        ", BETA9 },
  { "Replace",  "1.2                            ", BETA6 },
  { "ren",      "internal (aka rename)          ", INTERNAL },
  { "rem",      "internal                       ", INTERNAL },
  { "rd",       "internal (aka rmdir)           ", INTERNAL },
  { "Restore",  "Not Available                  ", NA },
  { "set",      "internal                       ", INTERNAL },
  { "Share",    "2004-08-26 pl1                 ", FINAL10 },
  { "shift",    "internal                       ", INTERNAL },
  { "Shsucdx",  "3.03a                          ", FINAL10 },
  { "Shsufdrv", "1.11                           ", FINAL10 },
  { "Sort",     "1.2                            ", BETA9 },
  { "Swsubst",  "3.2                            ", BETA9 },
  { "Sys",      "3.2                            ", BETA9 },
  { "time",     "internal                       ", INTERNAL },
  { "Tree",     "3.7.2                          ", BETA9 },
  { "truename", "internal                       ", INTERNAL },
  { "type",     "internal                       ", INTERNAL },
  { "Undelete", "2004-01-19                     ", BETA9 },
  { "Unformat", "0.8                            ", BETA6 },
  { "ver",      "internal                       ", INTERNAL },
  { "verify",   "internal                       ", INTERNAL },
  { "vol",      "internal                       ", INTERNAL },
  { "which",    "internal                       ", INTERNAL },
  { "Xcopy",    "1.3                            ", FINAL10 },
  { "XCDROM",   "2.3                            ", FINAL10 },
  { "XDMA",     "3.3                            ", FINAL10 },
};

#include <stdio.h>
#include <string.h>

void print_usage(void)
{
  printf("ripcord [PROGRAM | ALL | BETA[6|7|8|9] | 1.0 | "
#ifdef HOT_RELEASE
         "RIPCORD | UPDATED | "
#endif
         "NA | INTERNAL]\n");
  printf("  PROGRAM is the program to display version information for.\n");
  printf("  Do NOT include extension, eg use FORMAT not FORMAT.EXE\n");
  printf("\n");
  printf("  The remaining choices are keywords that will display\n");
  printf("  version information for the corresponding programs included\n");
  printf("  on the boot disk and in the base package:\n");
  printf("    ALL     : all programs on boot disk & in base (including INTERNAL)\n");
  printf("    BETA#   : only those updated in Beta [6/7/8/9]\n");
  printf("    1.0     : only those updated in 1.0 Final\n");
#ifdef HOT_RELEASE
  printf("    RIPCORD : all programs updated since Beta 8\n");
  printf("    UPDATED : all programs updated in this Ripcord release\n");
#endif
  printf("    NA      : \"standard\" programs not yet available\n");
  printf("    INTERNAL: programs internal to FreeCOM\n");
  printf("\n");
  printf("This program displays hard coded information and only\n");
  printf("reflects information for the distribution it is included with.\n");
  printf("\n");
  printf("Maintained by Kenneth J Davis <jeremyd@computer.org>\n");
  printf("It is public domain.\n");
}


/* Procedures */

int getIndexFor(char *progName)
{
  register int i;
  for (i = 0; i < NUM_PROGRAMS; i++)
    if (strnicmp(progName, programs[i].name, 8) == 0)
      return i;

  return -1;
}

void display_info(int index)
{
  printf("%8s : %s [%s]\n", programs[index].name, programs[index].version,
         ((programs[index].updated & BETA6)?"Beta6" :
         ((programs[index].updated & BETA7)?"Beta7" :
         ((programs[index].updated & BETA8)?"Beta8" :
         ((programs[index].updated & BETA9)?"Beta9" :
         ((programs[index].updated & FINAL10)?"1.0 Final" :
         ((programs[index].updated & RIPCORD)?"Updated since Beta8" : 
         ((programs[index].updated & UPDATED)?"Updated this release" :
         ((programs[index].updated & NA)?"NA" :
         ((programs[index].updated & INTERNAL)?"Internal to FreeCom" : "Unknown status" ))))))))));
}

void display(int flags)
{
  register int i;
  for (i = 0; i < NUM_PROGRAMS; i++)
    if (programs[i].updated & flags)
      display_info(i);
}

int main(int argc, char *argv[])
{
  register int i;
  register int index;

  printf(DIST_VERSION "\n\n");

  for (i = 1; i < argc; i++)
  {
    if ((argv[i][1] == '?') || (strcmpi(argv[i]+1, "HELP") == 0))
    {
      print_usage();
      return 0;
    }
    else if (strcmpi(argv[i], "ALL") == 0)
    {
      display(ALL);
    }
    else if (strcmpi(argv[i], "NA") == 0)
    {
      display(NA);
    }
    else if (strcmpi(argv[i], "INTERNAL") == 0)
    {
      display((signed int)INTERNAL);
    }
    else if (strcmpi(argv[i], "BETA") == 0)
    {
      display(BETA);
    }
    else if (strcmpi(argv[i], "BETA6") == 0)
    {
      display(BETA6);
    }
    else if (strcmpi(argv[i], "BETA7") == 0)
    {
      display(BETA7);
    }
    else if (strcmpi(argv[i], "BETA8") == 0)
    {
      display(BETA8);
    }
    else if (strcmpi(argv[i], "BETA9") == 0)
    {
      display(BETA9);
    }
    else if (strcmpi(argv[i], "1.0") == 0)
    {
      display(FINAL10);
    }

#ifdef HOT_RELEASE
    else if (strcmpi(argv[i], "RIPCORD") == 0)
    {
      display(RIPCORD | UPDATED);
    }
    else if (strcmpi(argv[i], "UPDATED") == 0)
    {
      display(UPDATED);
    }
#endif
    else /* probably a specific program */
    {
      index = getIndexFor(argv[i]);
      if ((index >= 0) && (index < NUM_PROGRAMS))
        display_info(index);
      else
        printf("Unknown program [%s]\n", argv[i]);
    }
  }

  return 0;
}
