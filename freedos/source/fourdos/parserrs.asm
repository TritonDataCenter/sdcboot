

;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
;
;  (1) The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
;
;  (2) The Software, or any portion of it, may not be compiled for use on any
;  operating system OTHER than FreeDOS without written permission from Rex Conn
;  <rconn@jpsoft.com>
;
;  (3) The Software, or any portion of it, may not be used in any commercial
;  product without written permission from Rex Conn <rconn@jpsoft.com>
;
;  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.


          ;
          ;  Parse error text to be INCLUDEd in 4DLINIT.ASM
          ;
          ;  Copyright 1991, JP Software Inc., All Rights Reserved
          ;  Author:  Tom Rawson  05/11/91
          ;
          ; This module includes only an error message table.  Routines for
          ; accessing this table are in ERRORMSG.ASM.
          ;
          ; ****************************************************************
          ; NOTE:  This text has been compressed and copied from
          ; PARSERRS.TXT.  Any changes should be made there, not here!
          ; ****************************************************************
          ;
          public    ParsErrs
          ;
          ;
          ; NOTE:  This text is loaded initially in 4DLINIT.ASM.  It is
          ; then moved by 4DLINIT code into the resident portion of 4DOS
          ; if necessary.  This move invalidates the "dw  offset ParsBad"
          ; below.  This is fixed by adjusting this value inside the
          ; subroutine LoadMod in 4DLINIT.  Be aware of this if you change
          ; the structure of the data below!
          ;
          ;
ParsErrs  label     byte                ;beginning of message table
          dw        offset ParsBad      ;store offset of bad error number msg
          ;
          ; Translation table
          ;
          db        0DEh
          db        061h, 065h, 020h, 072h, 074h, 06Eh, 06Fh, 06Ch
          db        06Dh, 069h, 064h, 076h, 077h, 049h, 050h, 063h
          db        070h, 073h, 075h, 067h, 079h, 052h, 054h, 062h
          db        066h, 068h, 06Bh, 071h, 000h, 000h
          ; Error messages
          ;
          ; #1 Too many parameters
          db        1, 017h, 000h, 018h, 088h, 04Ah, 027h, 016h, 041h
          db        022h, 052h, 0A3h, 063h, 051h, 030h
          ; #2 Required parameter missing
          db        2, 021h, 000h, 017h, 031h, 0D1h, 04Bh, 053h, 0C4h
          db        012h, 025h, 02Ah, 036h, 035h, 04Ah, 0B1h, 031h
          db        03Bh, 071h, 050h
          ; #3 Invalid switch
          db        3, 011h, 000h, 0F7h, 0D2h, 09Bh, 0C4h, 013h, 0EBh
          db        061h, 011h, 0B0h
          ; #4 Invalid keyword
          db        4, 011h, 000h, 0F7h, 0D2h, 09Bh, 0C4h, 01Ch, 031h
          db        06Eh, 085h, 0C0h
          ; #6 Parameter value not in allowed range
          db        6, 027h, 000h, 010h, 025h, 02Ah, 036h, 035h, 04Dh
          db        029h, 014h, 034h, 078h, 064h, 0B7h, 042h, 099h
          db        08Eh, 03Ch, 045h, 027h, 015h, 030h
          ; #7 Parameter value not allowed
          db        7, 01Dh, 000h, 010h, 025h, 02Ah, 036h, 035h, 04Dh
          db        029h, 014h, 034h, 078h, 064h, 029h, 098h, 0E3h
          db        0C0h
          ; #8 Parameter value not allowed
          db        8, 01Dh, 000h, 010h, 025h, 02Ah, 036h, 035h, 04Dh
          db        029h, 014h, 034h, 078h, 064h, 029h, 098h, 0E3h
          db        0C0h
          ; #9 Parameter format not correct
          db        9, 020h, 000h, 010h, 025h, 02Ah, 036h, 035h, 041h
          db        0A8h, 05Ah, 026h, 047h, 086h, 041h, 018h, 055h
          db        031h, 016h
          ; #10 Invalid parameter
          db        10, 012h, 000h, 0F7h, 0D2h, 09Bh, 0C4h, 012h, 025h
          db        02Ah, 036h, 035h
          ; #11 Invalid parameter combination
          db        11, 020h, 000h, 0F7h, 0D2h, 09Bh, 0C4h, 012h, 025h
          db        02Ah, 036h, 035h, 041h, 018h, 0A1h, 09Bh, 072h
          db        06Bh, 087h
          ;
          db        0FFh, 0             ;end of table
          ;
ParsBad   db        1, 0                ;no message if not found

