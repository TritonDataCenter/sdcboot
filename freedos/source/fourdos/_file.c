

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/***
*_file.c - perprocess file and buffer data declarations
*
*   Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   file and buffer data declarations
*
*******************************************************************************/

#include "product.h"

#include <file2.h>

/* Number of files */

#define _NFILE_ 40

/*
 * FILE2 descriptors; preset for stdin/out/err/aux/prn
 */

FILE2 _iob2[ _NFILE_ ] = {
    /* flag2,   charbuf */
    {
    0, '\0', 0  }
    ,
    {
    0,  '\0', 0 }
    ,
    {
    0,  '\0', 0 }
    ,
#if _DOS
    {
    0,  '\0', 0 }
    ,
    {
    0,  '\0', 0 }
    ,
#endif
};

