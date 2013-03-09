/*			FC: FreeDOS File compare

This program implements Paul Heckel's algorithm from the Communications
of the Association for Computing Machinery, April 1978 for detecting the
differences between two files.
This algorithm has the advantage over more commonly used compare algorithms
that it is fast and can detect differences of an arbitrary number of lines.
For most applications the algorithm isolates differences similar to those
isolated by the longest common subsequence.
******************************************************************************
A design limitation of this program is that only the first
MAX_LINES lines are compared; the remaining lines are ignored.
******************************************************************************
Version 2.01: Fixed a conceptual bug in MatchNames
	      Added the /S switch and relevant routines
Version 2.10: Fixed a stupid bug in ScanArguments in case of
	      a comparison of just two files
	      Added cats for easier internationalisation
Version 2.20: Added the /LBn and /nnn switches
	      Used CRC15 as hashing function
Version 2.21: Used the current DOS code page for the upcase routine
Version 2.22: Bug fix for ASCII compare
Version 3.00: Bug fix for the /S switch: failed because of filenames
	      used as directory name
	      Added automatic support for long filenames
	      Added the /R switch for a final report (always active when using /S)
	      Added the /Q switch for suppressing the list of differences
	      Moved some routines to FCTOOLS.C
Version 3.01: Used the simpler kitten instead of cats
Version 3.02: Bug fix: file not found when the target pattern extension was
	      shorter than the source pattern extension
	      Bug fix: ASCII compare result display routine rewritten to avoid
	      an endless loop condition
	      Added the /U switch to show the source files who don't have
	      a correspondent target file
Version 3.03: Bug fix: /M was not allowed
	      Added a switch termination check
*************************************************************************** */

#define USE_KITTEN	/* Define this to use kitten */

#if !defined(__LARGE__)
  #error Must be compiled with the LARGE model.
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <sys\stat.h>
#ifdef USE_KITTEN
  #include "Kitten.h"
#endif
#include "FCTools.h"
/* ------------------------------------------------------------------------ */
#define MAX_LINES			32765
#define DEFAULT_LINES_FOR_RESYNC	2
#define DEFAULT_BINARY_MAX_DIFF		20  /* Bytes */

/* Must be signed and able to count at least up to MAX_LINES */
typedef signed int line_counter;
/* ------------------------------------------------------------------------ */
/* Values for ERRORLEVEL */
#define EL_NO_DIFF		0
#define EL_DIFFERENT		1
#define EL_PARAM_ERROR		2
#define EL_NO_FILE_ERROR	3
#define EL_OPEN_ERROR		4
/* ------------------------------------------------------------------------ */
#define TAB_SIZE	8
#define BITS_PER_BYTE	8
/* ------------------------------------------------------------------------ */
/* Option flags */
struct	{
  int Quiet		:1;
  int Report		:1;
  int ScanSubdirs	:1;
  int LineNumber	:1;
  int IgnoreCase	:1;
  int PackSpace		:1;
  int NoTabExpand	:1;
  int Brief		:1;
  int ShowUnmatched	:1;
} OptFlags = {0};
/* ------------------------------------------------------------------------ */
#define FILE_SPECS	2	/* Two files to be compared each time */
#define FIRST_FILE	0
#define SECOND_FILE	1
/* ------------------------------------------------------------------------ */
#define STANDARD_CONTEXT	1   /* Lines */
#define NO_CONTEXT		0   /* Lines */
/* ------------------------------------------------------------------------ */
FILE *File1, *File2;
char FileSpecs[FILE_SPECS][MAXPATHLFN]; /* Full pathnames */
int MaxBinDifferences = DEFAULT_BINARY_MAX_DIFF;
int MaxASCIIDiffLines = MAX_LINES;	/* Virtually unlimited */
int LinesForResync = DEFAULT_LINES_FOR_RESYNC;
int ContextLines = STANDARD_CONTEXT;	/* 0 = don't show context lines */
enum { DEFAULT, ASCII, BINARY } CompareMode = DEFAULT;
enum { MatchingNames, CommonSource, CommonTarget } NameMatching;
char SourceNamePattern[MAXNAMELFN];
char TargetNamePattern[MAXNAMELFN];
unsigned FileCount, FilesDifferent, UnmatchedCount, DirectoryCount;
/* ------------------------------------------------------------------------ */
const char AsciiDiffStart[] = "***** %s\n";
const char AsciiDiffEnd[] = "*****\n\n";
const char Omissis[] = "...\n";
const char AllFiles[] = "*.*";
const char DummyFileSpec[] = "\\.";
const char InvalidSwitch[] = "Invalid switch: %s";
const char TooManyFiles[] = "Too many filespecs";
const char InvalidFilename[] = "Invalid filename";
const char Differ[] = "The files are different";
const char NoDifferences[] = "No differences";
const char NoCorrespondent[] = "File %s has no correspondent (%s)";

#define NewLine puts("")
/* ------------------------------------------------------------------------ */
/* Kitten section names */
#define HELP		1
#define MESSAGE		2
#define REPORT_TEXT	3
/* ------------------------------------------------------------------------ */
#ifdef USE_KITTEN
  #define Format(setnum, msgnum, message) kittengets(setnum, msgnum, message)
#else
  #define Format(dummy1, dummy2, message)  message
