@echo off
set FULL=-r
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap ENGLISH
ren command.com cmd-en.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap dutch
ren command.com cmd-nl.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap french
ren command.com cmd-fr.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap german
ren command.com cmd-de.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap italian
ren command.com cmd-it.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap polish
ren command.com cmd-pl.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap pt_br
ren command.com cmd-pt.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap russian
ren command.com cmd-ru.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap serbian
ren command.com cmd-yu.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap spanish
ren command.com cmd-es.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap swedish
ren command.com cmd-se.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap yu437
ren command.com cmd-yu43.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
call build %FULL% xms-swap ukr
ren command.com cmd-ua.com
del strings\*.obj strings\*.exe lib\*.lib strings\*.lib command\*.lib cmd\*.lib
SET FULL=
