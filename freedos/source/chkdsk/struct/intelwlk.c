/*    
   Inteliwlk.c - Intelligently walk over directory entries and fat clusters.
   
   Copyright (C) 2000, 2002 Imre Leber

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
/*
   The following value is chosen, because MAX_PATH = 256 in win95.
   
   Therefor the path with the deepest directory level is:
     
                A\B\C\D\E\ ...
                
   Each level takes 2 character so the deepest directory level is 128.             
*/

#include <string.h>
#include <stdio.h>

#include "FTE.h"
#include "..\errmsgs\PrintBuf.h"
#include "..\errmsgs\errmsgs.h"
#include "FstTrMap.h"

#define DIR_STACK_DEPTH 128
#define _MAX_PATH_      256

struct StackElement 
{
   CLUSTER cluster;
   unsigned long index;
};

struct PipeStruct {
       int (*func) (RDWRHandle handle,
		    struct DirectoryPosition* position,
                    struct DirectoryEntry* entry,
                    char* filename,
		    void** structure);
       void** structure;
       
       char* filename;
       
       char* fatmap;

       BOOL stop;
};

static int ActionWalker(RDWRHandle handle,
			struct DirectoryPosition* pos,
			void** buffer);
                       
static void PushDirectoryEntry(struct StackElement* stack, int* top,
                               struct StackElement* element);
                               
static void PopDirectoryEntry(struct StackElement* stack, int* top, 
                              struct StackElement* element);   
                              
static void PushFileName(char* filename, 
                         int* PathNameIndexes,
                         int* index,
                         struct DirectoryEntry* entry);                              
                              
static void PopFileName(char* filename, 
                        int* PathNameIndexes,
                        int* index);                              
                              
static BOOL HasValidDots(RDWRHandle handle, CLUSTER cluster);                              


/**********************************************************************
***                       WalkDirectoryTree
***********************************************************************
*** Calls a function for every directory entry in a volume.
***
*** Pre order traversal:
***
*** eg.       c:\
***           c:\defrag
***           c:\defrag\source   
***           c:\defrag\source\msdefint
***           ... 
***           c:\defrag\bin
***           ... 
***********************************************************************/
                                 
