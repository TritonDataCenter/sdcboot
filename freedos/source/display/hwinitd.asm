
;   FreeDOS DISPLAY:  HWInit.ASM
;
;   ===================================================================
;
;   Hardware dependant initialisation routines for DISPLAY, and
;   table with the supported hardware
;
;   WWW:    http://www.freedos.org/
;
;   ===================================================================
;               .       .       .       .       .       line that rules
;

; =====================================================================
;
; Hardware Table
;
; =====================================================================

HardwareTables:
                DB      "EGA",0,0,0,0,0
                DW      EGAInit
                DW      0

                DB      "EGA 8",0,0,0
                DW      EGAInit
                DW      1

                DB      "LCD",0,0,0,0,0
                DW      EGAInit
                DW      1

                DB      "EGA 14",0,0
                DW      EGAInit
                DW      2

                DB      "VGA",0,0,0,0,0
                DW      EGAInit
                DW      3

                DB      "CGA",0,0,0,0,0
                DW      CGAInit
                DW      0

                DB      0               ; end of table



; =====================================================================
;
; Include the hardware dependent initialisation transient code
;
; =====================================================================


; Driver:       EGA/VGA (like MS-DISPLAY.SYS)
;               CGA     (like MS-GRAFTABL.COM)

%include        "cgaega.asm"


