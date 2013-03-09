'
' READCFG.BI - a FreeBASIC library which allows to read a configuration file.
'
' Written by Mateusz Viste <mateusz.viste@mail.ru>, using FreeBASIC v0.18.3
' License: LGPL
'
' How to use the ReadCFG function?
' ReadCFG(CFG_File AS STRING, CFG_Field AS STRING) AS STRING
'
' Where: CFG_File is the name of the conf file to be read (eg. "C:\PROG.CFG")
'        CFG_Field is the name of the field to read (eg. "Version")
'
' The function returns the field's content. If the file or the field doesn't
' exist, then the function returns an empty string.
'
' The configuration file should have the following structure:
' conf1=setting1
' conf2=setting2
' conf3=setting3
' conf4=setting4
' ...
'

DECLARE FUNCTION ReadCFG(CFGfile AS STRING, CFGField AS STRING) AS STRING

FUNCTION ReadCFG(CFGfile AS STRING, CFGField AS STRING) AS STRING
 DIM AS STRING CfgReturnString, CfgFieldName, CfgFieldData, CfgTmpBuffer
 DIM AS INTEGER CfgFileHandler, CfgColonPos

 CfgFileHandler = FREEFILE
 CfgReturnString = ""
 IF DIR(CFGfile) <> "" THEN
    OPEN CFGfile FOR INPUT AS CfgFileHandler
    DO
       LINE INPUT #CfgFileHandler, CfgTmpBuffer
       IF MID(TRIM(CfgTmpBuffer), 1, 1) <> "#" THEN
          CfgColonPos = INSTR(CfgTmpBuffer, "=")
          CfgFieldName = TRIM(MID(CfgTmpBuffer, 1, CfgColonPos - 1))
          CfgFieldData = TRIM(MID(CfgTmpBuffer, CfgColonPos + 1))
          IF UCASE(CfgFieldName) = UCASE(CFGField) THEN CfgReturnString = CfgFieldData
       END IF
    LOOP UNTIL EOF(CfgFileHandler) OR CfgReturnString <> ""
    CLOSE #CfgFileHandler
 END IF
 RETURN CfgReturnString
END FUNCTION
