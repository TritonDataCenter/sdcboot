

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


          title     SERVERRS - 4DOS high-memory server error messages
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  04/14/89

          This module includes only an error message table.  Routines for
          accessing this table are in ERRORMSG.ASM.

          ******************************************************************
          NOTE:  This text has been compressed and copied from SERVERRS.TXT.
          Any changes should be made there, not here!
          ******************************************************************

          } end description

          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     SERV                ;set server segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader parameters
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, ss:nothing, es:nothing
          ;
          ;
          ; Fatal errors detected by resident part of loader
          ;
          public    ServErrs
          ;
ServErrs  label     byte
          dw        offset ServBad      ;store offset of bad error number msg
          ;
          ; Translation table
          ;
          db        0DEh
          db        020h, 065h, 069h, 061h, 072h, 06Fh, 074h, 073h
          db        06Eh, 064h, 06Ch, 02Eh, 03Eh, 004h, 063h, 054h
          db        070h, 075h, 066h, 005h, 068h, 053h, 079h, 077h
          db        052h, 06Dh, 045h, 076h, 062h, 078h
          ;
          ; #E_TFREE  Memory deallocation error
          db        E_TFREE, 01Eh, 000h, 00Dh, 043h, 01Bh, 076h, 018h, 02Bh
          db        035h, 0CCh, 071h, 005h, 084h, 07Ah, 023h, 066h
          db        076h
          ; #E_TALLOC Memory allocation error
          db        E_TALLOC, 01Ch, 000h, 00Dh, 043h, 01Bh, 076h, 018h, 025h
          db        0CCh, 071h, 005h, 084h, 07Ah, 023h, 066h, 076h
          ; #E_QROOT  Attempt to exit from root shell
          db        E_QROOT, 027h, 000h, 001h, 048h, 083h, 01Bh, 012h, 082h
          db        087h, 023h, 01Fh, 048h, 021h, 046h, 071h, 0B2h
          db        067h, 078h, 029h, 016h, 03Ch, 0C0h
          ; #E_DAEMS  EMS deallocation failed
          db        E_DAEMS, 01Dh, 000h, 01Ch, 00Dh, 041h, 072h, 0B3h, 05Ch
          db        0C7h, 010h, 058h, 047h, 0A2h, 014h, 054h, 0C3h
          db        0B0h
          ; #E_DAXMS  XMS deallocation failed
          db        E_DAXMS, 01Eh, 000h, 008h, 050h, 0D4h, 017h, 02Bh, 035h
          db        0CCh, 071h, 005h, 084h, 07Ah, 021h, 045h, 04Ch
          db        03Bh
          ; #E_XMSMOV XMS move failed
          db        E_XMSMOV, 017h, 000h, 008h, 050h, 0D4h, 017h, 021h, 0B7h
          db        01Dh, 032h, 014h, 054h, 0C3h, 0B0h
          ; #E_EMSMAP EMS mapping failed
          db        E_EMSMAP, 01Ch, 000h, 01Ch, 00Dh, 041h, 072h, 01Bh, 051h
          db        021h, 024h, 0A0h, 076h, 021h, 045h, 04Ch, 03Bh
          ;#E_EMSMSR EMS map save or restore failed
          ; #E_SWSEEK Swap file seek failed
          db        E_SWSEEK, 01Ch, 000h, 017h, 019h, 051h, 022h, 014h, 04Ch
          db        032h, 093h, 030h, 0B6h, 021h, 045h, 04Ch, 03Bh
          ; #E_SWWRIT Swap file write failed
          db        E_SWWRIT, 01Ch, 000h, 017h, 019h, 051h, 022h, 014h, 04Ch
          db        032h, 019h, 064h, 083h, 021h, 045h, 04Ch, 03Bh
          ; #E_SWREAD Swap file read failed
          db        E_SWREAD, 01Ah, 000h, 017h, 019h, 051h, 022h, 014h, 04Ch
          db        032h, 063h, 05Bh, 021h, 045h, 04Ch, 03Bh
          ; #E_SWDBAD Disk swap file corrupted
          db        E_SWDBAD, 022h, 000h, 004h, 044h, 090h, 0B6h, 029h, 019h
          db        051h, 022h, 014h, 04Ch, 032h, 010h, 076h, 061h
          db        031h, 028h, 03Bh
          ; #E_NOHDL  No file handle available
          db        E_NOHDL, 01Eh, 000h, 00Eh, 047h, 021h, 044h, 0C3h, 021h
          db        065h, 0ABh, 0C3h, 025h, 01Dh, 054h, 0C5h, 01Eh
          db        0C3h
          ;
          ; 4DOS signon message
          ;
          ; #162 Copyright 1988-2003  Rex Conn & JP Software Inc.  All Rights Reserved
          db        162, 079h, 000h, 003h, 047h, 012h, 018h, 064h, 007h
          db        061h, 068h, 020h, 013h, 009h, 030h, 083h, 008h
          db        030h, 0D2h, 002h, 030h, 003h, 000h, 030h, 033h
          db        022h, 01Ah, 031h, 0F2h, 003h, 047h, 0AAh, 020h
          db        062h, 020h, 0A4h, 000h, 052h, 017h, 071h, 048h
          db        019h, 056h, 032h, 009h, 04Ah, 010h, 0D2h, 020h
          db        014h, 0CCh, 021h, 0A4h, 007h, 061h, 068h, 092h
          db        01Ah, 039h, 036h, 01Dh, 03Bh, 015h, 0F0h
	;
	if	_RT
          ; #166 4DOSRT.COM not correctly installed, execution cannot continue.
          db        166, 05Bh, 000h, 004h, 030h, 044h, 00Fh, 041h, 071h
          db        0A1h, 01Dh, 003h, 040h, 0F4h, 00Dh, 042h, 0A7h
          db        082h, 010h, 076h, 063h, 010h, 08Ch, 018h, 024h
          db        0A9h, 085h, 0CCh, 03Bh, 00Ch, 022h, 031h, 0F3h
          db        010h, 013h, 084h, 07Ah, 021h, 005h, 0AAh, 078h
          db        021h, 007h, 0A8h, 04Ah, 013h, 03Dh, 015h, 0F0h
          ; #167 4DOS Runtime internal error G%d/%d, contact JP Software.
          db        167, 058h, 000h, 004h, 030h, 044h, 00Fh, 041h, 072h
          db        01Ah, 013h, 0A8h, 041h, 0B3h, 024h, 0A8h, 036h
          db        0A5h, 0C2h, 036h, 067h, 062h, 007h, 040h, 052h
          db        0B0h, 0F2h, 005h, 02Bh, 00Ch, 022h, 010h, 07Ah
          db        085h, 010h, 082h, 00Ah, 040h, 005h, 021h, 077h
          db        014h, 081h, 095h, 063h, 0D1h, 05Fh
	else
          ; #166 4DOS.COM not correctly installed, execution cannot continue.
          db        166, 057h, 000h, 004h, 030h, 044h, 00Fh, 041h, 07Dh
          db        003h, 040h, 0F4h, 00Dh, 042h, 0A7h, 082h, 010h
          db        076h, 063h, 010h, 08Ch, 018h, 024h, 0A9h, 085h
          db        0CCh, 03Bh, 00Ch, 022h, 031h, 0F3h, 010h, 013h
          db        084h, 07Ah, 021h, 005h, 0AAh, 078h, 021h, 007h
          db        0A8h, 04Ah, 013h, 03Dh, 015h, 0F0h
          ; #167 4DOS internal error M%d/%d, contact JP Software.
          db        167, 04Dh, 000h, 004h, 030h, 044h, 00Fh, 041h, 072h
          db        04Ah, 083h, 06Ah, 05Ch, 023h, 066h, 076h, 020h
          db        0D4h, 005h, 02Bh, 00Fh, 020h, 052h, 0B0h, 0C2h
          db        021h, 007h, 0A8h, 051h, 008h, 020h, 0A4h, 000h
          db        052h, 017h, 071h, 048h, 019h, 056h, 03Dh, 015h
          db        0F0h
	endif

          ife       _RT
          ; Restricted trial (Win9x)
          ; #168 >> Your evaluation period has expired.  Usage is now restricted to a limited>> number of commands in each session (for details see TRIAL.TXT).
          db        168, 0BEh, 000h, 0EEh, 020h, 095h, 071h, 036h, 023h
          db        01Dh, 05Ch, 013h, 058h, 047h, 0A2h, 012h, 036h
          db        047h, 0B2h, 016h, 059h, 023h, 01Fh, 012h, 046h
          db        03Bh, 0D2h, 020h, 055h, 095h, 007h, 063h, 024h
          db        092h, 0A7h, 019h, 026h, 039h, 086h, 041h, 008h
          db        03Bh, 028h, 072h, 052h, 0C4h, 01Bh, 048h, 03Bh
          db        015h, 0FEh, 0E2h, 0A1h, 031h, 0B1h, 0E3h, 062h
          db        071h, 042h, 010h, 071h, 0B1h, 0B5h, 0ABh, 092h
          db        04Ah, 023h, 051h, 001h, 062h, 093h, 099h, 047h
          db        0A2h, 008h, 021h, 047h, 062h, 0B3h, 085h, 04Ch
          db        092h, 093h, 032h, 011h, 01Ah, 009h, 040h, 014h
          db        00Ch, 04Dh, 011h, 008h, 051h, 010h, 092h, 0D1h
          db        05Fh
          ; Restricted trial (not Win9x)
          ; #169 >>  Your evaluation period has expired, and your use will be interrupted>>  periodically to remind you to purchase (for details see TRIAL.TXT).
          db        169, 0C2h, 000h, 0EEh, 022h, 009h, 057h, 013h, 062h
          db        031h, 0D5h, 0C1h, 035h, 084h, 07Ah, 021h, 023h
          db        064h, 07Bh, 021h, 065h, 092h, 031h, 0F1h, 024h
          db        063h, 0B0h, 0C2h, 025h, 0ABh, 021h, 087h, 013h
          db        062h, 013h, 093h, 021h, 094h, 0CCh, 021h, 0E3h
          db        024h, 0A8h, 036h, 061h, 031h, 028h, 03Bh, 015h
          db        0FEh, 0E2h, 021h, 023h, 064h, 07Bh, 041h, 005h
          db        0CCh, 018h, 028h, 072h, 063h, 01Bh, 04Ah, 0B2h
          db        018h, 071h, 032h, 087h, 021h, 021h, 036h, 010h
          db        016h, 059h, 032h, 008h, 021h, 047h, 062h, 0B3h
          db        085h, 04Ch, 092h, 093h, 032h, 011h, 01Ah, 009h
          db        040h, 014h, 00Ch, 04Dh, 011h, 008h, 051h, 010h
          db        092h, 0D1h, 05Fh
          ; Normal trial
          ; #170 >>  %d days remain in your evaluation period.  To learn about how the>>  evaluation period works see TRIAL.TXT.
          db        170, 096h, 000h, 0EEh, 022h, 005h, 02Bh, 02Bh, 051h
          db        089h, 026h, 031h, 0B5h, 04Ah, 024h, 0A2h, 018h
          db        071h, 036h, 023h, 01Dh, 05Ch, 013h, 058h, 047h
          db        0A2h, 012h, 036h, 047h, 0BDh, 022h, 011h, 072h
          db        0C3h, 056h, 0A2h, 051h, 0E7h, 013h, 082h, 016h
          db        071h, 092h, 081h, 063h, 015h, 0FEh, 0E2h, 023h
          db        01Dh, 05Ch, 013h, 058h, 047h, 0A2h, 012h, 036h
          db        047h, 0B2h, 019h, 076h, 00Bh, 069h, 029h, 033h
          db        021h, 011h, 0A0h, 094h, 001h, 040h, 0C4h, 0D1h
          db        010h, 085h, 011h, 0D1h, 05Fh
          ; Normal trial (1 day left)
          ; #171 >>  1 day remains in your evaluation period.  To learn about how the>>  evaluation period works see TRIAL.TXT.
          db        171, 095h, 000h, 0EEh, 022h, 001h, 032h, 0B5h, 018h
          db        026h, 031h, 0B5h, 04Ah, 092h, 04Ah, 021h, 087h
          db        013h, 062h, 031h, 0D5h, 0C1h, 035h, 084h, 07Ah
          db        021h, 023h, 064h, 07Bh, 0D2h, 021h, 017h, 02Ch
          db        035h, 06Ah, 025h, 01Eh, 071h, 038h, 021h, 067h
          db        019h, 028h, 016h, 031h, 05Fh, 0EEh, 022h, 031h
          db        0D5h, 0C1h, 035h, 084h, 07Ah, 021h, 023h, 064h
          db        07Bh, 021h, 097h, 060h, 0B6h, 092h, 093h, 032h
          db        011h, 01Ah, 009h, 040h, 014h, 00Ch, 04Dh, 011h
          db        008h, 051h, 01Dh, 015h, 0F0h
          ; Order message
          ; #172 To order call 410-810-8819, visit our web site at http://jpsoft.com/,or see the order form in ORDERS.TXT.
          db        172, 0B1h, 000h, 0F1h, 017h, 027h, 06Bh, 036h, 021h
          db        005h, 0CCh, 020h, 043h, 001h, 030h, 003h, 00Dh
          db        020h, 083h, 001h, 030h, 003h, 00Dh, 020h, 083h
          db        008h, 030h, 013h, 009h, 030h, 0C2h, 021h, 0D4h
          db        094h, 082h, 071h, 036h, 021h, 093h, 01Eh, 029h
          db        048h, 032h, 058h, 021h, 068h, 081h, 020h, 0A3h
          db        00Fh, 020h, 0F2h, 00Ah, 061h, 029h, 071h, 048h
          db        0D1h, 007h, 01Bh, 00Fh, 020h, 0C2h, 015h, 0F7h
          db        062h, 093h, 032h, 081h, 063h, 027h, 06Bh, 036h
          db        021h, 047h, 061h, 0B2h, 04Ah, 020h, 0F4h, 01Ah
          db        004h, 041h, 0C1h, 0A1h, 07Dh, 011h, 008h, 051h
          db        01Dh, 015h, 0F0h
          ; Expired trial
          ; #173 >>  Your evaluation period has expired.  Usage will be restricted>>  in %d day(s); see TRIAL.TXT for details.
          db        173, 099h, 000h, 0EEh, 022h, 009h, 057h, 013h, 062h
          db        031h, 0D5h, 0C1h, 035h, 084h, 07Ah, 021h, 023h
          db        064h, 07Bh, 021h, 065h, 092h, 031h, 0F1h, 024h
          db        063h, 0BDh, 022h, 005h, 059h, 050h, 076h, 032h
          db        019h, 04Ch, 0C2h, 01Eh, 032h, 063h, 098h, 064h
          db        010h, 083h, 0B1h, 05Fh, 0EEh, 022h, 04Ah, 020h
          db        052h, 0B2h, 0B5h, 018h, 008h, 029h, 009h, 020h
          db        0B3h, 029h, 033h, 021h, 011h, 0A0h, 094h, 001h
          db        040h, 0C4h, 0D1h, 010h, 085h, 011h, 021h, 047h
          db        062h, 0B3h, 085h, 04Ch, 09Dh, 015h, 0F0h
          ; Extended trial or pre-release
          ; #174 >>  Your usage period has been extended while your order is processed.>>  Usage will be restricted in %d day(s).
          db        174, 094h, 000h, 0EEh, 022h, 009h, 057h, 013h, 062h
          db        013h, 095h, 007h, 063h, 021h, 023h, 064h, 07Bh
          db        021h, 065h, 092h, 01Eh, 033h, 0A2h, 031h, 0F8h
          db        03Ah, 0B3h, 0B2h, 019h, 016h, 04Ch, 032h, 018h
          db        071h, 036h, 027h, 06Bh, 036h, 024h, 092h, 012h
          db        067h, 010h, 039h, 093h, 0BDh, 015h, 0FEh, 0E2h
          db        020h, 055h, 095h, 007h, 063h, 021h, 094h, 0CCh
          db        021h, 0E3h, 026h, 039h, 086h, 041h, 008h, 03Bh
          db        024h, 0A2h, 005h, 02Bh, 02Bh, 051h, 080h, 082h
          db        090h, 092h, 0D1h, 05Fh
          ; #175 If additional information is needed please contact JP Software atsales@jpsoft.com, or use the numbers listed in README.TXT.
          db        175, 0ADh, 000h, 0F0h, 094h, 014h, 025h, 0BBh, 048h
          db        047h, 0A5h, 0C2h, 04Ah, 014h, 076h, 01Bh, 058h
          db        047h, 0A2h, 049h, 02Ah, 033h, 0B3h, 0B2h, 012h
          db        0C3h, 059h, 032h, 010h, 07Ah, 085h, 010h, 082h
          db        00Ah, 040h, 005h, 021h, 077h, 014h, 081h, 095h
          db        063h, 025h, 081h, 05Fh, 095h, 0C3h, 090h, 004h
          db        00Ah, 061h, 029h, 071h, 048h, 0D1h, 007h, 01Bh
          db        00Ch, 022h, 076h, 021h, 039h, 032h, 081h, 063h
          db        02Ah, 013h, 01Bh, 01Eh, 036h, 092h, 0C4h, 098h
          db        03Bh, 024h, 0A2h, 01Ah, 01Ch, 001h, 040h, 044h
          db        00Dh, 041h, 0CDh, 011h, 008h, 051h, 01Dh, 015h
          db        0F0h
          ; Expired pre-release
          ; #176 >>  This pre-release version has expired, and usage will be restricted in>>  %d day(s).  See PREREL.TXT for details on obtaining a newer version.
          db        176, 0C2h, 000h, 0EEh, 022h, 011h, 016h, 049h, 021h
          db        026h, 030h, 0D2h, 063h, 0C3h, 059h, 032h, 01Dh
          db        036h, 094h, 07Ah, 021h, 065h, 092h, 031h, 0F1h
          db        024h, 063h, 0B0h, 0C2h, 025h, 0ABh, 021h, 039h
          db        050h, 076h, 032h, 019h, 04Ch, 0C2h, 01Eh, 032h
          db        063h, 098h, 064h, 010h, 083h, 0B2h, 04Ah, 015h
          db        0FEh, 0E2h, 020h, 052h, 0B2h, 0B5h, 018h, 008h
          db        029h, 009h, 02Dh, 022h, 017h, 033h, 020h, 005h
          db        01Ah, 01Ch, 01Ah, 01Ch, 00Ch, 04Dh, 011h, 008h
          db        051h, 012h, 014h, 076h, 02Bh, 038h, 054h, 0C9h
          db        027h, 0A2h, 071h, 0E8h, 054h, 0A4h, 0A0h, 076h
          db        025h, 02Ah, 031h, 093h, 062h, 01Dh, 036h, 094h
          db        07Ah, 0D1h, 05Fh
          ; Normal pre-release
          ; #177 >>  This pre-release version expires in %d day(s).  After this time usage>>  may be restricted; see PREREL.TXT for details.
          db        177, 0ABh, 000h, 0EEh, 022h, 011h, 016h, 049h, 021h
          db        026h, 030h, 0D2h, 063h, 0C3h, 059h, 032h, 01Dh
          db        036h, 094h, 07Ah, 023h, 01Fh, 012h, 046h, 039h
          db        024h, 0A2h, 005h, 02Bh, 02Bh, 051h, 080h, 082h
          db        090h, 092h, 0D2h, 020h, 014h, 014h, 083h, 062h
          db        081h, 064h, 092h, 084h, 01Bh, 032h, 013h, 095h
          db        007h, 063h, 015h, 0FEh, 0E2h, 021h, 0B5h, 018h
          db        021h, 0E3h, 026h, 039h, 086h, 041h, 008h, 03Bh
          db        00Bh, 032h, 093h, 032h, 000h, 051h, 0A1h, 0C1h
          db        0A1h, 0C0h, 0C4h, 0D1h, 010h, 085h, 011h, 021h
          db        047h, 062h, 0B3h, 085h, 04Ch, 09Dh, 015h, 0F0h
          ; Restricted pre-release
          ; #178 >>  This pre-release version has expired, and usage is now restricted.  See>>  PREREL.TXT for details and information on obtaining the final release.
          db        178, 0C0h, 000h, 0EEh, 022h, 011h, 016h, 049h, 021h
          db        026h, 030h, 0D2h, 063h, 0C3h, 059h, 032h, 01Dh
          db        036h, 094h, 07Ah, 021h, 065h, 092h, 031h, 0F1h
          db        024h, 063h, 0B0h, 0C2h, 025h, 0ABh, 021h, 039h
          db        050h, 076h, 032h, 049h, 02Ah, 071h, 092h, 063h
          db        098h, 064h, 010h, 083h, 0BDh, 022h, 017h, 033h
          db        015h, 0FEh, 0E2h, 020h, 005h, 01Ah, 01Ch, 01Ah
          db        01Ch, 00Ch, 04Dh, 011h, 008h, 051h, 012h, 014h
          db        076h, 02Bh, 038h, 054h, 0C9h, 025h, 0ABh, 024h
          db        0A1h, 047h, 061h, 0B5h, 084h, 07Ah, 027h, 0A2h
          db        071h, 0E8h, 054h, 0A4h, 0A0h, 076h, 028h, 016h
          db        032h, 014h, 04Ah, 05Ch, 026h, 03Ch, 035h, 093h
          db        0D1h, 05Fh
          ; Expired beta
          ; #179 >> This beta version has expired, and usage will be restricted in>> %d day(s).  See README.TXT for details.
          db        179, 096h, 000h, 0EEh, 021h, 011h, 064h, 092h, 01Eh
          db        038h, 052h, 01Dh, 036h, 094h, 07Ah, 021h, 065h
          db        092h, 031h, 0F1h, 024h, 063h, 0B0h, 0C2h, 025h
          db        0ABh, 021h, 039h, 050h, 076h, 032h, 019h, 04Ch
          db        0C2h, 01Eh, 032h, 063h, 098h, 064h, 010h, 083h
          db        0B2h, 04Ah, 015h, 0FEh, 0E2h, 005h, 02Bh, 02Bh
          db        051h, 080h, 082h, 090h, 092h, 0D2h, 021h, 073h
          db        032h, 01Ah, 01Ch, 001h, 040h, 044h, 00Dh, 041h
          db        0CDh, 011h, 008h, 051h, 012h, 014h, 076h, 02Bh
          db        038h, 054h, 0C9h, 0D1h, 05Fh
          ; Normal beta
          ; #180 >>  This beta version expires in %d day(s).  After this time usage>>  may be restricted; see README.TXT for details.
          db        180, 0A3h, 000h, 0EEh, 022h, 011h, 016h, 049h, 021h
          db        0E3h, 085h, 021h, 0D3h, 069h, 047h, 0A2h, 031h
          db        0F1h, 024h, 063h, 092h, 04Ah, 020h, 052h, 0B2h
          db        0B5h, 018h, 008h, 029h, 009h, 02Dh, 022h, 001h
          db        041h, 048h, 036h, 028h, 016h, 049h, 028h, 041h
          db        0B3h, 021h, 039h, 050h, 076h, 031h, 05Fh, 0EEh
          db        022h, 01Bh, 051h, 082h, 01Eh, 032h, 063h, 098h
          db        064h, 010h, 083h, 0B0h, 0B3h, 029h, 033h, 021h
          db        0A1h, 0C0h, 014h, 004h, 040h, 0D4h, 01Ch, 0D1h
          db        010h, 085h, 011h, 021h, 047h, 062h, 0B3h, 085h
          db        04Ch, 09Dh, 015h, 0F0h
          ; Restricted beta
          ; #181 >>  This beta version has expired, and usage is now restricted.  See>>  README.TXT for details.
          db        181, 082h, 000h, 0EEh, 022h, 011h, 016h, 049h, 021h
          db        0E3h, 085h, 021h, 0D3h, 069h, 047h, 0A2h, 016h
          db        059h, 023h, 01Fh, 012h, 046h, 03Bh, 00Ch, 022h
          db        05Ah, 0B2h, 013h, 095h, 007h, 063h, 024h, 092h
          db        0A7h, 019h, 026h, 039h, 086h, 041h, 008h, 03Bh
          db        0D2h, 021h, 073h, 031h, 05Fh, 0EEh, 022h, 01Ah
          db        01Ch, 001h, 040h, 044h, 00Dh, 041h, 0CDh, 011h
          db        008h, 051h, 012h, 014h, 076h, 02Bh, 038h, 054h
          db        0C9h, 0D1h, 05Fh
          ; Exiting due to restrictions
          ; #182 Exiting ...
          db        182, 015h, 000h, 015h, 0FFh, 01Ch, 01Fh, 048h, 04Ah
          db        007h, 062h, 0DDh, 0DFh, 0F0h
          ; Extension code error
          ; #183 Invalid (%d), please contact JP Software
          db        183, 041h, 000h, 009h, 04Ah, 01Dh, 05Ch, 04Bh, 020h
          db        082h, 005h, 02Bh, 009h, 020h, 0C2h, 021h, 02Ch
          db        035h, 093h, 021h, 007h, 0A8h, 051h, 008h, 020h
          db        0A4h, 000h, 052h, 017h, 071h, 048h, 019h, 056h
          db        031h, 05Fh, 0F0h
         ; Shareware encryption key
          ; #185 ûÃxyûÃxyûÃxyíŒ“¡íŒ“¡íŒ“¡
          db        185, 042h, 000h, 00Eh, 090h, 0CCh, 01Fh, 018h, 00Eh
          db        090h, 0CCh, 01Fh, 018h, 00Eh, 090h, 0CCh, 01Fh
          db        018h, 002h, 090h, 0ECh, 002h, 0D0h, 01Ch, 002h
          db        090h, 0ECh, 002h, 0D0h, 01Ch, 002h, 090h, 0ECh
          db        002h, 0D0h, 01Ch
          ; Extension code character set
          ; #186 83RS6TPMW9DNUXKC4B7VHQLYJIFAG52E
          db        186, 05Ch, 000h, 008h, 030h, 033h, 01Ah, 017h, 006h
          db        031h, 010h, 005h, 00Dh, 040h, 075h, 009h, 030h
          db        044h, 00Eh, 040h, 055h, 008h, 050h, 0B4h, 003h
          db        040h, 043h, 002h, 040h, 073h, 006h, 050h, 084h
          db        001h, 050h, 0C4h, 009h, 050h, 0A4h, 009h, 040h
          db        064h, 001h, 040h, 074h, 005h, 030h, 023h, 01Ch
			 ; Cannot find help file
          ; #188 Cannot locate 4DOS directory -- check your COMSPEC / InstallPath settings.
          db        188, 07Bh, 000h, 0F0h, 034h, 05Ah, 0A7h, 082h, 0C7h
          db        010h, 058h, 032h, 004h, 030h, 044h, 00Fh, 041h
          db        072h, 0B4h, 063h, 010h, 087h, 061h, 082h, 00Dh
          db        020h, 0D2h, 021h, 001h, 063h, 010h, 00Bh, 062h
          db        018h, 071h, 036h, 020h, 034h, 00Fh, 040h, 0D4h
          db        017h, 000h, 051h, 0C0h, 034h, 020h, 0F2h, 020h
          db        094h, 0A9h, 085h, 0CCh, 000h, 055h, 081h, 062h
          db        093h, 088h, 04Ah, 007h, 069h, 0D1h, 05Fh, 0F0h
			 ; Wrong version of help file:
          ; #189 4DOS.HLP file is out of date, please update it.
          db        189, 049h, 000h, 0F0h, 043h, 004h, 040h, 0F4h, 017h
          db        0D0h, 084h, 00Ch, 040h, 005h, 021h, 044h, 0C3h
          db        024h, 092h, 071h, 038h, 027h, 014h, 02Bh, 058h
          db        030h, 0C2h, 021h, 02Ch, 035h, 093h, 021h, 031h
          db        02Bh, 058h, 032h, 048h, 0D1h, 05Fh, 0F0h
          ; Filename for shareware data duplicate
          ; #191 4DOS.DAT
          db        191, 014h, 000h, 004h, 030h, 044h, 00Fh, 041h, 07Dh
          db        004h, 040h, 014h, 011h
          ; Lines and section/item names to go into .INI file flag
          ; #193 [ReleaseInfo]
          db        193, 015h, 000h, 00Bh, 051h, 0A3h, 0C3h, 059h, 030h
          db        094h, 0A1h, 047h, 00Dh, 050h
          ; #194 Description
          db        194, 00Fh, 000h, 004h, 043h, 091h, 006h, 041h, 028h
          db        047h, 0A0h
          ; #195 %s version %s
          db        195, 012h, 000h, 005h, 029h, 021h, 0D3h, 069h, 047h
          db        0A2h, 005h, 029h
          ;
          endif
          ;
          db        0FFh, 0             ;end of table
          ;
ServBad   db        BErrLen, "Illegal server error"
BErrLen   equ       $ - ServBad - 1
          ;
@curseg   ends
          ;
          end