BOOL IntelligentWalkDirectoryTree(RDWRHandle handle,
		                  int (*func) (RDWRHandle handle,
				               struct DirectoryPosition* position,
                                               struct DirectoryEntry* entry,
                                               char* filename,
				               void** structure),
		                  void** structure)
{
    int top = 0, PathNameIndex=0;
    struct StackElement* stack;
    CLUSTER cluster = 0, temp;
    unsigned long current = 0, labelsinfat;
    struct DirectoryPosition pos;
    struct DirectoryEntry* entry;
    struct StackElement element;
    char*  PathName;
    int*   PathNameIndexes;
        
    struct PipeStruct pipe, *ppipe = &pipe;

    stack = (struct StackElement*)FTEAlloc(DIR_STACK_DEPTH * sizeof(struct StackElement));
    if (!stack)
       return FALSE;
    
    PathName = (char*) FTEAlloc(_MAX_PATH_);   
    if (!PathName)
    {
       FTEFree(stack);
       return FALSE;
    }   
    PathName[0] = '\0';   
       
    PathNameIndexes = (int*) FTEAlloc(DIR_STACK_DEPTH);   
    if (!PathNameIndexes)
    {
       FTEFree(stack);
       FTEFree(PathName);
       return FALSE;
    }
    
    labelsinfat = GetLabelsInFat(handle);
    pipe.fatmap = CreateBitField(labelsinfat);
    if (!pipe.fatmap)
    {
       FTEFree(stack);
       FTEFree(PathName);
       FTEFree(PathNameIndexes);            
       return FALSE;    
    }
    
    pipe.func      = func;
    pipe.structure = structure;
    pipe.stop      = FALSE;    
    pipe.filename  = PathName;
    
    if (!TraverseRootDir(handle, ActionWalker, (void**)&ppipe, TRUE))
    {
       FTEFree(stack);
       FTEFree(PathName);
       FTEFree(PathNameIndexes);  
       FTEFree(pipe.fatmap);          
       return FALSE;
    }
    if (pipe.stop)
    {
       FTEFree(stack);
       FTEFree(PathName);
       FTEFree(PathNameIndexes);  
       FTEFree(pipe.fatmap);            
       return TRUE;
    }
    
    for (;;)
    {
         /* If there still are sub directories in this directory, 
            push the cluster of that directory. */
         pos.sector = 0;
         pos.offset = 0;
         if (!GetNthSubDirectoryPosition(handle, cluster, current, &pos))
	 {
            FTEFree(stack);
            FTEFree(PathName);
            FTEFree(PathNameIndexes);
            FTEFree(pipe.fatmap);  
            return FALSE;
         }

	 if ((pos.sector != 0) || (pos.offset != 0))
	 {
	    entry = AllocateDirectoryEntry();
	    if (!entry)
	    {
	       FTEFree(stack);
               FTEFree(PathName);
               FTEFree(PathNameIndexes);  
               FTEFree(pipe.fatmap);  
	       return FALSE;
	    }

	    if (top < DIR_STACK_DEPTH)
	    {
	       element.cluster = cluster;
	       element.index   = current;

	       PushDirectoryEntry(stack, &top, &element);               
	    }
	    else
	    {
	       FreeDirectoryEntry(entry);   /* Directory level to deep!? */
	       FTEFree(stack);
               FTEFree(PathName);
               FTEFree(PathNameIndexes);
               FTEFree(pipe.fatmap);                 
	       return FALSE;
	    }

	    if (!GetDirectory(handle, &pos, entry))
	    {
	       FreeDirectoryEntry(entry);
	       FTEFree(stack);
               FTEFree(PathName);
               FTEFree(PathNameIndexes);     
               FTEFree(pipe.fatmap);  
	       return FALSE;
	    }
            
            PushFileName(PathName, PathNameIndexes, &PathNameIndex, entry);

	    /* Descend in the directory tree and call the function
               for every directory entry in that directory. */
            temp = GetFirstCluster(entry);
            
            /* Don't descend in any directory that is invalid. */
            if (temp && FAT_NORMAL(temp) && IsLabelValid(handle, temp) &&
                HasValidDots(handle, temp))
            {
	       current = 0;
               cluster = temp;

	       if (!TraverseSubdir(handle, cluster, ActionWalker, (void**) &ppipe, TRUE))
	       {
	          FreeDirectoryEntry(entry);
	          FTEFree(stack);
                  FTEFree(PathName);
                  FTEFree(PathNameIndexes);           
                  FTEFree(pipe.fatmap);  
                  return FALSE;
	       }

	       if (pipe.stop)
	       {
	          FreeDirectoryEntry(entry);
                  FTEFree(stack);
                  FTEFree(PathName);
                  FTEFree(PathNameIndexes);                
                  FTEFree(pipe.fatmap);  
	          return TRUE;
	       }
            }
            else /* cluster not valid, leave this directory */
            {   
	       if (top-1 > 0) /* Be carefull when there are no sub directories
		                 in the volume. */
	       {                     
	          PopDirectoryEntry(stack, &top, &element);
                  PopFileName(PathName, PathNameIndexes, &PathNameIndex);

	          current = element.index+1; /* Then find the next sub directory. */
	          cluster = element.cluster;
	       }
	       else
	       {
                  FreeDirectoryEntry(entry);     
	          break;
	       }
               
               IndicateTreeIncomplete();
            }
            
	    FreeDirectoryEntry(entry);
	 }
	 /* If there are no more sub directories in the current directory,
	    pop the current directory from the stack.                 */
	 else
	 {
	    if (top) /* Be carefull when there are no sub directories
			in the volume. */
	    {
	       PopDirectoryEntry(stack, &top, &element);
               PopFileName(PathName, PathNameIndexes, &PathNameIndex);
               
	       current = element.index+1; /* Then find the next sub directory. */
	       cluster = element.cluster;
	    }
	    else
	    {
	       break;
	    }
         }
    }
            
    FTEFree(PathName);
    FTEFree(PathNameIndexes);
    FTEFree(stack);        
    FTEFree(pipe.fatmap);  
    return TRUE;
}

/**********************************************************************
***                           ActionWalker      
***********************************************************************
*** Calls a function for every directory entry in the given directory.
***********************************************************************/

static int ActionWalker(RDWRHandle handle,
			struct DirectoryPosition* pos,
			void** buffer)
{
    int length, i;
    BOOL retVal, InRoot;
    struct PipeStruct* pipe = *((struct PipeStruct**) buffer);
    struct DirectoryEntry entry;
    char name[8], ext[3];
    unsigned char sectorspercluster;
    CLUSTER label;
       
    /* See wether there are any loops in the directory. */
    InRoot = IsRootDirPosition(handle, pos);
    if (InRoot == -1) return FAIL;
    
    if (!InRoot && (pos->offset == 0))
    {
       sectorspercluster = GetSectorsPerCluster(handle);    
       if (!sectorspercluster)
       { 
          return FAIL; 
       }
       
       if ((pos->sector % sectorspercluster) == 0)
       {
          label = DataSectorToCluster(handle, pos->sector);
          if (!label)
          {
             return FAIL;
          }
          
          if (GetBitfieldBit(pipe->fatmap, label))
          {
             IndicateTreeIncomplete();
             return FALSE;              /* Loop found in the directory */
          }
          SetBitfieldBit(pipe->fatmap, label);
       }
    }

    if (!GetDirectory(handle, pos, &entry))
       return FAIL;

    length = strlen(pipe->filename);
    strcat(pipe->filename, "\\");

    /* Make sure the file name is printable */
    memcpy(name, entry.filename, 8);
    memcpy(ext, entry.extension, 3);
    
    for (i = 0; i < 8; i++)
        if (name[i] < 32)
           name[i] = '?'; 
           
    for (i = 0; i < 3; i++)
        if (ext[i] < 32)
           ext[i] = '?';             
    
    ConvertEntryPath(&pipe->filename[length+1], name, ext);    

    retVal = pipe->func(handle, pos, &entry, pipe->filename, pipe->structure);
    if (retVal == FALSE) pipe->stop = TRUE;

    pipe->filename[length] = '\0';
    
    return retVal; 	/* Should not be executed */
}


