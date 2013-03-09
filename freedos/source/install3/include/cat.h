/* Copyright (C) 1998,2000 Jim Hall <jhall@freedos.org> 
   modified by Jeremy Davis <jeremyd@computer.org> 
   (all modifications by Jeremy copyright Jim Hall)
*/

/*
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
*/


#ifndef CAT_FILE_H
#define CAT_FILE_H

#ifdef __cplusplus
extern "C" {
#endif


/* displays maxlines of file given by filename, if maxlines is -1 prints whole file
   trys (in this order)
   .\%LANG%\filename.TXT
   .\%LANG%\filename
   .\filename.%LANG%
   if %LANG% in format "aa-BB", such as "en-UK" then try
   {
   .\aa\filename.TXT   (ie .\en\filename.TXT)
   .\aa\filename
   .\filename.aa       (ie .\filename.en )
   }
   .\filename.TXT
   .\filename
   repeat above search, except instead of ., use each path in %NLSPATH%
   if still not found then return failure (-1)
   if found then returns number of lines printed
*/
int cat_file (const char *filename, int maxlines);

/* 
  This function based on catopen from Jim Hall's catgets 
  Attempts to open file <filename> using above search criteria.
  If successfully opened returns the FILE * returned by fopen(<file path & name>, flags),
  otherwise returns NULL
  If PATHSPECIFIED is defined when cat.c is compiled, then if filename contains a
  directory separator assume fully qualified name and only tries to open it 
  (ie performs straight fopen(filename, flags) and does not use search alg)
*/
FILE * langfopen(const char *filename, const char *flags);

/*
If CAT_FILE_PROGRAM is defined when cat.c is compiled, then includes a simple main()
so can be used to cat the proper file based on language defined by user, ie
a simple cat that chooses which actual file to display based on above search alg.
*/


#ifdef __cplusplus
}
#endif

#endif /* CAT_FILE_H */