#endif
/* ************************************************************************ */
void HelpMessage(void)
{
  puts("FreeDOS FC version 3.03");
  printf(Format(HELP, 0,"Compares two files or sets of files and displays the differences between them")); NewLine;
  NewLine;
  printf(Format(HELP, 1,"FC [switches] [drive1:][path1]filename1 [drive2][path2]filename2 [switches]")); NewLine;
  printf(Format(HELP, 2," /A    Display only first and last lines for each set of differences")); NewLine;
  printf(Format(HELP, 3," /B    Perform a binary comparison")); NewLine;
  printf(Format(HELP, 4," /C    Disregard the case of letters")); NewLine;
  printf(Format(HELP, 5," /L    Compare files as ASCII text")); NewLine;
  printf(Format(HELP,13," /LBn  Set the maximum number of consecutive different ASCII lines to n")); NewLine;
  printf(Format(HELP, 6," /Mn   Set the maximum differences in binary comparison to n bytes")); NewLine;
  printf(Format(HELP, 7,"       (default = %d, 0 = unlimited, /M = /M0)"),DEFAULT_BINARY_MAX_DIFF); NewLine;
  printf(Format(HELP, 8," /N    Display the line numbers on a text comparison")); NewLine;
  printf(Format(HELP,17," /Q    Don't show the list of differences")); NewLine;
  printf(Format(HELP,16," /R    Show a brief final report (always active when using /S)")); NewLine;
  printf(Format(HELP, 9," /S    Extend the scan to the files in subdirectories")); NewLine;
  printf(Format(HELP,10," /T    Do not expand tabs to spaces")); NewLine;
  printf(Format(HELP,18," /U    Show the filenames of the files without a correspondent")); NewLine;
  printf(Format(HELP,11," /W    Pack white space (tabs and spaces) for text comparison")); NewLine;
  printf(Format(HELP,12," /X    Do not show context lines in text comparison")); NewLine;
  printf(Format(HELP,14," /nnn  Set the minimum number of consecutive matching lines to nnn")); NewLine;
  printf(Format(HELP,15,"       for comparison resynchronization")); NewLine;
}
/* ************************************************************************ */
/* Scan arguments for switches and files */
bool ScanArguments(void)
{
#define MAX_DIGITS	4
  int FileCounter = 0;
  char *Arguments;
  int i;
  char Temp;

  /* Set up a pointer to the argument string */
  Arguments = (char*)MK_FP(_psp, 0x80);
  i = *Arguments;
  Arguments++;
  Arguments[i] = END_OF_STRING;

  /* Arguments parsing */
  while (*Arguments != END_OF_STRING)
    switch (*Arguments)
    {
      case ' ': /* Separators */
      case '\t':
	Arguments++;
	break;

      case '/':
      case '-': /* Switch chars */
	switch (UpCase(Arguments[1]))
	{
	  case 'A':     /* The following two switches are incompatibles */
	    OptFlags.Brief = TRUE;
	    ContextLines = STANDARD_CONTEXT;
	    Arguments += 2;
	    break;

	  case 'X':
	    OptFlags.Brief = FALSE;
	    ContextLines = NO_CONTEXT;
	    Arguments += 2;
	    break;

	  case 'B':
	    CompareMode = BINARY;
	    Arguments += 2;
	    break;

	  case 'L':
	    CompareMode = ASCII;
	    if (UpCase(Arguments[2]) != 'B')
	      Arguments += 2;
	    else
	    {
	      i = 3;
	      while (isdigit(Arguments[i])) i++;
	      Temp = Arguments[i];
	      Arguments[i] = END_OF_STRING;
	      MaxASCIIDiffLines = 0;
	      if ((i > 3) && (i <= 3 + MAX_DIGITS))
		MaxASCIIDiffLines = atoi(&Arguments[3]);
	      if (MaxASCIIDiffLines == 0)
	      {
		printf(Format(MESSAGE,0,(char*)InvalidSwitch), Arguments); NewLine;
		return FALSE;
	      }
	      Arguments += i;
	      *Arguments = Temp;
	    }
	    break;

	  case 'C':
	    OptFlags.IgnoreCase = TRUE;
	    Arguments += 2;
	    break;

	  case 'N':
	    OptFlags.LineNumber = TRUE;
	    Arguments += 2;
	    break;

	  case 'T':
	    OptFlags.NoTabExpand = TRUE;
	    Arguments += 2;
	    break;

	  case 'W':
	    OptFlags.PackSpace = TRUE;
	    Arguments += 2;
	    break;

	  case 'S':     /* The scan of subdirectories implies a final report */
	    OptFlags.ScanSubdirs = TRUE;
	  case 'R':
	    OptFlags.Report = TRUE;
	    Arguments += 2;
	    break;

	  case 'U':
	    OptFlags.ShowUnmatched = TRUE;
	    Arguments += 2;
	    break;

	  case 'Q':
	    OptFlags.Quiet = TRUE;
	    Arguments += 2;
	    break;

	  case 'M':
	    i = 2;
	    while (isdigit(Arguments[i])) i++;
	    Temp = Arguments[i];
	    Arguments[i] = END_OF_STRING;
	    if ((i >= 2) && (i <= 2 + MAX_DIGITS))
	      MaxBinDifferences = atoi(&Arguments[2]);
	    else
	    {
	      printf(Format(MESSAGE,0,(char*)InvalidSwitch), Arguments); NewLine;
	      return FALSE;
	    }
	    Arguments += i;
	    *Arguments = Temp;
	    break;

	  case '?':
	  case 'H':
	    HelpMessage();
	    return FALSE;

	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	    i = 2;
	    while (isdigit(Arguments[i])) i++;
	    Temp = Arguments[i];
	    Arguments[i] = END_OF_STRING;
	    if ((i > 2) && (i <= 2 + MAX_DIGITS))
	      LinesForResync = atoi(&Arguments[2]);
	    else
	    {
	      printf(Format(MESSAGE,0,(char*)InvalidSwitch), Arguments); NewLine;
	      return FALSE;
	    }
	    Arguments += i;
	    *Arguments = Temp;
	    if (LinesForResync < 1) LinesForResync = 1;
	    break;

	  default:
	    Arguments[2] = END_OF_STRING;
	    printf(Format(MESSAGE,0,(char*)InvalidSwitch), Arguments); NewLine;
	    return FALSE;
	}
	/* Check for a correct switch termination */
	switch (*Arguments)
	{
	  case END_OF_STRING:
	  case ' ': /* Separators */
	  case '\t':
	  case '/': /* Switch chars */
	  case '-':
	    break;
	  default:
	    printf(Format(MESSAGE,0,(char*)InvalidSwitch), Arguments); NewLine;
	    return FALSE;
	}
	break;

      case '"': /* (Long) filename start marker */
	Arguments++;
	i = 0;
	while (Arguments[i] != '"')
	{
	  if (Arguments[i] == END_OF_STRING)
	  {
	    printf(Format(MESSAGE,2,(char*)InvalidFilename)); NewLine;
	    return FALSE;
	  }
	  i++;
	}
	Arguments[i] = END_OF_STRING;

	if (FileCounter >= FILE_SPECS)
	{
	  printf(Format(MESSAGE,1,(char*)TooManyFiles)); NewLine;
	  return FALSE;
	}

	if (FullPath(FileSpecs[FileCounter], Arguments,
		     sizeof(FileSpecs[0])) == NULL)
	{
	  printf(Format(MESSAGE,2,(char*)InvalidFilename)); NewLine;
	  return FALSE;
	}
	Arguments += i + 1;
	FileCounter++;
	break;

      default:	/* Must be a filename */
	if (FileCounter >= FILE_SPECS)
	{
	  printf(Format(MESSAGE,1,(char*)TooManyFiles)); NewLine;
	  return FALSE;
	}
	i = 0;
	while ((Arguments[i] != ' ') && (Arguments[i] != '\t') &&
	       (Arguments[i] != END_OF_STRING))
	  i++;
	Temp = Arguments[i];
	Arguments[i] = END_OF_STRING;
	if (FullPath(FileSpecs[FileCounter], Arguments,
		     sizeof(FileSpecs[0])) == NULL)
	{
	  printf(Format(MESSAGE,2,(char*)InvalidFilename)); NewLine;
	  return FALSE;
	}
	Arguments += i;
	*Arguments = Temp;
	FileCounter++;
    }

  switch (FileCounter)
  {
    case 0:
      printf(Format(MESSAGE,3,"No file specified")); NewLine;
      return FALSE;

    case 1:		/* Default: the current directory */
      strcpy(FileSpecs[SECOND_FILE], AllFiles);
  }

  for (i = FIRST_FILE; i < FILE_SPECS; i++)
  {
    int LastChar = strlen(FileSpecs[i]) - 1;
    if ((FileSpecs[i][LastChar] == '\\') ||  /* Path only */
	(FileSpecs[i][LastChar] == ':'))     /* Disk drive only */
					     /* Assume "All files" */
      strcpy(&FileSpecs[i][LastChar + 1], AllFiles);
    else
      if (!HasWildcards(FileSpecs[i]) &&
	  IsADirectory(FileSpecs[i]))	     /* Directory */
					     /* Assume "All files" */
	sprintf(&FileSpecs[i][LastChar + 1], "\\%s", AllFiles);
   /* else regular file (possibly with wildcards) */
  }

  {
    /* Pointers to the name part of the files */
    char* FirstFileName = FileNameStart(FileSpecs[FIRST_FILE]);
    char* SecondFileName = FileNameStart(FileSpecs[SECOND_FILE]);

    NameMatching = MatchingNames;
    if (!HasWildcards(SecondFileName))
      NameMatching = CommonTarget;
    if (!HasWildcards(FirstFileName) &&
	(strcmp(SecondFileName, AllFiles) != 0))
      NameMatching = CommonSource;

    strncpy(SourceNamePattern, FirstFileName,
	    sizeof(SourceNamePattern) - 1);
    SourceNamePattern[sizeof(SourceNamePattern) - 1] = END_OF_STRING;

    strncpy(TargetNamePattern, SecondFileName,
	    sizeof(TargetNamePattern) - 1);
    TargetNamePattern[sizeof(TargetNamePattern) - 1] = END_OF_STRING;
  }
  return TRUE;
}

