#ifndef __FCTOOLS__
#define __FCTOOLS__
			/* Auxiliary routines for FC */
#include <stdio.h>
#include <dos.h>
#include <dir.h>

typedef unsigned char bool;
#define FALSE		0
#define TRUE		(!FALSE)

#define END_OF_STRING	'\0'
/* ------------------------------------------------------------------------ */
#define MAXPATHLFN	261
#define MAXNAMELFN	256

/* ************************************************************************ */
typedef struct
{
  bool UseLFN;			/* TRUE if LFN supported */
  char Filename[MAXNAMELFN];
  unsigned char Attributes;
  int Handle;			/* Handle for LFN FindNext */
  struct ffblk SearchRec;	/* Standard DOS search record */
} find_data;

int FindFirst(const char* PathName, find_data* FindData, int Attrib);
int FindNext(find_data* FindData);
int FindClose(find_data* FindData);
char* FullPath(char* Buffer, char *Path, int BufferSize);
FILE* FileOpen(const char* Filename, const char* Mode);
/* ************************************************************************ */
bool IsADirectory(char Filename[]);
bool HasWildcards(char* Filename);
/* ------------------------------------------------------------------------ */
/* Try to find a match between the filename pattern in Pattern and the
   filename in Name. If the match is found, the result is in Target and
   the function returns TRUE */
bool MatchNames(char Pattern[], char Name[], char Target[]);
/* ------------------------------------------------------------------------ */
/* Check if the file extension indicates a binary file */
bool BinaryFile(const char Filename[]);
/* ------------------------------------------------------------------------ */
/* Pointer to the start of the file name with path stripped off */
char* FileNameStart(char Filename[]);
/* ************************************************************************ */
/* Case modifier routines */
void UpCaseInit(void);
unsigned char UpCase(unsigned char c);

#endif
