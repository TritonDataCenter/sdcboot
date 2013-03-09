          ; Translated by Fritz 04-14-92  corr. 10.9.1992


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
          db        065h, 020h, 061h, 06Ch, 067h, 068h, 072h, 06Eh
          db        069h, 053h, 073h, 074h, 063h, 066h, 06Fh, 070h
          db        075h, 062h, 049h, 054h, 064h, 077h, 02Ah, 045h
          db        02Eh, 04Eh, 04Fh, 044h, 046h, 04Dh
          ;
          ; #E_TFREE  Speicherfreigabe fehlgeschlagen
          db        E_TFREE, 021h, 0B1h, 012h, 0AEh, 072h, 08Fh, 082h, 0A6h
          db        041h, 032h, 03Fh, 027h, 056h, 02Ch, 0E7h, 054h
          db        062h, 090h
          ; #E_TALLOC Speicherbelegung fehlgeschlagen
          db        E_TALLOC, 022h, 0B1h, 012h, 0AEh, 072h, 081h, 032h, 052h
          db        061h, 029h, 063h, 0F2h, 075h, 062h, 0CEh, 075h
          db        046h, 029h
          ; #E_QROOT  Versuch, die primÑre Shell zu verlassen
          db        E_QROOT, 037h, 006h, 052h, 08Ch, 012h, 0E7h, 00Ch, 023h
          db        016h, 0A2h, 031h, 018h, 0A0h, 0D6h, 004h, 088h
          db        023h, 0B7h, 025h, 053h, 00Ah, 071h, 023h, 006h
          db        072h, 085h, 04Ch, 0C2h, 090h
          ; #E_DAEMS  EMS Freigabe fehlgeschlagen
          db        E_DAEMS, 01Fh, 019h, 01Fh, 0B3h, 01Eh, 082h, 0A6h, 041h
          db        032h, 03Fh, 027h, 056h, 02Ch, 0E7h, 054h, 062h
          db        090h
          ; #E_DAXMS  XMS Freigabe fehlgeschlagen
          db        E_DAXMS, 020h, 008h, 051h, 0FBh, 031h, 0E8h, 02Ah, 064h
          db        013h, 023h, 0F2h, 075h, 062h, 0CEh, 075h, 046h
          db        029h
          ; #E_XMSMOV XMS Verlagerung fehlgeschlagen
          db        E_XMSMOV, 024h, 008h, 051h, 0FBh, 030h, 065h, 028h, 054h
          db        062h, 081h, 029h, 063h, 0F2h, 075h, 062h, 0CEh
          db        075h, 046h, 029h
          ; #E_EMSMAP EMS Zuordnungsfehler
          db        E_EMSMAP, 01Ch, 019h, 01Fh, 0B3h, 00Ah, 051h, 021h, 008h
          db        016h, 091h, 029h, 06Ch, 0F2h, 075h, 028h
          ;#E_EMSMSR EMS Tabelle kann nicht gespeichert/gelesen werden
          ; #E_SWSEEK Swapdatei suchen fehlgeschlagen
          db        E_SWSEEK, 023h, 0B1h, 074h, 011h, 016h, 04Dh, 02Ah, 03Ch
          db        012h, 0E7h, 029h, 03Fh, 027h, 056h, 02Ch, 0E7h
          db        054h, 062h, 090h
          ; #E_SWWRIT Swapdatei speichern fehlgeschlagen
          db        E_SWWRIT, 026h, 0B1h, 074h, 011h, 016h, 04Dh, 02Ah, 03Ch
          db        011h, 02Ah, 0E7h, 028h, 093h, 0F2h, 075h, 062h
          db        0CEh, 075h, 046h, 029h
          ; #E_SWREAD Swapdatei lesen fehlgeschlagen
          db        E_SWREAD, 021h, 0B1h, 074h, 011h, 016h, 04Dh, 02Ah, 035h
          db        02Ch, 029h, 03Fh, 027h, 056h, 02Ch, 0E7h, 054h
          db        062h, 090h
          ; #E_SWDBAD Disk Swapdatei zerstîrt
          db        E_SWDBAD, 021h, 01Dh, 0ACh, 00Bh, 063h, 0B1h, 074h, 011h
          db        016h, 04Dh, 02Ah, 030h, 0A7h, 028h, 0CDh, 004h
          db        098h, 0D0h
          ; #E_NOHDL  Datei Handling nicht verfÅgbar
          db        E_NOHDL, 027h, 01Dh, 04Dh, 02Ah, 030h, 084h, 049h, 016h
          db        05Ah, 096h, 039h, 0AEh, 07Dh, 030h, 067h, 028h
          db        0F0h, 018h, 061h, 034h, 080h
          ;
          ; 4DOS signon messages
          ;
          ; #244 Copyright 1988-1992  JP Software Inc. & Computer Solutions Software GmbH
          db        244, 07Dh, 003h, 041h, 001h, 010h, 097h, 08Ah, 067h
          db        0D3h, 001h, 030h, 093h, 008h, 030h, 083h, 00Dh
          db        020h, 013h, 009h, 030h, 093h, 002h, 033h, 030h
          db        0A4h, 000h, 053h, 0B1h, 00Fh, 0D1h, 074h, 082h
          db        031h, 049h, 0E1h, 0A3h, 006h, 023h, 003h, 041h
          db        000h, 0D6h, 011h, 012h, 0D2h, 083h, 0B1h, 005h
          db        012h, 0DAh, 010h, 09Ch, 03Bh, 010h, 0FDh, 017h
          db        048h, 023h, 007h, 040h, 0D6h, 013h, 008h, 040h
          ; #245 Alle Rechte vorbehalten.  Vertrieb unter Lizenz von JP Software.
          db        245, 05Dh, 001h, 045h, 052h, 030h, 025h, 02Eh, 07Dh
          db        023h, 006h, 071h, 008h, 013h, 027h, 045h, 0D2h
          db        091h, 0A3h, 030h, 065h, 028h, 0D8h, 0A2h, 013h
          db        031h, 029h, 0D2h, 083h, 00Ch, 04Ah, 00Ah, 072h
          db        090h, 0A7h, 030h, 067h, 010h, 093h, 00Ah, 040h
          db        005h, 03Bh, 010h, 0FDh, 017h, 048h, 021h, 0A0h
          ifndef    SECURE
          ; #248 *** BETA TEST VERSION.  CONFIDENTIAL, NOT FOR DISTRIBUTION. ***
          db        248, 07Eh, 018h, 018h, 018h, 030h, 024h, 019h, 015h
          db        001h, 043h, 015h, 019h, 0B1h, 053h, 006h, 051h
          db        090h, 025h, 0B1h, 041h, 0C1h, 0B1h, 0A3h, 030h
          db        034h, 01Ch, 01Bh, 01Eh, 014h, 01Dh, 019h, 01Bh
          db        015h, 014h, 001h, 040h, 0C4h, 00Ch, 023h, 01Bh
          db        01Ch, 015h, 031h, 0E1h, 0C0h, 025h, 031h, 0D1h
          db        04Bh, 015h, 002h, 051h, 040h, 024h, 005h, 051h
          db        051h, 041h, 0C1h, 0B1h, 0A3h, 018h, 018h, 018h
          endif
          ;
          db        0FFh, 0             ;end of table
          ;
ServBad   db        BErrLen, "Illegaler Ladefehler"
BErrLen   equ       $ - ServBad - 1
          ;
@curseg   ends
          ;
          end

