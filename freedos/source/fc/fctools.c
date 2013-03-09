		       /* Auxiliary routines for FC */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys\stat.h>
#include "FCTools.h"

#define CARRY	1	/* Carry flag */
#define DOS	0x21	/* DOS interrupt */

#define LFN_NOT_SUPPORTED	0x7100

typedef union
{
  struct
  {
    long DOSFileTime;
    long Unused;
  } DOSTime;
  unsigned char Win98FileTime[8];
} file_time;

typedef struct		/* Windows95 long filename find record */
{
  unsigned long Attributes;
  file_time CreationTime;
  file_time LastAccessTime;
  file_time LastModificationTime;
  unsigned long SizeHi;
  unsigned long SizeLo;
  char Reserved[8];
  char LongFilename[260];
  char ShortFilename[14];
} lfn_find_results;

int FindFirst(const char* PathName, find_data* FindData, int Attrib)
{
  struct REGPACK r;
  lfn_find_results FindResults;

  FindData->Filename[0] = END_OF_STRING;
  FindData->UseLFN = TRUE;
  r.r_ax = 0x714E;		/* Find first matching file */
  r.r_cx = Attrib;
  r.r_si = 1;			/* MS-DOS style time */
  r.r_ds = FP_SEG(PathName);	 r.r_dx = FP_OFF(PathName);
  r.r_es = FP_SEG(&FindResults); r.r_di = FP_OFF(&FindResults);
  r.r_flags = CARRY;		/* Set carry to prepare for failure */
  intr(DOS, &r);
  if ((r.r_flags & CARRY) == 0) /* No carry -> Ok */
  {
    FindData->Handle = r.r_ax;
    errno = 0;
    if (FindResults.LongFilename != END_OF_STRING)
      strncpy(FindData->Filename, FindResults.LongFilename,
	      sizeof(FindData->Filename) - 1);
    else
      strncpy(FindData->Filename, FindResults.ShortFilename,
	      sizeof(FindData->Filename) - 1);
    FindData->Attributes = FindResults.Attributes & 0xFF;
  }
  else
  {
    if (r.r_ax == LFN_NOT_SUPPORTED) /* Function not supported */
    {
      FindData->UseLFN = FALSE; /* Use the standard functions */
      errno = findfirst(PathName, &(FindData->SearchRec), Attrib);
      if (errno == 0)
      {
	strncpy(FindData->Filename, FindData->SearchRec.ff_name,
		sizeof(FindData->Filename) - 1);
	FindData->Attributes = FindData->SearchRec.ff_attrib;
      }
    }
    else			/* A true error */
      errno = r.r_ax;		/* DOS error code */
  }
  return errno;
}
/* ------------------------------------------------------------------------ */
int FindNext(find_data* FindData)
{
  FindData->Filename[0] = END_OF_STRING;
  if (FindData->UseLFN)
  {
    struct REGPACK r;
    lfn_find_results FindResults;

    r.r_ax = 0x714F;		  /* Find next matching file */
    r.r_bx = FindData->Handle;
    r.r_si = 1;			  /* MS-DOS style time */
    r.r_es = FP_SEG(&FindResults); r.r_di = FP_OFF(&FindResults);
    r.r_flags = CARRY;		  /* Set carry to prepare for failure */
    intr(DOS, &r);
    if ((r.r_flags & CARRY) == 0) /* No carry -> Ok */
    {
      errno = 0;
      if (FindResults.LongFilename != END_OF_STRING)
	strncpy(FindData->Filename, FindResults.LongFilename,
		sizeof(FindData->Filename) - 1);
      else
	strncpy(FindData->Filename, FindResults.ShortFilename,
		sizeof(FindData->Filename) - 1);
      FindData->Attributes = FindResults.Attributes & 0xFF;
    }
    else
      errno = r.r_ax;		  /* DOS error code */

  }
  else
  {
    errno = findnext(&(FindData->SearchRec));
    if (errno == 0)
    {
      strncpy(FindData->Filename, FindData->SearchRec.ff_name,
	      sizeof(FindData->Filename) - 1);
      FindData->Attributes = FindData->SearchRec.ff_attrib;
    }
  }
  return errno;
}
/* ------------------------------------------------------------------------ */
int FindClose(find_data* FindData)
{
  if (FindData->UseLFN)
  {
    struct REGPACK r;

    r.r_ax = 0x71A1;		  /* Terminate directory search */
    r.r_bx = FindData->Handle;
    intr(DOS, &r);
    errno = 0;
    if ((r.r_flags & CARRY) != 0) errno = r.r_ax; /* DOS error code */
    return errno;
  }
  return 0;
}

