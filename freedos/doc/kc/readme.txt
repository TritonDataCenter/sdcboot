     KC: a KEY language compiler
     ===========================

  ===========================================================================

   KC: compiles keyboard descriptors in KEY language to a KeybCB,
   wrapped in a KL file (for use of FD-KEYB 2.X)

   KLIB: groups KL files in libraries to be used by KEYBOARD.SYS

   Copyright (C) 2004 by Aitor SANTAMARIA_MERINO
   aitorsm@inicia.es

   Version  : 2.00
   Last edit: 2005-08-13 (ASM)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc., 
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  ===========================================================================



0.- INTRODUCTION


KC is a compiler of keyboard descriptor, that produces a binary form (a
KeybCB) from a text description in the KEY language.

The output can be a raw KeybCB, or alternatively, a complete KL file, useable
for the FD-KEYB 2.0 keyboard driver.

KLIB is a librarian for KL files: groups KL files into a single library file.
KLIB allows you to create libraries, list, add and remove KL files inside a
library, and finally to make it no longer editable (close it).


0.1.- Who should use the software

The compiler is to be used for those that need to create customized keyboard
layout data files (KL files) for KEYB, or those that, for some reason, need to
create binary KeybCBs images from programs written in the KEY language,
descriptor of keyboard layouts suited for humans.

If you are an average user of KEYB you may easily find already compiled packs
of layouts, and you probably won't need to recompile them. However, you can use
the compiler to customise them.


0.2.- Compatibility

The compiler requires a PC compatible machine of any kind, with enough room in
the filesystem to create your output files.

The compiler is able to understand the standard KEY language, with a few
particularities that are described in this file. It is able to create bare
KeybCBs, and KL files with the format suited for FD-KEYB 2.X series.

The compiler comes with the KEYCODE program, that helps you scan the output of
the keyboard driver.


0.3.- Limitations

The binary size of each data section must be at most of 5000 bytes (and a
maximum of 32 data sections).

It admits a maximum of 16 submappings.

There are not known incompatibilities.


0.4.- Version

Versions of the binaries:
KC        Version 2.0
KLIB      Version 1.0
KEYCODE   Version 1.10

It understands the KEY language as explained in revision 1.20 of the
specification.

It creates KeybCBs as explained in the revision 2.00, and KL files according
to the revision 1.10 of those documents.


0.5.- Copyright

Versions 1.X and 2.X of this program are distributed under the GNU General
Public License version 2.0 or higher.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc., 
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


0.6.- Installation

The compiler is the standalone file KC.EXE. Copy it wherever you need it.

The librarian is the standalone file KLIB.EXE. Copy it whenever you need it.

The character/scancode scanner KEYCODE also comes in the standalone
KEYCODE.EXE.


0.7.- Known bugs and reporting bugs

There are not known bugs. If you want to report bugs, please fill a bug
report on the FreeDOS Bugzilla:

http://www.freedos.org/bugs/bugzilla/

Alternatively, write an email to

Aitor SANTAMARIA MERINO
aitorsm@inicia.es


0.8.- File history

2004-03-04 This documentation is written
2005-08-13 Documentation updated for KC 2.0

0.9.- Acknowledgements

I am very grateful for their help to:

* Henrique Peron, for his patience and enormous help in testing the compiler.
* Axel Frinke, Matthias Paul and T. Kabe for their valuable comments.
* T. Kabe for his help with the Japanese support.
* All the FreeDOS team, for their support and help.
* KEYB users and FD-xkeyb users, for their feedback.


1.0.- COMPILER USSAGE


This chapter describes the generalities of using the compiler.


1.1.- Commandline syntax

The commandline syntax of the compiler is given by

  KC   <sourcefilename> [<targetfilename>] [/B]
  KC   /?

<sourcefilename>   The name of the source file, in KEY language, to be
                   compiled.
                   If extension is omitted, KEY extension is appended
                   automatically
<targetfilename>   The name of the KL file to be written (unless /B is used)
                   If extension is omitted, KL extension is appended
                   automatically
/B                 [Bare] Creates a bare binary KeybCB image, instead of a KL
                   file
/?                 Shows fast help

The exit codes are 0 for ok, the syntax error number for syntax error, and
add 100 to the critical error code, for critical error codes of KC.


1.2.- Creation of target file

The target file (KL or bare KeybCB) is created by KC in two phases, where the
whole file is re-read in the two phases.

The first phase is the COMPILING phase, in which the data sections are parsed
and stored. These sections are KEYS, DIACRITICS and STRINGS.

The second phase is the LINKING phase, where the KeybCB is assembled, and the
KL header is optionally added.


2.- LANGUAGE SPECIFIC ISSUES


This chapter describes particularities of the KEY files created for KC.


