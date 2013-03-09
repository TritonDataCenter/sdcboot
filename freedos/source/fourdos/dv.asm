

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


          title     DV - Provide DESQView interface
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1991, JP Software Inc., All Rights Reserved

          Author:  Tom Rawson  10/5/91


          These routines provide an interface to allow 4DOS to clean up
          its act if a DESQView close window command occurs.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     SERV                ;set server segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   dvapi.inc           ;DESQView API
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:@curseg, es:nothing, ss:nothing
          ;
          ; Near externals
          ;
          extrn     SrvClean:near
          ;
          ;
          ; Local data
          ;
WHandle   dd        ?                   ;our window handle
MBHandle  dd        ?                   ;our mailbox handle
          ;
          ; Manager stream messages
          ;
SetNotfy  db        1Bh, 10h            ;message to set close window notify
          dw        1
          db        46h
          ;
ClrNotfy  db        1Bh, 10h            ;message to clear close window notify
          dw        1
          db        66h
          ;
SetAsync  db        1Bh, 10h            ;message to set async notification
          dw        5
          db        8ah
          dw        offset Closing
ASSeg     dw        0                   ;segment, filled in by DVSET
          ;
ClrAsync  db        1Bh, 10h            ;message to clear async notification
          dw        5
          db        8ah
          dd        0
          ;
          ;
          ; DVSet - Set up DV notification
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          .public   DVSet               ;create public name, with prefix
          .proc     DVSet               ;set up procedure
          ;
          pushm     si,ds               ;save registers
          loadseg   ds,cs               ;set DS
          ;
          mov       bx,200h             ;level 2.00
          @CALL     APILEVEL            ;set the API level
          @SEND     HANDLE,ME           ;get our window handle
          @POP      WHandle             ;save it
          @SEND     HANDLE,MAILME       ;get our mailbox handle
          @POP      MBHandle            ;save it
          mov       si,offset SetNotfy  ;get set notify msg
          call      DVSend              ;send that message
          mov       ASSeg,cs            ;set segment for notify
          mov       si,offset SetAsync  ;get set async notification msg
          call      DVSend              ;send it too
          ;
          popm      ds,si               ;restore registers
          ret                           ;and return
          ;
          .endp     DVSet               ;all done
          ;
          ;
          ; DVClear - Clear DV notification
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         AX, BX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          .public   DVClear             ;create public name, with prefix
          .proc     DVClear             ;set up procedure
          ;
          pushm     si,ds               ;save registers
          loadseg   ds,cs               ;set DS
          ;
          mov       si,offset ClrAsync  ;get clear async notification msg
          call      DVSend              ;send it
          mov       si,offset ClrNotfy  ;get clear notify msg
          call      DVSend              ;send it too
          ;
          popm      ds,si               ;restore registers
          ret                           ;and return
          ;
          .endp     DVClear             ;all done
          ;
          ;
          ; DVSend - Send a Manager Stream message to DESQView
          ;
          ; On entry:
          ;         SI = Message address, length in third and fourth bytes
          ;              of message
          ;
          ; On exit:
          ;         AX, BX, CX, DX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          .proc     DVSend              ;set up procedure
          ;
          @PUSH     DSSI                ;push message address
          mov       cx,2[si]            ;get length of command
          add       cx,4                ;add overhead to get total
          xor       dx,dx               ;clear high byte
          @PUSH     DXCX                ;push length
          @SEND     WRITE,ME            ;write the message to our window
          ret                           ;and return
          ;
          .endp     DVSend              ;all done
          ;
          ;
          ; Closing - Routine called when window is being closed
          ;
          ; On entry:
          ;         Set up by DV
          ;
          ; On exit:
          ;         If called and it isn't really a close window (which
          ;         shouldn't happen) just returns with SS and SP unchanged.
          ;         Otherwise, terminates task.
          ;
          .proc     Closing             ;set up procedure
          ;
          loadseg   ds,cs               ;set DS to local
          @SEND     SIZEOF,MAILME       ;see if we have a message
          @POP      DXCX                ;get number of messages in queue
          or        dx,cx               ;anything there?
           jz       ClosDone            ;if not forget it
          ;
ClosRead: push      cx                  ;save CX
          @SEND     READ,MAILME         ;read the mailbox
          @POP      BXAX                ;get length in bytes
          @POP      ESDI                ;get message address
          cmp       bptr es:[di],46h    ;is it a close window notification?
           jne      ClosNext            ;if not go on
          mov       si,offset WHandle   ;check if it's our window handle
          mov       cx,2                ;compare 2 words
          cld                           ;go forward
          inc       di                  ;skip to window number
          repe      cmpsw               ;same window handle?
           je       CloseGo             ;if so go close up
          ;
ClosNext: pop       cx                  ;get back message count
          loop      ClosRead            ;go read next message
          jmp       short ClosDone      ;if no more get out
          ;
CloseGo:  call      SrvClean            ;clean up swapping and memory alloc
          @SEND     FREE,ME             ;close this task down (never returns)
          ;
ClosDone: retf                          ;return to DV window manager
          ;
          .endp     Closing             ;all done
          ;
@curseg   ends                          ;close text segment
          ;
          end