/* ************************************************************************ */
char* FullPath(char* Buffer, char *Path, int BufferSize)
{
  struct REGPACK r;
  char Drive;
  int Work;
  char* WorkPointer = Buffer;

  *Buffer = END_OF_STRING;
  if (BufferSize < MAXPATHLFN) return Buffer;

  if (isalpha(Path[0]) && (Path[1] == ':'))    /* Drive specified */
  {
    Drive = WorkPointer[0] = Path[0];
    Path += 2;
  }
  else					 /* Default drive */
    Drive = WorkPointer[0] = getdisk() + 'A';
  WorkPointer[1] = ':';
  WorkPointer += 2; BufferSize -= 2;

  if (islower(Drive)) Drive = _toupper (Drive); /* Upper case drive */
  Drive = Drive - 'A' + 1;

  *WorkPointer = '\\';
  WorkPointer++; BufferSize -= 1;
  if (*Path == '\\')
    Path++;			/* Absolute path */
  else
  {				/* Relative path */
    r.r_ax = 0x7147;		/* Get current directory (LFN) */
    r.r_dx = Drive;
    r.r_ds = FP_SEG(WorkPointer);  r.r_si = FP_OFF(WorkPointer);
    r.r_flags = CARRY;	/* Set carry to prepare for failure */
    intr(DOS, &r);
    if ((r.r_flags & CARRY) != 0)	/* Carry -> Not ok */
      getcurdir(Drive, WorkPointer);	/* Use the SFN function */

    if (*WorkPointer != END_OF_STRING)
    {
      Work = strlen(WorkPointer);
      WorkPointer += Work;
      *WorkPointer = '\\';
      WorkPointer++;
      BufferSize -= Work + 1;
    }
  }

  strncpy(WorkPointer, Path, BufferSize);  /* Add the rest of the path */

  WorkPointer = Buffer;		/* Expand the "." directories */
  do
  {
    WorkPointer = strchr(WorkPointer, '.');
    if (WorkPointer == NULL) break;

    switch (WorkPointer[1])
    {
      case '\\':                        /* ".\" */
	stpcpy(WorkPointer, WorkPointer + 2);
	break;

      case END_OF_STRING:		/* Terminal '.' */
	if (WorkPointer[-1] == '\\')    /* "\." */
	  *WorkPointer = END_OF_STRING;
	else
	  WorkPointer++;		/* "FOO." */
	break;

      case '.':
	if (WorkPointer[-1] == '\\')    /* "\.." */
	{
	  Path = WorkPointer + 1;
	  do { Path++; } while (*Path == '.');

	  if ((*Path != '\\') && (*Path != END_OF_STRING))
	    WorkPointer = Path;
	  else
	  {
	    Work = (int)(Path - WorkPointer - 1);
	    do
	    {
	      WorkPointer--;
	      do
	      {
		WorkPointer--;
		/* Handle the case (absurd) C:\.. */
		if (*WorkPointer == ':')
		{
		  *Buffer = END_OF_STRING;
		  return Buffer;
		}
	      } while (*WorkPointer != '\\');
	      Work--;
	    } while (Work > 0);
	    stpcpy(WorkPointer, Path);
	  }
	}
	break;

      default:
	WorkPointer++;
    }
  } while (TRUE);

  /* Remove the last '\' if not the root directory */
  Work = strlen(Buffer) - 1;
  if ((Work > 3) && (Buffer[Work] == '\\')) Buffer[Work] = END_OF_STRING;

  return Buffer;
}
/* ------------------------------------------------------------------------ */
FILE* FileOpen(const char* Filename, const char* Mode)
{
  struct REGPACK r;
  char ShortFilename[128];

  r.r_ax = 0x7160;		/* Get short filename */
  r.r_cx = 0X8001;
  r.r_ds = FP_SEG(Filename);	  r.r_si = FP_OFF(Filename);
  r.r_es = FP_SEG(ShortFilename); r.r_di = FP_OFF(ShortFilename);
  r.r_flags = CARRY;		/* Set carry to prepare for failure */
  intr(DOS, &r);
  if ((r.r_flags & CARRY) == 0) /* No carry -> Ok */
    return fopen(ShortFilename, Mode);

  return fopen(Filename, Mode);
}

