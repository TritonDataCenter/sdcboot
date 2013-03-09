#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "fte.h"
#include "sortfatf.h"
#include "misc.h"
#include "sortcfgf.h"
//#include "cext.h"
//#include "apmlib.h"

#define VERSION "beta 0.1"
#define BACKUP_HEAP_SIZE 16384

void Usage(char switchar);

char cdecl SwitchChar(void){return '/';}

int cdecl main(int argc, char** argv)
{
    int len, mustreboot=FALSE;
    char switchar;
    RDWRHandle handle;

    switchar = SwitchChar();
    if ((argc < 3) || (argc > 4))

    {  
       Usage(switchar);
       return 1;
    }
      
   /* Determine what to sort on. */
   len = strlen(argv[2]);
   if ((len != 4) && (len != 5))
   {
      Usage(switchar);
      return 1;
   }

   if ((argv[2][0] == switchar)     && 
       (toupper(argv[2][1]) == 'O') &&
       (argv[2][2] == ':'))
   {
      switch(toupper(argv[2][3]))
      {
        case 'N':
             SetCompareFunction(CompareNames);    
             break;
        case 'D':
             SetCompareFunction(CompareDateTime);
             break;   
        case 'E':
             SetCompareFunction(CompareExtension);   
             break;
        case 'S':
             SetCompareFunction(CompareSize);
             break;
        default:
             printf("Wrong sorting option");
             return 1;
      }
            
      if (len == 5) 
      {
         if (argv[2][4] == '-')
         {
            SetFilterFunction(DescendingFilter);
         }
         else
         {
            printf("Please use '-' to sort in descending order");
            return 1;                     
         }
      }
      else
      {
         SetFilterFunction(AscendingFilter);
      }
   }
   else
   {
      printf("Invalid parameter sorting order.");
   }
   
   if (argc == 4)
   {
      if ((argv[3][0] == switchar) && (toupper(argv[3][1]) == 'B'))
      {
         mustreboot = TRUE;
      }
      else
      {
         printf("Invalid parameter instead of entering '/B' to reboot");
         return 1;
      }
   }

   /* Initialise the heap memory manager. */
//   if ((AllocateFTEMemory(0, 0, BACKUP_HEAP_SIZE)) != ALLOC_SUCCESS)
//   {
 //     printf("Insufficient memory to start sorting.");
  //    return 1;
 //  }

   /* Try opening the drive or image file. */
   if (InitReadWriteSectors(argv[1], &handle))
   {
      time_t t, t1;

      time(&t);

      if (SortDirectoryTree(handle))
      {
         time(&t1);

	 printf("Elapsed: %fs\n", difftime(t1, t));
         
         printf("Directories successfully sorted\n");
      }
      else
      {
	 printf("Error reading from disk!\n");
      }
   }
   else
   {
      printf("Could not open Drive or image file.");
   }
   
   if (mustreboot)
   {
      int counter = 10;       

//      if (FlushAllCaches() != 0)
      {
         time_t t, prev_t = 0;
         
	 counter = 10;            /* Wait 10 seconds for any caches to settle */

         do
         {
           do
           {
             time(&t);
           } while (t == prev_t);

           prev_t = t;

           printf("Rebooting in: %d\n", counter--);
         } while (TRUE);
      }
//      else
      {
//         delay(2000);
      }
      
//      Reboot(FALSE);
   }

   return 0;
}

void Usage(char switchar)
{
   printf("Sortdir " VERSION "\n"
          "Sorts all the directories in a volume.\n"
          "\n"
     "Syntax: SortDir <drive> [%cO:{N|D|E|S}[-]] [%cB]\n"
     "\n"
     "drive: drive to sort all the entries in.\n"
     "\n"
     "%cO:\tDetermines what to sort on (required).\n"
     "\t\tN by name (alphabetic)             E by Extension (alphabetic)\n"
     "\t\tD by date & time (earliest first)  S by Size (smallest first)\n"
     "\tAdd '-' to sort in descending order\n"
     "\n"
     "%cB\treboot your computer after reset\n",
     switchar,
     switchar,
     switchar,
     switchar);
}

void DrawOnDriveMap(CLUSTER cluster, int symbol)
{
     cluster=cluster;
     symbol=symbol;
}

void LogMessage(char* message)
{
     puts(message);
}

int QuerySaveState(void)
{
     RETURN_FTEERR(FALSE);
}

void IndicatePercentageDone(CLUSTER cluster, CLUSTER totalclusters)
{
     cluster=cluster; totalclusters=totalclusters;	     
}

int CommitCache(void)
{
    return TRUE;
}
