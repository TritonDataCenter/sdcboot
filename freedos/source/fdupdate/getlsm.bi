'
' GETLSM.BI - a FreeBASIC library which allows to easily extract informations
'             from *.LSM files.
'
' Written by Mateusz Viste <mateusz.viste@mail.ru>, using FreeBASIC v0.18.3
' License: LGPL
'
' How to use the GetLSM function?
' GetLSM(LSM_File AS STRING, LSM_Field AS STRING) AS STRING
'
' Where: LSM_File is the name of the LSM file to be read (eg. "C:\INFO.LSM")
'        LSM_Field is the name of the field to read (eg. "Version")
'
' The function returns the field's content. If the file or the field doesn't
' exist, or the LSM file is broken, then the function returns an empty
' string.
'
' If you would like to check the GETLSM version, just call the GETLSMversion
' constant.
'

DECLARE FUNCTION GetLSM(LSMfile AS STRING, LSMField AS STRING) AS STRING

CONST GETLSMversion = "0.91"


PRIVATE FUNCTION FoxTrim(Stuff AS STRING) AS STRING
 DIM Wynik AS STRING
 Wynik = TRIM(Stuff)
 Wynik = TRIM(Wynik, CHR(9))
 RETURN Wynik
END FUNCTION


FUNCTION GetLSM(LSMfile AS STRING, LSMField AS STRING) AS STRING
 DIM AS STRING LsmReturnString, LsmFieldName, LsmFieldData, LsmTmpBuffer
 DIM AS INTEGER LsmFileHandler, LsmColonPos

 LsmFileHandler = FREEFILE
 LsmReturnString = ""
 IF DIR(LSMfile) <> "" THEN
   OPEN LSMfile FOR INPUT AS LsmFileHandler
   LINE INPUT #LsmFileHandler, LsmTmpBuffer
   IF UCASE(LsmTmpBuffer) <> "BEGIN3" THEN GOTO NotLsmFormat ' Checking for the correct LSM header
   DO
     LINE INPUT #LsmFileHandler, LsmTmpBuffer
     IF MID(LsmTmpBuffer, 1, 1) = " " OR MID(LsmTmpBuffer, 1, 1) = CHR(9) THEN
         LsmFieldData += " " + FoxTrim(LsmTmpBuffer)
       ELSE
         IF UCASE(LsmFieldName) = UCASE(LSMField) THEN
             LsmReturnString = LsmFieldData
             LsmTmpBuffer = "END"
           ELSE
             LsmColonPos = INSTR(LsmTmpBuffer, ":")
             LsmFieldName = MID(LsmTmpBuffer, 1, LsmColonPos - 1)
             LsmFieldData = FoxTrim(MID(LsmTmpBuffer, LsmColonPos + 1))
         END IF
     END IF
   LOOP UNTIL UCASE(LsmTmpBuffer) = "END" OR EOF(LsmFileHandler)
   NotLsmFormat:
   CLOSE #LsmFileHandler
 END IF
 RETURN LsmReturnString
END FUNCTION