/* ************************************************************************ */
bool BinaryCompare(void)
{
  unsigned char a, b;
  unsigned long Offset, FileSize1, FileSize2;
  int Differences = 0;

  FileSize1 = filelength(fileno(File1));
  FileSize2 = filelength(fileno(File2));
  if (FileSize1 != FileSize2)
  {
    if (OptFlags.Quiet)
    {
      printf(Format(MESSAGE,13,"The files are of different size")); NewLine;
      NewLine;
      return FALSE;
    }
    printf(Format(MESSAGE,4,"Warning: the files are of different size!")); NewLine;
    if (FileSize1 < FileSize2) FileSize1 = FileSize2;
  }
  for (Offset = 0; Offset < FileSize1; Offset++)
  {
    a = fgetc(File1); b = fgetc(File2);
    if (a != b)
    {
      if (OptFlags.Quiet)
      {
	printf(Format(MESSAGE,14,(char*)Differ)); NewLine;
	NewLine;
	return FALSE;
      }
      Differences++;
      if ((MaxBinDifferences > 0) && (Differences > MaxBinDifferences))
      {
	printf(Format(MESSAGE,5,"Comparison stopped after %d mismatches"),
	       MaxBinDifferences); NewLine;
	NewLine;
	return FALSE;
      }
      printf("%08lX:     %02X  %c      %02X  %c\n", Offset,
	     a, (iscntrl(a) ? ' ' : a), b, (iscntrl(b) ? ' ' : b));
    }
  }
  if (Differences == 0)
  {
    printf(Format(MESSAGE,6,(char*)NoDifferences)); NewLine;
    NewLine;
    return TRUE;
  }
  return FALSE;
}

