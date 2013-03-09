/*    
   Lstackd.c - low stack directory routines.
   
   Copyright (C) 2000 Imre Leber

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

#include <stdlib.h>

#include "rdwrsect.h"
#include "direct.h"
#include "bool.h"
#include "fat.h"
#include "subdir.h"
#include "lchdir.h"
#include "lstackd.h"

struct Node {
     struct DirectoryPosition pos;
     struct Node* next;
};

static struct Node* Header = NULL;

int low_pushd(void)
{
     struct Node* node = (struct Node*) malloc(sizeof(struct Node));

     if (node == NULL) return FALSE;

     node->pos = low_getcwd();

     if (Header == NULL)
     {
        Header = node;
        node->next = NULL;
     }
     else
     {
        node->next = Header;
        Header = node;
     }

     return TRUE;
}

int low_popd(void)
{
    struct Node* node;

    if (Header)
    {
       node = Header->next;
       free(Header);
       Header = node;
    }
    else
       return FALSE;

    return TRUE;
}

void low_popall(void)
{
     while (low_popd());
}

int low_topd(void)
{
     if (low_popd())
        return low_pushd();
     else
        return FALSE;
}
