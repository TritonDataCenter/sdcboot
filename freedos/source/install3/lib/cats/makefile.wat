#
# Makefile for the cats library by Jim Hall (jhall@freedos.org).
#

# compiler = wcc
# lib      = wlib

CC=wcl
LIB=wlib -q
FLAGS=-c -wx -oasl -bt=DOS -zq -ml -fo=.obj
# options = -wx -oas -d0

OBJ=kitten.obj

libs: catsdb.lib

.c.obj:
	wcl $(FLAGS) $@.c

all: catsdb.lib

catsdb.lib: $(OBJ)
    @if exist catsdb.lib rm catsdb.lib
#    @$(LIB) catsdb.lib +get_line.obj
#    @$(LIB) catsdb.lib +db.obj
#    @$(LIB) catsdb.lin +catgets.obj
   @$(LIB) catsdb.lib +kitten.obj

get_line.obj: get_line.c
    @$(CC) $(FLAGS) get_line.c

db.obj: db.c
    @$(CC) $(FLAGS) db.c

catgets.obj: catgets.c
    @$(CC) $(FLAGS) catgets.c

kitten.obj: kitten.c
   @$(CC) $(FLAGS) kitten.c

clean: .SYMBOLIC
    @if exist kitten.obj rm kitten.obj
    @if exist catsdb.lib rm catsdb.lib
