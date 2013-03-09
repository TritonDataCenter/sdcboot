#
# NMAKE include file to link a 4xxx EXE file
#
        cd $(OBJ)
        $(LINKCMD)
!IFDEF POSTLINKCMD
        $(POSTLINKCMD)
!ENDIF
#!IF "$(_LANGUAGE)" == "ENGLISH"
#        copy $(EXENAME) ..\*.*
#        copy $(MAPNAME) ..\*.*
#        -del $(EXENAME)
#	 -del $(MAPNAME)
#!ENDIF
	cd ..

