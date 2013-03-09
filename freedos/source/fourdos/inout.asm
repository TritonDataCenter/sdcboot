

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


          title     INOUT - Replacement for C input and output functions
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Author:  Tom Rawson  6/13/90


          These routines replace the MSC sscanf and sprintf routines with
          smaller and faster assembler code.  The formatting options sup-
          ported are a small subset of those supported in C -- only those
          needed for basic I/O.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          ;
          ;
          ; External references
          ;

_TEXT     segment   word public 'CODE'  ;define code segment for externals
          extrn     __write:far        ; interface to C write routine
Write     equ       __write
          extrn     CheckOnError:far
CheckErr	equ	CheckOnError
_TEXT     ends

          ;
_DATA     segment   para public 'DATA'  ;transient portion data segment
          extrn     _gaCountryInfo:byte
_DATA     ends
          ;
DGROUP    group     _DATA               ;define group
          ;
          ;
          ; Parameters
          ;
OUTSIZE   equ       256                 ;size of internal output buffer
DEFMAX    equ       7FFFh               ;default max for output precision
                                        ; and input width
DEFTRUNC  equ       36                  ;default max for filename truncation
TRFILL    equ       0FAFAh              ;2x truncation fill character
          ;
          ;

ASM_TEXT  segment   word public 'CODE'  ;define code segment
          ;
          assume    cs:ASM_TEXT, ds:DGROUP, es:nothing, ss:nothing

          ;
          ; _FMTOUT - Format a string for output, and write it to a file
          ;           in chunks if requested
          ;
          ; Arguments:
          ;
          ;         int  handle         File handle for output, set to -1
          ;                               for output to BUF
          ;         char far *buf       Buffer to hold output
          ;         char *fmt           Format string
          ;         void *arglist       Pointer to list of args to be output
          ;
          ;    The format string appears as follows:
          ;
          ;          [text][fspec]..[text][fspec].. ...
          ;
          ;    where "text" represents text to be output, there is one
          ;    "fspec" for each of arg1..argn, and "fspec" is of the form:
          ;
          ;         %[flag][width][.prec][mod]type
          ;
          ;         flag      0 to use zero-fill, otherwise blank fill is
          ;                     used
          ;         width     minimum output width
          ;         prec      maximum output width if type is s
          ;         mod       F if arg is far; otherwise arg assumed near
          ;                   l if integer arg is long; otherwise assumed
          ;                     short
          ;                   L same as l but output with separators
          ;         type      s for null-terminated string
          ;                   c for a single character
          ;                   u for an unsigned decimal integer
          ;                   d for a signed decimal integer
          ;                   x for an unsigned hex integer
          ;
          ;    width and prec values may be specified as "*", in which case
          ;    the value comes from the argument list
          ;
          ; Returns:
          ;
          ;    Number of characters stored in buffer, excluding the
          ;    trailing null.
          ;
          ; Local jump tables:
          ;
OutPScan  db        3, "FlL"            ;3 bytes, F, l, and L
OutPJump  label     near                ;prefix jump table
          dw        OFarData            ;F = far data 
          dw        OLongInt            ;l = long integer
          dw        OLongSep            ;L = long integer with separators
          ;
OutTScan  db        5, "sducx"          ;5 bytes
OutTJump  label     near                ;format string jump table
          dw        OutStrng            ;s = string
          dw        OutInt              ;d = signed integer
          dw        OutUns              ;u = unsigned integer
          dw        OutChar             ;c = character
          dw        OutHex              ;x = unsigned hex
          ;
          ;
          entry     _fmtout,varframe,far
          ;
          argW      ArgList             ;start of the variable arg list
          argW      FmtAdr              ;address of the format string
          argD      BufAdr              ;far address of the output buffer
          argW      Handle              ;output file handle
          ;
          var       CvtBuf,15           ;integer conversion buffer, room
                                        ;  for 13 bytes + sign + terminator
CvtBEnd   equ       CvtBuf + 14         ;end of buffer
          varW      MinWidth            ;width
          varW      Prec                ;precision
          varB      Sign                ; sign flag
          varB      Fill                ;fill character
          varB      FarFlag             ;far data flag
          varB      LongFlag            ;longint flag
          varW      ThouSep             ;thousands separator
          varW      OutSub              ;pointer to output instruction
          var       OutBuf,OUTSIZE      ;output buffer for file I/O
OBEnd     equ       Outbuf + OUTSIZE    ;end of output buffer + 1
          varW      BytesWritten        ; number of bytes written
          varend                        ;end of variables
          ;
          ;
          pushm     si,di               ;save registers
          cld                           ;everything goes forward
	;
          les       di,BufAdr           ;get output buffer segment:offset
          cmp       wptr Handle,-1      ;using file?
          ;
          if        (_DOS ne 0) and (_WIN eq 0)  ;in DOS check for huge ptr
          jne       SetFile             ;if using file go set it up
          call      HPAdj               ;otherwise adjust pointer
          dstore    <wptr [BufAdr]>,di,es  ;save buffer address back on stack
          jmp       GetFmt              ;and then go on
          else
          je        GetFmt              ;if not using file go on
          endif
          ;
SetFile:  mov       word ptr BytesWritten,0
          loadseg   es,ss               ;output address on stack
          lea       di,OutBuf           ;get local buffer address
          ;
GetFmt:   mov       si,FmtAdr           ;source comes from format string
          ;
OScnStrt: mov       cx,si               ;save source address
          ;
OScnLoop: lodsb                         ;get a byte of format string
          or        al,al               ;end of string?
          jne       OScnCont            ;if not go on
          jmp       OScnDone            ;otherwise exit
          ;