/* ------------------------------------------------------------------------ */
/* Warning: no final '\' and no root directories */
bool IsADirectory(char Filename[])
{
  struct REGPACK r;
  lfn_find_results FindResults;

  errno = 0;
  r.r_ax = 0x714E;		/* Find first matching file */
  r.r_cx = FA_DIREC | FA_RDONLY;
  r.r_si = 1;			/* MS-DOS style time */
  r.r_ds = FP_SEG(Filename);	 r.r_dx = FP_OFF(Filename);
  r.r_es = FP_SEG(&FindResults); r.r_di = FP_OFF(&FindResults);
  r.r_flags = CARRY;		/* Set carry to prepare for failure */
  intr(DOS, &r);
  if ((r.r_flags & CARRY) == 0) /* No carry -> Ok */
  {
    r.r_bx = r.r_ax;		/* Handle */
    r.r_ax = 0x71A1;		/* Terminate directory search */
    intr(DOS, &r);
    if ((r.r_flags & CARRY) != 0) errno = r.r_ax; /* DOS error code */
    return ((FindResults.Attributes & FA_DIREC) != 0);
  }

  if (r.r_ax == LFN_NOT_SUPPORTED) /* Function not supported */
  {
    struct ffblk SearchRec;	/* Use the standard functions */

    errno = findfirst(Filename, &SearchRec, FA_DIREC | FA_RDONLY);
    if (errno == 0)
      return ((SearchRec.ff_attrib & FA_DIREC) != 0);
  }

  /* A true error */
  errno = r.r_ax;		/* DOS error code */
  return FALSE;
}

/* ************************************************************************ */
bool HasWildcards(char* Filename)
{
  while (*Filename != END_OF_STRING)
  {
    if ((*Filename == '*') || (*Filename == '?')) return TRUE;
    Filename++;
  }
  return FALSE;
}

/* ------------------------------------------------------------------------ */
/* Try to find a match between the filename pattern in Pattern and the
   filename in Name. If the match is found, the result is in Target and
   the function returns TRUE */
bool MatchNames(char Pattern[], char Name[], char Target[])
{
  char* PatternExtensionStart = strrchr(Pattern, '.');
  char* NameExtensionStart = strrchr(Name, '.');

  if (PatternExtensionStart == NULL)			/* No extension */
    PatternExtensionStart = Pattern + strlen(Pattern);	/* End of string */
  if (NameExtensionStart == NULL)			/* No extension */
    NameExtensionStart = Name + strlen(Name);		/* End of string */

  /* Parse the name part */
  while ((Name < NameExtensionStart) &&
	 (Pattern < PatternExtensionStart) && (*Pattern != '*'))
  {
    if (*Pattern == '?')
      *Target = *Name;
    else
      *Target = *Pattern;
    Pattern++; Name++; Target++;
  }
  if (Name == NameExtensionStart)	/* Name was shorter than Pattern */
    while ((Pattern < PatternExtensionStart) &&
	   (*Pattern != '?') && (*Pattern != '*'))
    {
      *Target = *Pattern;
      Pattern++; Target++;
    }
  if (*Pattern == '*')
  {
    Pattern = PatternExtensionStart;
    while (Name < NameExtensionStart)
    {
      *Target = *Name;
      Name++; Target++;
    }
  }
  *Target = END_OF_STRING;
  /* Here to have a match *Pattern must be a dot or the end of string */
  if (*Pattern == '.')
    Pattern++;
  else
    if (*Pattern != END_OF_STRING) return FALSE;

  if (*Name == '.')
    Name++;
  else
    if (*Name != END_OF_STRING) return FALSE;
  *Target = '.'; Target++;

  /* Parse the ext part */
  while ((*Pattern != END_OF_STRING) && (*Name != END_OF_STRING) &&
	 (*Pattern != '*'))
  {
    if (*Pattern == '?')
      *Target = *Name;
    else
      *Target = *Pattern;
    Pattern++; Name++; Target++;
  }
  /* The Name ext was shorter than the Pattern one */
  if (*Name == END_OF_STRING)
    while ((*Pattern != END_OF_STRING) &&
	   (*Pattern != '?') && (*Pattern != '*'))
    {
      *Target = *Pattern;
      Pattern++; Target++;
    }
  /* The Pattern ext was shorter than the Name one */
  if (*Pattern == END_OF_STRING)
    while (*Name != END_OF_STRING) Name++;
  if (*Pattern == '*')
  {
    Pattern++;
    *Pattern = END_OF_STRING;
    while (*Name != END_OF_STRING)
    {
      *Target = *Name;
      Name++; Target++;
    }
  }
  *Target = END_OF_STRING;
  return ((*Name == END_OF_STRING) && (*Pattern == END_OF_STRING));
}

