set langf=strings.en
nasm display.asm -o display.com -i.\egavga\ -dlangfile=strings.%LANG%
com2exe display.com display.exe
upx display.exe --8086