OScnCont: cmp       al,'%'              ;format spec?
          jne       OScnLoop            ;if not keep looking
          ;
          call      FmtFlush            ;flush text from format string
          xor       ax,ax               ;get zero
          mov       bptr FarFlag,al     ;clear far data
          mov       bptr LongFlag,al    ;clear long integer
          mov       bptr Sign, al       ; clear sign flag
          mov       wptr ThouSep,ax     ;clear separator

          lodsb                         ; get possible sign character
          cmp       al, '-'
          jne       NotSign
          mov       bptr Sign, 1
          lodsb                         ;get possible fill character
NotSign:
          cmp       al,'0'              ;zero?
          je        SaveFill            ;if so go on
          dec       si                  ;if not back up
          mov       al,' '              ;and set fill character to blank
          ;
SaveFill: mov       Fill,al             ;save fill character
          ;
          mov       wptr Prec,DEFMAX    ;default precision
          call      ReadFInt            ;get width
          cmp       bptr Sign, 0
          je        NotNegative

                                        ;negative -- twos complement AX
          xor       ax,0FFFFh           ;complement low-order
          inc       ax                  ;increment low-order

NotNegative:
          mov       MinWidth,ax         ;save it
          cmp       bptr [si],'.'       ;precision spec?
          jne       OFmtLoop            ;if not go on
          inc       si                  ;skip period
          call      ReadFInt            ;get precision
          mov       Prec,ax             ;save it
          ;
          ; Loop to process a format spec
          ;
OFmtLoop: lodsb                         ;get next character
          ;
          ; Check for a prefix character
          ;
          mov       bx,offset OutPScan  ;get character list address
          call      TabScan             ;scan for the character
          jc        ONoPfx              ;go on if not a prefix character
          jmp       wptr cs:OutPJump[bx]  ;jump to handler
          ;
          ;
OFarData: mov       bptr FarFlag,1      ;set far data
          jmp       short OFmtLoop      ;back for next character
          ;
OLongSep: mov       al,_gaCountryInfo.CY_TSEP  ;get separator (will be 0 if
                                               ;  no separator is used for
                                               ;  this country)
          mov       bptr ThouSep,al     ;stash it (high byte is not used),
                                        ;  and drop through to set long flag
          ;
OLongInt: mov       bptr LongFlag,1     ;set long integer
          jmp       short OFmtLoop      ;back for next character
          ;
          ; Not a prefix character, try for the type
          ;
ONoPfx:   mov       bx,offset OutTScan  ;get character list address
          call      TabScan             ;scan for the character
          jc        ONoType             ;go on if not a type character
          pushm     si,ds               ;save format string address
          jmp       wptr cs:OutTJump[bx]  ;jump to handler
          ;
ONoType:  mov       cx,si               ;get new start address
          dec       cx                  ;start at bad type character
          jmp       OScnLoop      ;and loop for more

          ;
          ;
          ; %s - string
          ;
;OutStrng: call      StrSetup            ;get string address
OutStrng:	call      GetArg              ;get next argument
          mov       si,ax               ;copy as string address
          cmp       bptr FarFlag,0      ;far string?
          je	OSReady		;if not go on
          call      GetArg              ;get next argument
	;
          if        (_DOS ne 0) and (_WIN eq 0)  ;in DOS check for huge ptr
          cmp       si,8000h            ;address over 32K?
          jb        OSSetSeg		;if not go on
          sub       si,8000h            ;if so reduce offset
          add       ax,800h             ;adjust it appropriately
	endif
	;
OSSetSeg:	mov       ds,ax               ;save as segment
	;
OSReady:	mov       cx,Prec             ;get maximum width
          jmp       short StrOut        ;and go do output
          ;
          ;
          ; %c - character
          ;
OutChar:  call      GetArg              ;get the character
          stosb                         ;output it
          jmp       short OFmtDone      ;and go on
          ;
          ;
          ; %d - integer
          ;
OutInt:   mov       cx,10               ;get base
          call      GetInt              ;get integer argument
          cmp       bptr LongFlag,0     ;long arg?
          jne       ChkSign             ;yes, go on
          cwd                           ;extend sign
          ;
ChkSign:  mov       bx,dx               ;save sign
          or        dx,dx               ;check sign of value
          jge       IntCom              ;go on if positive
          neg       ax                  ;negate low-order
          adc       dx,0                ;handle carry
          neg       dx                  ;negate low-order
          jmp       short IntCom        ;go to common code
          ;
          ;
          ; %x - hex
          ;
OutHex:   mov       cx,16               ;get unsigned base
          jmp       short UnsCom        ;and go on
          ;
          ;
          ; %u - unsigned integer
          ;
OutUns:   mov       cx,10               ;get decimal base
          ;
UnsCom:   xor       dx,dx               ;default high-order
          call      GetInt              ;get integer argument
          xor       bx,bx               ;clear sign
          ;
IntCom:   pushm     di,es               ;save current output address
          push      bx                  ;save sign
          lea       di,CvtBEnd          ;get end address
          loadseg   es,ss               ;get segment
          mov       si,ThouSep          ;get thousands separator
          call      ULASCII             ;convert to ASCII
          pop       bx                  ;get back sign
          or        bx,bx               ;was it negative?
          jge       PutInt              ;if not go on
          stocb     '-'                 ;if so store minus
          ;
PutInt:   mov       si,di               ;get start address - 1
          inc       si                  ;get start address
          popm      es,di               ;restore output address
          mov       cx,14               ;biggest an integer can be
          ;
          ; Copy a formatted string from DS:SI to the output buffer at ES:DI
          ;
StrOut:   jcxz      OFmtDone            ;if nothing there, skip output
          call      GetSLen             ;get input string length
          ;
