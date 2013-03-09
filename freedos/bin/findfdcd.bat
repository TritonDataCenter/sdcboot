:start
@echo off
finddisk +FDOS_BETA9
if %ERRORLEVEL%==0  goto no
if %ERRORLEVEL%==1 set cdrep=A:
if %ERRORLEVEL%==2 set cdrep=B:
if %ERRORLEVEL%==3 set cdrep=C:
if %ERRORLEVEL%==4 set cdrep=D:
if %ERRORLEVEL%==5 set cdrep=E:
if %ERRORLEVEL%==6 set cdrep=F:
if %ERRORLEVEL%==7 set cdrep=G:
if %ERRORLEVEL%==8 set cdrep=H:
if %ERRORLEVEL%==9 set cdrep=I:
if %ERRORLEVEL%==10 set cdrep=J:
if %ERRORLEVEL%==11 set cdrep=K:
if %ERRORLEVEL%==12 set cdrep=L:
if %ERRORLEVEL%==13 set cdrep=M:
if %ERRORLEVEL%==14 set cdrep=N:
if %ERRORLEVEL%==15 set cdrep=O:
if %ERRORLEVEL%==16 set cdrep=P:
if %ERRORLEVEL%==17 set cdrep=Q:
if %ERRORLEVEL%==18 set cdrep=R:
if %ERRORLEVEL%==19 set cdrep=S:
if %ERRORLEVEL%==20 set cdrep=T:
if %ERRORLEVEL%==21 set cdrep=U:
if %ERRORLEVEL%==22 set cdrep=V:
if %ERRORLEVEL%==23 set cdrep=W:
if %ERRORLEVEL%==24 set cdrep=X:
if %ERRORLEVEL%==25 set cdrep=Y:
if %ERRORLEVEL%==26 set cdrep=Z:
goto end
:no
echo FreeDOS CD-ROM not found.  If you want to be able to use 'FDCDROM',
echo please insert the CD-ROM now.
choice /c:yn Have you inserted the CD-ROM
if "%errorlevel%"=="2" echo You will have to manually set up the 'CDREP' environment variable if you wish to use 'FDCDROM'
if "%errorlevel%"=="2" goto end
goto start
:end
