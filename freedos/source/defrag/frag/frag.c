#include "FTE.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#ifdef _WIN32
#undef BOOL
#include <windows.h>
#endif

#ifdef _WIN32
#define randomize() srand((unsigned)time(NULL));
#endif

#define RELOCCOUNT 10000

CLUSTER random_cluster(RDWRHandle handle, unsigned long labelsinfat);
CLUSTER random_freeSpace(RDWRHandle handle, unsigned long labelsinfat);
//CLUSTER GetNthCircularCluster(RDWRHandle handle, unsigned long labelsinfat, unsigned long n);
//CLUSTER GetNthCircFreeCluster(RDWRHandle handle, unsigned long labelsinfat, unsigned long n);
void usage();

//void CreateLogFileName(unsigned long i, CLUSTER from, CLUSTER to, char* file)
//{
//    sprintf(file, "d:\\temp\\out\\%lu_%lu_%lu.txt", i, from, to);
//}

//void CreateVFSLogFileName(unsigned long i, CLUSTER from, CLUSTER to, char* file)
//{
//    sprintf(file, "d:\\temp\\out\\%lu_%lu_%lu_VFS.txt", i, from, to);
//}

BOOL FragmentVolume(RDWRHandle handle, unsigned long index, CLUSTER* from, CLUSTER* to)
{
    unsigned long labelsinfat;

    CLUSTER current;
    CLUSTER freeSpace;
    char acFName[1000];
    char acVFSFName[1000];

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) 
        return FALSE;

    current   = random_cluster(handle, labelsinfat);
    freeSpace = random_freeSpace(handle, labelsinfat);

    *from = current;
    *to   = freeSpace;

//    CreateLogFileName(index, current, freeSpace, acFName);
//    CreateVFSLogFileName(index, current, freeSpace, acVFSFName);

//    if (!OpenLogFile(acFName))
//        return FALSE;

//    if (!OpenVFSlogFile(acVFSFName))
//        return FALSE;

 //   if (IsClusterUsed(handle, freeSpace) == FALSE)
    {
        if (!EnsureClustersFree(handle, freeSpace, 1))
            return FALSE;

        if (!RelocateCluster(handle, current, freeSpace))
            return FALSE;
    }

//    CloseLogFile();
//    CloseVFSLogFile();

    return TRUE;
}

CLUSTER random_cluster(RDWRHandle handle, unsigned long labelsinfat)
{
    unsigned long randomnumber;
    unsigned random1, random2;
    CLUSTER label;

    int fatlabelsize = GetFatLabelSize(handle);
    if (!fatlabelsize) return FALSE;

    random1 = rand() * (labelsinfat % 65536L);
    random2 = rand() * (labelsinfat / 65536L);

    randomnumber = random1 * 65536L + random2;

    randomnumber = (randomnumber % (labelsinfat-2))+2;

    while (TRUE)
    {
        if (fatlabelsize == FAT32)
        {
            if (randomnumber == GetFAT32RootCluster(handle))
            {
                randomnumber++;
                continue;
            }
        }

        if (!GetNthCluster(handle, randomnumber, &label))
            return FALSE;
        
        if (FAT_NORMAL(label) || FAT_LAST(label))
        {
            return randomnumber;
        }

        randomnumber++;
        if (randomnumber == labelsinfat)
       randomnumber = 2;
    }
}

CLUSTER random_freeSpace(RDWRHandle handle, unsigned long labelsinfat)
{
    unsigned long randomnumber;
    unsigned random1, random2;
    CLUSTER label;

    random1 = rand() * (labelsinfat % 65536L);
    random2 = rand() * (labelsinfat / 65536L);

    randomnumber = random1 * 65536L + random2;

    randomnumber = (randomnumber % (labelsinfat-2))+2;

    while (TRUE)
    {
        if (!GetNthCluster(handle, randomnumber, &label))
            return FALSE;
        
        if (FAT_FREE(label))
        {
            return randomnumber;
        }

        randomnumber++;
        if (randomnumber == labelsinfat)
       randomnumber = 2;
    }
}

void usage()
{
    printf("Frag 0.1\n"
           "Syntax:\n"
           "\tfrag <drive>\n");  
}

void CreateFileName(unsigned long i, CLUSTER from, CLUSTER to, char* file)
{
    sprintf(file, "d:\\temp\\out\\%lu_%lu_%lu", i, from, to);
}



int main(int argc, char** argv)
{
    RDWRHandle handle;
    CLUSTER from, to;
    unsigned long i;

    printf("frag v0.1");

    //randomize();

    if (argc != 2)
        usage();

    if (strcmp(argv[1], "/?") == 0)
        usage();

  //  InitSectorCache();
  //  StartSectorCache();
                       
    if (!InitReadWriteSectors(argv[1], &handle))
    {
        printf("Cannot initialise drive %s\n", argv[1]);
        return 1;
    }

    for (i=0; i < RELOCCOUNT; i++)
    {   
        printf("%lu\n", i);

        if (!FragmentVolume(handle, i, &from, &to))
        {
            printf("Fragmentation failed!\n");
            return 1;
        }   
    }

    if (!BackupFat(handle))
    {
        printf("Could not back up FAT\n");
        return 1;
    }

   // CommitCache();
  //  CloseSectorCache();
    CloseReadWriteSectors(&handle);

//    CreateFileName(i, from, to, acFName);
//    if (CopyFile(argv[1], acFName, FALSE) == 0)
//    {
//        printf("Could not copy file %s", acFName);
//        break;
//    }

    return 0;
}
