'
'       BAS-cubs v0.5 by Mateusz Viste <mateusz.viste@mail.ru>
'             Written and designed for FreeBASIC v0.18.2
'
'                 http://mateusz.viste.free.fr/dos
'
'
' This library is free software; you can redistribute it and/or
' modify it under the terms of the GNU General Public License
' as published by the Free Software Foundation; either version 2.1
' of the License, or (at your option) any later version.
'
' This library is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
' GNU General Public License for more details.
'
' You should have received a copy of the GNU General Public License
' along with this library; if not, write to the Free Software
' Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
'
'
' USAGE:  NLSMessage(x AS INTEGER, y AS INTEGER [,FallBack AS STRING])
'

#INCLUDE ONCE "file.bi"

DECLARE FUNCTION NLSMessage(N1 AS INTEGER, N2 AS INTEGER, FallBack AS STRING = "") AS STRING

FUNCTION NLSMessage(N1 AS INTEGER, N2 AS INTEGER, FallBack AS STRING) AS STRING

DIM AS INTEGER x, N3, N4, NLSFileHandler
DIM AS STRING NLSpath, NLSfile, NLSline, NLStemp, WorkString, CurName, Lang

REM *** Detecting the executable's name (without extension) ***
x = LEN(COMMAND(0))
N3 = 0
N4 = 0
DO
   IF MID(COMMAND(0), x, 1) = "." THEN N4 = x
   IF MID(COMMAND(0), x, 1) = "/" OR MID(COMMAND(0), x, 1) = "\" THEN N3 = x + 1
   IF x = 1 THEN
     IF N3 = 0 THEN N3 = x               ' Should never happen,
     IF N4 = 0 THEN N4 = LEN(COMMAND(0)) ' but who knows...
   END IF
   x -= 1
LOOP UNTIL N3 <> 0 AND N4 <> 0
CurName = MID(COMMAND(0), N3, N4 - N3)
REM *** executable's name detected ***

N3 = -1
N4 = -1
NLSfile = ""
NLSpath = ENVIRON("NLSPATH")
Lang = ENVIRON("LANG")

IF Lang <> "" THEN
  IF NLSpath <> "" THEN
    IF FileExists(NLSpath + "\" + Lang + "\" + CurName) <> 0 THEN NLSfile = NLSpath + "\" + Lang + "\" + CurName
    IF FileExists(NLSpath + "\" + CurName + "." + Lang) <> 0 THEN NLSfile = NLSpath + "\" + CurName + "." + Lang
  END IF
  IF FileExists(EXEPATH + "\" + CurName + "." + Lang) <> 0 THEN NLSfile = EXEPATH + "\" + CurName + "." + Lang
END IF

WorkString = FallBack

IF NLSfile <> "" THEN
  NLSFileHandler = FREEFILE
  OPEN NLSfile FOR INPUT AS NLSFileHandler
    DO
      LINE INPUT #NLSFileHandler, NLSline
      IF MID(NLSline, 1, 1) = "#" OR TRIM(NLSline) = "" THEN GOTO SkipComment
      x = 0
      NLStemp = ""
      DO
        x += 1
        IF MID(NLSline, x, 1) <> "." THEN NLStemp += MID(NLSline, x, 1)
      LOOP UNTIL MID(NLSline, x, 1) = "." OR x >= LEN(NLSline)
      IF x >= LEN(NLSline) THEN GOTO SkipComment
      N3 = VAL(NLStemp)
      NLStemp = ""
      DO
        x += 1
        IF MID(NLSline, x, 1) <> ":" THEN NLStemp += MID(NLSline, x, 1)
      LOOP UNTIL MID(NLSline, x, 1) = ":" OR x > LEN(NLSline)
      N4 = VAL(NLStemp)
      NLStemp = MID(NLSline, x + 1)
      IF N1 = N3 AND N2 = N4 THEN WorkString = NLStemp
      SkipComment:
    LOOP UNTIL EOF(NLSFileHandler) OR (N1 = N3 AND N2 = N4)
    CLOSE NLSFileHandler
END IF

RETURN WorkString
END FUNCTION
