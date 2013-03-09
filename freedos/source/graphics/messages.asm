
helloworld	db "Driver to make 'shift PrtScr' key work",13,10
		db "even in CGA, EGA, VGA, MCGA graphics",13,10
		db "modes loaded, in "
%ifdef EPSON
		db                  "Epson"
%endif
%ifdef POSTSCRIPT
		db                  "PostScript"
%endif
%ifdef HPPCL
		db                  "HP PCL"
%endif
		db                            " mode.",13,10,"$"

helptext:	; help text is only 40 columns wide ;-)     *<- 40th!
%ifdef POSTSCRIPT
		db "Usage: GRAPHICS [/B] [/I] [/C] [/E] [/n]",13,10
%else
%ifdef EPSON
		; using [] around options would make message > 40 columns
		db "Usage: GRAPHICS /B /I /C /9 /X /R /E /n",13,10
%else
		db "Usage:",13,10
		db "GRAPHICS [/B] [/I] [/C] [/R] [/E] [/n]",13,10
%endif
%endif
		db " /B recognize non-black CGA color 0",13,10
		db " /I inverse printing (for dark images)",13,10
		db " /C compatibility mode"
%ifdef EPSON
		db                        " (120x60 dpi)",13,10
		db " /9 9-pin mode (120x72, with /C 60x72)",13,10
		db " /X extend block width to page width",13,10
%endif
%ifdef POSTSCRIPT
		db                        " (HP Laserjet)",13,10
%endif
%ifdef HPPCL
		db                        " (300 dpi)",13,10
%endif
%ifndef POSTSCRIPT
		db " /R use random dither, not ordered one",13,10
%endif
		db " /E economy mode (only 50% density)",13,10
		db " /n Use LPTn (value: 1..3, default 1)",13,10
		db " /? show this help text, do not load",13,10
		db 13,10
		db "After loading the driver you can print",13,10
		db "screenshots using 'shift PrintScreen'",13,10
		db "even in graphics modes. This GRAPHICS",13,10
		db "is for "
%ifdef EPSON
		db         "Epson (e.g. 180 dpi)"
%endif
%ifdef POSTSCRIPT
		db         "PostScript grayscale"
%endif
%ifdef HPPCL
		db         "HP PCL 600 dpi"
%endif
		db                             " printers.",13,10
		db "This is free software (GPL2 license).",13,10
		db "Copyleft by Eric Auer 2003, 2008.",13,10
		db "$"