StrOutGo: mov       dx,cx               ;copy to DX for DoFill
          mov       cx,MinWidth         ;get minimum output width
          call      DoFill              ;do the fill
          mov       cx,dx               ;get string length back
          mov       bx,offset OutCopy   ;get copy instruction address
          call      DoOutput            ;go do output in chunks
          mov       cx,MinWidth         ;get width again
          neg       cx                  ;invert to calculate trailing fill
          call      DoFill              ;do the fill
          ;
          ;
OFmtDone: popm      ds,si               ;restore format string address
          jmp       OScnStrt            ;continue scanning format
          ;
          ;
OScnDone: call      FmtFlush            ;flush any remaining format text
          cmp       wptr Handle,-1      ;output to file?
          je        OutTerm             ;if not go terminate buffer

          lea       ax,OutBuf           ;get buffer start
          mov       cx,di               ;get current output address
          sub       cx,ax               ;get count
          jcxz      OutDone             ;go on if nothing to output
          call      WriteBuf            ;write the buffer
          mov       ax,BytesWritten
          jmp       short OutDone       ;go on
          ;
OutTerm:  stosb                         ;terminate output (AL = 0 from format
                                        ;  string)
          mov       ax,di               ;get output end address
          sub       ax,BufAdr           ;get output character count
          dec       ax                  ;don't include terminator
          ;
OutDone:  popm      di,si               ;restore registers
          exit                          ;all done
          ;
          ;
          ; Instruction called by DoOutput to copy the output string
          ;
OutCopy:  rep       movsb               ;do the copy
          ret                           ;and go back to DoOutput

          ;
          ; _FMTIN - Format a string for input
          ;
          ; Arguments:
          ;
          ;         char far *source    Buffer of input
          ;         char *fmt           Format string
          ;         void *arglist       Pointer to list of args to be output
          ;
          ;    The format string appears as follows:
          ;
          ;          [text][fspec]..[text][fspec].. ...
          ;
          ;    where "text" represents text which must match the input, and
          ;    "fspec" is a format specification of the form:
          ;
          ;         %[ign][table][width][mod]type
          ;
          ;         ign       "*" to ignore the following field
          ;         table     table of characters to accept or reject, in []
          ;         width     maximum input width
          ;         mod       F if arg is far; otherwise arg assumed near
          ;                   l if integer arg is long; otherwise assumed
          ;                     short
          ;         type      s for null-terminated string
          ;                   c for a single character
          ;                   u for an unsigned decimal integer
          ;                   d for a signed decimal integer
          ;                   x for a hex integer
          ;                   [xx] for a list of characters
          ;
          ;    There must be one argument for each format specification.
          ;    Scanning stops when either text in the format string does
          ;    not match text in the source string, or the end of the
          ;    format string is reached.
          ;
          ; Returns:
          ;
          ;    Number of fields found.
          ;
CHTBYTES  equ       256                 ;bytes in 256-character table
CHTWORDS  equ       128                 ;words in 256-character table
          ;
          ; Local jump tables:
          ;
InPScan   db        2, "Fl"             ;2 bytes, F and l
InPJump   label     near                ;prefix jump table
          dw        IFarData            ;F = far data 
          dw        ILongInt            ;l = long integer
          ;
InTScan   db        6, "sducnx"         ;6 bytes
InTJump   label     near                ;format string jump table
          dw        InStrng             ;s = string
          dw        InInt               ;d = signed integer
          dw        InInt               ;u = unsigned integer
          dw        InChar              ;c = character
          dw        InCnt               ;n = character count
          dw        InHex               ;x = hex integer
          ;
TJMPOFF   equ       0                   ;jump table offset for %[]
SJMPOFF   equ       2                   ;jump table offset for %s
CJMPOFF   equ       4                   ;jump table offset for %c
          ;
BrkTab    label     word                ;string input break check addresses
          dw        BrkList             ;include / exclude list (%[])
          dw        BrkStrng            ;string (%s)
          dw        StrCont             ;char (%c) - no break
          ;
TrmTab    label     word                ;string input terminator addresses
          dw        TrmNull             ;include / exclude list (%[])
          dw        TrmNull             ;string (%s)
          dw        StrDone             ;char (%c) - no terminator
          ;
          ;
          entry     _fmtin,varframe,far
          ;
          argW      InArgs              ;start of the variable arg list
          argW      InFormat            ;address of the format string
          argD      InSource            ;far address of the input buffer
          ;
          varW      InMax               ;max input width
          varW      FieldCnt            ;field count
          varB      InFar               ;variable is far
          varB      InLong              ;integer is long
          varB      Ignore              ;ignore current spec
          var       CharTab, CHTBYTES   ;Character table for [] specs
          varend                        ;end of variables
          ;
          pushm     si,di               ;save registers
          cld                           ;everything goes forward
	;
          les       di,InSource         ;get input segment:offset
          mov       si,InFormat         ;get format string address
          mov       wptr FieldCnt,0     ;clear field count
          ;
          if        (_DOS ne 0) and (_WIN eq 0)  ;in DOS ...
          call      HPAdj               ;... check for huge pointer problems
          dstore    <wptr [InSource]>,di,es  ;save source address back on stack
          endif
          ;
IScnLoop: lodsb                         ;get a byte of format string
          or        al,al               ;end of string?
          je        IScnQuit            ;if so exit
          cmp       al,'%'              ;format spec?
          je        IFmtSpec            ;if so go process it
          call      IsSpace             ;is it a space?
          jne       InMatch             ;if not go see if format == source
          call      SkSpace             ;skip spaces in format
          call      PtrSwap             ;swap source / format pointers
          call      SkSpace             ;skip spaces in source
          call      PtrSwap             ;swap pointers back
          jmp       short IScnLoop      ;and loop for more
          ;
          ;
