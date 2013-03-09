#ifndef CHECKS_H_
#define CHECKS_H_

#include "boot\bootchk.h"
#include "dirs\dirschk.h"
#include "fat\fatchks.h"

/* This file is to be included in the main chkdsk file only. */

struct CheckAndFix TheChecks[] =
{
   /* The first label of the FAT does not contain the media descriptor.
      The first label in the FAT is adjusted. */
   {CheckDescriptorInFat, PlaceDescriptorInFat
#ifdef ENABLE_LOGGING
   , "descriptor in fat"
#endif   
   },

   /* The string that gives an indication of the kind of file system
      is wrong, the right string is put in the place. */
   {CheckFSString, CorrectFSString
#ifdef ENABLE_LOGGING
   , "file system string"
#endif   
   },

   /* FAT contains invalid cluster numbers. Cluster is changed to EOF. */
   {CheckFatLabelValidity, MakeFatLabelsValid
#ifdef ENABLE_LOGGING
   , "fat label validity"
#endif   
   },

   /* Anything wrong in the directories. */
   {CheckDirectoryEntries, FixDirectoryEntries
#ifdef ENABLE_LOGGING
   , "sub directories" 
#endif   
   },

   /* Lost clusters, they are converted to files. */
   {FindLostClusters, ConvertLostClustersToFiles
#ifdef ENABLE_LOGGING           
   , "lost clusters"
#endif   
   },

   /* Cross linked files, all the files get a copy of the cross linked
      clusters. */
   {FindCrossLinkedFiles, SplitCrossLinkedFiles
#ifdef ENABLE_LOGGING           
   , "cross linked clusters"
#endif   
   }
};

#define AMOFCHECKS (sizeof(TheChecks)/sizeof(*TheChecks))

#endif