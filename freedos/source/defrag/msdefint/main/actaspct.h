/*    
   Actaspct.h - experimental implementation of the logging aspect on
                actions.c.

   Copyright (C) 2000 Imre Leber

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@worldonline.be
*/
/*
   To whomever cares:

   Aspect orientation without a weaver seems to work fine. There are still
   multiple concerns (component code and crosscuts) but this way at least
   crosscuts are documented in the source code.
*/

#ifndef LOGGING_ACTIONS_H_
#define LOGGING_ACTIONS_H_

/* The logging aspect on the routines in actions.c */

#define CROSSCUT_BEGIN_OPTIMIZATION \
             LogPrint("Optimization started.");

#define CROSSCUT_SET_OPTIONS                                        \
        {                                                           \
         char buf[75];                                              \
         char* criteriums[] = {"unsorted",                          \
                              "file name",                          \
                              "extension",                          \
                              "date & time",                        \
                              "file size"};                         \
                                                                    \
         sprintf(buf,                                               \
                 "Sort criterium changed to %s.\n"                  \
                 "Sort Order changed to %s.",                       \
                 criteriums[options.SortCriterium],                 \
                 (options.SortOrder == ASCENDING) ? "ascending":    \
                                                    "descending");  \
                                                                    \
         LogPrint(buf);                                             \
        }

#define CROSSCUT_SET_METHOD                                                    \
        {                                                                   \
         char buf[75];                                                      \
         sprintf(buf,                                                       \
                 "Optimization method is %s.",                              \
                 OptimizationMethods[method]);                              \
         LogPrint(buf);                                                     \
        }

#define CROSSCUT_SET_DRIVE                               \
        {                                                \
           char buf[24];                                 \
           sprintf(buf,                                  \
                   "Drive changed to %c:", drive);       \
           LogPrint(buf);                                \
        }

#define CROSSCUT_QUERY_BEFORE                            \
        {                                                \
           LogPrint("Checking disk integrity.");         \
        }

#define CROSSCUT_QUERY_OK                            \
        {                                            \
           LogPrint("Check passed.");                \
           LogPrint("Getting disk information.");    \
        }

#define CROSSCUT_QUERY_ERROR                        \
        {                                           \
           LogPrint("Disk corrupted!");             \
        }

#endif