IFmtSpec: xor       al,al               ;get zero
          mov       InFar,al            ;clear far
          mov       InLong,al           ;clear long
          cmp       bptr [si],'*'       ;ignore flag?
          jne       IgnSave             ;if not go on
          inc       al                  ;set ignore flag
          inc       si                  ;skip '*'
          ;
IgnSave:  mov       Ignore,al           ;save ignore flag
          call      ReadFInt            ;get max width
          or        ax,ax               ;valid value?
          jnz       IWSave              ;if so go on
          mov       ax,DEFMAX           ;get default max width
          ;
IWSave:   mov       InMax,ax            ;save max width
          ;
          ; Loop to process a format spec
          ;
IFmtLoop: lodsb                         ;get next character
          ;
          ; Check for a prefix character
          ;
          mov       bx,offset InPScan   ;get character list address
          call      TabScan             ;scan for the character
          jc        INoPfx              ;go on if not a prefix character
          jmp       wptr cs:InPJump[bx] ;jump to handler
          ;
          ;
IFarData: inc       bptr InFar          ;set far data
          jmp       short IFmtLoop      ;back for next character
          ;
ILongInt: inc       bptr InLong         ;set long integer
          jmp       short IFmtLoop      ;back for next character
          ;
          ; Not a prefix character, try for the type
          ;
INoPfx:   cmp       al,'['              ;is it a list?
          je        InList              ;if so handle specially
          mov       bx,offset InTScan   ;get character list address
          call      TabScan             ;scan for the character
          jc        InMatch             ;if not a type character check match
          pushm     si,ds               ;save format address
          call      PtrSwap             ;move source pointers to DS:SI
          jmp       wptr cs:InTJump[bx] ;jump to handler
          ;
          ;
          ; Handle source / format comparison -- come here if no format
          ; spec or an invalid format spec is used
          ;
InMatch:  cmp       al,es:[di]          ;does source match format?
          jne       IScnQuit            ;if no match, quit
          inc       di                  ;it matches, bump source pointer
          jmp       short IScnLoop      ;and loop for more
          ;