/* ------------------------------------------------------------------------ */
/* Check if the file extension indicates a binary file */
bool BinaryFile(const char Filename[])
{
  const char* BinExt[] = { ".EXE", ".COM", ".SYS", ".OBJ", ".LIB", ".BIN", ".DLL" };
  int i = strlen(Filename);

  while (i > 0)
  {
    i--;
    if ((Filename [i] == ':') || (Filename [i] == '\\'))
      return FALSE;	/* No extension -> assume not binary */

    if (Filename [i] == '.')
    {
      char* Ext = (char*)(&(Filename [i]));

      for (i = 0; i < sizeof (BinExt)/sizeof (BinExt[0]); i++)
	if (strcmpi(BinExt[i], Ext) == 0) return TRUE;
      return FALSE;
    }
  }
  return FALSE;		/* No extension -> assume not binary */
}

/* ------------------------------------------------------------------------ */
/* Pointer to the start of the file name with path stripped off */
char* FileNameStart(char Filename[])
{
  char* CurrPos = Filename;
  while (*CurrPos != END_OF_STRING)
  {
    if ((*CurrPos == ':') || (*CurrPos == '\\')) Filename = CurrPos + 1;
    CurrPos++;
  }
  return Filename;
}

/* ************************************************************************ */
			/* Case modifier routines */
				/* First char in table */
#define UPCASE_TABLE_START	((unsigned char)'\x80')
#define UPCASE_TABLE_SIZE	128	/* Number of chars in table */
typedef unsigned char char_table;
char_table far* UpCaseTable;		/* DOS case conversion table */

void UpCaseInit(void)
{
  UpCaseTable = NULL;
  if ((_osmajor > 3) || ((_osmajor >= 3) && (_osminor >= 3)))
  {
    struct {
	char ID;	/* The same present in AL */
	struct {
	    unsigned int Size;	/* UPCASE_TABLE_SIZE */
	    char_table far Table[/*Size*/];
	} far* Data;
    } UpCaseBuf;
    struct REGPACK r;

    /* Ask DOS for the current uppercase table for chars 128..255 */
    r.r_ax = 0x6502;	     /* get upper case table */
    r.r_bx = 0xFFFF;	     /* default codepage     */
    r.r_dx = 0xFFFF;	     /* default country	     */
    r.r_cx = sizeof(UpCaseBuf);
    r.r_es = FP_SEG(&UpCaseBuf); r.r_di = FP_OFF(&UpCaseBuf);
    intr(DOS, &r);
    if (((r.r_flags & CARRY) == 0) &&  /* No carry -> Ok */
	(UpCaseBuf.ID == 2) &&
	(UpCaseBuf.Data->Size == UPCASE_TABLE_SIZE))
       UpCaseTable = UpCaseBuf.Data->Table;
  }
}
/* ------------------------------------------------------------------------ */
unsigned char UpCase(unsigned char c)
{
  if ((c < UPCASE_TABLE_START) || (UpCaseTable == NULL))
    return toupper(c);

  return UpCaseTable[c - UPCASE_TABLE_START];
}
