/*    
   Altkey.c - routine to detect wether alt key is pressed and released.
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
   email me at:  imre.leber@vub.ac.be
*/

#include "keyboard.h"

/*
   Returns wether an altkey is pressed and released and meanwhile no other
   key was pressed.
*/

int AltKeyPressed (void)
{
    if (!AltKeyDown()) return 0;

    while (AltKeyDown())
          if (KeyPressed()) 
             return 0;

    return 1;
}