IscnQuit: jmp       IScnDone            ;mismatch or end -- time to quit
          ;
          ;
          ; %[ - include or exclude list
          ;
InList:   xor       ax,ax               ;assume normal list, get zero
          cmp       bptr [si],'^'       ;reject list?
          jne       LoadTab             ;if not go on
          mov       ax,0101h            ;doing rejects, get ones
          inc       si                  ;skip '^'
          ;
LoadTab:  pushm     di,es               ;save source address
          lea       di,CharTab          ;get table address
          loadseg   es,ss               ;use stack segment
          mov       cx,CHTWORDS         ;get table size
          rep       stosw               ;fill table
          popm      es,di               ;restore source address
          xor       ah,ah               ;clear high byte for table access
          lea       dx,CharTab          ;get table address for use 
                                        ;  throughout string processing
          ;
TabLoop:  lodsb                         ;get next format byte
          or        al,al               ;end of string?
          je        TLDone              ;if so go on
          cmp       al,']'              ;end of spec?
          je        TLDone              ;if so go on
          mov       bx,dx               ;get table address
          add       bx,ax               ;add byte to get offset
          xor       bptr ss:[bx],1      ;toggle the byte
          jmp       short TabLoop       ;loop for more
          ;
TLDone:   pushm     si,ds               ;save format address
          call      PtrSwap             ;move source pointers to DS:SI
          mov       bx,TJMPOFF          ;get %[] jump table offset
          jmp       short StrCom        ;go to common code
          ;
          ;
          ; %s - string
          ;
InStrng:  call      SkSpace             ;skip white space in source text
          mov       bx,SJMPOFF          ;get %s jump table offset
          jmp       short StrCom        ;go to common code
          ;
          ;
          ; %c - character
          ;
InChar:   cmp       wptr InMax,DEFMAX   ;is width not set?
          jne       ICGo                ;go on if set
          mov       wptr InMax,1        ;default %c to 1 character
          ;
ICGo:     mov       bx,CJMPOFF          ;get %c jump table offset
          ;
          ;
          ; The following code processes strings for all three possible
          ; formats:  %[], %s, and %c.  In all cases the loop loads a
          ; character and checks for terminating null.  It then performs
          ; format-specific break checking, saves the character if
          ; necessary, and checks the width counter.  When there is a null, 
          ; a break, or the width is exceeded, a format-specific terminator
          ; is placed in the output string.  Format-specific branching is
          ; controlled by the branch tables BrkTab and TrmTab above.
          ;
          ;
StrCom:   call      GetPtr              ;get next argument pointer in ES:DI
          xor       ah,ah               ;clear high byte for include list
                                        ;  table access
          ;
          ; Get next source character and check if end.  If not, branch to
          ; break check code for this format type
          ;
StrLoop:  lodsb                         ;get next source byte
          or        al,al               ;end of source?
          je        StrBrk              ;if so generate a break
          jmp       wptr BrkTab[bx]     ;check for other break conditions
          ;
BrkStrng: call      IsSpace             ;space?
          jne       StrCont             ;if not continue
          jmp       short StrBrk        ;if so quit
          ;
BrkList:  push      bx                  ;save jump pointer
          mov       bx,dx               ;get table address
          add       bx,ax               ;add byte to get offset
          cmp       bptr ss:[bx],0      ;check the byte
          pop       bx                  ;restore jump pointer
          jz        StrBrk              ;if not in list then break
          ;
          ; No break - store character if necessary
          ;
StrCont:  cmp       bptr Ignore,0       ;ignoring data?
          jnz       StrWidth            ;if so skip save
          stosb                         ;save byte
          ;
          ; Check if output width exceeded
          ;
StrWidth: dec       wptr InMax          ;decrement width
          jg        StrLoop             ;if still room, go on
          jmp       short StrEnd        ;no room, we're done
          ;
          ; Break occurred - back up over break character
          ;
StrBrk:   dec       si                  ;break character found, back up
          ;
          ; Processing done - count field, store terminator (if any)
          ;
StrEnd:   cmp       bptr Ignore,0       ;ignoring data?
          jnz       StrDone             ;if so skip terminator
          inc       wptr FieldCnt       ;no ignore, bump field count
          jmp       wptr TrmTab[bx]     ;branch to terminator code
          ;
TrmNull:  stocb     0                   ;null-terminate string
          ;
StrDone:  jmp       short IFmtDone      ;format spec done, loop for more
          ;
          ;
          ; %n - character count
          ;
InCnt:    mov       ax,si               ;copy source pointer
          sub       ax,InSource         ;get count
          xor       dx,dx               ;clear high-order
          jmp       short InICom        ;go to common code
          ;
          ; %x - hex
          ;
InHex:    call      SkSpace             ;skip space
          mov       cx,InMax            ;get maximum width
          call      ReadHex             ;read the hex value
          jmp       short InICom        ;go to common code
          ;
          ; %d - integer; %u - unsigned
          ;
InInt:    call      SkSpace             ;skip space
          mov       cx,InMax            ;get maximum width
          call      ReadInt             ;read the integer
          ;
InICom:   cmp       bptr Ignore,0       ;ignoring data?
          jnz       IFmtDone            ;if so go on
          call      GetPtr              ;get next argument pointer in ES:DI
          mov       es:[di],ax          ;store low-order
          cmp       bptr InLong,0       ;check if long
          jz        IIEnd               ;if not go on
          mov       es:2[di],dx         ;store high-order
          ;
IIEnd:    cmp       cx,InMax            ;were any digits found?
          je        IFmtDone            ;if not skip field count increment
          inc       wptr FieldCnt       ;bump field count
          ;
          ;
          ; Format spec done - restore pointers and keep scanning
          ;
IFmtDone: call      PtrSwap             ;get source pointers back in ES:DI
          popm      ds,si               ;restore format address
          jmp       IScnLoop            ;continue scanning format
          ;
          ;
          ; Input scan is done, return
          ;
IScnDone: mov       ax,FieldCnt         ;get field count
          popm      di,si               ;restore registers
          exit                          ;all done

          ;
          ;
          ; ReadFInt - Read an integer from format string
          ;
          ; On entry:
          ;         DS:SI = pointer to format string
          ;
          ; On exit:
          ;         DX:AX = integer found, zero if none found
          ;         BX, CX destroyed
          ;         DS:SI = updated pointer
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     ReadFInt,noframe,,local  ;start ReadFInt

          xor       cl,cl
          cmp       bptr [si],'-'       ; "format left"?
          jne       no_left
          inc       cl
          inc       si
no_left:
          cmp       bptr [si],'*'       ;get value from argument list?
          jne       RFRead              ;no, go on
          inc       si                  ;skip asterisk
          call      GetArg              ;get argument value in AX
          jmp       short RFSign        ;and return

RFRead:
          push      cx                            ; save the sign
          mov       cx,DEFMAX           ;no limit on digits
          call      ReadInt             ;read the integer
          pop       cx

RFSign:
          or        cl,cl               ;check sign
          je        RFDone              ;if positive go on
                                        ;negative -- twos complement AX
          xor       ax,0FFFFh           ;complement low-order
          inc       ax                  ;increment low-order

RFDone:
          exit                          ;all done

          ;
          ;
          ; ReadInt - Read an integer from format spec or source
          ;
          ; On entry:
          ;         CX = maximum number of characters to process
          ;         DS:SI = pointer to string to read
          ;
          ; On exit:
          ;         DX:AX = integer found
          ;         BX destroyed
          ;         CX = entry value less number of characters processed
          ;         DS:SI = updated pointer
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     ReadInt,noframe,,local  ;start ReadInt
          ;
          push      di                  ;save DI
          xor       bl,bl               ;clear sign flag
          cmp       bptr [si],'+'       ;start with a plus sign?
          je        SkipSign            ;yes, go on
          cmp       bptr [si],'-'       ;start with a minus sign?
          jne       ReadInit            ;no, go scan for digits
          inc       bl                  ;set sign flag
          ;
SkipSign: inc       si                  ;skip the sign
          ;
ReadInit: push      bx                  ;save sign flag
          xor       bx,bx               ;clear accumulating result
          xor       dx,dx               ;clear high-order part
          ;
ReadLoop: lodsb                         ;get input character
          xor       ah,ah               ;clear high byte
          sub       al,'0'              ;convert to binary
          jb        ReadDone            ;if < '0' we're done
          cmp       al,9                ;too big?
          ja        ReadDone            ;if so quit
          dshl      bx,dx               ;multiply - now DX:BX = (old*2)
          mov       di,dx               ;copy high-order
          add       ax,bx               ;add (old*2) low order + new digit
          adc       di,0                ;add any carry - DI:AX = (old*2)+new
          dshln     bx,dx,2             ;multiply by 4 more - DX:BX = old*8
          add       bx,ax               ;add (old*2+new) + (old*8) low-order
          adc       dx,di               ;add high-order and carry - now
                                        ;  DX:BX=(old*10)+new
          dec       cx                  ;count characters
          jg        ReadLoop            ;back for more if not past limit
          inc       si                  ;disable SI adjust if we hit limit
          ;
ReadDone: dec       si                  ;back up to last digit
          mov       ax,bx               ;copy low-order result
          pop       bx                  ;get back sign flag
          or        bl,bl               ;check sign
          je        ReadExit            ;if positive go on
                                        ;negative -- twos complement DX:AX
          xor       ax,0FFFFh           ;complement low-order
          xor       dx,0FFFFh           ;complement high-order
          add       ax,1                ;increment low-order
          adc       dx,0                ;and propagate any carry
          ;
ReadExit: pop       di                  ;restore DI
          ;
          exit                          ;all done

          ;
          ; ReadHex - Read a hex integer (upper case only) from format spec
          ;           or source
          ;
          ; On entry:
          ;         CX = maximum number of characters to process
          ;         DS:SI = pointer to string to read
          ;
          ; On exit:
          ;         DX:AX = integer found
          ;         BX destroyed
          ;         CX = entry value less number of characters processed
          ;         DS:SI = updated pointer
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     ReadHex,noframe,,local

          xor       bx,bx               ;clear low accumulating result
          xor       dx,dx               ;clear high accumulating result
          xor       ax,ax               ;clear high byte for new digits
          ;
RHLoop:   lodsb                         ;get input character
          sub       al,'0'              ;convert to binary
          jb        RHDone              ;if < '0' we're done
          cmp       al,9                ;too big for a digit?
          jbe       RHAdd               ;if not we have 0 - 9
          sub       al,('A' - '0')      ;OK, check upper case alpha
          jb        RHDone              ;if too low we're done
          add       al,10               ;adjust up to 10 - 15
          cmp       al,0Fh              ;check high end
          ja        RHDone              ;if in range go add it in
          ;
RHAdd:    dshln     bx,dx,4             ;DX:BX = DX:BX * 16
          add       bx,ax               ;add new digit, DX:BX = (DX:BX * 16) + AX
          loop	RHLoop              ;back for more if not past limit
          inc       si                  ;disable SI adjust if we hit limit
          ;
RHDone:   dec       si                  ;back up to last digit
          mov       ax,bx               ;copy low-order result
          exit                          ;all done

          ;
          ;
          ; GetArg - Get the next output argument
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX = argument from stack
          ;         BX destroyed
          ;         ArgList pointer updated
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     GetArg,noframe,,local  ;start GetArg
          ;
          mov       bx,ArgList          ;get next argument pointer
          mov       ax,ss:[bx]          ;get argument
          add       wptr ArgList,2      ;and move pointer
          ;
          exit                          ;all done
          ;
          ;
          ; GetInt - Get the next integer output argument
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX = argument from stack if LongFlag = 0
          ;         DX:AX = argument from stack if LongFlag = 1
          ;         BX destroyed
          ;         NextArg updated
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     GetInt,noframe,,local  ;start GetInt
          ;
          call      GetArg              ;get low-order
          cmp       bptr LongFlag,0     ;is it a long?
          je        GIDone              ;if not go on
          mov       dx,ax               ;save low-order
          call      GetArg              ;get high-order
          xchg      dx,ax               ;set up as dx:ax
          ;
GIDone:   exit                          ;all done
          ;
          ;
          ; GetPtr - Get the next input pointer argument
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         ES:DI = pointer from stack
          ;         InArgs pointer updated
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     GetPtr,noframe,,local  ;start GetPtr
          ;
          cmp       bptr Ignore,0       ;ignoring this argument?
          jnz       GPQuit              ;if so don't get pointer
          push      bx                  ;save BX
          loadseg   es,ss               ;default to data segment (ASSUMES
                                        ;   small model)
          mov       bx,InArgs           ;get next argument pointer
          mov       di,ss:[bx]          ;get argument offset
          add       bx,2                ;and move pointer
          cmp       bptr InFar,0        ;is it far?
          jz        GPDone              ;if not we're done
          mov       es,ss:[bx]          ;get argument segment
          add       bx,2                ;and move pointer
GPDone:   mov       InArgs,bx           ;put back new pointer
          pop       bx                  ;restore BX
          ;
          if        (_DOS ne 0) and (_WIN eq 0)  ;in DOS ...
          call      HPAdj               ;... check for huge pointer problems
          endif
          ;
GPQuit:   exit                          ;all done
          ;
          ;
          ; FmtFlush - Flush text from format string to output buffer
          ;
          ; On entry:
          ;         CX = offset of start of current text to be flushed
          ;         DS:SI = current format string pointer, points *two*
          ;                   bytes beyond end of text to output
          ;
          ; On exit:
          ;         BX, CX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     FmtFlush,noframe,,local  ;start FmtFlush
          ;
          push      si                  ;save SI
          xchg      cx,si               ;point back to first character, get
                                        ;  end address
          sub       cx,si               ;get count + 1
          dec       cx                  ;get count
          jcxz      FFDone              ;if nothing to output, skip it
          mov       bx,offset OutCopy   ;point to output code
          call      DoOutput            ;dump text from format string to
                                        ;  output area
          ;
FFDone:   pop       si                  ;restore SI
          ;
          exit                          ;all done
;          ;
;          ;
;          ; StrSetup - Set up for string output
;          ;
;          ; On entry:
;          ;         DS = default data segment
;          ;         String argument detected
;          ;
;          ; On exit:
;          ;         AX, BX destroyed
;          ;         DS:SI = pointer to start of output string
;          ;         All other registers and interrupt state unchanged
;          ;
;          ;
;          entry     StrSetup,noframe,,local  ;start StrSetup
;          ;
;          call      GetArg              ;get next argument
;          mov       si,ax               ;copy as string address
;          cmp       bptr FarFlag,0      ;far string?
;          je        SSDone              ;if not go on
;          call      GetArg              ;get next argument
;          mov       ds,ax               ;save as segment
;          ;
;SSDone:   exit                          ;all done
          ;
          ;
          ; GetSLen - Get string length for output
          ;
          ; On entry:
          ;         CX = Maximum length to scan
          ;         DS:SI = pointer to start of string
          ;
          ; On exit:
          ;         AX destroyed
          ;         CX = length of string for output
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     GetSLen,noframe,,local  ;start GetSLen
          ;
          pushm     di,es               ;save registers used
          mov       di,si               ;get input address in di
          loadseg   es,ds               ;set segment for scan
          xor       al,al               ;get zero
          cld                           ;go forward
          repne     scasb               ;find null terminator
          jne       GLCount             ;if not found go on
          dec       di                  ;adjust for extra character scanned
          ;
GLCount:  mov       cx,di               ;copy end of scan
          sub       cx,si               ;get number of characters scanned
          popm      es,di               ;restore registers used
          ;
          exit                          ;all done
          ;
          ;
          ; DoFill - Fill the output string with the filler character
          ;          (called via DoOutput)
          ;
          ; On entry:
          ;         CX = number of characters to fill up to, may be negative
          ;           in which case no fill is done
          ;         DX = width of actual output string
          ;         ES:DI = output address
          ;
          ; On exit:
          ;         AX, CX destroyed
          ;         ES:DI = next character after fill
          ;         All other registers and interrupt state unchanged
          ;
          entry     DoFill,noframe,,local  ;start DoFill
          ;
          sub       cx,dx               ;get space to fill
          jle       FillDone            ;if nothing to fill go on
          mov       al,Fill             ;get fill character
          mov       bx,offset OutFill   ;get fill instruction address
          call      DoOutput            ;go do output in chunks
          ;
FillDone: exit                          ;all done
          ;
          ; Instruction called by DoOutput to store the fill character
          ;
OutFill:  rep       stosb               ;store fill characters
          ret                           ;and go back to DoOutput
          ;
          ;
          ; DoOutput - Copy some output to the output buffer, with length
          ;            limitation and file output if requested.
          ;
          ; On entry:
          ;         BX = address of actual output routine
          ;         CX = number of characters to output
          ;         ES:DI = output address
          ;
          ; On exit:
          ;         BX, CX destroyed
          ;         ES:DI = next character after output
          ;         All other registers and interrupt state unchanged except
          ;           as modified by the output routine
          ;
          ; NOTE:  No frame can be defined here -- this routine references
          ; the argument and variable frames used by _fmtout.
          ;
          entry     DoOutput,noframe,,local  ;start DoOutput
          ;
          mov       OutSub,bx           ;save output routine
          cmp       wptr Handle,-1      ;output to file?
          je        DOGo                ;if not just go dump it to buffer
          ;
DOLoop:   lea       bx,OBEnd            ;get buffer end + 1
          sub       bx,di               ;calculate remaining room
          cmp       cx,bx               ;will this stuff fit?
          jl        DOGo                ;if it fits go on
          sub       cx,bx               ;adjust CX for next time (may be 0 if
                                        ;  output exactly fills buffer)
          push      cx                  ;save it
          mov       cx,bx               ;only output what fits
          call      wptr [OutSub]       ;do the output
          mov       cx,OUTSIZE          ;flush the full buffer
          pushm     ax,dx               ;save AX, DX (WriteBuf destroys)
          call      WriteBuf            ;write the buffer
          popm      dx,ax               ;restore AX, DX
          lea       di,OutBuf           ;start at beginning of buffer
          pop       cx                  ;get back remainder
          jcxz      DODone              ;if nothing left get out
          jmp       short DOLoop        ;go try again
          ;
DOGo:     call      wptr [OutSub]       ;do the output
          ;
DODone:   exit                          ;all done
          ;
          ;
          ; WriteBuf - Write the output buffer
          ;
          ; On entry:
          ;         CX = number of characters to write
          ;
          ; On exit:
          ;         AX, BX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          entry     WriteBuf,noframe,,local  ;start WriteBuf

          pushm     ds,es               ;save segment regs

          push      cx                  ;save count
          loadseg   ds,ss               ;set data segment
          lea       ax,OutBuf           ;get output buffer address
          push      ax                  ;store buffer address
          push      Handle              ;store output handle
          call      Write               ;call the C output routine
          add       sp,6                ;fix up the stack

          cmp       ax,-1		;check for error return
          jne       short NoWError
          call      CheckErr		;handle batch file ON ERROR
NoWError:

          popm      es,ds               ;restore segment regs
          add       BytesWritten,ax
          ;
          exit                          ;all done
          ;
          ;
          ; SkSpace - Skip spaces in input string
          ;
          ; On entry:
          ;         DS:SI = string address
          ;
          ; On exit:
          ;         AX destroyed
          ;         DS:SI = address of first non-space character
          ;         All other registers and interrupt state unchanged
          ;
          entry     SkSpace,noframe,,local  ;start SkSpace
          ;
SSLoop:   lodsb                         ;get next byte
          call      IsSpace             ;is it a space?
          je        SSLoop              ;if so keep looking
          dec       si                  ;back up to first non-space char
          exit                          ;all done

          ;
          ; IsSpace - See if character is a space
          ;
          ; On entry:
          ;         AL = character to test
          ;
          ; On exit:
          ;         AH destroyed
          ;         Zero flag set if AL was a space
          ;         All other registers and interrupt state unchanged
          ;
          entry     IsSpace,noframe,,local  ;start IsSpace
          ;
          cmp       al,' '              ;space?
          je        SpDone              ;if so we're done
          cmp       al,9                ;other white space
          jb        SpDone              ;if not, quit
          cmp       al,13               ;check high end of range
          ja        SpDone              ;if not in white space range exit
          xor       ah,ah               ;it was white space, set ZF
          ;
SpDone:   exit                          ;all done
          ;
          ;
          ; TabScan - Scan a character table
          ;
          ; On entry:
          ;         AL = character to search for
          ;         BX = table offset
          ;
          ; On exit:
          ;         BX = offset of character within table * 2
          ;         CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     TabScan,noframe,,local  ;start TabScan
          ;
          pushm     di,es               ;save registers
          mov       di,bx               ;point to table for scan
          loadseg   es,cs               ;set scan segment
          mov       cl,es:[di]          ;get length
          xor       ch,ch               ;clear high byte
          inc       di                  ;skip length byte
          cld                           ;go forward
          repne     scasb               ;see if character is in table
          jne       NotFound            ;if not in table complain
          xchg      bx,di               ;swap addresses
          sub       bx,di               ;get offset
          sub       bx,2                ;adjust for length byte and overscan
          shl       bx,1                ;make word offset
          clc                           ;all OK
          jmp       short TabRet        ;and return
          ;
NotFound: stc                           ;not found, complain
          ;
TabRet:   popm      es,di               ;restore registers
          ;
          exit                          ;all done
          ;
          ;
          ; PtrSwap - Swap source and format pointers for _fmtin
          ;
          ; On entry:
          ;         DS:SI = one pointer
          ;         ES:DI = other pointer
          ;
          ; On exit:
          ;         DS:SI = old ES:DI
          ;         ES:DI = old DS:SI
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     PtrSwap,noframe,,local  ;start PtrSwap
          ;
          xchg      si,di               ;swap offsets
          pushm     ds,es               ;save segments
          popm      ds,es               ;restore in reverse order
          ;
          exit                          ;all done
          ;
          ;
          ; HPAdj - Adjust huge pointer in DOS (assumes 32K max range)
          ;
          ; On entry:
          ;         ES:DI = string pointer
          ;
          ; On exit:
          ;         ES:DI adjusted for possible wraparound
          ;         All other registers and interrupt state unchanged
          ;
          ;
          if        (_DOS ne 0) and (_WIN eq 0)
          entry     HPAdj,noframe,,local  ;start HPAdj
          ;
          cmp       di,8000h            ;address over 32K?
          jb        HPDone              ;if not go on
          sub       di,8000h            ;if so reduce offset
          push      ax                  ;save AX
          mov       ax,es               ;get segment
          add       ax,800h             ;adjust it appropriately
          mov       es,ax               ;put it back
          pop       ax                  ;restore AX
          ;
HPDone:   exit                          ;all done

; Windows 3.1 code for Tom to convert to assembly!
; 
; First we have to allocate a new selector, then set our current ES:DI to
;  it, then set a max limit of 64K for the new selector.  When we're
;  finished in INOUT.ASM, we'll need to free the selector
;
; ULONG dwBase;
; UINT uSelector;
;
; uSelector = AllocSelector(0);
; dwBase = es:di;
; SetSelectorBase(uSelector, dwBase);		// these two functions are
; SetSelectorLimit(uSelector, 0xFFFFL);		// "semi-documented"
; ..... 
; When we're leaving INOUT, free the selector so we don't have resource leaks
; FreeSelector(uSelector);
 
          endif
          ;
          ;
          ; ULASCII - Convert unsigned long integer to ASCII
          ;
          ; Algorithm from MSC 5.1 runtime library, slightly revised
          ;
          ; On entry:
          ;         DX:AX = value to convert
          ;         CX = radix
          ;         ES:DI = address of END of buffer
          ;         SI = 0 for no separators, or separator character
          ;           
          ;
          ; On exit:
          ;         AX = number of digits converted
          ;         BX, CX, DX destroyed
          ;         Result stored in buffer, null-terminated, right-justified
          ;         DI = address of last digit stored - 1
          ;         Direction flag set
          ;         All other registers and interrupt state unchanged
          ;
          ;
          entry     ulascii,noframe,,local  ;start ulascii
          push      bp                  ;save BP
          std                           ;storage goes backwards
          push      di                  ;save start address
          mov       bx,ax               ;copy low order
          xor       al,al               ;get terminating null
          stosb                         ;store it
          mov       bp,4                ;initialize counter for separators
          ;
DivLoop:  mov       ax,dx               ;copy high order to low
          xor       dx,dx               ;clear high
          or        ax,ax               ;any dividend?
          jz        SkipHigh            ;if not skip it
          div       cx                  ;now dx = high remainder, ax = high
                                        ;  quotient
          ;
SkipHigh: xchg      ax,bx               ;bx = high quotient, ax = low
          div       cx                  ;divide high rem:low to get dx =
                                        ;  total remainder, ax = low quotient
          xchg      ax,dx               ;ax = tot remainder, dx = low quot
          xchg      dx,bx               ;bx = high quotient, dx = low
                                        ;  quotient --> dx:bx = new dividend
          add       al,'0'              ;convert remainder to digit
          cmp       al,'9'              ;within 0 - 9?
          jbe       ChkSep              ;yes, go on
          add       al,'A'-'0'-10       ;no, convert to alpha
          ;
ChkSep:   or        si,si               ;check for separators
          jz        SaveDig             ;if none go on
          dec       bp                  ;count digits
          jnz       SaveDig             ;if not there yet go on
          mov       bp,3                ;reset counter for next time
          xchg      ax,si               ;get separator
          stosb                         ;dump into string
          xchg      ax,si               ;get digit back
          ;
SaveDig:  stosb                         ;store the digit
          mov       ax,dx               ;get high dividend
          or        ax,bx               ;check if all zero
          jnz       DivLoop             ;if not go get next digit
          ;
          pop       ax                  ;get back original address
          sub       ax,di               ;get character count
          ;
          pop       bp                  ;restore BP
          exit                          ;all done
          ;
          if        _DOS
ASM_TEXT  ends
          else
_TEXT     ends
          endif
          ;
          end