/* ************************************************************************ */
/* Skip to the next line in the file File */
void SkipLine(FILE* File)
{
  int c;

  do
    c = fgetc(File);
  while ((c != '\n') && (c != EOF));
}
/* ------------------------------------------------------------------------ */
/* Display a line from the file File */
void OutputLine(FILE* File, line_counter LineNumber)
{
  unsigned long LineLength = 0;
  int c;

  if (OptFlags.LineNumber) printf("%5u: ", LineNumber);

  while (TRUE)
  {
    c = fgetc(File);
    if ((c == '\n') || (c == EOF)) /* End of line */
    {
      putchar('\n'); break;
    }
    if ((c == '\t') && !OptFlags.NoTabExpand) /* Tab */
      do
      {
	putchar(' '); LineLength++;
      } while ((LineLength % TAB_SIZE) != (TAB_SIZE - 1));
    else
      if (c != '\r')    /* Ignore the carriage return */
      {
	putchar(c); LineLength++;
      }
  }
}
/* ************************************************************************ */
		/* Hashing procedures */

#define CRC_BITS	15	/* 15 bits CRC */
#define CRC_POLY	0xC599	/* X^15+X^14+X^10+X^8+X^7+X^4+X^3+X^0 */
#define CRC_ARRAY_ITEMS (1 << BITS_PER_BYTE)
#define CRC_ARRAY_SIZE	(sizeof(*CRCTable)*CRC_ARRAY_ITEMS)
#define HASH_BUCKETS	(1 << CRC_BITS)
#define HASH_ARRAY_SIZE (sizeof(hashing_values)*MAX_LINES)

typedef signed int hashing_values; /* Must be signed */
typedef hashing_values* hashing_values_ptr;
hashing_values HashValue;
typedef unsigned int crc_values;   /* Max value = (1 << CRC_BITS) - 1 */
typedef crc_values* crc_values_ptr;
crc_values_ptr CRCTable = NULL;
/* ------------------------------------------------------------------------ */
/* Initialise the CRC register and create (only once) the CRC table
   used to speed-up the hashing function */
void InitCRC(void)
{
  int i, j;
  crc_values CRC;

  HashValue = (hashing_values)(-1);	/* Initialise the CRC register */
  if (CRCTable != NULL) return;		/* Table already created */

  CRCTable = (crc_values_ptr)malloc(CRC_ARRAY_SIZE);
  if (CRCTable == NULL) return;		/* Insufficient memory */

  for (i = 0; i < CRC_ARRAY_ITEMS; i++)
  {
    CRC = i << (CRC_BITS - BITS_PER_BYTE);
    for (j = 0; j < BITS_PER_BYTE; j++)
    {
      CRC <<= 1;
      if ((CRC & (1 << CRC_BITS)) != 0) CRC ^= CRC_POLY;
    }
    CRCTable[i] = CRC;
  }
}
/* ------------------------------------------------------------------------ */
/* Hash a character */
void Hash(char c)
{
  /* Update the CRC */
  HashValue = (HashValue << (CRC_BITS - BITS_PER_BYTE)) ^
	      CRCTable[c ^ ((HashValue >> (CRC_BITS - BITS_PER_BYTE)) & 0xFF)];
}

/* ************************************************************************ */
/* occurr indicate the number of occurrences for a hash value. */
typedef enum { NO_OCCURR, SINGLE_OCCURR, MULTIPLE_OCCURR } occurr;
/* Minimum number of bits to represent occurr */
#define BITS_PER_OCCUR	   2
/* The occurrence data codes are packed to save memory */
typedef unsigned char packed_occurr;
typedef packed_occurr* packed_occurr_ptr;
#define OCCUR_PER_BYTE	   ((BITS_PER_BYTE*sizeof(packed_occurr))/BITS_PER_OCCUR)
#define OCCUR_BIT_MASK	   ((1 << BITS_PER_OCCUR) - 1)
#define PACKED_ARRAY_ITEMS ((HASH_BUCKETS + OCCUR_PER_BYTE - 1)/OCCUR_PER_BYTE)
#define OCCURR_ARRAY_SIZE  (sizeof(packed_occurr)*PACKED_ARRAY_ITEMS)

