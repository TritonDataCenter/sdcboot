/*  HTML Help Header File

    Copyright (c) Express Software 1998.
    All Rights Reserved.

    Created by: Joseph Cosentino.

*/

#ifndef _HTML_H
#define _HTML_H

/* Many and various definitions and global variables have been
   removed from here, to reduce binary size and improve code
   structure. Many were also no longer needed. RP 11-mar-04 */

void show_usage (void);
void get_home_page (char *home_page, char *path);
void get_base_dir (char *base_dir, char *path);
int lang_add (char *base_dir, const char *home_page);

int checkForFile (char *givenname);

/* in parse.c: */
int setExtendedSubs(int codepage);
extern const char supportedCodepages[];

#endif