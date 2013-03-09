

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


          title     MODULES - Area for 4DOS resident loadable modules
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  12/8/90


          This module provides the space for loadable modules to be filled
          in by the initialization code in 4DLINIT.ASM.  It therefore MUST
          be linked in the proper order!  This space is also used as a
          temporary large stack while calling INIParse, which needs a
          large stack for its line input buffers, and as temporary storage
          for moving parts of the INI file around in 4DLINIT.

          Use caution if the size of this area is reduced -- it must be left
          large enough to handle the various purposes for which it is used
          by 4DLINIT.

          } end description
          ;
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          public    TempStor, ModArea, ModGuard, BigTOS
          ;
          ;
          ; Area for loadable modules
          ;
TempStor  equ       $                   ;temporary storage for 4DLINIT
ModArea   db        MODMAX dup (?)      ;room for loadable modules
BigTOS    equ       $                   ;top of big stack
ModGuard  db        ?                   ;if we go past here we're in trouble
          ;
          ;
@curseg   ends
          ;
          end

