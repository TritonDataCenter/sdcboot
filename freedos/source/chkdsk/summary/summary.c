/*
   Summary.c - summary printer.
   Copyright (C) 2002 Imre Leber

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

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be
*/

#include <stdio.h>
#include <string.h>

#include "fte.h"

#include "fatsum.h"
#include "filessum.h"

static void PrintSummaryLine(unsigned long digits, char* kind);

BOOL ReportVolumeSummary(RDWRHandle handle)
{
  int i;
  unsigned long serialnum, clustersindataarea, totalnumberofsectors;
  struct FatSummary fatinfo;
  struct FilesSummary filesinfo;
  unsigned char sectorspercluster;
  struct DirectoryPosition volumepos;
  struct DirectoryEntry volumeentry;
  BOOL IsProblematic;

  char buffer[80];

  unsigned bytespercluster;

  IsProblematic = !GetFATSummary(handle, &fatinfo) ||
                  !GetFilesSummary(handle, &filesinfo);

  sectorspercluster = GetSectorsPerCluster(handle);
  if (!sectorspercluster)
  {
     printf("Cannot get summary information\n");
     return FALSE;
  }
  bytespercluster = (unsigned) sectorspercluster * BYTESPERSECTOR;

  clustersindataarea = GetClustersInDataArea(handle);
  if (!clustersindataarea)
  {
     printf("Cannot get summary information\n");
     return FALSE;
  }

  /* See wether there is a volume label in the root directory, if so
     print it. */
  switch (GetRootDirVolumeLabel(handle, &volumepos))
  {
     case TRUE:
          if (!GetDirectory(handle, &volumepos, &volumeentry))
          {
             printf("Cannot get summary information\n");
             return FALSE;
          }

          printf("Volume label is ");

          for (i = 0; i < 8; i++)
              printf("%c", volumeentry.filename[i]);
          for (i = 0; i < 3; i++)
	      printf("%c", volumeentry.extension[i]);

          printf(" created on ");

          /* TODO: make the string according to the language format */
	  printf("%02d-%02d-%02d, %02d:%02d:%02d",
		 (int)volumeentry.LastWriteDate.year+1980,
                 (int)volumeentry.LastWriteDate.month,
                 (int)volumeentry.LastWriteDate.day,

                 (int)volumeentry.LastWriteTime.hours,
                 (int)volumeentry.LastWriteTime.minute,
                 (int)volumeentry.LastWriteTime.second);

          puts("");
                 
          break;

     /* Otherwise print the volume serial number. */
     case FALSE:
          switch (IsVolumeDataFilled(handle))
          {
             case TRUE:
                  serialnum = GetDiskSerialNumber(handle);
                  if (serialnum)
                  {
                     printf("The disk serial number is %0X-%0X\n",
                            (unsigned) (serialnum & 0xFFFF),
                            (unsigned) (serialnum >> 16));
                  }
                  else
                  {
                     printf("Cannot get summary information\n");
                     return FALSE;
                  }
                  break;
                  
             case 0xff:
                  printf("Cannot get summary information\n");
                  return FALSE;
          }
          break;
       
     case FAIL:
          printf("Cannot get summary information\n");
          return FALSE;
  }

  puts("");

  /* Print the total number of bytes in the volume. */
  /* See if we have to put it as Kb's.
     We calculate the total number of sectors.
     If this is smaller then (2^32 / 512) == 8388608 we put it as bytes,
     otherwise we divide the number of sectors by 2 and print it out as Kb.
  */
  totalnumberofsectors = clustersindataarea * sectorspercluster;
  if (totalnumberofsectors >= 8388608L)
  {
     PrintSummaryLine(totalnumberofsectors / 2, /* Assuming */
                      "Kb total drive size\n"); /*   BYTESPERSECTOR == 512 */
  }
  else
  {
     PrintSummaryLine(totalnumberofsectors * BYTESPERSECTOR,
                      "bytes total drive size\n");
  }

  /* Print the files summary information. */
  if (filesinfo.SizeOfAllFiles[1])
  {
     sprintf(buffer, "Kb in a total of %lu files", filesinfo.TotalFileCount);
     PrintSummaryLine((filesinfo.SizeOfAllFiles[1] << 22) +
                                  (filesinfo.SizeOfAllFiles[0] >> 10),
                      buffer);
  }
  else
  {
     sprintf(buffer, "bytes in a total of %lu files",
             filesinfo.TotalFileCount);
             
     PrintSummaryLine(filesinfo.SizeOfAllFiles[0], buffer);
  }

  if (filesinfo.HiddenFileCount)
  {
     if (filesinfo.SizeOfHiddenFiles[1])
     {
	sprintf(buffer, "Kb in %lu hidden files", filesinfo.HiddenFileCount);
	PrintSummaryLine((filesinfo.SizeOfHiddenFiles[1] << 22) +
				    (filesinfo.SizeOfHiddenFiles[0] >> 10),
			 buffer);
     }
     else
     {
	sprintf(buffer, "bytes in %lu hidden files", filesinfo.HiddenFileCount);
	PrintSummaryLine(filesinfo.SizeOfHiddenFiles[0], buffer);
     }
  }

  if (filesinfo.SystemFileCount)
  {
     if (filesinfo.SizeOfSystemFiles[1])
     {
	sprintf(buffer, "Kb in %lu system files", filesinfo.SystemFileCount);
	PrintSummaryLine((filesinfo.SizeOfSystemFiles[1] << 22) +
				   (filesinfo.SizeOfSystemFiles[0] >> 10),
			 buffer);
     }
     else
     {
	sprintf(buffer, "bytes in %lu system files", filesinfo.SystemFileCount);
	PrintSummaryLine(filesinfo.SizeOfSystemFiles[0], buffer);
     }
  }

  if (filesinfo.SizeOfDirectories[1])
  {
     sprintf(buffer, "Kb in %lu directories", filesinfo.DirectoryCount);
     PrintSummaryLine((filesinfo.SizeOfDirectories[1] << 22) +
                                  (filesinfo.SizeOfDirectories[0] >> 10),
                      buffer);
  }
  else
  {
     sprintf(buffer, "bytes in %lu directories", filesinfo.DirectoryCount);
     PrintSummaryLine(filesinfo.SizeOfDirectories[0], buffer);
  }

  /* Print the total size of files */
  if (filesinfo.SizeOfAllFiles[1])
  {
     PrintSummaryLine((filesinfo.TotalSizeofFiles[1] << 22) +
                                  (filesinfo.TotalSizeofFiles[0] >> 10),
                      "Kb total size of files");
  }
  else
  {
     PrintSummaryLine(filesinfo.TotalSizeofFiles[0],
                      "total size of files");
  }

  /* Print the free space (same method as for the total drive size). */
  totalnumberofsectors = fatinfo.numoffreeclusters * sectorspercluster;
  if (totalnumberofsectors >= 8388608L)
  {
     PrintSummaryLine(totalnumberofsectors / 2,
                      "Kb available on the volume");
  }
  else
  {
     PrintSummaryLine(totalnumberofsectors * BYTESPERSECTOR,
                      "bytes available on the volume");
  }

  puts("");

  /* Print the FAT summary information. */
  PrintSummaryLine(bytespercluster, "bytes in every cluster");
  PrintSummaryLine(fatinfo.totalnumberofclusters, "total number of clusters");

  if (fatinfo.numofbadclusters)
     PrintSummaryLine(fatinfo.numofbadclusters, "number of bad clusters");

  if (fatinfo.numoffreeclusters)
     PrintSummaryLine(fatinfo.numoffreeclusters, "number of free clusters");

  if (IsProblematic || IsTreeIncomplete())
  {
     printf("\nThere was a problem getting disk information, the summary may be wrong\n");
  }
     
  return TRUE;
}

static void PrintSummaryLine(unsigned long digits, char* kind)
{
   int len, i;
   char buffer[33];

   if (digits < 1000)
   {
      sprintf(buffer, "%lu", digits);
   }
   if (digits >= 1000)
   {
      sprintf(buffer, "%lu.%03lu", digits / 1000, digits % 1000);
   }
   if (digits >= 1000000L)
   {
      sprintf(buffer, "%lu.%03lu.%03lu", digits / 1000000L,
					 (digits % 1000000L) / 1000,
					 digits % 1000);
   }

   len = strlen(buffer);

   if (len < 15)
      for (i = 0; i < (15 - len); i++)
          printf(" ");

   printf("%s ", buffer);
   printf("%s\n", kind);
}