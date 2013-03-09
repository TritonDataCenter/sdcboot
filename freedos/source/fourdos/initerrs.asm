

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


          title     INITERRS - 4DOS loader transient portion error messages
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  04/14/89

          This module includes only an error message table.  Routines for
          accessing this table are in ERRORMSG.ASM.

          ******************************************************************
          NOTE:  This text has been compressed and copied from 4DLTERRS.TXT.
          Any changes should be made there, not here!
          ******************************************************************

          } end description

          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader parameters
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          ;
          ; Fatal errors detected by transient part of loader
          ;
          public    InitErrs
          ;
InitErrs  label     byte
          dw        offset InitBad      ;store offset of bad error number msg
          ;
          ; Translation table
          ;
          db        0DEh
          db        065h, 020h, 061h, 069h, 073h, 064h, 06Ch, 06Fh
          db        072h, 074h, 066h, 06Eh, 06Dh, 063h, 067h, 070h
          db        075h, 077h, 041h, 053h, 062h, 079h, 043h, 049h
          db        04Dh, 04Fh, 06Bh, 07Ah, 000h, 000h
          ;
          ; #E_SWSEEK Swap file seek failed
          db        E_SWSEEK, 019h, 000h, 015h, 013h, 041h, 013h, 0C5h, 082h
          db        036h, 022h, 01Ch, 03Ch, 045h, 082h, 070h
          ; #E_SWWRIT Swap file write failed
          db        E_SWWRIT, 01Ah, 000h, 015h, 013h, 041h, 013h, 0C5h, 082h
          db        031h, 03Ah, 05Bh, 023h, 0C4h, 058h, 027h
          ;
          ; #E_DSIZE Cannot reduce data segment size
          db        E_DSIZE, 023h, 000h, 018h, 04Dh, 0D9h, 0B3h, 0A2h, 071h
          db        02Fh, 023h, 074h, 0B4h, 036h, 021h, 00Eh, 02Dh
          db        0B3h, 065h, 01Dh, 020h
          ; #E_MEMDA Memory deallocation error
          db        E_MEMDA, 01Bh, 000h, 01Ah, 02Eh, 09Ah, 017h, 037h, 024h
          db        088h, 09Fh, 04Bh, 059h, 0D3h, 02Ah, 0A9h, 0A0h
          ; #E_NOMEM Out of memory
          db        E_NOMEM, 010h, 000h, 01Bh, 012h, 0B3h, 09Ch, 03Eh, 02Eh
          db        09Ah, 017h
          ; #E_MODFIT Insufficient load space
          db        E_MODFIT, 01Ah, 000h, 019h, 0D6h, 012h, 0CCh, 05Fh, 052h
          db        0DBh, 038h, 094h, 073h, 061h, 014h, 0F2h
          ; #E_RELOC Address table missing
          db        E_RELOC, 018h, 000h, 014h, 077h, 0A2h, 066h, 03Bh, 041h
          db        068h, 023h, 0E5h, 066h, 05Dh, 010h
          ; #E_RLSIZE Address table too large
          db        E_RLSIZE, 01Ah, 000h, 014h, 077h, 0A2h, 066h, 03Bh, 041h
          db        068h, 023h, 0B9h, 093h, 084h, 0A1h, 002h
        ;
          db        0FFh, 0             ;end of table
          ;
InitBad   db        BErrLen, "Illegal initialization error"
BErrLen   equ       $ - InitBad - 1
          ;
@curseg   ends
          ;
          end

