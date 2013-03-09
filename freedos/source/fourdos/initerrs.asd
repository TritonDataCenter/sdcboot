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
          db        065h, 020h, 068h, 072h, 061h, 069h, 06Ch, 06Eh
          db        073h, 063h, 064h, 067h, 070h, 074h, 053h, 062h
          db        066h, 075h, 077h, 041h, 07Ah, 03Bh, 046h, 04Ch
          db        055h, 05Ah, 06Fh, 0E1h, 000h, 000h
          ;
          ; #E_SWSEEK Swapdatei suchen fehlgeschlagen
          db        E_SWSEEK, 023h, 010h, 014h, 06Eh, 0C6h, 0F2h, 073h, 0A1h
          db        03Bh, 042h, 093h, 012h, 024h, 08Dh, 02Ah, 0B4h
          db        086h, 0D2h, 090h
          ; #E_SWWRIT Swapdatei speichern fehlgeschlagen
          db        E_SWWRIT, 025h, 010h, 014h, 06Eh, 0C6h, 0F2h, 073h, 0AEh
          db        027h, 0B4h, 025h, 093h, 012h, 024h, 08Dh, 02Ah
          db        0B4h, 086h, 0D2h, 090h
          ;
          ; #E_MEMDA Fehler bei der Speicherfreigabe
          db        E_MEMDA, 024h, 018h, 024h, 082h, 053h, 011h, 027h, 03Ch
          db        025h, 031h, 00Eh, 027h, 0B4h, 025h, 012h, 052h
          db        07Dh, 061h, 012h
          ; #E_NOMEM Zu wenig Speicher
          db        E_NOMEM, 015h, 01Bh, 013h, 031h, 042h, 097h, 0D3h, 010h
          db        0E2h, 07Bh, 042h, 050h
          ; #E_MODFIT Unzureichender Ladespeicher
          db        E_MODFIT, 01Fh, 01Ah, 091h, 061h, 035h, 027h, 0B4h, 029h
          db        0C2h, 053h, 019h, 06Ch, 02Ah, 0E2h, 07Bh, 042h
          db        050h
          ; #E_RELOC Adressentabelle fehlt
          db        E_RELOC, 018h, 015h, 0C5h, 02Ah, 0A2h, 09Fh, 061h, 012h
          db        088h, 023h, 012h, 024h, 08Fh
          ; #E_RLSIZE Adressentabelle zu groá
          db        E_RLSIZE, 01Dh, 015h, 0C5h, 02Ah, 0A2h, 09Fh, 061h, 012h
          db        088h, 023h, 016h, 013h, 03Dh, 051h, 0C1h, 0D0h
        ;
          db        0FFh, 0             ;end of table
          ;
InitBad   db        BErrLen, "Illegaler Initialisierungsfehler"
BErrLen   equ       $ - InitBad - 1
          ;
@curseg   ends
          ;
          end

