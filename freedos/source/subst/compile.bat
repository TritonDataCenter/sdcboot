@echo off
set model=s
set lng=english
set compiler=bc45
set freedos=d:\freedos
set bcdir=c:\bc45

set fdsrc=%freedos%\src
set fdbin=%fdsrc%\bin
set fdlib=%fdsrc%\lib\%compiler%
set fdinc=%fdsrc%\include

echo -W- -X- -H -H=swsubst.csm -f- -ff- -RT- >BccCfg.Cfg
echo -m%model% >>BccCfg.Cfg
echo -DNDEBUG=1 -O2 -O -Ol -Z -Obeimpv >>BccCfg.Cfg
echo -I.;%fdinc%;%bcdir%\include >>BccCfg.Cfg
echo -L.;%fdlib%;%bcdir%\lib >>BccCfg.Cfg

%fdbin%\msgcomp %fdinc%\%lng% %lng% comp.rsp lib.rsp yerror.h
bcc.exe +BccCfg.Cfg -c @comp.rsp
lib.exe /SCW msg.lib @lib.rsp, msg.lst
bcc.exe +BccCfg.Cfg -eswsubst.exe @src.rsp msg.lib suppl_%model%.lib %model%_%lng%.lib
