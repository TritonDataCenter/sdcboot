/*    
   Entshift.c - routines to swap LFN entries on disk.

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

#include <string.h>

#include "fte.h"


static BOOL PrivateRotateEntriesRight(RDWRHandle handle, CLUSTER dirclust, 
                                      unsigned long begin, unsigned long end,
                                      unsigned long n, BOOL ignoreerror);

static BOOL PrivateRotateEntriesLeft(RDWRHandle handle, CLUSTER dirclust, 
                                     unsigned long begin, unsigned long end,
                                     unsigned long n, BOOL ignoreerror);


static BOOL PrivateRotateEntriesRight(RDWRHandle handle, CLUSTER dirclust, 
                                      unsigned long begin, unsigned long end,
                                      unsigned long n, BOOL ignoreerror)
{
     unsigned long i;
     unsigned long j;
     
     struct DirectoryPosition head, pos1, pos2;
     struct DirectoryEntry last, entry1, entry2;

     /* Rotate all the entries n times. */
     for (i = 0; i < n; i++)
     {
         /* Get the last entry position */
         if (!GetNthDirectoryPosition(handle, dirclust, end-1, &pos1))
         {
            if (!ignoreerror)
            {        
               PrivateRotateEntriesLeft(handle, dirclust,
                                        begin, end, i, TRUE);           
               return FALSE;
            }
         }
     
         /* Store the last entry in memory. */
         if (!GetDirectory(handle, &pos1, &last))
         {
            PrivateRotateEntriesLeft(handle, dirclust,
                                     begin, end, i, TRUE);   
            
            if (!ignoreerror) return FALSE;
         }
         
         /* Get the first directory entry. */
         if (!GetNthDirectoryPosition(handle, dirclust, begin, &head))
         {
            PrivateRotateEntriesLeft(handle, dirclust,
                                     begin, end, i, TRUE); 
            
            if (!ignoreerror) return FALSE;
         }
             
         /* Store it in entry1. */
         if (!GetDirectory(handle, &head, &entry1))
         {           
            PrivateRotateEntriesLeft(handle, dirclust,
                                     begin, end, i, TRUE); 
            if (!ignoreerror) return FALSE;
         }

         /* Move the entries all to the end. */
         for (j = begin+1; j < end; j++)
         {
             /* Get the next position */
             if (!GetNthDirectoryPosition(handle, dirclust, j, &pos2))
             {
                if (!ignoreerror)
                {        
                   PrivateRotateEntriesLeft(handle, dirclust,
                                            begin, j, 1, TRUE);
                   GetNthDirectoryPosition(handle, dirclust, j-1, &pos2);
                   WriteDirectory(handle, &pos2, &entry1);     
                   PrivateRotateEntriesLeft(handle, dirclust, 
                                            begin, end, i, TRUE);       
                   return FALSE;
                }
             }

             /* Read the next entry in memory */
             if (!GetDirectory(handle, &pos2, &entry2))
             {
                if (!ignoreerror) 
                {        
                   PrivateRotateEntriesLeft(handle, dirclust,
                                            begin, j, 1, TRUE);
                   GetNthDirectoryPosition(handle, dirclust, j-1, &pos2);
                   WriteDirectory(handle, &pos2, &entry1);     
                   PrivateRotateEntriesLeft(handle, dirclust, 
                                            begin, end, i, TRUE);
                   return FALSE;
                }
             }

             
             /* Store the previous entry in the position of the next
                entry. */
             if (!WriteDirectory(handle, &pos2, &entry1))
             {
                if (!ignoreerror)
                {
                   PrivateRotateEntriesLeft(handle, dirclust,
                                            begin, j, 1, TRUE);
                   GetNthDirectoryPosition(handle, dirclust, j-1, &pos2);
                   WriteDirectory(handle, &pos2, &entry1);     
                   PrivateRotateEntriesLeft(handle, dirclust, 
                                            begin, end, i, TRUE);
                   return FALSE; 
                }
             }

             /* Take care that on the next iteration we store the
                following entry. */
             memcpy(&entry1, &entry2, sizeof(struct DirectoryEntry));
        }

        /* Eventually, move the last entry to the front. */
        if (!WriteDirectory(handle, &head, &last))
        {
           if (!ignoreerror) 
              return FALSE;
        }
    }
    return TRUE;
}

