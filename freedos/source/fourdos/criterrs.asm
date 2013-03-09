

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
          ;  Critical error text to be INCLUDEd in 4DLINIT.ASM
          ;
          ;  Copyright 1989 - 1992, JP Software Inc., All Rights Reserved
          ;  Author:  Tom Rawson  04/14/89
          ;
          ; This module includes only an error message table.  Routines for
          ; accessing this table are in ERRORMSG.ASM.
          ;
          ; ****************************************************************
          ; NOTE:  This text has been compressed and copied from
          ; CRITERRS.TXT.  Any changes should be made there, not here!
          ; ****************************************************************
          ;
          public    CritErrs
          ;
          ;
          ; NOTE:  This text is loaded initially in 4DLINIT.ASM.  It is
          ; then moved by 4DLINIT code into the resident portion of 4DOS
          ; if necessary.  This move invalidates the "dw  offset CritBad"
          ; below.  This is fixed by adjusting this value inside the
          ; subroutine LoadMod in 4DLINIT.  Be aware of this if you change
          ; the structure of the data below!
          ;
          ;
CritErrs  label     byte                ;beginning of message table
          dw        offset CritBad      ;store offset of bad error number msg
          ;
          ; Translation table
          ;
          db        0DEh
          db        065h, 020h, 06Fh, 072h, 074h, 06Eh, 061h, 069h
          db        064h, 06Ch, 063h, 073h, 06Dh, 075h, 070h, 066h
          db        076h, 04Eh, 079h, 049h, 044h, 04Fh, 042h, 053h
          db        06Bh, 02Dh, 043h, 052h, 062h, 04Dh
          ;
          ; Basic errors
          ;
          ; #1 Bad function
          db        1, 00Eh, 000h, 018h, 08Ah, 031h, 01Fh, 07Ch, 069h
          db        047h
          ; #2 File not found
          db        2, 011h, 000h, 006h, 049h, 0B2h, 037h, 046h, 031h
          db        014h, 0F7h, 0A0h
          ; #3 Invalid path
          db        3, 011h, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 010h
          db        086h, 008h, 060h
          ; #4 Too many open files
          db        4, 018h, 000h, 004h, 054h, 043h, 0E8h, 071h, 043h
          db        041h, 002h, 073h, 011h, 09Bh, 02Dh
          ; #5 Access denied
          db        5, 00Fh, 000h, 001h, 04Ch, 0C2h, 0DDh, 03Ah, 027h
          db        092h, 0A0h
          ; #6 Invalid handle
          db        6, 012h, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 008h
          db        068h, 07Ah, 0B2h
          ; #7 Memory destroyed
          db        7, 013h, 000h, 01Fh, 02Eh, 045h, 014h, 03Ah, 02Dh
          db        065h, 041h, 042h, 0A0h
          ; #8 Out of memory
          db        8, 010h, 000h, 017h, 0F6h, 034h, 011h, 03Eh, 02Eh
          db        045h, 014h
          ; #9 Bad memory block
          db        9, 014h, 000h, 018h, 08Ah, 03Eh, 02Eh, 045h, 014h
          db        031h, 0EBh, 04Ch, 01Ah
          ; #10 Bad environment
          db        10, 011h, 000h, 018h, 08Ah, 032h, 071h, 029h, 054h
          db        07Eh, 027h, 060h
          ; #11 Bad format
          db        11, 00Ch, 000h, 018h, 08Ah, 031h, 014h, 05Eh, 086h
          ; #12 Invalid access code
          db        12, 015h, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 08Ch
          db        0C2h, 0DDh, 03Ch, 04Ah, 020h
          ; #13 Invalid data
          db        13, 00Eh, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 0A8h
          db        068h
          ; #14 Internal DOS error
          db        14, 016h, 000h, 015h, 076h, 025h, 078h, 0B3h, 016h
          db        017h, 019h, 032h, 055h, 045h
          ; #15 Invalid drive
          db        15, 010h, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 0A5h
          db        091h, 022h
          ; #16 Can't remove current directory
          db        16, 023h, 000h, 01Ch, 087h, 007h, 026h, 035h, 02Eh
          db        041h, 022h, 03Ch, 0F5h, 052h, 076h, 03Ah, 095h
          db        02Ch, 064h, 051h, 040h
          ; #17 Not same device
          db        17, 011h, 000h, 013h, 046h, 03Dh, 08Eh, 023h, 0A2h
          db        012h, 09Ch, 020h
          ; #18 File not found
          db        18, 011h, 000h, 006h, 049h, 0B2h, 037h, 046h, 031h
          db        014h, 0F7h, 0A0h
          ;
          ; DOS INT 24 (critical error) messages
          ;
          ; #19 Disk is write protected
          db        19, 01Ch, 000h, 016h, 09Dh, 01Ah, 039h, 0D3h, 007h
          db        075h, 096h, 023h, 010h, 054h, 062h, 0C6h, 02Ah
          ; #20 Bad disk unit
          db        20, 00Fh, 000h, 018h, 08Ah, 03Ah, 09Dh, 01Ah, 03Fh
          db        079h, 060h
          ; #21 Drive not ready--close door
          db        21, 020h, 000h, 016h, 059h, 012h, 023h, 074h, 063h
          db        052h, 08Ah, 014h, 01Bh, 01Bh, 0CBh, 04Dh, 023h
          db        0A4h, 045h
          ; #22 Bad disk command
          db        22, 012h, 000h, 018h, 08Ah, 03Ah, 09Dh, 01Ah, 03Ch
          db        04Eh, 0E8h, 07Ah
          ; #23 Data error
          db        23, 00Bh, 000h, 016h, 086h, 083h, 025h, 054h, 050h
          ; #24 Bad call format
          db        24, 011h, 000h, 018h, 08Ah, 03Ch, 08Bh, 0B3h, 011h
          db        045h, 0E8h, 060h
          ; #25 Seek error
          db        25, 00Ch, 000h, 019h, 022h, 01Ah, 032h, 055h, 045h
          ; #26 Non-DOS disk
          db        26, 012h, 000h, 013h, 047h, 01Bh, 016h, 017h, 019h
          db        03Ah, 09Dh, 01Ah
          ; #27 Sector not found
          db        27, 012h, 000h, 019h, 02Ch, 064h, 053h, 074h, 063h
          db        011h, 04Fh, 07Ah
          ; #28 Out of paper
          db        28, 010h, 000h, 017h, 0F6h, 034h, 011h, 031h, 008h
          db        010h, 025h
          ; #29 Write error
          db        29, 00Dh, 000h, 007h, 055h, 096h, 023h, 025h, 054h
          db        050h
          ; #30 Read error
          db        30, 00Bh, 000h, 01Dh, 028h, 0A3h, 025h, 054h, 050h
          ; #31 General failure
          db        31, 012h, 000h, 007h, 042h, 072h, 058h, 0B3h, 011h
          db        089h, 0BFh, 052h
          ;
          ; DOS 3 extended errors (mostly network errors)
          ;   used for error messages generated by 4DOS INT 24 handler
          ;
          ; #32 Sharing violation
          db        32, 017h, 000h, 019h, 008h, 068h, 059h, 070h, 076h
          db        031h, 029h, 04Bh, 086h, 094h, 070h
          ; #33 Lock violation
          db        33, 012h, 000h, 00Ch, 044h, 0C1h, 0A3h, 012h, 094h
          db        0B8h, 069h, 047h
          ; #34 Invalid disk change
          db        34, 01Ah, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 0A9h
          db        0D1h, 0A3h, 0C0h, 086h, 087h, 007h, 062h
          ; #35 FCB unavailable
          db        35, 015h, 000h, 006h, 041h, 0C1h, 083h, 0F7h, 081h
          db        028h, 09Bh, 081h, 0EBh, 020h
          ; #36 Sharing buffer overflow
          db        36, 023h, 000h, 019h, 008h, 068h, 059h, 070h, 076h
          db        031h, 0EFh, 011h, 011h, 025h, 034h, 012h, 025h
          db        011h, 0B4h, 007h, 070h
          ; #40 Not ready
          db        40, 00Bh, 000h, 013h, 046h, 035h, 028h, 0A1h, 040h
          ; #41 File allocation table bad
          db        41, 01Dh, 000h, 006h, 049h, 0B2h, 038h, 0BBh, 04Ch
          db        086h, 094h, 073h, 068h, 01Eh, 0B2h, 031h, 0E8h
          db        0A0h
          ; #50 Invalid net request
          db        50, 017h, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 072h
          db        063h, 052h, 001h, 07Fh, 02Dh, 060h
          ; #51 Remote computer not listening
          db        51, 021h, 000h, 01Dh, 02Eh, 046h, 023h, 0C4h, 0E1h
          db        00Fh, 062h, 053h, 074h, 063h, 0B9h, 0D6h, 027h
          db        097h, 007h, 060h
          ; #52 Duplicate name on net
          db        52, 017h, 000h, 016h, 0F1h, 00Bh, 09Ch, 086h, 023h
          db        078h, 0E2h, 034h, 073h, 072h, 060h
          ; #53 Net name not found
          db        53, 014h, 000h, 013h, 026h, 037h, 08Eh, 023h, 074h
          db        063h, 011h, 04Fh, 07Ah
          ; #54 Net busy
          db        54, 00Bh, 000h, 013h, 026h, 031h, 0EFh, 0D1h, 040h
          ; #55 Net device no longer exists
          db        55, 021h, 000h, 013h, 026h, 03Ah, 021h, 029h, 0C2h
          db        037h, 043h, 0B4h, 070h, 076h, 025h, 032h, 008h
          db        079h, 0D6h, 0D0h
          ; #56 NetBIOS command limit exceeded
          db        56, 025h, 000h, 013h, 026h, 018h, 015h, 017h, 019h
          db        03Ch, 04Eh, 0E8h, 07Ah, 03Bh, 09Eh, 096h, 032h
          db        008h, 07Ch, 022h, 0A2h, 0A0h
          ; #57 Net adapter hardware error
          db        57, 020h, 000h, 013h, 026h, 038h, 0A8h, 010h, 062h
          db        053h, 008h, 068h, 05Ah, 007h, 078h, 052h, 032h
          db        055h, 045h
          ; #58 Bad response from net
          db        58, 018h, 000h, 018h, 08Ah, 035h, 02Dh, 010h, 047h
          db        0D2h, 031h, 015h, 04Eh, 037h, 026h
          ; #59 Unexpected net error
          db        59, 019h, 000h, 005h, 057h, 020h, 087h, 010h, 02Ch
          db        062h, 0A3h, 072h, 063h, 025h, 054h, 050h
          ; #60 Incompatible remote adapter
          db        60, 01Fh, 000h, 015h, 07Ch, 04Eh, 010h, 086h, 091h
          db        0EBh, 023h, 052h, 0E4h, 062h, 038h, 0A8h, 010h
          db        062h, 050h
          ; #61 Print queue full
          db        61, 015h, 000h, 000h, 055h, 097h, 063h, 001h, 07Fh
          db        02Fh, 023h, 011h, 0FBh, 0B0h
          ; #62 Queue not full
          db        62, 011h, 000h, 001h, 05Fh, 02Fh, 023h, 074h, 063h
          db        011h, 0FBh, 0B0h
          ; #63 No room for print file
          db        63, 01Ah, 000h, 013h, 043h, 054h, 04Eh, 031h, 014h
          db        053h, 010h, 059h, 076h, 031h, 019h, 0B2h
          ; #64 Net name was deleted
          db        64, 017h, 000h, 013h, 026h, 037h, 08Eh, 023h, 007h
          db        078h, 0D3h, 0A2h, 0B2h, 062h, 0A0h
          ; #65 Access denied
          db        65, 00Fh, 000h, 001h, 04Ch, 0C2h, 0DDh, 03Ah, 027h
          db        092h, 0A0h
          ; #66 Net device type incorrect
          db        66, 01Dh, 000h, 013h, 026h, 03Ah, 021h, 029h, 0C2h
          db        036h, 014h, 010h, 023h, 097h, 0C4h, 055h, 02Ch
          db        060h
          ; #67 Net name not found
          db        67, 014h, 000h, 013h, 026h, 037h, 08Eh, 023h, 074h
          db        063h, 011h, 04Fh, 07Ah
          ; #68 Net name limit exceeded
          db        68, 01Ah, 000h, 013h, 026h, 037h, 08Eh, 023h, 0B9h
          db        0E9h, 063h, 020h, 087h, 0C2h, 02Ah, 02Ah
          ; #69 NetBIOS session limit exceeded
          db        69, 025h, 000h, 013h, 026h, 018h, 015h, 017h, 019h
          db        03Dh, 02Dh, 0D9h, 047h, 03Bh, 09Eh, 096h, 032h
          db        008h, 07Ch, 022h, 0A2h, 0A0h
          ; #70 Temporarily paused
          db        70, 017h, 000h, 004h, 052h, 0E1h, 004h, 058h, 059h
          db        0B1h, 043h, 010h, 08Fh, 0D2h, 0A0h
          ; #71 Net request not accepted
          db        71, 01Ch, 000h, 013h, 026h, 035h, 020h, 017h, 0F2h
          db        0D6h, 037h, 046h, 038h, 0CCh, 021h, 006h, 02Ah
          ; #72 Redirection is paused
          db        72, 017h, 000h, 01Dh, 02Ah, 095h, 02Ch, 069h, 047h
          db        039h, 0D3h, 010h, 08Fh, 0D2h, 0A0h
          ; #80 File exists
          db        80, 00Fh, 000h, 006h, 049h, 0B2h, 032h, 008h, 079h
          db        0D6h, 0D0h
          ; #82 Can't make directory entry
          db        82, 020h, 000h, 01Ch, 087h, 007h, 026h, 03Eh, 081h
          db        0A2h, 03Ah, 095h, 02Ch, 064h, 051h, 043h, 027h
          db        065h, 014h
          ; #83 Fail on INT 24
          db        83, 018h, 000h, 006h, 048h, 09Bh, 034h, 073h, 015h
          db        013h, 004h, 053h, 002h, 030h, 043h
          ; #84 Too many redirections
          db        84, 018h, 000h, 004h, 054h, 043h, 0E8h, 071h, 043h
          db        052h, 0A9h, 052h, 0C6h, 094h, 07Dh
          ; #85 Duplicate redirection
          db        85, 017h, 000h, 016h, 0F1h, 00Bh, 09Ch, 086h, 023h
          db        052h, 0A9h, 052h, 0C6h, 094h, 070h
          ; #86 Invalid password
          db        86, 015h, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 010h
          db        08Dh, 0D0h, 077h, 045h, 0A0h
          ; #87 Invalid parameter
          db        87, 014h, 000h, 015h, 071h, 028h, 0B9h, 0A3h, 010h
          db        085h, 08Eh, 026h, 025h
          ; #88 Net device fault
          db        88, 013h, 000h, 013h, 026h, 03Ah, 021h, 029h, 0C2h
          db        031h, 018h, 0FBh, 060h
          ; #100 CD-ROM unknown error
          db        100, 01Dh, 000h, 01Ch, 016h, 01Bh, 01Dh, 017h, 01Fh
          db        03Fh, 071h, 0A7h, 040h, 077h, 073h, 025h, 054h
          db        050h
          ; #101 CD-ROM not ready
          db        101, 017h, 000h, 01Ch, 016h, 01Bh, 01Dh, 017h, 01Fh
          db        037h, 046h, 035h, 028h, 0A1h, 040h
          ; #102 CD-ROM EMS memory bad
          db        102, 021h, 000h, 01Ch, 016h, 01Bh, 01Dh, 017h, 01Fh
          db        030h, 054h, 01Fh, 019h, 03Eh, 02Eh, 045h, 014h
          db        031h, 0E8h, 0A0h
          ; #103 CD-ROM not High Sierra or ISO-9660
          db        103, 03Bh, 000h, 01Ch, 016h, 01Bh, 01Dh, 017h, 01Fh
          db        037h, 046h, 030h, 084h, 090h, 076h, 008h, 063h
          db        019h, 092h, 055h, 083h, 045h, 031h, 051h, 091h
          db        071h, 0B0h, 093h, 006h, 030h, 063h, 000h, 030h
          ; #104 CD-ROM door open
          db        104, 017h, 000h, 01Ch, 016h, 01Bh, 01Dh, 017h, 01Fh
          db        03Ah, 044h, 053h, 041h, 002h, 070h
          ;
          db        0FFh, 0             ;end of table
          ;
CritBad   db        BErrLen, "Undefined error"   ;message if not found
BErrLen   equ       $ - CritBad - 1

