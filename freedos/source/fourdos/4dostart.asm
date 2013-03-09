

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


          title     4DOSTART - 4DOS Initialization / Resident Portion Startup
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  12/2/90

          This routine takes the 4DOS resident and initialization code
          at offset 100 of the code segment, moves it down to offset
          zero, adjusts CS to point to the PSP, and starts up 4DLINIT.

          This makes the resident part of 4DOS behave just like a
          .COM file, to ensure compatibility with other products that
          require the resident command processor's CS to be the same
          as its PSP address.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader / server parameters
          ;
          ;
          ; External references
          ;
          extrn     Init:near
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          page
          ;
          ;
          ; Slide the initialization and resident portions of 4DOS down
          ; to just above the PSP.  This code runs on the default stack
          ; set up for the 4DOS EXE file; 4DLINIT will switch to the
          ; local stack later.
          ;
          public    Startup
          ;
Startup:  
          if        DEBUG               ;;only assemble in debugging mode
          jmp       $$001
$dump     db        "Before move  CS="
$CSVal    db        6 dup (' ')
          db        "DS="
$DSVal    db        6 dup (' ')
          db        "ES="
$ESVal    db        6 dup (' ')
          db        "Env="
$EnvVal   db        6 dup (' ')
          db        CR, LF, '$'
$$001:
  extrn     HexOutW:near
  pushm     ax,bx,cx,dx,ds,es
  mov       bx,ds
  mov       cx,es
  mov       dx,ds:[PSP_ENV]
  loadseg   ds,cs
  loadseg   es,cs
  mov       di,offset $CSVal
  mov       ax,cs
  call      HexOutW
  mov       di,offset $DSVal
  mov       ax,bx
  call      HexOutW
  mov       di,offset $ESVal
  mov       ax,cx
  call      HexOutW
  mov       di,offset $EnvVal
  mov       ax,dx
  call      HexOutW
  mov       dx,offset $dump
  calldos   MESSAGE
  popm      es,ds,dx,cx,bx,ax

          endif

          loadseg   ds,cs               ;get local segment and copy to DS for
                                        ;  source (ES already points to PSP
                                        ;  segment as destination)
          mov       di,PSP_LEN          ;get starting destination offset
          mov       si,di               ;source offset is the same
          mov       cx,offset Startup   ;point to ouselves = end of move + 1
          sub       cx,si               ;subtract start to get move length
          cld                           ;forward move
          rep       movsb               ;move everything down
          push      es                  ;push new code segment
          mov       ax,offset Init      ;get initialization offset
          push      ax                  ;push that
          retf                          ;and start the initialization code
          ;
          ;
          ; Define load endpoint; leave room for an MCB after it, so that 
          ; we can reduce memory allocation to this point without the new
          ; MCB clobbering the transient portion.
          ;
loadend   db        ?
          db        16 dup (?)
          ;
          ;
@curseg   ends                          ;close segment
          ;
          end       Startup             ;that's all folks

