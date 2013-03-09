@echo off
rem creates MEMSTAT.EXE with JWasm
jwasm.exe -c -nologo -mz -I.. -Fl MEMSTAT.asm
