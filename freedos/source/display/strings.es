; Miscellaneous DISPLAY loading error messages
errAlready       DB     "DISPLAY ya est  presente en memoria", 0dH, 0aH, "$"
errNoDRDOS       DB     "FD-DISPLAY es incompatible con esta versi¢n de DR-KEYB", 0dH, 0aH, "$"
sMemAllocatedBuffers
                 DB     "Buffers reservados en memoria: $"
sInTPA           DB     " en convencional, $"
sInXMS           DB     " en extendida$"

; Hardware-driver specific messages
errAcient        DB     "FD-DISPLAY requiere al menos un adaptador EGA", 0dH, 0aH, "$"

; Commandline parsing error messages
SyntaxErrorStr:         DB      "Error de sintaxis($"
SES_ParamRequired       DB      ") Falta el par metro requerido", 0dH, 0aH, "$"
SES_UnexpectedEOL       DB      ") Final de l¡nea inesperado", 0dH, 0aH, "$"
SES_IllegalChar         DB      ") Car cter no v lido", 0dH, 0aH, "$"
SES_NameTooLong         DB      ") Nombre de dispositivo demasiado largo", 0dH, 0aH, "$"
SES_OpenBrExpected      DB      ") Se esperaba ( ", 0dH, 0aH, "$"
SES_WrongHwName         DB      ") Tipo de hardware desconocido", 0dH, 0aH, "$"
SES_CommaExpected       DB      ") Se esperaba , ", 0dH, 0aH, "$"
SES_CloseBrExpected     DB      ") Se esperaba ) ", 0dH, 0aH, "$"
SES_WrongNumberPars     DB      ") N£mero de par metros err¢neo", 0dH, 0aH, "$"
SES_TooManyPools        DB      ") Demasiados c¢digos de p gina de software (MAX=5)", 0dH, 0aH, "$"
SES_ListTooLong         DB      ") Lista demasiado larga", 0dH, 0aH, "$"
SES_TooManyHWPools      DB      ") Demasiados c¢digos de p gina de hardware", 0dh, 0ah, "$"
SES_NoAllocatedBufs     DB      ") No hay memoria suficiente para buffers", 0dH, 0aH, "$"

