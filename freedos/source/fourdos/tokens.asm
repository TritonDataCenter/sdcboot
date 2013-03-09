

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


          title     TOKENS - Token handling for INIPARSE and KEYPARSE
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1991, JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  4/17/91
                               revised as separate routine 9/17/91

          These routines are used to support token-based operations in
          INIPARSE and KEYPARSE.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          include   4dlparms.asm        ;4DOS parameters
          ;
          .cseg
          ;
          include   model.inc           ;Spontaneous Assembly memory models
          include   util.inc            ;Spontaneous Assembly util macros
          ;
          .defcode
          ;
          assume    cs:@curseg,ds:nothing  ;set up CS assume
          ;
          ; Externals
          ;
          .extrn    MEM_CMPI:auto, STR_PBRKN:auto, STR_CSPN:auto
          ;
          ;
          ; NextTok - Get next token for INI file and keystroke name 
          ; processing
          ;
          ; On entry:
          ;         ES:DI = address of token delimiter tables (two null-
          ;           terminated tables stored sequentially, first table
          ;           for characters to skip at start of token, second for
          ;           terminators at end)
          ;         DS:SI = address of end of old token + 1
          ;         
          ; On exit:
          ;         Flags set to:
          ;           JA      token found, address in SI, length in CX
          ;           JE      comment (;) found
          ;           JB      no more tokens on line
          ;         AX = Address of end of token + 1
          ;         CX = token length if JA
          ;         SI = token address if JA
          ;         Other registers and interrupt state unchanged
          ;
          entry     NextTok,noframe,far  ;define entry point
          ;
          call      STR_PBRKN           ;skip leading characters
           jb       NTExit              ;if line is empty we're done
          mov       al,[si]             ;get first byte
          cmp       al,';'              ;comment?
           je       NTExit              ;if so get out
          xor       al,al               ;get null
          mov       cx,0FFFFh           ;scan regardless of table length
          repne     scasb               ;skip past first table
          call      STR_CSPN            ;find end of token
          mov       ax,si               ;copy start
          add       ax,cx               ;add to get end + 1
          cmp       cl,0                ;set JA
          ;
NTExit:   exit                          ;and return
          ;
          ;
          ; TokList - See if the current token is in a list, using minimum
          ;           matching
          ;
          ; On entry:
          ;         CX = length of the token to check
          ;         DS:SI = address of the token to check
          ;         DS:DI = address of the list of possible values, in the
          ;                 following structure:
          ;
          ;                   db        nitems    ;# of items in list
          ;                   db        fixbytes  ;bytes of fixed data in
          ;                                       ;each item after name
          ;                   db        ItemName  ;name for 1st item
          ;                   db        Fixed     ;fixed data for 1st item
          ;                   ...                 ;repeat for add'l items
          ;         
          ; On exit:
          ;         Flags set to:
          ;           JA      token is in the list, non-unique match
          ;           JE      token is in the list, unique match
          ;           JB      token is not in the list
          ;         AL = token position in list if not JC; 0-based
          ;         AH = number of characters of token that matched
          ;         DS:DI = address of first byte after name of first matched
          ;                 item, ie first byte of fixed data (if any)
          ;         Other registers and interrupt state unchanged
          ;
          entry     TokList,varframe,far  ;define entry point
          ;
          varB      MatchCnt            ;number of matches found
          varB      MatChars            ;number of characters matched
          varW      MatchAdr            ;pointer to first match fixed data
          varend                        
          ;
          .push     bx,cx,dx,es         ;save registers
          loadseg   es,ds               ;set ES to local data
          mov       dl,[di]             ;get number of choices
          push      dx                  ;save it
          mov       bh,1[di]            ;get bytes of fixed data to skip
          mov       dh,cl               ;save token length
          add       di,2                ;skip choice count and fixed count
          mov       bptr MatchCnt,0     ;clear match count
          xor       ah,ah               ;clear high byte for string lengths
          ;
TLLoop:   mov       al,[di]             ;get item name length
          inc       di                  ;point to text
          cmp       cl,al               ;check if token is larger
           ja       NextItem            ;if so it can't be a match
;           jbe      CompItem            ;if not go compare
;          mov       cl,al               ;use item length for comparison
;          ;
;CompItem: 
          call      MEM_CMPI            ;compare names
           jne      NextItem            ;if no match go on
          inc       bptr MatchCnt       ;bump match counter
          mov       bl,dl               ;save last match item counter
          mov       MatChars,cl         ;save match length for return
          add       di,ax               ;skip name part
          mov       MatchAdr,di         ;save last match fixed data address
          cmp       dh,al               ;it matched, was length equal also?
           je       TLCalc              ;if so quit trying (note TLCalc
                                        ;  relies on JE state)
          xor       ax,ax               ;kill any addition below
          ;
NextItem: add       di,ax               ;move past string
          mov       al,bh               ;copy fixed data length
          add       di,ax               ;skip fixed data
          mov       cl,dh               ;get back token length in CX
          dec       dl                  ;count list items
           ja       TLLoop              ;if more to compare go on
          pop       ax                  ;get back saved item count
          cmp       bptr MatchCnt,1     ;check match count
           jb       TLDone              ;no match, return JB
          push      ax                  ;adjust for next instruction
          ;
TLCalc:   pop       ax                  ;get back total item count
          pushf                         ;save return flags (JA or JE)
          sub       al,bl               ;get relative position of last match
          mov       di,MatchAdr         ;get back match address
          mov       ah,MatChars         ;get back character count
          popf                          ;restore JA / JE state
          ;
TLDone:   .pop      bx,cx,dx,es         ;restore registers
          exit                          ;and return
          ;
          ;
@curseg   ends                          ;close segment
          ;
          end

