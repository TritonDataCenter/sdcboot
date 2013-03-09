#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mkhelp.h"
#include "checksum.h"

#define VERBOSE

static void TrimBuffer(char* buffer);
static int  InitialiseHeader(FILE* ofptr, int filecount);
static int SetHelpFilePosition(FILE* ofptr, int pos, unsigned long offset,
                          int index);
static int SetCurrentHlpFilePosition(FILE* ofptr, int pos, int index);
static int SetOrigFileLength(FILE* ofptr, int pos, unsigned long length);

static int  CountFiles(FILE* fptr);
static int  MoveTextFiles(FILE* ifptr, FILE* ofptr);
static int  AddTextFile(char* buffer, FILE* ofptr, int pos);

static int ParseListFileLine(char* buffer, char** file, int* pagenum);

/* Indexes of the data to write to the header. */
#define PAGE_START_POSITION   0
#define PAGE_STOP_POSITION    1
#define PAGE_ORIGLEN_POSITION 2

int CreateHelpFile(char* listfile, char* helpfile)
{
     FILE *ifptr, *ofptr;
     int filecount, i;
     unsigned csum;

     ifptr = fopen(listfile, "rb");
     if (!ifptr)
     {
        printf("Listfile cannot be opened.");
        return 1;
     }

     ofptr = fopen(helpfile, "wb");
     if (!ofptr)
     {
   fclose(ifptr);
        printf("Unable to open destination help file.");
        return 1;
     }

     filecount = CountFiles(ifptr);
     if (filecount == 0) return 0;

     if ((filecount < 0)                      ||
    (InitialiseHeader(ofptr, filecount)) ||
         (MoveTextFiles(ifptr, ofptr)))
     {
   printf("Error processing listfile.\n");

   fclose(ifptr);
   fclose(ofptr);

        return 1;
     }

     fclose(ifptr);
     fclose(ofptr);

     csum = CalculateCheckSum(helpfile);

#ifdef VERBOSE
     printf("Checksum = %X\n", csum);
#endif

     ofptr = fopen(helpfile, "ab");
     if (!ofptr)
     {
   fclose(ifptr);
        printf("Unable to open destination help file.");
        return 1;
     }

     fseek(ofptr, 0, SEEK_END);

     for (i = 0; i < sizeof(unsigned); i++)
    fputc(*(((char*)(&csum))+i), ofptr);

     fclose(ofptr);

     return 0;
}


static void TrimBuffer(char* buffer)
{
   char *begin = buffer, *end;

   if (!buffer || !(*buffer)) return;

   end = strchr(buffer, NULL)-1;

   while ((end != buffer) &&
     ((*end == ' ') || (*end == '\t') || (*end == '\r') || (*end == '\n')))
    end--;

   if (end == buffer)
      *buffer = '\0';
   else
      *(end+1) = '\0';

   while ((*begin == ' ') || (*begin == '\t')) begin++;

   if (*begin == '#')
      buffer[0] = '\0';
   else  
      strcpy(buffer, begin);
}

static int InitialiseHeader(FILE* ofptr, int filecount)
{
     int i, j;
     fseek(ofptr, 0, SEEK_SET);

     /*
        If compression is added, filecount should be interpreted as an
        unsigned 15 bit number and the 16th bit should be 1 to indicate
        compression.
     */
     for (i = 0; i < sizeof(int); i++)
         fputc(*(((char*)(&filecount))+i), ofptr);

     for (i = 0; i < filecount; i++)
    for (j = 0; j < sizeof(unsigned long)*3; j++)
        if (fputc(0, ofptr) == EOF)
                return 1;

     return 0;
}

static int ParseListFileLine(char* linetxt, char** file, int* pagenum)
{
   char* sep = strchr(linetxt, ':');
   if (!sep || (sep == linetxt)) 
      return 1;
   
   *pagenum = atoi(linetxt);
   *file    = sep+1;
   if (**file == '\0') 
      return 1;
      
   return 0;     
}

static int SetHelpFilePosition(FILE* ofptr, int pos, unsigned long offset,
                          int index)
{
     int i;
     unsigned long fpos = ftell(ofptr);

     if (fseek(ofptr, pos * sizeof(unsigned long)*3+
                       sizeof(unsigned long)*index+
                            sizeof(int), SEEK_SET))
        return 1;

     for (i = 0; i < sizeof(unsigned long); i++)
    if (fputc(*(((char*)(&offset))+i), ofptr) == EOF)
       return 1;

     if (fseek(ofptr, fpos, SEEK_SET))
        return 1;

     return 0;
}

static int SetCurrentHlpFilePosition(FILE* ofptr, int pos, int index)
{
     return SetHelpFilePosition(ofptr, pos, ftell(ofptr), index);
}

static int SetOrigFileLength(FILE* ofptr, int pos, unsigned long length)
{
     return SetHelpFilePosition(ofptr, pos, length, PAGE_ORIGLEN_POSITION);
}

static int CountFiles(FILE* fptr)
{
     char* result;
     int   counter = 0;
     char  buffer[1024];

     fseek(fptr, 0, SEEK_SET);

     for (;;)
     {
         result = fgets(buffer, 1024, fptr);

         if (!result) break;

         TrimBuffer(buffer);
         if (buffer[0] != '\0')
            counter++;
     }

     return counter;
}

static int MoveTextFiles(FILE* ifptr, FILE* ofptr)
{
     char buffer[1024];
     char *result, *file;
     int pagenum=0;

     fseek(ifptr, 0, SEEK_SET);

     for (;;)
     {
         result = fgets(buffer, 1024, ifptr);

         if (!result) break;

         TrimBuffer(buffer);
         if (buffer[0] != '\0')
         {
             /* A line in the help file is of the form <pagenum>:<file>
                Get the pagenumber and the file name. */
             if (ParseListFileLine(buffer, &file, &pagenum))
                return 1;
         
            /* Position of the start of the help page in the output
               help file */     
       if (SetCurrentHlpFilePosition(ofptr, pagenum, PAGE_START_POSITION))
          return 1;
            /* The original length of the help page */   
       if (AddTextFile(file, ofptr, pagenum))
          return 1;
            /* Position of the end of the help page in 
               the input help file */
       if (SetCurrentHlpFilePosition(ofptr, pagenum, PAGE_STOP_POSITION))
          return 1;
    }
     }

     return 0;
}

static int AddTextFile(char* textfile, FILE* ofptr, int pos)
{
    int c;
    unsigned long origlen;
    FILE* ifptr;
    
#ifdef VERBOSE
    printf("Adding text file: %s\n", textfile);
#endif
    
    /* Get length of original file. */
    ifptr = fopen(textfile, "rb");
    if (!ifptr)
    {        
        printf("Cannot open file %s\n", textfile);
        fclose(ifptr);
        return 1;
    }
   
    fseek(ifptr, 0, SEEK_END);
    origlen = ftell(ifptr);
    fseek(ifptr, 0, SEEK_SET);
    
/* Compression not currently supported  
   fclose(ifptr);

    if (CompressFile(textfile, TEMPFILE))
       return 1;

    ifptr = fopen(TEMPFILE, "rb");
    if (!ifptr)
       return 1;
*/
    while ((c = fgetc(ifptr)) != EOF)
          if (fputc(c, ofptr) == EOF)
             return 1;
            
/* Compression not currently supported
    unlink(TEMPFILE);
*/
    
    fclose(ifptr);
    return SetOrigFileLength(ofptr, pos, origlen);
}

