This is the README for the FreeDOS Install program.

Maintained by Jeremy Davis, <jeremyd@computer.org>
Copyright (C) 1998-2011 Jim Hall, <jhall@freedos.org>

The Install program is distributed under the terms of the GNU GPL (see
the file COPYING for details.)  You may use the Install program for
pretty much anything: commercial distributors may even use it as an
installer for their own software.

For installation instructions, see the INSTALL file.

Before running INSTALL.EXE please correctly set the following 
enviroment variables:
TZ=EST5DST or proper timezone
LANG=ES or proper language, if missing hard-coded english values used
NLSPATH=.\NLS or proper path, if missing hard-coded english values used

E.g.
  CD SAMPLE
  SET TZ=EST5DST
  SET LANG=ES
  SET NLSPATH=.\NLS
  INSTALL


* To run simply type:

  C:\INSTALL> install
