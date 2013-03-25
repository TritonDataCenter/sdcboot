@echo off
rem Regression test for GNU grep.
rem Usage: check [386]

rem The Khadafy test is brought to you by Scott Anderson . . .
..\grep%1 -E -f khadafy.reg khadafy.lin >khadafy.out
cmp khadafy.lin khadafy.out
if not errorlevel 1 goto spencer
echo Khadafy test failed -- output left on khadafy.out
set failures=1

:spencer
rem . . . and the following by Henry Spencer. (MS-DOSified by Steve McConnel)
test %1

exit %failures%