/* ------------------------------------------------------------------------ */
/* Extract bits from the packed occurrence array
   Index must be >= 0 */
occurr GetOccurrences(const packed_occurr Array[], hashing_values Index)
{
  Index %= HASH_BUCKETS;
  return ((Array[Index/OCCUR_PER_BYTE] >>
	   (BITS_PER_OCCUR*(Index % OCCUR_PER_BYTE))) & OCCUR_BIT_MASK);
}
/* ------------------------------------------------------------------------ */
/* Store bits in the packed occurrence array
   Index must be >= 0 */
void SetOccurrences(packed_occurr Array[], hashing_values Index, occurr Occurrence)
{
  unsigned Shifts;

  Index %= HASH_BUCKETS;
  Shifts = BITS_PER_OCCUR*(Index % OCCUR_PER_BYTE);
  Index /= OCCUR_PER_BYTE;
  Array[Index] &= ~(OCCUR_BIT_MASK << Shifts);
  Array[Index] |= (Occurrence << Shifts);
}

/* ************************************************************************ */
/* Read file, build hash & occurrence tables.
   Returns the number of lines in the file. */
line_counter HashFile(FILE* file, hashing_values HashVect[],
		      packed_occurr Occurrences[])
{
  int c;
  bool Skip = FALSE;
  line_counter LineCounter = 0;
  unsigned long LineLength = 0;

  HashValue = 0;
  do
  {
    c = fgetc(file);
    /* Beware of order! There are fall-throughs */
    switch (c)
    {
      case EOF:
	if (LineLength == 0) break;	/* End of file */
	/* Unterminated line: assume line feed */
      case '\n':
	/* This is needed to avoid problems with trailing
	   blanks when blanks are packed */
	if (!Skip) Hash(' ');

	/* HashValue must be > 0 and HashVect[] must be < 0 */
	if (HashValue < 0)
	  HashValue = -HashValue;
	else
	  if (HashValue == 0) HashValue = 1;
	HashVect[LineCounter] = -HashValue;
	switch (GetOccurrences(Occurrences, HashValue))
	{
	  case NO_OCCURR:
	    SetOccurrences(Occurrences, HashValue, SINGLE_OCCURR);
	    break;
	  case SINGLE_OCCURR:
	    SetOccurrences(Occurrences, HashValue, MULTIPLE_OCCURR);
	}
	HashValue = 0; LineCounter++; LineLength = 0;
	Skip = FALSE;
	break;

      case '\t':
	/* Tabs must be expanded and blanks are not packed */
	if (!OptFlags.NoTabExpand && !OptFlags.PackSpace)
	{
	  do
	  {
	    Hash(' '); LineLength++;
	  } while ((LineLength % TAB_SIZE) != 0);
	  break;
	}
      case ' ':
      case '\r':
	/* Blank and tab are packed */
	if (OptFlags.PackSpace)
	{
	  if (!Skip)
	  {
	    Hash(' '); LineLength++;
	    Skip = TRUE;
	  }
	  break;
	}
      default:
	if (OptFlags.IgnoreCase) c = UpCase(c);
	Hash(c); LineLength++;
	Skip = FALSE;
    }
  } while ((c != EOF) && (LineCounter < MAX_LINES));

  if (LineCounter >= MAX_LINES)
  {
    printf(Format(MESSAGE,7,"Warning: comparison interrupted after %d lines"),
	   LineCounter); NewLine;
  }
  return LineCounter;
}