2.1.- Regarding identifiers and reserved words

(1) KC is case insensitive, in the keywords (KEYS:, LSHIFT, etc) and in the
identifiers.

(2) Identifiers are a sequence of up to 20 characters (a-zA-Z0-9_)


2.2.- Options in the [General] section

The following options are added to the [GENERAL] section. These options are
used to fulfill data on the KL header, and thus, will be ignored if the /B
option is used.

Name=[<id>],<layoutname>

  This option is used to introduce the identifier word and string for the KL
  file, so that KEYB can correctly identify the KL file being used.
  Typically, the commandline for KEYB will be:

       KEYB  <layoutname>,[<codepage>][,<KLfilename>]  [/ID:<id>]

  Leaving blank the numeric <id> will turn it to 0. KEYB will not require the
  /ID: option in this case.

Flags=<flags>

  Where <flags> is a list of letters, each of which identifies a flag that must
  be turned on on the flag list for the KL file.
  The list of flags is given in the documentation for the KL file. The
  following table is an excerpt from the documentation. PLEASE ALWAYS CHECK
  the latest version of the documentation of the KL file format.
  The flags are used to force KEYB to load certain management modules that
  could be otherwise unloaded and not be available.
  Note that the compiler will usually set the required flags for you, and you
  won't need this option.
  However, you might want to force to load the APM commands ('A' flag) if you
  know that these commands will be used through the API (int 2Fh, AX=AD8Dh).

============================================================================
Flag    Value   Meaning
-----  ------- ------------------------------------------------------------
C  0    0001h   Diacritics. The KeybCB uses diacritics
S  1    0002h   Beep. The KeybCB beeps (beep command)
B  2    0004h   Basic commands. One command other than 0, 160 is used
E  3    0008h   Extended commands. One of these commands is used:
                 80..99, 120..139, 140..159, 162..199
L  4    0010h   Default Shifts. The KeybCB remaps the usual shift/lock keys
                somewhere else (shift, ctrl, alt, ...)
J  5    0020h   Request KEYB to load the Japanese extensions
A  7    0080h   Ussage of APM commands (150..154)
X  9    0200h   Strings. the KeybCB uses strings
============================================================================


2.3.- Specialities

The specialities described here are very specific to KC. Note that by using
them you will not be following the pure KEY specification, and your files may
break compatibility with other KEY software other than KC.
Specific advices are given to avoid breaking compatibility.

(1) In the PLANES section, "KanaLock" and "UserKey8" are synonims, because
FD-KEYB does not track the KanaLock flag separated from the User Key flags.
In most cases this won't be a problem, because it is rare that a keyboard
requires the ussage of KanaLock and OTHER user keys.
Should this be the case, avoid using UserKey1 (use UserKey2, UserKey3,...)
instead and you will be safe.

(2) You are allowed to enter scancodes over 128 to signify breakcodes, and the
compiler will not complain.
Note however that you are strongly recommended to use scancodes up to 127, and
use the 'R' flag to signify the breakcodes.
Notice that the rows with the 'R' flag MUST BE GROUPED at the bottom of the
KEYS section.

(3) In the KEYS section, the rows with 'R' flag are grouped at the end of the
section ordered ascending by scancode. The rest of the rows must also be
ordered ascending by scancode.

(4) When defining commands, the compiler gives you an extra facility for
entering commands belonging to certain family of commands. This is made by
adding a letter between the '!' command mark and the command number.
Thus, the "!C" sequence is to denote the diacritic sequences, and you can use
"!C1" as a short for "!200".
If you want to maintain KEY file compatibility, avoid using these sequences.

The following is the list of allowable sequences:

   Sequence   Real command  Meaning
   ---------- ------------- ----------------------------------------------
   !Cn        n+199         n-th diacritic sequence starter
   !Ln        n+99          Switch to n-th layout
                            (n=0: turn KEYB off)
   !Sn        n+119         Switch to n-th submapping
   !An        n+169         ALTernative character introduction, cypher n
   !On        n+187         Turn n-th user key ON
   !Hn        n+179         Turn n-th user key OFF
   -----------------------------------------------------------------------


3.- ADVANCED TOPICS


This section covers several aspects regarding optimization of the KEY files,
and other advanced topics, such as building the compiler.

Please bear in mind that KC does not make automatic optimisations of any kind.
The guidelines of this section will help you optimise the KEY files yourself.
Beware that at times, you won't be able to optimise for both memory and speed,
and you will have to choose.


3.1.- Ordering and optimising for speed

By changing the ordering of the elements in the KEY file, you will be able to
create identical and compatible files, that nevertheless, may make the
keyboard management faster and better responsive to key pressing, reducing the
chance of missing key presses.

