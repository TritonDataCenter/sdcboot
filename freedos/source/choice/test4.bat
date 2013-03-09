@ECHO OFF
CHOICE /Tn,TWO "do you like programming? (incorrect time given)"
IF ERRORLEVEL 2 GOTO N
IF ERRORLEVEL 1 GOTO Y
GOTO FAIL

:Y
ECHO Yes!
GOTO DOS

:N
ECHO You suck!
GOTO DOS

:FAIL
ECHO Failed!

:DOS
