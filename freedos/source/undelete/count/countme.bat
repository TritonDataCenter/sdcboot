set COUNT=0000
:loop
if not exist file%COUNT%.txt copy template.txt file%COUNT%.txt
count
if not x%COUNT%==x0042 goto loop