(1) The diacritics and strings are accessed sequentially from the begining.
Thus, place those diacritics and strings that will be used more often first.

(2) Plane computation is made in the same order that appears in the [PLANES]
section. Thus, place the most likely planes first.

(3) The order of the particular submappings is important, specially because of
the codepage numbers.
First remember the semantical restriction that a '0' in the codepage will mean
that it can be used for ANY codepage.
When KEYB changes codepage (in particular, when KEYB boots), it will start from
the first particular submapping. Thus, after the first submapping with codepage
'0' that is found, the condition for finding an appropriate codepage will be
fullfilled, and the remaining submappings will never be accessed through a
codepage change (but will do through the submapping change commands).
Notice that changing with these commands to a submapping with a different
codepage will leave KEYB in an inconsistent state. KC will allow you to do
this, but will noneteless complain about it.
Appart from that, the search is sequential from the begining, so be sure to
place the most frequent codepages first.
Finally, notice that if no codepage is given by commandline (or by DISPLAY.SYS)
KEYB will use the first particular submapping as active, and thus its codepage
becomes the codepage of KEYB.


3.2.- Optimising for memory ussage.

Note that FD-KEYB is designed to recycle, as much as possible, BIOS resources.
Keys or key combinations (that is, scancode/plane entries) that are not found
in the appropriate KEYS tables are left for BIOS to processing. Thus you should
try to leave for BIOS the behaviour of the keys that doesn't change in the
layout that you are trying to define.

Just one more word about it: BIOS does NOT know about diacritics. Thus, you
should make sure that all the "second character"s on the DIACRITICs sections
will be caught by KEYB.

The best way to try and reduce the memory ussage is to be care of the column
ordering of the Key tables.
Notice first that the less entries a lookup table row has, the smaller the
resulting file is. Thus, try to avoid the trailing !0's. Thus, in a file with
five planes (three user defined), the entry

8  !0  /  !0  !0  !0

may well be abbreviated to

8  !0  /

Notice, therefore, that you can save more memory by optimising the ordering of
planes in the file. Suppose that you have

7  a  A  !0  x
8  b  B  !0  y
9  c  C  z

If you switch the order of planes 3 and 4, you will be able to encode that
table as

7  a  A  x
8  b  B  y
9  c  C  !0 z

which will certainly save 1 byte. In general, many bytes may be saved by
playing with the plane ordering.


3.3.- Smartlinking

KC compiler features smartlinking. By this it is meant that, although all the
data sections (KEYS, DIACRITICS, STRINGS) are parsed and checked for
correctedness, those data sections that are NOT used in the submappings section
will not be included into the resulting KeybCB file (because would be unused).


3.4.- Building the compiler

The compiler is composed of a single C source code file, compatible with the
Borland C++ 3.0 compiler. 

It has been compiled in the compact memory model, and UPX-ed to reduce its
size.


4.0.- LIBRARIAN USSAGE


This chapter describes the generalities of using the librarian.


4.1.- Commandline syntax

The syntax for using the librarian is:

KLIB  lib_filename [{+|-}KL_filename] [/B] [/Z] [/M=manufacturer] [/D=Desc]
KLIB  /?

lib_filename      The name of the library file

KL_filename       Name of the KL file to be added or removed
                  Use + to ADD the KL file (the file should exist)
                  Use - to REMOVE the KL file (it should be in the library)

/B                Avoids creating backup files

/Z                After the operation the library is closed and not any
                  longer editable

Manufacturer      Name of the library manufacturer (creation only)

Desc              Description of the library (creation only)

The exit codes are 0 for ok, or the error listed below.


4.2.- Operation mode

The first operation you do is to create the library. You can use

KLIB lib_filename

to do so. You will be asked to introduce the manufacturer name and a brief
description. Of course, the given lib_filename must not exist.

Once created, you use the +KL_filename to add file_names to it. You can as
well add KL files as you create the library.

For both methods of creating the library you can include the /D and /M
options to avoid being asked for them.

When you have created the library, you can see its contents (not adding any
+KL_filename or -KL_filename). You can also add or remove KL filenames from
it.

Finaly, you can close the library, make it non-editable, by using the /Z
option.

It might happen that, under certain security switches on some versions of
KEYB only closed libraries are affected. This is a security measure to
avoid it to be edited after creation.


5.- EXAMPLES

We propose three examples to illustrate the potential of the compiler and the
KEY files.

5.1.- The standard PC keyboard

The following example illustrates a standalone PC keyboard driver that does not
make use of the BIOS, but of the FD-KEYB commands.

There are little punctual missbehaviours (at times limited by the enormous
number of planes required by fulkl management), but at least this would well
illustrate the use of commands. For example, the key scroll-lock is undefined,
because of the enormous number of planes that would be required.

