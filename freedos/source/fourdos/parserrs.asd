          ;  Translated by Fritz / 04-14-92
          ;
          ;  Parse error text to be INCLUDEd in 4DLINIT.ASM
          ;
          ;  Copyright 1991, JP Software Inc., All Rights Reserved
          ;  Author:  Tom Rawson  05/11/91
          ;
          ; This module includes only an error message table.  Routines for
          ; accessing this table are in ERRORMSG.ASM.
          ;
          ; ****************************************************************
          ; NOTE:  This text has been compressed and copied from
          ; PARSERRS.TXT.  Any changes should be made there, not here!
          ; ****************************************************************
          ;
          public    ParsErrs
          ;
          ;
          ; NOTE:  This text is loaded initially in 4DLINIT.ASM.  It is
          ; then moved by 4DLINIT code into the resident portion of 4DOS
          ; if necessary.  This move invalidates the "dw  offset ParsBad"
          ; below.  This is fixed by adjusting this value inside the
          ; subroutine LoadMod in 4DLINIT.  Be aware of this if you change
          ; the structure of the data below!
          ;
          ;
ParsErrs  label     byte                ;beginning of message table
          dw        offset ParsBad      ;store offset of bad error number msg
          ;
          ; Translation table
          ;
          db        065h, 072h, 020h, 074h, 061h, 069h, 073h, 06Ch
          db        06Dh, 06Eh, 068h, 050h, 067h, 063h, 075h, 06Fh
          db        07Ah, 084h, 055h, 057h, 066h, 042h, 06Bh, 03Bh
          db        045h, 046h, 04Bh, 053h, 05Ah, 062h
          ; Error messages
          ;
          ; #1 Zu viele Parameter
          db        1, 016h, 01Eh, 010h, 040h, 067h, 072h, 092h, 04Dh
          db        063h, 06Ah, 025h, 023h
          ; #2 Erforderlicher Parameter fehlt
          db        2, 024h, 01Ah, 031h, 061h, 013h, 004h, 062h, 039h
          db        07Fh, 0C2h, 034h, 0D6h, 036h, 0A2h, 052h, 034h
          db        016h, 02Ch, 095h
          ; #3 UnzulÑssiger Schalter
          db        3, 01Ah, 014h, 0B1h, 021h, 009h, 013h, 088h, 07Eh
          db        023h, 041h, 0DFh, 0C6h, 095h, 023h
          ; #4 UngÅltiges Befehlswort
          db        4, 01Eh, 014h, 0BEh, 001h, 089h, 057h, 0E2h, 084h
          db        017h, 021h, 062h, 0C9h, 080h, 077h, 011h, 035h
          ; #6 Parameter Wert nicht im zulÑssigen Bereich
          db        6, 02Fh, 0D6h, 036h, 0A2h, 052h, 034h, 015h, 023h
          db        054h, 0B7h, 0FCh, 054h, 07Ah, 041h, 021h, 009h
          db        013h, 088h, 07Eh, 02Bh, 041h, 072h, 032h, 07Fh
          db        0C0h
          ; #7 Parameter Wert nicht zulÑssig
          db        7, 021h, 0D6h, 036h, 0A2h, 052h, 034h, 015h, 023h
          db        054h, 0B7h, 0FCh, 054h, 012h, 010h, 091h, 038h
          db        087h, 0E0h
          ; #8 Parameter Wert nicht zulÑssig
          db        8, 021h, 0D6h, 036h, 0A2h, 052h, 034h, 015h, 023h
          db        054h, 0B7h, 0FCh, 054h, 012h, 010h, 091h, 038h
          db        087h, 0E0h
          ; #9 Parameter Format nicht korrekt
          db        9, 023h, 0D6h, 036h, 0A2h, 052h, 034h, 01Bh, 011h
          db        03Ah, 065h, 04Bh, 07Fh, 0C5h, 041h, 081h, 013h
          db        032h, 018h, 050h
          ; #10 UnzulÑssiger Parameter
          db        10, 01Ah, 014h, 0B1h, 021h, 009h, 013h, 088h, 07Eh
          db        023h, 04Dh, 063h, 06Ah, 025h, 023h
          ; #11 UnzulÑssige Parameter Kombination
          db        11, 029h, 014h, 0B1h, 021h, 009h, 013h, 088h, 07Eh
          db        024h, 0D6h, 036h, 0A2h, 052h, 034h, 01Ch, 011h
          db        0A1h, 0F7h, 0B6h, 057h, 011h, 0B0h
          ;
          db        0FFh, 0             ;end of table
          ;
ParsBad   db        1, 0                ;no message if not found

