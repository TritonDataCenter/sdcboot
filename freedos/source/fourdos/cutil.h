

//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  (1) The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  (2) The Software, or any portion of it, may not be compiled for use on any
//  operating system OTHER than FreeDOS without written permission from Rex Conn
//  <rconn@jpsoft.com>
//
//  (3) The Software, or any portion of it, may not be used in any commercial
//  product without written permission from Rex Conn <rconn@jpsoft.com>
//
//  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.


/*******************************************************************
   Common definitons for CUTIL.LIB
 *******************************************************************/
#ifndef __CUTIL__
  #define __CUTIL__

  /* definitions for the history-function */
  #define HISTORYTEMPLATE "%1: %2 (%3 / %4)"    /* Outline of history line */

  #define CCHMAXHISTORY         64          /* Max. length of line in protocol field  */
  #define MAXHISTORYLINES       256         /* Max. number of lines in protokol field */

  #define EA_HISTORYNAME    ".HISTORY"      /* EA-name for history */

  /* structure for entry of data to EAWriteMVMT */
  typedef struct _STRUC_EAT_DATA
      {
      USHORT          usEAType;             /* EAT_* */
      USHORT          uscValue;             /* length of pValue */
      PBYTE           pValue;               /* buffer */
      } STRUC_EAT_DATA;
  typedef STRUC_EAT_DATA *PSTRUC_EAT_DATA;

  BOOL   _System EAWriteASCII (PCHAR, PCHAR, PCHAR);
  BOOL   _System EAReadASCII  (PCHAR, PCHAR, PCHAR, PUSHORT);
  BOOL   _System EAWrite (PCHAR, PCHAR, PSTRUC_EAT_DATA);
  BOOL   _System EARead  (PCHAR, PCHAR, PSTRUC_EAT_DATA);
  BOOL   _System EAWriteMV (PCHAR, PCHAR, USHORT, PSTRUC_EAT_DATA);
  BOOL   _System EAReadMV  (PCHAR, PCHAR, USHORT, PSTRUC_EAT_DATA);
  BOOL   _System History (PCHAR, PCHAR, PCHAR);
  APIRET _System GetDateTime (PDATETIME, PCHAR, PCHAR);

#endif /* __CUTIL__ */