/* ************************************************************************ */
/* Show a block of differences */
void ShowBlockOfDiff(line_counter Start, line_counter Finish,
		     FILE* File, char FileName[],
		     line_counter* LinesRead, line_counter LinesInFile)
{
  line_counter LineCounter = *LinesRead;

		    /* Go to the starting line (of the context) */
  while (LineCounter + ContextLines < Start)
  {
    LineCounter++; SkipLine(File);
  }
  *LinesRead = LineCounter;

  /* If there are no differences and the context is not required
     we are finished */
  if ((Finish < Start) && (ContextLines <= NO_CONTEXT)) return;

  printf(AsciiDiffStart, FileName);
		    /* Show the starting context if needed */
  while (LineCounter < Start)
  {
    LineCounter++; OutputLine(File, LineCounter);
  }
		    /* Show the differing lines */
  if (OptFlags.Brief)
  {
    printf(Omissis);
    while (LineCounter < Finish)
    {
      LineCounter++; SkipLine(File);
    }
  }
  else
    while (LineCounter < Finish)
    {
      LineCounter++; OutputLine(File, LineCounter);
    }
		    /* Show the final context */
  while ((LineCounter < LinesInFile) &&
	 (LineCounter < Finish + ContextLines))
  {
    LineCounter++; OutputLine(File, LineCounter);
  }

  *LinesRead = LineCounter;
}
/* ------------------------------------------------------------------------ */
bool AsciiCompare(void)
{
  bool Linked;
  bool Different = FALSE;
  line_counter i;
  line_counter LinesInFile1, LinesInFile2, LinesRead1, LinesRead2;
  line_counter Line1, Line2;
  hashing_values_ptr Hash1, Hash2;
  /* Arrays of packed occurrences */
  packed_occurr_ptr Occurr1, Occurr2;

  InitCRC();	/* Initialise the CRC for hashing */

  /* Allocate the big structures on the heap */
  Occurr1 = (packed_occurr_ptr)malloc(OCCURR_ARRAY_SIZE);
  Occurr2 = (packed_occurr_ptr)malloc(OCCURR_ARRAY_SIZE);
  Hash1 = (hashing_values_ptr)malloc(HASH_ARRAY_SIZE);
  Hash2 = (hashing_values_ptr)malloc(HASH_ARRAY_SIZE);
  if ((CRCTable == NULL) ||
      (Hash1 == NULL) || (Hash2 == NULL) ||
      (Occurr1 == NULL) || (Occurr2 == NULL))
  {
    printf(Format(MESSAGE,8,"Insufficient memory")); NewLine;
    return FALSE;
  }
  memset(Hash1, 0, HASH_ARRAY_SIZE);
  memset(Hash2, 0, HASH_ARRAY_SIZE);
  memset(Occurr1, 0, OCCURR_ARRAY_SIZE); /* NO_OCCURR */
  memset(Occurr2, 0, OCCURR_ARRAY_SIZE); /* NO_OCCURR */

  /* Step 1: read files and hash them */
  LinesInFile1 = HashFile(File1, Hash1, Occurr1);
  LinesInFile2 = HashFile(File2, Hash2, Occurr2);

  /* Step 2: identify lines that are unique in both files
	     i.e. have a single occur of a hash value
     N.B. Warning: to speed things up this exploits the peculiarity
	  of the definition of occurr */
  for (i = 0; i < PACKED_ARRAY_ITEMS; i++) Occurr1[i] &= Occurr2[i];

  /* Step 3: link together the nearest matching unique lines
     N.B. Hashx is originally < 0 and is set to the matching
	  line number (>= 0) if a match is found */
  for (i = 0; i < LinesInFile1; i++)
    if (GetOccurrences(Occurr1, -Hash1[i]) == SINGLE_OCCURR)
    {
      line_counter Forward = i, Backward = i;
      while ((Forward < LinesInFile2) || (Backward >= 0))
      {
	if (Forward < LinesInFile2)
	{
	  if (Hash2[Forward] == Hash1[i])
	  {
	    Hash1[i] = Forward; Hash2[Forward] = i;
	    break;
	  }
	  Forward++;
	}

	if (Backward >= 0)
	{
	  if (Hash2[Backward] == Hash1[i])
	  {
	    Hash1[i] = Backward; Hash2[Backward] = i;
	    break;
	  }
	  Backward--;
	}
      }
    }
  free (Occurr1); free (Occurr2);	/* No more needed */

  /* Step 4: link the first and last lines, if possible */
  if ((Hash1[0] < 0) && (Hash1[0] == Hash2[0])) Hash1[0] = Hash2[0] = 0;
  if ((Hash1[LinesInFile1 - 1] < 0) &&
      (Hash1[LinesInFile1 - 1] == Hash2[LinesInFile2 - 1]))
  {
    Hash1[LinesInFile1 - 1] = LinesInFile2 - 1;
    Hash2[LinesInFile2 - 1] = LinesInFile1 - 1;
  }

  /* Step 5: starting from linked lines, link following lines that match
     N.B. Hashx was set to matching line number (>= 0) if match found */
  Linked = FALSE;
  for (i = 0; i < LinesInFile1; i++)
    if (Hash1[i] >= 0)
      Linked = TRUE;
    else
      if (Linked)
      {
	Line2 = Hash1[i - 1] + 1;
	if (Hash1[i] == Hash2[Line2])
	{
	  Hash1[i] = Line2;
	  Hash2[Line2] = i;
	}
	else
	  Linked = FALSE;
      }

  /* Step 6: link matching lines that precede linked lines */
  Linked = FALSE;
  for (i = LinesInFile1 - 1; i >= 0; i--)
    if (Hash1[i] >= 0)
      Linked = TRUE;
    else
      if (Linked)
      {
	Line2 = Hash1[i + 1] - 1;
	if (Hash1[i] == Hash2[Line2])
	{
	  Hash1[i] = Line2;
	  Hash2[Line2] = i;
	}
	else
	  Linked = FALSE;
      }

  /* Step 7: display the results (greedy algorithm) */
  free (Hash2);		/* No more needed: redundant */
  rewind(File1); rewind(File2);
  LinesRead1 = LinesRead2 = 0;
  Line1 = Line2 = 0;
  while (TRUE)
  {
    line_counter i;
    line_counter ResyncLine;
			      /* Skip matching lines */
    while ((Line1 < LinesInFile1) && (Line2 < LinesInFile2) &&
	   (Hash1[Line1] == Line2))
    {
      Line1++; Line2++;
    }
			      /* Compare terminated */
    if ((Line1 >= LinesInFile1) && (Line2 >= LinesInFile2)) break;

    if (OptFlags.Quiet)
    {
      printf(Format(MESSAGE,14,(char*)Differ)); NewLine;
      NewLine;
      Different = TRUE;
      break;
    }

    ResyncLine = Line1;
    i = 0;
    do		    /* Find the end of the mismatching block */
    {
      ResyncLine += i;

      i = 0;	    /* Skip mismatching lines */
      while ((ResyncLine + 1 < LinesInFile1) &&
	     ((Hash1[ResyncLine] <= Line2) ||
	      (Hash1[ResyncLine] + 1 != Hash1[ResyncLine + 1])))
      {
	i++; ResyncLine++;
	if (i > MaxASCIIDiffLines)
	{
	  printf(Format(MESSAGE,12,"Resync failed: files too different")); NewLine;
	  NewLine;
	  Different = TRUE;
	  break;
	}
      }
      /* Found at least 2 consecutive line matching unless eof */

      i = ResyncLine + 1; /* Count matching lines */
      while ((i - ResyncLine < LinesForResync) && (i < LinesInFile1) &&
	     (Hash1[i] >= 0) && (Hash1[i] - 1 == Hash1[i - 1]))
	i++;

      i -= ResyncLine;
    } while ((i < LinesForResync) && (ResyncLine < LinesInFile1));

    ShowBlockOfDiff(Line1, ResyncLine, File1, FileSpecs[FIRST_FILE],
		    &LinesRead1, LinesInFile1);
    Line1 = ResyncLine;
    if (ResyncLine < LinesInFile1)
      ResyncLine = Hash1[ResyncLine];
    else
      ResyncLine = LinesInFile2; /* No resync up to the end of the file */
    ShowBlockOfDiff(Line2, ResyncLine,
		    File2, FileSpecs[SECOND_FILE],
		    &LinesRead2, LinesInFile2);
    Line2 = ResyncLine;
    printf(AsciiDiffEnd);
    Different = TRUE;
  }
  free (Hash1);

  if (!Different)
  {
    printf(Format(MESSAGE,6,(char*)NoDifferences)); NewLine;
    NewLine;
    return TRUE;
  }
  return FALSE;
}

