/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  If you have any questions, comments, suggestions, or fixes please
 *  email me at:  imre.leber@worldonline.be   
 */
 
#include <string.h>
#include <dir.h>

#define DIR_SEPARATOR "\\"
 
/*-------------------------------------------------------------------------*/
/* Appends a trailing directory separator to the path, but only if it is   */
/* missing.                                                                */
/*-------------------------------------------------------------------------*/

char *addsep(char *path)
{
    int path_length;

    path_length=strlen(path);
    if (path[path_length-1] != *DIR_SEPARATOR)
    {
	path[path_length]=*DIR_SEPARATOR;
        path[path_length+1]='\0';
    } // end if.

    return path;

} // end addsep.
