#
# Makefile for command line interface.
#

all: cmdefint.lib
     @echo Command line interface compiled.

cmdefint.lib: libs
     @-del cmdefint.lib
     @-del cmdefint.bak
     @tlib cmdefint.lib+main\main.lib
     @tlib cmdefint.lib+ovlhost\ovlimpl.lib
     @-del cmdefint.bak

libs:
     @cd main
     @make
     @cd ..
     @cd ovlhost
     @make
     @cd ..

clean:
     @cd main
     @make clean
     @cd ..
     @cd ovlhost
     @make clean
     @cd ..
     @-del cmdefint.bak
     @-del cmdefint.lib