Windows keys and multimedia keys are not considered either.

==============================================================================
[General]
Name=,PC
DecimalChar=.

[Submappings]
0    -
437  k

[Planes]
Ctrl       |  Alt
Alt        |  Ctrl
Ctrl Alt
E0         |  Shift  Alt Ctrl
E0  Shift  |  Alt    Ctrl


[KEYS:k]
  1  #27 #27  #27 #0  #0
  2S  2/1  2/#33  2/#0  120/#0  120/#0
  3S  3/2  3/@    3/#0  121/#0  121/#0
  4S  4/3  4/#35  4/#0  122/#0  122/#0
  5S  5/4  5/$    5/#0  123/#0  123/#0
  6S  6/5  6/%    6/#0  124/#0  124/#0
  7S  7/6  7/^    7/#30 125/#0  125/#0
  8S  8/7  8/&    8/#0  126/#0  126/#0
  9S  9/8  9/*    9/#0  127/#0  127/#0
 10S  10/9 10/(   10/#0 128/#0  128/#0
 11S  11/0 11/)   11/#0 129/#0  129/#0
 12S  12/- 12/_   12/#31 130/#0 130/#0
 13S  13/= 13/+   13/#0 131/#0  131/#0
 14  #8  #8 #127  #0  #0
 15  #9  #0   #0  #0  #0
 16C  q  Q   #17  #0  #0
 17C  w  W   #23  #0  #0
 18C  e  E    #5  #0  #0
 19C  r  R   #18  #0  #0
 20C  t  T   #20  #0  #0
 21C  y  Y   #25  #0  #0
 22C  u  U   #21  #0  #0
 23C  i  I    #9  #0  #0
 24C  o  O   #15  #0  #0
 25C  p  P   #16  #0  #0
 26  [   {   #27  #0  #0
 27  ]   }   #29  #0  #0
 28  #13 #13 #10  #0  #0  #13  #13
 29  !94 !94 !94 !94 !94  !90  !90
R29  !84 !84 !84 !84 !84  !80  !80
 30C  a  A    #1  #0  #0
 31C  s  S   #19  #0  #0
 32C  d  D    #4  #0  #0
 33C  f  F    #6  #0  #0
 34C  g  G    #7  #0  #0
 35C  h  H    #8  #0  #0
 36C  j  J   #10  #0  #0
 37C  k  K   #11  #0  #0
 38C  l  L   #12  #0  #0
 39   ;  :    #0  #0  #0
 40   '  #34  #0  #0  #0
 41   `  ~    #0  #0  #0
 42 !93 !93  !93 !93 !93
R42 !83 !83  !83 !83 !83
 43   \  |   #28  #0  #0
 44C  z  Z   #26  #0  #0
 45C  x  X   #24  #0  #0
 46C  c  C    #3  #0  #0
 47C  v  V   #22  #0  #0
 48C  b  B    #2  #0  #0
 49C  n  N   #14  #0  #0
 50C  m  M   #13  #0  #0
 51   ,  <    #0  #0  #0
 52   .  >    #0  #0  #0
 53   /  ?    #0  #0  #0    /   /
 54 !92 !92  !92 !92 !92
R54 !82 !82  !82 !82 !82
 55S   55/*  55/#0   114/#0  55/#0  55/#0  55/!143 55/!140
 56 !95 !95  !95 !95 !95   !91 !91
R56 !85 !85  !85 !85 !85   !81 !81
 57  #32 #32 #32 #32 #32
 58 !98 !88  !98 !98 !98
 59S  59/#0 84/#0  94/#0  104/#0 104/!100
 60S  60/#0 85/#0  95/#0  105/#0 105/!101
 61S  61/#0 86/#0  96/#0  106/#0 106/#0
 62S  62/#0 87/#0  97/#0  107/#0 107/#0
 63S  63/#0 88/#0  98/#0  108/#0 108/#0
 64S  64/#0 89/#0  99/#0  109/#0 109/#0
 65S  65/#0 90/#0  100/#0 110/#0 110/#0
 66S  66/#0 91/#0  101/#0 111/#0 111/#0
 67S  67/#0 92/#0  102/#0 112/#0 112/#0
 68S  68/#0 93/#0  103/#0 113/#0 113/#0
 69  !97 !87 !163 !97 !97
 70 !160 !160 !142 !160 !160
 71NS  71/#0  71/7   119/#0 71/!177 71/#0   71/#0  71/#0
 72NS  72/#0  72/8   141/#0 72/!178 72/#0   72/#0  72/#0
 73NS  73/#0  73/9   132/#0 73/!179 73/#0   73/#0  73/#0
 74   -   -   #0  #0  #0
 75NS  75/#0  75/4   115/#0 75/!174 75/#0   75/#0  75/#0
 76NS  76/#0  76/5   143/#0 76/!175 76/#0   76/#0  76/#0
 77NS  77/#0  77/6   116/#0 77/!176 77/#0   77/#0  77/#0
 78   +   +   #0  #0  #0
 79NS  79/#0  79/1   117/#0 79/!171 79/#0   79/#0  79/#0
 80NS  80/#0  80/2   145/#0 80/!172 80/#0   80/#0  80/#0
 81NS  81/#0  81/3   118/#0 81/!173 81/#0   81/#0  81/#0
 82NS  82/#0  82/0   146/#0 82/!170 82/#0   82/#0  82/#0
 83NS  83/#0 83/#161 147/#0 83/#0   83/!151 83/#0  83/#0
 86   \   |   #0  #0  #0
 87S   87/#0 135/#0   137/#0  139/#0  139/#0
 88S   88/#0 136/#0   138/#0  140/#0  140/#0
==============================================================================


5.2.- How to improve accessibility for the alternative character introduction

The alternative character introduction is a powerful way to produce characters
that are not represented in the keyboard by using the ordering byte in the
current codepage table.

Whereas it is usually performed by pressing Left-Alt, then the numeric pad
keys and finally releasing Alt, it may be tricky for disabled people to
acceed to these contents.

The following trick will remap the introduction to the function keys, that are
normally useless under DOS. To introduce such a character, press the
appropriate numbers in the function keys (F5 for the number 5), and finally,
press LAlt to produce the number.

Replace or add these lines to your keyboard layout.

============================================
 59S  !171/#0 84/#0  94/#0  104/#0 104/!100
 60S  !172/#0 85/#0  95/#0  105/#0 105/!101
 61S  !173/#0 86/#0  96/#0  106/#0 106/#0
 62S  !174/#0 87/#0  97/#0  107/#0 107/#0
 63S  !175/#0 88/#0  98/#0  108/#0 108/#0
 64S  !176/#0 89/#0  99/#0  109/#0 109/#0
 65S  !177/#0 90/#0  100/#0 110/#0 110/#0
 66S  !178/#0 91/#0  101/#0 111/#0 111/#0
 67S  !179/#0 92/#0  102/#0 112/#0 112/#0
 68S  !170/#0 93/#0  103/#0 113/#0 113/#0
============================================


5.3.- Absolute capital lock

Some German keyboard layouts from Microsoft KEYB introduced some features that
made the keyboard behave in a similar way as a typewritter's machine does.

One of these features is the fact that the CapsLock status affects the whole of
the symbol keys.

This can be easily reproduced by adding the 'C' flag to all the symbolic keys.
For example, replace

  2S   2/1  2/#33  2/#0  120/#0  120/#0

with

  2CS  2/1  2/#33  2/#0  120/#0  120/#0


== APPENDIX =================================================================


APPENDIX A: The KeyCode program


The compiler KC comes with a small program called KEYCODE, which can be used to
determine the scancode/character pair that is produced by a keyboard enhancer
(such as FD-KEYB) into the BIOS keyboard buffer.

To execute it, just enter KEYCODE in the commandline.
Press the keys or key combinations that you want to guess.

To exit, press Esc or F1


APPENDIX B: Scancode reference

We excerpt two documents that list the scancodes of keys. Also note the
following two good online references:

  http://panda.cs.ndsu.nodak.edu/~achapwes/PICmicro/keyboard/scancodes1.html
  http://www.geocities.com/SiliconValley/Program/6366/keyboard/keyboard002.htm

The following table is an excerpt from the Ralf Brown Interrupt List, ver. 61
http://www-2.cs.cmu.edu/afs/cs/user/ralf/pub/WWW/files.html

==============================================================================
(Table 00006)
Values for keyboard make/break (scan) code:
 01h	Esc		 31h	N
 02h	1 !		 32h	M
 03h	2 @		 33h	, <		 63h	F16
 04h	3 #		 34h	. >		 64h	F17
 05h	4 $		 35h	/ ?		 65h	F18
 06h	5 %		 36h	Right Shift	 66h	F19
 07h	6 ^		 37h	Grey*		 67h	F20
 08h	7 &		 38h	Alt		 68h	F21 (Fn) [*]
 09h	8 *		 39h	SpaceBar	 69h	F22
 0Ah	9 (		 3Ah	CapsLock	 6Ah	F23
 0Bh	0 )		 3Bh	F1		 6Bh	F24
 0Ch	- _		 3Ch	F2		 6Ch	--
 0Dh	= +		 3Dh	F3		 6Dh	EraseEOF
 0Eh	Backspace	 3Eh	F4
 0Fh	Tab		 3Fh	F5		 6Fh	Copy/Play
 10h	Q		 40h	F6
 11h	W		 41h	F7
 12h	E		 42h	F8		 72h	CrSel
 13h	R		 43h	F9		 73h	<delta> [*]
 14h	T		 44h	F10		 74h	ExSel
 15h	Y		 45h	NumLock		 75h	--
 16h	U		 46h	ScrollLock	 76h	Clear
 17h	I		 47h	Home		 77h	[Note2] Joyst But1
 18h	O		 48h	UpArrow		 78h	[Note2] Joyst But2
 19h	P		 49h	PgUp		 79h	[Note2] Joyst Right
 1Ah	[ {		 4Ah	Grey-		 7Ah	[Note2] Joyst Left
 1Bh	] }		 4Bh	LeftArrow	 7Bh	[Note2] Joyst Up
 1Ch	Enter		 4Ch	Keypad 5	 7Ch	[Note2] Joyst Down
 1Dh	Ctrl		 4Dh	RightArrow	 7Dh	[Note2] right mouse
 1Eh	A		 4Eh	Grey+		 7Eh	[Note2] left mouse
 1Fh	S		 4Fh	End
 20h	D		 50h	DownArrow
 21h	F		 51h	PgDn
 22h	G		 52h	Ins
 23h	H		 53h	Del
 24h	J		 54h	SysReq		---non-key codes---
 25h	K		 55h	[Note1] F11	 00h	kbd buffer full
 26h	L		 56h	left \| (102-key)
 27h	; :		 57h	F11		 AAh	self-test complete
 28h	' "		 58h	F12		 E0h	prefix code
 29h	` ~		 59h	[Note1] F15	 E1h	prefix code
 2Ah	Left Shift	 5Ah	PA1		 EEh	ECHO
 2Bh	\ |		 5Bh	F13 (LWin)	 F0h	prefix code (key break)
 2Ch	Z		 5Ch	F14 (RWin)	 FAh	ACK
 2Dh	X		 5Dh	F15 (Menu)	 FCh	diag failure (MF-kbd)
 2Eh	C					 FDh	diag failure (AT-kbd)
 2Fh	V					 FEh	RESEND
 30h	B					 FFh	kbd error/buffer full
Notes:	scan codes 56h-E1h are only available on the extended (101/102-key)
	  keyboard and Host Connected (122-key) keyboard; scan codes 5Bh-5Dh
	  are only available on the 122-key keyboard and the Microsoft Natural
	  Keyboard; scan codes 5Eh-76h are only available on the 122-key
	  keyboard
	in the default configuration, break codes are the make scan codes with
	  the high bit set; make codes 60h,61h,70h, etc. are not available
	  because the corresponding break codes conflict with prefix codes
	  (code 2Ah is available because the self-test result code AAh is only
	  sent on keyboard initialization).  An alternate keyboard
	  configuration can be enabled on AT and later systems with enhanced
	  keyboards, in which break codes are the same as make codes, but
	  prefixed with an F0h scan code
	prefix code E0h indicates that the following make/break code is for a
	  "gray" duplicate to a key which existed on the original PC keyboard;
	  prefix code E1h indicates that the following make code has no
	  corresponding break code (currently only the Pause key generates no
	  break code)
	the Microsoft Natural Keyboard sends make codes 5Bh, 5Ch, and 5Dh
	  (all with an E0h prefix) for the Left Windows, Right Windows, and
	  Menu keys on the bottom row
	the European "Cherry G81-3000 SAx/04" keyboard contains contacts for
	  four additional keys, which can be made available by a user
	  modification; the three new keys located directly below the cursor
	  pad's Delete, End, and PgDn keys send make codes 66h-68h (F19-F21);
	  the fourth new key, named <delta>, sends make code 73h
	the SysReq key is often labeled SysRq
	the "Accord" ergonomic keyboard with optional touchpad (no other
	  identification visible on keyboard or in owner's booklet) has an
	  additional key above the Grey- key marked with a left-pointing
	  triangle and labeled "Fn" in the owner's booklet which returns
	  scan codes E0h 68h on make and E0h E8h on break
	the "Preh Commander AT" keyboard with additional F11-F22 keys treats
	  F11-F20 as Shift-F1..Shift-F10 and F21/F22 as Ctrl-F1/Ctrl-F2; the
	  Eagle PC-2 keyboard with F11-F24 keys treated those additional keys
	  in the same way
	[Note1] the "Cherry G80-0777" keyboard has additional F11-F15 keys
	  which generate make codes 55h-59h; some other extended keyboards
	  generate codes 55h and 56h for F11 and F12, which cannot be managed
	  by standard DOS keyboard drivers
	[Note2] the Schneider/Amstrad PC1512 PC keyboards contain extra keys,
	  a mouse, and a digital joystick, which are handled like extra keys.
	  The joystick's motion scancodes are converted into standard arrow
	  keys by the BIOS, and the joystick and mouse button scan codes are
	  converted to FFFFh codes in the BIOS keyboard buffer
	  (see CMOS 15h"AMSTRAD").
	  In addition to the keys listed in the table above, there are
	    Del-> (delete forward)	70h
	    Enter			74h
==============================================================================


Finally, this table, listing the special multimedia keys, is courtesy of
Matthias Paul (co-author of FreeKEYB). This link contains further information
about FreeKEYB, an alternative free keyboard enhancer:

  http://www.freekeyb.org/

(note: the driver is not configurable through KL files, and it is independent
of FD-KEYB).

==============================================================================
Scancodes in hexadecimal / all of them are E0-prefixed:

  Power			5E
  Sleep			5F
  Wake			63
  Next track		19
  Previous track	10
  Stop			24
  Play/Pause		22
  Mute			20
  Volume Up		30
  Volume Down		2E
  Media Select		6D
  E-Mail		6C
  Calculator		21
  My computer		6B
  WWW Search		65
  WWW Home		32
  WWW Back		6A
  WWW Forward		69
  WWW Stop		68
  WWW Refresh		67
  WWW Favourites	66

(courtesy of Matthias Paul)
==============================================================================


APPENDIX C: Compiler error reference

What follows is a list of compiler errors and possible causes.

SYNTAX ERRORS

1: Unknown flag

  Flag character for KEY sequence was not recognised. The only allowed chars
  are N, C, S, X

2: Syntax error

  Generic syntax error. Please check the documentation about the KEY language

3: Too many planes

  For a single scancode, only a maximum of 10 planes can be defined.

5: Unexpected EOL

  The line finished before it was expected. Please check the KEY language
  syntax

6: Too many Diacritic pairs

  Only up to 128 pairs of characters can be defined for each diacritic
  sequence

7: Invalid Key Name

  Given identifier was not recognised as a key identifier in the string

8: Invalid Function key name

  Given identifier was not recognised as a function key identifier in the
  string

9: Too many |'s

  Only one separator can be used for each PLANE defining row

10: Invalid Shift Key Name

  Given identifier was not recognised as a plane defining keyword

11: Identifier name too long

  Given identifier name is too long (must be limited to a maximum of 20
  characters)

12: Too many identifiers

  The submapping row admits only up to three identifiers (and the codepage)

13: Wrong name specification

  Syntax error specifying the name. Remember to always include the ','
  separator sign although you leave the id blank

14: Too many decimal chars

  You can define the decimal char only once in the file

15: Error in the definition of the decimal char

  There is a syntax error in the "DecimalChar=" option

17: Wrong data section name (1-19 chars)

  The data section should be finished by ']', and the identifier should have
  at least 1 character and at most 19.

18: Section header expected

  The header of a new section was expected

19: If flags C or N are used, then you must define at least 2 columns

  If you use the flags C or N in a KEYB section then you must define at least
  2 columns (planes) for that scancode

20: Separator character / expected

  The separator character '/' separating the scancode and character is required


CRITICAL ERRORS

1:  Wrong number of parameters specified

  At least one parameter is required

2:  Unable to open source file name

  The source (KEY) file couldn't be open

3:  Unable to open target file name

  The target file couldn't be open

6:  Syntax error

  Syntax error in one of the parameters

7:  Unknown switch

  Unknown switch (only /? and /B are admissible)

8:  Could't assign memory for block

  Memory for the data block couldn't be allocated. Please contact the author
  about this problem.

9:  Too many data blocks

  You can't define more than 32 data blocks.

12: Only ONE PLANES section is alowed

  You can define a single PLANES section for the whole file.

13: Only ONE SUBMAPPINGS sections is allowed

  You can define a single SUBMAPPINGS section for the whole file.

14: Only ONE GENERAL section is allowed

  You can define a single GENERAL section for the whole file.

15: Missing layout name option

  You must define at least one "NAME=" option

16: Section [SUBMAPPINGS] is mandatory for linking

  You need at least one SUBMAPPINGS section

17: Section [GENERAL] is mandatory for linking

  You need at least one GENERAL section

18: Wrong submapping change number in line

  The submapping you are trying to change to is not defined.

19: Too many diacritics

  The maximum number of definable diacritic lines is 35

20: Too many strings

  The maximum number of definable strings is 79

21: Too many planes

  The maximum number of definable additional planes is 8

22: Too many submappings

  The maximum number of submappings is 15

23: At least one particular submapping is required

  You must define at least one particular submappings (that is at least two
  submappings)

24: Number of predefined planes is exceeded on line XXX

  You can define at most as many columns as the planes defined.


APPENDIX D: Librarian error messages

1:  Unknown switch

  Unknown switch (only /B, /Z, /M, /D are admissible)

2:  Syntax error

  Syntax error in one of the parameters

3: Wrong number of parameters specified

  You have specified too few or too many parameters.

4: Too many actions (one action as a time)

  You can only add or remove a KL file at a time

5: Too many libraries were specified

  You can only operate on one library at a time

6: Missing required library file

  You must specify the name of the library

7: KL file to be added was not found

  The KL file that you want to add was not found

8: Incorrect version, or library file is damaged or has errors

  An error ocurred when opening the library: it is corrupted, or it
  corresponds to an unsupported version

9: Error creating temporary or BAK file

  There was a problem trying to create the backup file

10: Error when opening the library file

  The OS reported errors when trying to open the library file

11: Invalid checksum for file

  There was a checksum error for the KL file inside the library.
  The KL file there is possibly corrupt.

12: Error when reading from library file.

  The OS reported errors when trying to load from the library file.

13: Library file not found. Can't remove layouts.

  If you are removing layouts, the library must exist.

14: Layout in library corrupt.

  There is a layout in the library which is corrupt and can't be read.

15: Permission denied: library is marked readonly.

  The library has been closed and you can no longer edit it.

16: Such layout file has been already added to library.

  The KL file that you want to insert is already in the library

17: Incorrect version of the KL file, or it has damaged or has errors

  An error ocurred when opening the KL file: it is corrupted, or it
  corresponds to an unsupported version

18: All operations ok, but couldn't remove BAK file

  Everything was made ok, but the OS reported errors when trying to
  remove the backup file.



APPENDIX E: The KEY language reference for KC

<KEY file> = 
    <section>
    [<section>]
    ...

<section>= <datasection> | <logicalsection>

<datasection> = <keyssection> | <diacriticsection> | <stringsection>

<keyssection> = 
    [KEYS:<identifier>]
    <keysrow>
    [<keysrow>]
    ...

<identifier>=<char>[<char>]...

<char> = a-zA-Z0-9_

<cypher> = 0-9

<number> = <cypher>[<cypher>]...

<keysrow> = <charrow> | <pairrow> | <blockrow>

<charrow> = [R]<number>[<KEYSflag>]... <keybCHARcom> [<keybCHARcom>]...

<KEYSflag> = N | C

<keybCHARcom> = <keybCHAR> | <keybCOM>

<keybCHAR> = <singlechar> | ## | #! | #<number>

<singlechar> = character except #!

<keybCOM> = ![C|L|S|A|O|H]<number>

<pairrow> =  [R]<number>[<KEYSflag>]...S[<KEYSflag>]... <keybPAIR> [<keybPAIR>]...

<keybPAIR> = <number>/<keybCOM>

<blockrow> = [R]<number>X

<diacriticssection> = 
    [DIACRITICS:<identifier>]
    <diacriticsrow>
    [<diacriticsrow>]
    ...

<diacriticsrow> =  <keybCHAR> <keybCHAR><keybCHAR> [<keybCHAR><keybCHAR>] ...

<stringssection> = 
    [STRINGS:<identifier>]
    <KEYBstring>

<KEYBstring> = <stringchar>[<stringchar>]...

<stringchar>=<stringsinglechar> | \\ | \<stringpair>

<stringpair> = \K{<number>,<number>} | \S{<number>} | \C{<number>} | \n | \[<stringkeyname>]

<stringkeyname> = HOME | END | PGUP | PGDN | INS | DEL | UP | DOWN | LEFT | RIGHT | 
                  F<number> | CF<number> | AF<number> | SF<number>

<logicalsection> = <generalsection> | <planessection> | <submappingssection>

<generalsection> =
   [GENERAL]
   <generalrow>
   [<generalrow>]
   ...

<generalrow> = NAME=<namestring> | DECIMALCHAR=<keybCHAR> | FLAGS=<generalflag> | <ANY OTHER STRING 3rd party defined>

<namestring> = [<number>],<identifier>

<generalflag> = C|S|B|E|L|J|A|X

<planessection> =
    [PLANES]
    <planesrow>
    [<planesrow>]
    ...

<planesrow> = [<modifierkeyname>] ... [ | [<modifierkeyname>] ... ]

<modifierkeyname> = LSHIFT | RSHIFT | SHIFT | LCTRL | RCTRL | CTRL | LALT | ALTGR | ALT | E0 |
                    SCROLLLOCK | NUMLOCK | CAPSLOCK | KANALOCK | USERKEY<cypher>


<submappingssection> =
    [SUBMAPPINGS]
    <submappingsrow>
    [<submappingsrow>]
    ...

<submappingsrow> =  <number> <secname> [<secname>] ...

<secname> = - | <identifier>