static BOOL PrivateRotateEntriesLeft(RDWRHandle handle, CLUSTER dirclust, 
                                     unsigned long begin, unsigned long end,
                                     unsigned long n, BOOL ignoreerror)
{
     unsigned long i;
     unsigned long j;
     
     struct DirectoryPosition tail, pos1, pos2, last;
     struct DirectoryEntry head, entry1, entry2;
     
     /* Rotate all the entries n times. */
     for (i = 0; i < n; i++)
     {
         /* Get the position of the first entry */
         if (!GetNthDirectoryPosition(handle, dirclust, begin, &pos1))
         {
            if (!ignoreerror)
            {        
               PrivateRotateEntriesRight(handle, dirclust,
                                         begin, end, i, TRUE);  
               return FALSE;
            }
         }
     
         /* Read it in head. */
         if (!GetDirectory(handle, &pos1, &head))
         {
            if (!ignoreerror)
            {        
               PrivateRotateEntriesRight(handle, dirclust,
                                         begin, end, i, TRUE);                      
               return FALSE;
            }
         }

         /* Get the position of the last entry. */
         if (!GetNthDirectoryPosition(handle, dirclust, end-1, &tail))
         {
            if (!ignoreerror)
            {        
               PrivateRotateEntriesRight(handle, dirclust,
                                         begin, end, i, TRUE);  
               return FALSE;
            }
         }

         /* Read it in memory. */
         if (!GetDirectory(handle, &tail, &entry1))
         {
            if (!ignoreerror)
            {
               PrivateRotateEntriesRight(handle, dirclust,
                                         begin, end, i, TRUE);  
               return FALSE;
            }
         }

         /* Move the entries to the front */
         for (j = end-2; j >= begin; j++)
         {
             /* Get the position of the previous entry. */
             if (!GetNthDirectoryPosition(handle, dirclust, j, &pos2))
             {
                if (!ignoreerror)
                {
                   PrivateRotateEntriesRight(handle, dirclust,
                                             j+2, end, 1, TRUE);
                   GetNthDirectoryPosition(handle, dirclust, j+1, &pos2);
                   WriteDirectory(handle, &pos2, &entry1);     
                   PrivateRotateEntriesRight(handle, dirclust, 
                                             begin, end, i, TRUE);
                   return FALSE;
                }
             }
             
             /* Read it in memory. */
             if (!GetDirectory(handle, &pos2, &entry2))
             {
                if (!ignoreerror)
                {
                   PrivateRotateEntriesRight(handle, dirclust,
                                             j+2, end, 1, TRUE);
                   GetNthDirectoryPosition(handle, dirclust, j+1, &pos2);
                   WriteDirectory(handle, &pos2, &entry1);     
                   PrivateRotateEntriesRight(handle, dirclust, 
                                             begin, end, i, TRUE);
                   return FALSE;
                }                        
             }
             
             /* Write the following entry to the disk. */
             if (!WriteDirectory(handle, &pos2, &entry1))
             {
                if (!ignoreerror)
                {
                   PrivateRotateEntriesRight(handle, dirclust,
                                             j+2, end, 1, TRUE);
                   GetNthDirectoryPosition(handle, dirclust, j+1, &pos2);
                   WriteDirectory(handle, &pos2, &entry1);     
                   PrivateRotateEntriesRight(handle, dirclust, 
                                             begin, end, i, TRUE);
                   return FALSE;
                }
             }

             /* Make sure that on the next iteration the previous
                entry is written. */
             memcpy(&entry1, &entry2, sizeof(struct DirectoryEntry));
        }
        
        /* Eventually write the first entry as the last. */
        if (!WriteDirectory(handle, &last, &head))
        {
           if (!ignoreerror) 
              return FALSE;
        }
    }
    return TRUE;
}

BOOL RotateEntriesRight(RDWRHandle handle, CLUSTER dirclust, 
                        unsigned long begin, unsigned long end,
                        unsigned long n)
{
    return PrivateRotateEntriesRight(handle, dirclust, begin, end, n, FALSE);
}

BOOL RotateEntriesLeft(RDWRHandle handle, CLUSTER dirclust, 
                       unsigned long begin, unsigned long end,
                       unsigned long n)
{
    return PrivateRotateEntriesLeft(handle, dirclust, begin, end, n, FALSE);
}

/**************************************************************************
***                      SwapBasicEntries
***************************************************************************
*** Swaps the elementary entries at pos1 and pos2.
***************************************************************************/

BOOL SwapBasicEntries(RDWRHandle handle, 
                      struct DirectoryPosition* pos1,
                      struct DirectoryPosition* pos2)
{
     struct DirectoryEntry entry1, entry2;
     
     if (!GetDirectory(handle, pos1, &entry1))
     {
        return FALSE;
     }
     
     if (!GetDirectory(handle, pos2, &entry2))
     {
        return FALSE;  
     }
     
     if (!WriteDirectory(handle, pos1, &entry2))
     {
        WriteDirectory(handle, pos1, &entry1);       
        return FALSE;
     }
     
     if (!WriteDirectory(handle, pos2, &entry1))
     {
        WriteDirectory(handle, pos1, &entry1);
        WriteDirectory(handle, pos2, &entry2);
        return FALSE;
     }
     
     return TRUE;
}

