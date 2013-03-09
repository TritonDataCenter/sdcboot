/***********************************************************/
/* UTF-8 to DOS Codepage Converter                         */
/* Version 1.0                                             */
/***********************************************************/
/* Copyright (C) 2004 Robert Platt <worldwiderob@yahoo.co.uk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _UTFCP_H_INCLUDED
#define _UTFCP_H_INCLUDED

int selectCodepage(int codepage);
/* selectCodepage
 * ==============
 *
 * Use this function first to autodetect or specify the codepage.
 *
 * Parameters:
 *   codepage = 0     Auto detect codepage through display.sys int API.
 *   codepage = nnn   Specify the codepage directly.
 * Returns:
 *   the selected codepage if that codepage is supported, else it returns
 *   zero
 */

int UTF8ToCodepage (unsigned char *string);
/* UTF8ToCodepage
 * ==============
 *
 * Converts a UTF-8 string into the codepage (run selectCodepage first
 * to detect the active codepage, otherwise all unicode values greater
 * than 0x7F will be converted into '?')
 *
 * Parameters:
 *   string           The buffer which will be transformed into the
 *                    DOS codepage encoding. The string will be modified.
 * Returns:
 *   1                If the string was valid UTF-8.
 *   0                If the string contained invalid octets, such as
 *                    overlong ???. This function aborts as soon as it
 *                    finds such errors, without transforming any more
 *                    of the string.
 */


unsigned char unicodeToCodepage(unsigned long unicode);
/* unicodeToCodepage
 * =================
 *
 * Returns the codepage character that corresponds to the unicode value
 * given.
 *
 * Parameter:
 *   unicode         The unicode value to convert
 * Returns:
 *   '?' if the unicode value isn't represented in the codepage,
 *   otherwise the character code is returned.
 */

#endif /* _UTFCP_H_INCLUDED */