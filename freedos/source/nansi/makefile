# Makefile for nansi.sys
# $Log:	makefile $
# Revision 1.9  91/04/07  21:46:45  dan_kegel
# Added na_test.com test program for debugging with DEBUG.
# 
# Revision 1.8  91/04/07  18:23:15  dan_kegel
# Now supports /t commandline switch to tell nansi about new text modes.
# 
# Revision 1.7  91/04/06  21:40:37  dan_kegel
# *** empty log message ***
# 
# Revision 1.6  91/04/06  21:24:49  dan_kegel
# *** empty log message ***
# 
# Revision 1.5  90/07/03  21:33:46  dank
# *** empty log message ***
# 
# Revision 1.4  90/07/03  20:42:35  unknown
# *** empty log message ***
# 
#
default: nansi.sys
	rem Done.

nansi.obj:	nansi.asm
	masm nansi;

nansi_p.obj:	nansi_p.asm
	masm nansi_p;

nansi_f.obj:	nansi_f.asm
	masm nansi_f;

nansi_i.obj:	nansi_i.asm
	masm nansi_i;

nansi.sys:	nansi.obj nansi_p.obj nansi_f.obj nansi_i.obj
	link /map nansi nansi_p nansi_f nansi_i;
	exe2bin nansi nansi.sys
	rm nansi.exe

na_test.obj:	na_test.asm
	masm na_test;

na_test.com:	na_test.obj nansi.obj nansi_p.obj nansi_f.obj nansi_i.obj
	link /map na_test nansi nansi_p nansi_f nansi_i;
	exe2bin na_test na_test.com
	rm na_test.exe