/**************************************************************************
***                      SwapNBasicEntries
***************************************************************************
*** Swaps n elementary entries starting at position begin1 with n elementary
*** entries starting at position begin2.
***************************************************************************/

static BOOL SwapNBasicEntries(RDWRHandle handle, CLUSTER cluster, 
                              unsigned long begin1, unsigned long begin2,
                              unsigned long n)
{
    unsigned long i, j;
    struct DirectoryPosition pos1, pos2;

    /* For every entry that has to be swapped. */
    for (i = 0; i < n; i++)
    {
        /* Get the directory position of the first */
        if (!GetNthDirectoryPosition(handle, cluster, begin1+i, &pos1))
        { 
           for (j = 0; j < i; j++)
           {
               GetNthDirectoryPosition(handle, cluster, begin1+j, &pos1);
               GetNthDirectoryPosition(handle, cluster, begin2+j, &pos2);
               SwapBasicEntries(handle, &pos1, &pos2);         
           }                  
          
           return FALSE; 
        }
        
        /* Get the directory position of the second */
        if (!GetNthDirectoryPosition(handle, cluster, begin2+i, &pos2))
        { 
           for (j = 0; j < i; j++)
           {
               GetNthDirectoryPosition(handle, cluster, begin1+j, &pos1);
               GetNthDirectoryPosition(handle, cluster, begin2+j, &pos2);
               SwapBasicEntries(handle, &pos1, &pos2);         
           }                
                
           return FALSE; 
        }

        /* and swap */
        if (!SwapBasicEntries(handle, &pos1, &pos2))
        {
           for (j = 0; j < i; j++)
           {
               GetNthDirectoryPosition(handle, cluster, begin1+i, &pos1);
               GetNthDirectoryPosition(handle, cluster, begin2+i, &pos2);
               SwapBasicEntries(handle, &pos1, &pos2);         
           }
           return FALSE;
       }
    }
    return TRUE;
}

/**************************************************************************
***                      SwapLFNEntries
***************************************************************************
*** Swaps the LFN entries at the indicated positions directly on disk.
***************************************************************************/

BOOL SwapLFNEntries(RDWRHandle handle,
                    CLUSTER cluster,
                    unsigned long begin1,
                    unsigned long begin2)
{
    unsigned long end1=begin1, end2=begin2;
    
    unsigned long len1, len2, difference;
    
    struct DirectoryPosition pos1, pos2;
    struct DirectoryEntry entry1, entry2;

    /* Count the number of elementary entries in the first series of LFN
       entries. */
    do {
       if (!GetNthDirectoryPosition(handle, cluster, end1, &pos1))
       {
          return FALSE;
       }
       
       if (!GetDirectory(handle, &pos1, &entry1))
       {
          return FALSE;
       }
       
       end1++; 
    
    } while (IsLFNEntry(&entry1));

    /* Count the number of elementary entries in the second series of LFN
       entries. */
    do {
       if (!GetNthDirectoryPosition(handle, cluster, end2, &pos2))
       {
          return FALSE;
       }
       
       if (!GetDirectory(handle, &pos2, &entry2))
       {
          return FALSE;
       }
       
       end2++; 
    
    } while (IsLFNEntry(&entry2));

    len1 = end1-begin1;
    len2 = end2-begin2;
         
    if (len1 < len2)
    {
       difference = len2-len1;
       if (!RotateEntriesRight(handle, cluster, end1, begin2+difference, difference))
       {
          return FALSE;
       }
       
       if (!SwapNBasicEntries(handle, cluster, begin1, begin2+difference, len1))
       {
          RotateEntriesLeft(handle, cluster, end1, begin2+difference, difference);
          return FALSE;     
       }
            
       if (!RotateEntriesRight(handle, cluster, begin1, end1+difference, difference))
       {
          SwapNBasicEntries(handle, cluster, begin1, begin2+difference, len1);
          RotateEntriesLeft(handle, cluster, end1, begin2+difference, difference);
          return FALSE;
       }
    }
    else if (len1 > len2)
    {
       difference = len1-len2;
       if (!RotateEntriesLeft(handle, cluster, end1-difference, begin2, difference))
       {
          return FALSE;
       }
       
       if (!SwapNBasicEntries(handle, cluster, begin1, begin2, len2))
       {
          RotateEntriesRight(handle, cluster, end1-difference, begin2, difference);
          return FALSE;     
       }
            
       if (!RotateEntriesLeft(handle, cluster, begin1, end1+difference, difference))
       {
          SwapNBasicEntries(handle, cluster, begin1, begin2, len2);
          RotateEntriesRight(handle, cluster, end1-difference, begin2, difference);
          return FALSE;
       }    
    }
    else /* len1 == len2 */
    {
       return SwapNBasicEntries(handle, cluster, begin1, begin2, len1);
    }
    return TRUE;        
}