/**********************************************************************
***                           PushDirectoryEntry      
***********************************************************************
*** Pushes an entry to the stacks.
***********************************************************************/

static void PushDirectoryEntry(struct StackElement* stack, 
                               int* top,
                               struct StackElement* element)
{
    memcpy(&stack[*top], element, sizeof(struct StackElement));
    *top = (*top)+1;      
}

/**********************************************************************
***                           PopDirectoryEntry
***********************************************************************
*** Pops a directory entry from the stack.
***********************************************************************/

static void PopDirectoryEntry(struct StackElement* stack, 
                              int* top,
                              struct StackElement* element)
{
    *top = (*top)-1;
    memcpy(element, &stack[*top], sizeof(struct StackElement));
}

/**********************************************************************
***                           PushFileName
***********************************************************************
*** Adds a directory to a file name.
***********************************************************************/
static void PushFileName(char* filename, 
                         int* PathNameIndexes,
                         int* index,
                         struct DirectoryEntry* entry)
{
    int i;
    char name[8], ext[3];
    
    PathNameIndexes[*index] = strlen(filename);
   
    /* Make sure the file name is printable */
    memcpy(name, entry->filename, 8);
    memcpy(ext, entry->extension, 3);
    
    for (i = 0; i < 8; i++)
        if (name[i] < 32)
           name[i] = '?'; 
           
    for (i = 0; i < 3; i++)
        if (ext[i] < 32)
           ext[i] = '?';   
   
    if (PathNameIndexes[*index] < _MAX_PATH_ - 14)
    {
       strcat(filename, "\\");
       ConvertEntryPath(&filename[PathNameIndexes[*index]+1], 
                        name, ext);
    }

    (*index)++;
}
 
/**********************************************************************
***                           PushFileName
***********************************************************************
*** Removes a directory from a file name.
***********************************************************************/  
static void PopFileName(char* filename, 
                        int* PathNameIndexes,
                        int* index)                      
{
    (*index)--;     
    filename[PathNameIndexes[*index]] = '\0'; 
}

/**********************************************************************
***                           PopDirectoryEntry
***********************************************************************
*** Tries to guess wether a certain sub dir is valid by looking at the
*** '.' entry.
***********************************************************************/

static BOOL HasValidDots(RDWRHandle handle, CLUSTER cluster)
{
   CLUSTER firstcluster;
   struct DirectoryPosition pos = {0,0}; 
   struct DirectoryEntry entry;
   
   BOOL currentOK = FALSE, previousOK = FALSE;
    
   if (!GetNthDirectoryPosition(handle, cluster, 0, &pos))
      return FALSE;

   if ((pos.sector == 0) && (pos.offset == 0))
   {
      return FALSE;
   }

   if (!GetDirectory(handle, &pos, &entry))
      return FALSE;
      
   firstcluster = GetFirstCluster(&entry); 
   if ((memcmp(entry.filename, ".       ", 8) == 0) &&
       (memcmp(entry.extension, "   ", 3) == 0)     &&
       (firstcluster == cluster)                    &&
       (entry.attribute & FA_DIREC))
   {
      currentOK = TRUE;
   }
   
   pos.sector = 0;
   pos.offset = 0;
   
   if (!GetNthDirectoryPosition(handle, cluster, 1, &pos))
      return FALSE;
      
   if ((pos.sector != 0) || (pos.offset != 0))
   {
      if (!GetDirectory(handle, &pos, &entry))
         return FALSE;
      
      firstcluster = GetFirstCluster(&entry); 
      if ((memcmp(entry.filename, "..      ", 8) == 0) &&
          (memcmp(entry.extension, "   ", 3) == 0)     &&
          (entry.attribute & FA_DIREC))
      {
         previousOK = TRUE;
      }
   }
    
   return currentOK || previousOK;
}