/* ************************************************************************ */
/* Warning: this procedure alters the name part of FileSpecs
	    but leaves the path part unchanged */
bool CompareSetOfFiles(void)
{
  find_data FindData1, FindData2;
  bool Done1, Done2;
  /* Pointers to the name part of the files */
  char* FirstFileName = FileNameStart(FileSpecs[FIRST_FILE]);
  char* SecondFileName = FileNameStart(FileSpecs[SECOND_FILE]);

  /* Avoid path length overflow */
  if (FirstFileName - FileSpecs[FIRST_FILE] + strlen (SourceNamePattern) >=
      sizeof(FileSpecs[FIRST_FILE]))
    return FALSE;
  strcpy(FirstFileName, SourceNamePattern);

  Done1 = (FindFirst(FileSpecs[FIRST_FILE], &FindData1, FA_RDONLY) != 0);
  while (!Done1)
  {
    /* Avoid path length overflow */
    if (FirstFileName - FileSpecs[FIRST_FILE] + strlen (FindData1.Filename) >=
	sizeof(FileSpecs[FIRST_FILE]))
    {
      FindClose(&FindData1);
      return FALSE;
    }
    strcpy(FirstFileName, FindData1.Filename);
    Done2 = FALSE;
    switch (NameMatching)
    {
      case MatchingNames:
	Done2 = !MatchNames(TargetNamePattern, FirstFileName, SecondFileName);
	break;

      case CommonSource:
	Done2 = (FindFirst(FileSpecs[SECOND_FILE], &FindData2, FA_RDONLY) != 0);
	if (Done2)
	{
	  if (OptFlags.ShowUnmatched)
	  {
	    printf(Format(MESSAGE,15,(char*)NoCorrespondent),
		   FileSpecs[FIRST_FILE], FileSpecs[SECOND_FILE]); NewLine;
	    NewLine;
	  }
	  UnmatchedCount++;
	}
	else
	  /* Avoid path length overflow */
	  if (SecondFileName - FileSpecs[SECOND_FILE] +
	      strlen (FindData2.Filename) >= sizeof(FileSpecs[SECOND_FILE]))
	    Done2 = TRUE;
	  else
	    strcpy(SecondFileName, FindData2.Filename);
	break;
    }

    while (!Done2)
    {
      File2 = FileOpen(FileSpecs[SECOND_FILE], "rb");
      if (File2 == NULL)
      {
	if (OptFlags.ShowUnmatched)
	{
	  printf(Format(MESSAGE,15,(char*)NoCorrespondent),
		 FileSpecs[FIRST_FILE], FileSpecs[SECOND_FILE]); NewLine;
	  NewLine;
	}
	UnmatchedCount++;
      }
      else
      {
	File1 = FileOpen(FileSpecs[FIRST_FILE], "rb");
	if (File1 == NULL)
	{
	  fcloseall();
	  printf(Format(MESSAGE,9,"Error opening file %s"),
		 FileSpecs[FIRST_FILE]); NewLine;
	  FindClose(&FindData1);
	  if (NameMatching == CommonSource) FindClose(&FindData2);
	  return FALSE;
	}
	FileCount++;
	printf(Format(MESSAGE,10,"Comparing %s and %s"),
	       FileSpecs[FIRST_FILE], FileSpecs[SECOND_FILE]); NewLine;
	switch (CompareMode)
	{
	  case ASCII:
	    if (!AsciiCompare()) FilesDifferent++;
	    break;

	  case BINARY:
	    if (!BinaryCompare()) FilesDifferent++;
	    break;

	  default:
	    if (BinaryFile(FileSpecs[FIRST_FILE]) ||
		BinaryFile(FileSpecs[SECOND_FILE]))
	    {
	      if (!BinaryCompare()) FilesDifferent++;
	    }
	    else
	      if (!AsciiCompare()) FilesDifferent++;
	}
	fcloseall();
      }

      Done2 = TRUE;
      if (NameMatching == CommonSource)
      {
	Done2 = (FindNext(&FindData2) != 0);
	if (!Done2)
	  /* Avoid path length overflow */
	  if (SecondFileName - FileSpecs[SECOND_FILE] +
	      strlen (FindData2.Filename) >= sizeof(FileSpecs[SECOND_FILE]))
	    Done2 = TRUE;
	  else
	    strcpy(SecondFileName, FindData2.Filename);
      }
    }
    Done1 = (FindNext(&FindData1) != 0);
  }
  return TRUE;
}
/* ------------------------------------------------------------------------ */
/* Warning: this procedure appends data to FileSpecs */
bool ScanSubdirs(void)
{
  find_data FindData1;
  char* FirstSubdirName;  /* Pointers to the subdirectory names */
  char* SecondSubdirName;

  FirstSubdirName = FileNameStart(FileSpecs[FIRST_FILE]);
  SecondSubdirName = FileNameStart(FileSpecs[SECOND_FILE]);

  /* Avoid path length overflow */
  if (FirstSubdirName - FileSpecs[FIRST_FILE] + strlen (AllFiles) >=
      sizeof(FileSpecs[FIRST_FILE]))
    return FALSE;
  strcpy(FirstSubdirName, AllFiles); /* All subdirectories */

  if (FindFirst(FileSpecs[FIRST_FILE], &FindData1, FA_DIREC | FA_RDONLY) != 0)
    return FALSE;

  do
    if (((FindData1.Attributes & FA_DIREC) != 0) && /* Is a directory */
	(*(FindData1.Filename) != '.'))
    {
      find_data FindData2;
      /* Avoid path length overflow */
      if (SecondSubdirName - FileSpecs[SECOND_FILE] +
	  strlen (FindData1.Filename) >
	  sizeof(FileSpecs[SECOND_FILE]) - sizeof(DummyFileSpec))
      {
	FindClose(&FindData1);
	return FALSE;
      }
      /* If the same subdirectory exists in the second path */
      strcpy(SecondSubdirName, FindData1.Filename);
      FileSpecs[SECOND_FILE][sizeof(FileSpecs[SECOND_FILE]) - 1] = END_OF_STRING;
      if (FindFirst(FileSpecs[SECOND_FILE], &FindData2, FA_DIREC | FA_RDONLY) == 0)
      {
	FindClose(&FindData2);
	if ((FindData2.Attributes & FA_DIREC) == 0) break; /* Not a directory */

	/* Avoid path length overflow */
	if (FirstSubdirName - FileSpecs[FIRST_FILE] +
	    strlen (FindData1.Filename) >
	    sizeof(FileSpecs[FIRST_FILE]) - sizeof(DummyFileSpec))
	{
	  FindClose(&FindData1);
	  return FALSE;
	}
	strcpy(FirstSubdirName, FindData1.Filename);
	FileSpecs[FIRST_FILE][sizeof(FileSpecs[FIRST_FILE]) - 1] = END_OF_STRING;

	/* Do compare the files in the subdirs */
	DirectoryCount++;
	strcat(FileSpecs[FIRST_FILE], DummyFileSpec);
	strcat(FileSpecs[SECOND_FILE], DummyFileSpec);
	if (!CompareSetOfFiles() || !ScanSubdirs())
	{
	  FindClose(&FindData1);
	  return FALSE;
	}
      }
  } while (FindNext(&FindData1) == 0);
  return TRUE;
}
/* ------------------------------------------------------------------------ */
void OnExit(void)
{
  /* If created destroy the CRC speed-up table */
  if (CRCTable != NULL) free(CRCTable);
#ifdef USE_KITTEN
  kittenclose();	/* Close the NLS catalog */
#endif
}
/* ------------------------------------------------------------------------ */
int main(void)
{
#ifdef USE_KITTEN
  kittenopen("FC");     /* Try opening NLS catalog */
#endif
  atexit (OnExit);	/* Install the clean-up procedure */
  UpCaseInit();		/* Initialize the table for uppercase conversion */

  if (!ScanArguments()) return EL_PARAM_ERROR;

  FileCount = FilesDifferent = UnmatchedCount = 0;
  DirectoryCount = 1;

  if (!CompareSetOfFiles()) return EL_OPEN_ERROR;

  if (OptFlags.ScanSubdirs)
    if (!ScanSubdirs()) return EL_OPEN_ERROR;

  if (OptFlags.Report)
  {
    printf(Format(REPORT_TEXT,0,"Compared %d files"),FileCount);
    if (OptFlags.ScanSubdirs)
      printf(Format(REPORT_TEXT,1," in %d directories"),DirectoryCount);
    NewLine;
    printf(Format(REPORT_TEXT,2,"%d files match, %d files are different"),
	   FileCount - FilesDifferent, FilesDifferent); NewLine;
    if ((NameMatching == MatchingNames) || (UnmatchedCount > 0))
    {
      printf(Format(REPORT_TEXT,3,"%d files have no correspondent"),
	     UnmatchedCount);
      NewLine;
    }
  }

  if ((FileCount == 0) && !OptFlags.Report)
  {
    printf(Format(MESSAGE,11,"No such file or directory")); NewLine;
    return EL_NO_FILE_ERROR;
  }

  if (FilesDifferent > 0) return EL_DIFFERENT;
  return EL_NO_DIFF;
}
