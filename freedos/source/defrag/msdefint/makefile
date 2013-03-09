#
# Makefile for microsoft look alike interface.
#

all: Msdefint.lib
     @echo Interactive interface compiled.

msdefint.lib: libs
     @-del msdefint.lib
     @tlib msdefint.lib+dialog\dialog.lib
     @tlib msdefint.lib+event\event.lib
     @tlib msdefint.lib+keyboard\keyboard.lib
     @tlib msdefint.lib+mouse\mouse.lib
     @tlib msdefint.lib+screen\screen.lib
     @tlib msdefint.lib+winman\winman.lib
     @tlib msdefint.lib+ovlhost\ovlhost.lib
     @tlib msdefint.lib+main\main.lib
     @tlib msdefint.lib+logman\logman.lib
     @tlib msdefint.lib+helpsys\helpsys.lib
     @-del msdefint.bak

libs:
     @cd main
     @make
     @cd ..
     @cd helpsys
     @make
     @cd ..
     @cd ovlhost
     @make
     @cd ..
     @cd logman
     @make
     @cd ..
     @cd dialog
     @make
     @cd ..
     @cd winman
     @make
     @cd ..
     @cd event
     @make
     @cd ..
     @cd keyboard
     @make
     @cd ..
     @cd mouse
     @make
     @cd ..
     @cd screen
     @make
     @cd ..

clean:
     @cd main
     @make clean
     @cd ..
     @cd helpsys
     @make clean
     @cd ..
     @cd logman
     @make clean
     @cd ..
     @cd dialog
     @make clean
     @cd ..
     @cd event
     @make clean
     @cd ..
     @cd keyboard
     @make clean
     @cd ..
     @cd ovlhost
     @make clean
     @cd ..
     @cd mouse
     @make clean
     @cd ..
     @cd screen
     @make clean
     @cd ..
     @cd winman
     @make clean
     @cd ..
     @-del msdefint.lib
