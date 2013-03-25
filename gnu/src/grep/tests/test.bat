@echo off
set failures=0
echo abc| ..\grep%1 -E -e "abc" > nul
if errorlevel 1 goto test1failed
goto test1done
:test1failed
echo Spencer test 1 failed
set failures=1
:test1done
echo xbc| ..\grep%1 -E -e "abc" > nul
if errorlevel 2 goto test2failed
if not errorlevel 1 goto test2failed
goto test2done
:test2failed
echo Spencer test 2 failed
set failures=1
:test2done
echo axc| ..\grep%1 -E -e "abc" > nul
if errorlevel 2 goto test3failed
if not errorlevel 1 goto test3failed
goto test3done
:test3failed
echo Spencer test 3 failed
set failures=1
:test3done
echo abx| ..\grep%1 -E -e "abc" > nul
if errorlevel 2 goto test4failed
if not errorlevel 1 goto test4failed
goto test4done
:test4failed
echo Spencer test 4 failed
set failures=1
:test4done
echo xabcy| ..\grep%1 -E -e "abc" > nul
if errorlevel 1 goto test5failed
goto test5done
:test5failed
echo Spencer test 5 failed
set failures=1
:test5done
echo ababc| ..\grep%1 -E -e "abc" > nul
if errorlevel 1 goto test6failed
goto test6done
:test6failed
echo Spencer test 6 failed
set failures=1
:test6done
echo abc| ..\grep%1 -E -e "ab*c" > nul
if errorlevel 1 goto test7failed
goto test7done
:test7failed
echo Spencer test 7 failed
set failures=1
:test7done
echo abc| ..\grep%1 -E -e "ab*bc" > nul
if errorlevel 1 goto test8failed
goto test8done
:test8failed
echo Spencer test 8 failed
set failures=1
:test8done
echo abbc| ..\grep%1 -E -e "ab*bc" > nul
if errorlevel 1 goto test9failed
goto test9done
:test9failed
echo Spencer test 9 failed
set failures=1
:test9done
echo abbbbc| ..\grep%1 -E -e "ab*bc" > nul
if errorlevel 1 goto test10failed
goto test10done
:test10failed
echo Spencer test 10 failed
set failures=1
:test10done
echo abbc| ..\grep%1 -E -e "ab+bc" > nul
if errorlevel 1 goto test11failed
goto test11done
:test11failed
echo Spencer test 11 failed
set failures=1
:test11done
echo abc| ..\grep%1 -E -e "ab+bc" > nul
if errorlevel 2 goto test12failed
if not errorlevel 1 goto test12failed
goto test12done
:test12failed
echo Spencer test 12 failed
set failures=1
:test12done
echo abq| ..\grep%1 -E -e "ab+bc" > nul
if errorlevel 2 goto test13failed
if not errorlevel 1 goto test13failed
goto test13done
:test13failed
echo Spencer test 13 failed
set failures=1
:test13done
echo abbbbc| ..\grep%1 -E -e "ab+bc" > nul
if errorlevel 1 goto test14failed
goto test14done
:test14failed
echo Spencer test 14 failed
set failures=1
:test14done
echo abbc| ..\grep%1 -E -e "ab?bc" > nul
if errorlevel 1 goto test15failed
goto test15done
:test15failed
echo Spencer test 15 failed
set failures=1
:test15done
echo abc| ..\grep%1 -E -e "ab?bc" > nul
if errorlevel 1 goto test16failed
goto test16done
:test16failed
echo Spencer test 16 failed
set failures=1
:test16done
echo abbbbc| ..\grep%1 -E -e "ab?bc" > nul
if errorlevel 2 goto test17failed
if not errorlevel 1 goto test17failed
goto test17done
:test17failed
echo Spencer test 17 failed
set failures=1
:test17done
echo abc| ..\grep%1 -E -e "ab?c" > nul
if errorlevel 1 goto test18failed
goto test18done
:test18failed
echo Spencer test 18 failed
set failures=1
:test18done
echo abc| ..\grep%1 -E -e "^abc$" > nul
if errorlevel 1 goto test19failed
goto test19done
:test19failed
echo Spencer test 19 failed
set failures=1
:test19done
echo abcc| ..\grep%1 -E -e "^abc$" > nul
if errorlevel 2 goto test20failed
if not errorlevel 1 goto test20failed
goto test20done
:test20failed
echo Spencer test 20 failed
set failures=1
:test20done
echo abcc| ..\grep%1 -E -e "^abc" > nul
if errorlevel 1 goto test21failed
goto test21done
:test21failed
echo Spencer test 21 failed
set failures=1
:test21done
echo aabc| ..\grep%1 -E -e "^abc$" > nul
if errorlevel 2 goto test22failed
if not errorlevel 1 goto test22failed
goto test22done
:test22failed
echo Spencer test 22 failed
set failures=1
:test22done
echo aabc| ..\grep%1 -E -e "abc$" > nul
if errorlevel 1 goto test23failed
goto test23done
:test23failed
echo Spencer test 23 failed
set failures=1
:test23done
echo abc| ..\grep%1 -E -e "^" > nul
if errorlevel 1 goto test24failed
goto test24done
:test24failed
echo Spencer test 24 failed
set failures=1
:test24done
echo abc| ..\grep%1 -E -e "$" > nul
if errorlevel 1 goto test25failed
goto test25done
:test25failed
echo Spencer test 25 failed
set failures=1
:test25done
echo abc| ..\grep%1 -E -e "a.c" > nul
if errorlevel 1 goto test26failed
goto test26done
:test26failed
echo Spencer test 26 failed
set failures=1
:test26done
echo axc| ..\grep%1 -E -e "a.c" > nul
if errorlevel 1 goto test27failed
goto test27done
:test27failed
echo Spencer test 27 failed
set failures=1
:test27done
echo axyzc| ..\grep%1 -E -e "a.*c" > nul
if errorlevel 1 goto test28failed
goto test28done
:test28failed
echo Spencer test 28 failed
set failures=1
:test28done
echo axyzd| ..\grep%1 -E -e "a.*c" > nul
if errorlevel 2 goto test29failed
if not errorlevel 1 goto test29failed
goto test29done
:test29failed
echo Spencer test 29 failed
set failures=1
:test29done
echo abc| ..\grep%1 -E -e "a[bc]d" > nul
if errorlevel 2 goto test30failed
if not errorlevel 1 goto test30failed
goto test30done
:test30failed
echo Spencer test 30 failed
set failures=1
:test30done
echo abd| ..\grep%1 -E -e "a[bc]d" > nul
if errorlevel 1 goto test31failed
goto test31done
:test31failed
echo Spencer test 31 failed
set failures=1
:test31done
echo abd| ..\grep%1 -E -e "a[b-d]e" > nul
if errorlevel 2 goto test32failed
if not errorlevel 1 goto test32failed
goto test32done
:test32failed
echo Spencer test 32 failed
set failures=1
:test32done
echo ace| ..\grep%1 -E -e "a[b-d]e" > nul
if errorlevel 1 goto test33failed
goto test33done
:test33failed
echo Spencer test 33 failed
set failures=1
:test33done
echo aac| ..\grep%1 -E -e "a[b-d]" > nul
if errorlevel 1 goto test34failed
goto test34done
:test34failed
echo Spencer test 34 failed
set failures=1
:test34done
echo a-| ..\grep%1 -E -e "a[-b]" > nul
if errorlevel 1 goto test35failed
goto test35done
:test35failed
echo Spencer test 35 failed
set failures=1
:test35done
echo a-| ..\grep%1 -E -e "a[b-]" > nul
if errorlevel 1 goto test36failed
goto test36done
:test36failed
echo Spencer test 36 failed
set failures=1
:test36done
echo -| ..\grep%1 -E -e "a[b-a]" > nul
if errorlevel 2 goto test37failed
if not errorlevel 1 goto test37failed
goto test37done
:test37failed
echo Spencer test 37 failed
set failures=1
:test37done
echo -| ..\grep%1 -E -e "a[]b" > nul
if not errorlevel 2 goto test38failed
goto test38done
:test38failed
echo Spencer test 38 failed
set failures=1
:test38done
echo -| ..\grep%1 -E -e "a[" > nul
if not errorlevel 2 goto test39failed
goto test39done
:test39failed
echo Spencer test 39 failed
set failures=1
:test39done
echo a]| ..\grep%1 -E -e "a]" > nul
if errorlevel 1 goto test40failed
goto test40done
:test40failed
echo Spencer test 40 failed
set failures=1
:test40done
echo a]b| ..\grep%1 -E -e "a[]]b" > nul
if errorlevel 1 goto test41failed
goto test41done
:test41failed
echo Spencer test 41 failed
set failures=1
:test41done
echo aed| ..\grep%1 -E -e "a[^bc]d" > nul
if errorlevel 1 goto test42failed
goto test42done
:test42failed
echo Spencer test 42 failed
set failures=1
:test42done
echo abd| ..\grep%1 -E -e "a[^bc]d" > nul
if errorlevel 2 goto test43failed
if not errorlevel 1 goto test43failed
goto test43done
:test43failed
echo Spencer test 43 failed
set failures=1
:test43done
echo adc| ..\grep%1 -E -e "a[^-b]c" > nul
if errorlevel 1 goto test44failed
goto test44done
:test44failed
echo Spencer test 44 failed
set failures=1
:test44done
echo a-c| ..\grep%1 -E -e "a[^-b]c" > nul
if errorlevel 2 goto test45failed
if not errorlevel 1 goto test45failed
goto test45done
:test45failed
echo Spencer test 45 failed
set failures=1
:test45done
echo a]c| ..\grep%1 -E -e "a[^]b]c" > nul
if errorlevel 2 goto test46failed
if not errorlevel 1 goto test46failed
goto test46done
:test46failed
echo Spencer test 46 failed
set failures=1
:test46done
echo adc| ..\grep%1 -E -e "a[^]b]c" > nul
if errorlevel 1 goto test47failed
goto test47done
:test47failed
echo Spencer test 47 failed
set failures=1
:test47done
echo abc| ..\grep%1 -E -f msdos/test48.pat > nul
if errorlevel 1 goto test48failed
goto test48done
:test48failed
echo Spencer test 48 failed
set failures=1
:test48done
echo abcd| ..\grep%1 -E -f msdos/test49.pat > nul
if errorlevel 1 goto test49failed
goto test49done
:test49failed
echo Spencer test 49 failed
set failures=1
:test49done
echo def| ..\grep%1 -E -e "()ef" > nul
if errorlevel 1 goto test50failed
goto test50done
:test50failed
echo Spencer test 50 failed
set failures=1
:test50done
echo -| ..\grep%1 -E -e "()*" > nul
if errorlevel 1 goto test51failed
goto test51done
:test51failed
echo Spencer test 51 failed
set failures=1
:test51done
echo -| ..\grep%1 -E -e "*a" > nul
if errorlevel 2 goto test52failed
if not errorlevel 1 goto test52failed
goto test52done
:test52failed
echo Spencer test 52 failed
set failures=1
:test52done
echo -| ..\grep%1 -E -e "^*" > nul
if errorlevel 1 goto test53failed
goto test53done
:test53failed
echo Spencer test 53 failed
set failures=1
:test53done
echo -| ..\grep%1 -E -e "$*" > nul
if errorlevel 1 goto test54failed
goto test54done
:test54failed
echo Spencer test 54 failed
set failures=1
:test54done
echo -| ..\grep%1 -E -e "(*)b" > nul
if errorlevel 2 goto test55failed
if not errorlevel 1 goto test55failed
goto test55done
:test55failed
echo Spencer test 55 failed
set failures=1
:test55done
echo b| ..\grep%1 -E -e "$b" > nul
if errorlevel 2 goto test56failed
if not errorlevel 1 goto test56failed
goto test56done
:test56failed
echo Spencer test 56 failed
set failures=1
:test56done
echo -| ..\grep%1 -E -e "a\\" > nul
if not errorlevel 2 goto test57failed
goto test57done
:test57failed
echo Spencer test 57 failed
set failures=1
:test57done
echo a(b| ..\grep%1 -E -e "a\(b" > nul
if errorlevel 1 goto test58failed
goto test58done
:test58failed
echo Spencer test 58 failed
set failures=1
:test58done
echo ab| ..\grep%1 -E -e "a\(*b" > nul
if errorlevel 1 goto test59failed
goto test59done
:test59failed
echo Spencer test 59 failed
set failures=1
:test59done
echo a((b| ..\grep%1 -E -e "a\(*b" > nul
if errorlevel 1 goto test60failed
goto test60done
:test60failed
echo Spencer test 60 failed
set failures=1
:test60done
echo a\x| ..\grep%1 -E -e "a\x" > nul
if errorlevel 2 goto test61failed
if not errorlevel 1 goto test61failed
goto test61done
:test61failed
echo Spencer test 61 failed
set failures=1
:test61done
echo -| ..\grep%1 -E -e "abc)" > nul
if not errorlevel 2 goto test62failed
goto test62done
:test62failed
echo Spencer test 62 failed
set failures=1
:test62done
echo -| ..\grep%1 -E -e "(abc" > nul
if not errorlevel 2 goto test63failed
goto test63done
:test63failed
echo Spencer test 63 failed
set failures=1
:test63done
echo abc| ..\grep%1 -E -e "((a))" > nul
if errorlevel 1 goto test64failed
goto test64done
:test64failed
echo Spencer test 64 failed
set failures=1
:test64done
echo abc| ..\grep%1 -E -e "(a)b(c)" > nul
if errorlevel 1 goto test65failed
goto test65done
:test65failed
echo Spencer test 65 failed
set failures=1
:test65done
echo aabbabc| ..\grep%1 -E -e "a+b+c" > nul
if errorlevel 1 goto test66failed
goto test66done
:test66failed
echo Spencer test 66 failed
set failures=1
:test66done
echo -| ..\grep%1 -E -e "a**" > nul
if errorlevel 1 goto test67failed
goto test67done
:test67failed
echo Spencer test 67 failed
set failures=1
:test67done
echo -| ..\grep%1 -E -e "a*?" > nul
if errorlevel 1 goto test68failed
goto test68done
:test68failed
echo Spencer test 68 failed
set failures=1
:test68done
echo -| ..\grep%1 -E -e "(a*)*" > nul
if errorlevel 1 goto test69failed
goto test69done
:test69failed
echo Spencer test 69 failed
set failures=1
:test69done
echo -| ..\grep%1 -E -e "(a*)+" > nul
if errorlevel 1 goto test70failed
goto test70done
:test70failed
echo Spencer test 70 failed
set failures=1
:test70done
echo -| ..\grep%1 -E -f msdos/test71.pat > nul
if errorlevel 1 goto test71failed
goto test71done
:test71failed
echo Spencer test 71 failed
set failures=1
:test71done
echo -| ..\grep%1 -E -f msdos/test72.pat > nul
if errorlevel 1 goto test72failed
goto test72done
:test72failed
echo Spencer test 72 failed
set failures=1
:test72done
echo ab| ..\grep%1 -E -f msdos/test73.pat > nul
if errorlevel 1 goto test73failed
goto test73done
:test73failed
echo Spencer test 73 failed
set failures=1
:test73done
echo ab| ..\grep%1 -E -f msdos/test74.pat > nul
if errorlevel 1 goto test74failed
goto test74done
:test74failed
echo Spencer test 74 failed
set failures=1
:test74done
echo ab| ..\grep%1 -E -f msdos/test75.pat > nul
if errorlevel 1 goto test75failed
goto test75done
:test75failed
echo Spencer test 75 failed
set failures=1
:test75done
echo cde| ..\grep%1 -E -e "[^ab]*" > nul
if errorlevel 1 goto test76failed
goto test76done
:test76failed
echo Spencer test 76 failed
set failures=1
:test76done
echo -| ..\grep%1 -E -e "(^)*" > nul
if errorlevel 1 goto test77failed
goto test77done
:test77failed
echo Spencer test 77 failed
set failures=1
:test77done
echo -| ..\grep%1 -E -f msdos/test78.pat > nul
if errorlevel 1 goto test78failed
goto test78done
:test78failed
echo Spencer test 78 failed
set failures=1
:test78done
echo -| ..\grep%1 -E -e ")(" > nul
if not errorlevel 2 goto test79failed
goto test79done
:test79failed
echo Spencer test 79 failed
set failures=1
:test79done
echo | ..\grep%1 -E -e "abc" > nul
if errorlevel 2 goto test80failed
if not errorlevel 1 goto test80failed
goto test80done
:test80failed
echo Spencer test 80 failed
set failures=1
:test80done
echo | ..\grep%1 -E -e "abc" > nul
if errorlevel 2 goto test81failed
if not errorlevel 1 goto test81failed
goto test81done
:test81failed
echo Spencer test 81 failed
set failures=1
:test81done
echo | ..\grep%1 -E -e "a*" > nul
if errorlevel 1 goto test82failed
goto test82done
:test82failed
echo Spencer test 82 failed
set failures=1
:test82done
echo abbbcd| ..\grep%1 -E -e "([abc])*d" > nul
if errorlevel 1 goto test83failed
goto test83done
:test83failed
echo Spencer test 83 failed
set failures=1
:test83done
echo abcd| ..\grep%1 -E -e "([abc])*bcd" > nul
if errorlevel 1 goto test84failed
goto test84done
:test84failed
echo Spencer test 84 failed
set failures=1
:test84done
echo e| ..\grep%1 -E -f msdos/test85.pat > nul
if errorlevel 1 goto test85failed
goto test85done
:test85failed
echo Spencer test 85 failed
set failures=1
:test85done
echo ef| ..\grep%1 -E -f msdos/test86.pat > nul
if errorlevel 1 goto test86failed
goto test86done
:test86failed
echo Spencer test 86 failed
set failures=1
:test86done
echo -| ..\grep%1 -E -f msdos/test87.pat > nul
if errorlevel 1 goto test87failed
goto test87done
:test87failed
echo Spencer test 87 failed
set failures=1
:test87done
echo abcdefg| ..\grep%1 -E -e "abcd*efg" > nul
if errorlevel 1 goto test88failed
goto test88done
:test88failed
echo Spencer test 88 failed
set failures=1
:test88done
echo xabyabbbz| ..\grep%1 -E -e "ab*" > nul
if errorlevel 1 goto test89failed
goto test89done
:test89failed
echo Spencer test 89 failed
set failures=1
:test89done
echo xayabbbz| ..\grep%1 -E -e "ab*" > nul
if errorlevel 1 goto test90failed
goto test90done
:test90failed
echo Spencer test 90 failed
set failures=1
:test90done
echo abcde| ..\grep%1 -E -f msdos/test91.pat > nul
if errorlevel 1 goto test91failed
goto test91done
:test91failed
echo Spencer test 91 failed
set failures=1
:test91done
echo hij| ..\grep%1 -E -e "[abhgefdc]ij" > nul
if errorlevel 1 goto test92failed
goto test92done
:test92failed
echo Spencer test 92 failed
set failures=1
:test92done
echo abcde| ..\grep%1 -E -f msdos/test93.pat > nul
if errorlevel 2 goto test93failed
if not errorlevel 1 goto test93failed
goto test93done
:test93failed
echo Spencer test 93 failed
set failures=1
:test93done
echo abcdef| ..\grep%1 -E -f msdos/test94.pat > nul
if errorlevel 1 goto test94failed
goto test94done
:test94failed
echo Spencer test 94 failed
set failures=1
:test94done
echo abcd| ..\grep%1 -E -f msdos/test95.pat > nul
if errorlevel 1 goto test95failed
goto test95done
:test95failed
echo Spencer test 95 failed
set failures=1
:test95done
echo abc| ..\grep%1 -E -f msdos/test96.pat > nul
if errorlevel 1 goto test96failed
goto test96done
:test96failed
echo Spencer test 96 failed
set failures=1
:test96done
echo abc| ..\grep%1 -E -e "a([bc]*)c*" > nul
if errorlevel 1 goto test97failed
goto test97done
:test97failed
echo Spencer test 97 failed
set failures=1
:test97done
echo abcd| ..\grep%1 -E -e "a([bc]*)(c*d)" > nul
if errorlevel 1 goto test98failed
goto test98done
:test98failed
echo Spencer test 98 failed
set failures=1
:test98done
echo abcd| ..\grep%1 -E -e "a([bc]+)(c*d)" > nul
if errorlevel 1 goto test99failed
goto test99done
:test99failed
echo Spencer test 99 failed
set failures=1
:test99done
echo abcd| ..\grep%1 -E -e "a([bc]*)(c+d)" > nul
if errorlevel 1 goto test100failed
goto test100done
:test100failed
echo Spencer test 100 failed
set failures=1
:test100done
echo adcdcde| ..\grep%1 -E -e "a[bcd]*dcdcde" > nul
if errorlevel 1 goto test101failed
goto test101done
:test101failed
echo Spencer test 101 failed
set failures=1
:test101done
echo adcdcde| ..\grep%1 -E -e "a[bcd]+dcdcde" > nul
if errorlevel 2 goto test102failed
if not errorlevel 1 goto test102failed
goto test102done
:test102failed
echo Spencer test 102 failed
set failures=1
:test102done
echo abc| ..\grep%1 -E -f msdos/test103.pat > nul
if errorlevel 1 goto test103failed
goto test103done
:test103failed
echo Spencer test 103 failed
set failures=1
:test103done
echo abcd| ..\grep%1 -E -e "((a)(b)c)(d)" > nul
if errorlevel 1 goto test104failed
goto test104done
:test104failed
echo Spencer test 104 failed
set failures=1
:test104done
echo alpha| ..\grep%1 -E -e "[A-Za-z_][A-Za-z0-9_]*" > nul
if errorlevel 1 goto test105failed
goto test105done
:test105failed
echo Spencer test 105 failed
set failures=1
:test105done
echo abh| ..\grep%1 -E -f msdos/test106.pat > nul
if errorlevel 1 goto test106failed
goto test106done
:test106failed
echo Spencer test 106 failed
set failures=1
:test106done
echo effgz| ..\grep%1 -E -f msdos/test107.pat > nul
if errorlevel 1 goto test107failed
goto test107done
:test107failed
echo Spencer test 107 failed
set failures=1
:test107done
echo ij| ..\grep%1 -E -f msdos/test108.pat > nul
if errorlevel 1 goto test108failed
goto test108done
:test108failed
echo Spencer test 108 failed
set failures=1
:test108done
echo effg| ..\grep%1 -E -f msdos/test109.pat > nul
if errorlevel 2 goto test109failed
if not errorlevel 1 goto test109failed
goto test109done
:test109failed
echo Spencer test 109 failed
set failures=1
:test109done
echo bcdd| ..\grep%1 -E -f msdos/test110.pat > nul
if errorlevel 2 goto test110failed
if not errorlevel 1 goto test110failed
goto test110done
:test110failed
echo Spencer test 110 failed
set failures=1
:test110done
echo reffgz| ..\grep%1 -E -f msdos/test111.pat > nul
if errorlevel 1 goto test111failed
goto test111done
:test111failed
echo Spencer test 111 failed
set failures=1
:test111done
echo -| ..\grep%1 -E -e "((((((((((a))))))))))" > nul
if errorlevel 2 goto test112failed
if not errorlevel 1 goto test112failed
goto test112done
:test112failed
echo Spencer test 112 failed
set failures=1
:test112done
echo a| ..\grep%1 -E -e "(((((((((a)))))))))" > nul
if errorlevel 1 goto test113failed
goto test113done
:test113failed
echo Spencer test 113 failed
set failures=1
:test113done
echo uh-uh| ..\grep%1 -E -e "multiple words of text" > nul
if errorlevel 2 goto test114failed
if not errorlevel 1 goto test114failed
goto test114done
:test114failed
echo Spencer test 114 failed
set failures=1
:test114done
echo multiple words, yeah| ..\grep%1 -E -e "multiple words" > nul
if errorlevel 1 goto test115failed
goto test115done
:test115failed
echo Spencer test 115 failed
set failures=1
:test115done
echo abcde| ..\grep%1 -E -e "(.*)c(.*)" > nul
if errorlevel 1 goto test116failed
goto test116done
:test116failed
echo Spencer test 116 failed
set failures=1
:test116done
echo (.*)\)| ..\grep%1 -E -e "\((.*)," > nul
if errorlevel 2 goto test117failed
if not errorlevel 1 goto test117failed
goto test117done
:test117failed
echo Spencer test 117 failed
set failures=1
:test117done
echo ab| ..\grep%1 -E -e "[k]" > nul
if errorlevel 2 goto test118failed
if not errorlevel 1 goto test118failed
goto test118done
:test118failed
echo Spencer test 118 failed
set failures=1
:test118done
echo abcd| ..\grep%1 -E -e "abcd" > nul
if errorlevel 1 goto test119failed
goto test119done
:test119failed
echo Spencer test 119 failed
set failures=1
:test119done
echo abcd| ..\grep%1 -E -e "a(bc)d" > nul
if errorlevel 1 goto test120failed
goto test120done
:test120failed
echo Spencer test 120 failed
set failures=1
:test120done
echo ac| ..\grep%1 -E -e "a[-]?c" > nul
if errorlevel 1 goto test121failed
goto test121done
:test121failed
echo Spencer test 121 failed
set failures=1
:test121done
echo beriberi| ..\grep%1 -E -e "(....).*\1" > nul
if errorlevel 1 goto test122failed
goto test122done
:test122failed
echo Spencer test 122 failed
set failures=1
:test122done
exit %%failures%%
