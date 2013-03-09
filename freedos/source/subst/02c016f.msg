$Id: 02c016f.msg 1.1 2002/11/28 06:20:29 ska Exp ska $
$Locker: ska $ $Name:  $	$State: Exp $

    Message definition file for local messages for English.
    Copyright (C) 1995  Steffen Kaiser

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

>>>>>>E_hlpScreen 127
usage: %s {option} [command]
   /#: view the statistic of JOIN'ed drives in the CDS table
   /!: like /#; initialize the JOIN-flag, if it differs
   /f: view non-printable characters hexadecimal
   /a: view the CDS table after the command
   /k: create the pathes mentioned within the command
   /t: like /k, in SUBST check only, if path exist
   /_: dump all attributes within the CDS table
   /o=#: use this backslash offset, do not calculate itself
   /x=??: List of drive letters to be ignored while a ::label: search

   without command: displays the CDS table
   command:
     -: initialize all SUBST'ed and JOIND'ed drives
     --: initialize all non-networked drives
     dr [-]path: like %s dr [-]path
     /u dr [-]path: substitute dr to point to path
     /j dr [-]path: join dr into path
     /d dr | dr /d | dr - : initialize drive dr (& break SUBST/JOIN)
     /s dr1 dr2: swap dr1 and dr2
     /n [dr1 dr2]: swap unit numbers of the same device driver (Std.: floppies)
     /w dr: set ERRORLEVEL to the drive letter of dr
     /m [-]path: create path including all leading up-directories
     /c [-]path: like /m, then chdir into it
     dr:[+-=]{off,on,phys,net,join,subst,hidden}: set drive flags directly
     /q dr:[+-]{phys,net,join,subst,hidden}: query the drive flags

>>>>>>E_SUBST_E 127
Substitutes a path by a drive letter.

SUBST [drive1: [drive2:]path]
SUBST drive1: /D

 drive1:        drive letter to substitute
 [drive2:]path  drive and path to be accessed via drive1:
 /D             breaks off the relation between drive1 and drive2:path

SUBST without any parameters views all virtual drives created by SUBST.

>>>>>>E_JOIN_E 127
Joins a drive into a directory of another drive.

JOIN [drive1: [drive2:]path]
JOIN drive1: /D

 drive1:        drive to be accessed via path drive2:path
 [drive2:]path  drive and path, in which drive1 is to be joined.
 /D             breaks off the relation between drive1 and drive2:path

JOIN without any parameters views all virtual drives created by JOIN.

>>>>>>E_subst 20
Drive %c: can neither be SUBST'ed nor JOIN'ed
>>>>>>E_mkDir 20
Physical path %s cannot be made
>>>>>>E_swap 20
Drives %s and %s cannot be swapped
>>>>>>E_number 20
'%s' is no number or is greater than %u
>>>>>>E_criticPath 20
Critical error occured, while translating of '%s'
>>>>>>E_nrJoin 20
Unkown failure: Number of JOIN'ed drives not accessable
>>>>>>M_CDS_1
Number of JOIN'ed drives per CDS: %u   per flag: %u  =>
>>>>>>M_CDS_2
seems OK

>>>>>>M_CDS_3
difference

>>>>>>M_CDS_4
Resetting flag 'Number of JOIN'ed drives' to %u

>>>>>>E_setDisk 20
Cannot change to drive %c:
>>>>>>E_chDir 20
Cannot change to directory %s:%s
>>>>>>E_cds 20
CDS of drive %c: not accessable
>>>>>>E_struc 20
Internal structures not initialized properly, possibly DOS version too old
>>>>>>E_drive 20
Drive '%s' is unkown or not accessable this time
>>>>>>E_path 20
'%s' is syntactically no path specification or drive does not respond
>>>>>>E_local 20
Path '%s' is not local (networked)
>>>>>>E_pathLen 20
Path '%s' too long
>>>>>>E_drvSetting 20
Errorous drive setting flag '%s'
>>>>>>E_searchList 40
>>>>>>M_mcbChainWide
0x%04x %c 0x%04x
>>>>>>M_mcbChain
0x%04x %c
>>>>>>M_systemMCB
 sys:%s
>>>>>>M_codeMCB
code
>>>>>>M_dataMCB
data
>>>>>>M_memoryMCB
memory
>>>>>>M_noneMCB
unknown
>>>>>>M_devMCB
device_driver
>>>>>>M_extraDevMCB
device_extra_code
>>>>>>M_ifsMCB
IFS
>>>>>>M_filesMCB
FILES
>>>>>>M_fcbMCB
FCBS
>>>>>>M_emsBufMCB
BUFFERS_in_EMS
>>>>>>M_bufMCB
BUFFERS
>>>>>>M_lstDrvMCB
LASTDRIVE
>>>>>>M_stckMCB
STACKS
>>>>>>M_instMCB
INSTALL_transient_code
>>>>>>M_eocMCB
end_of_chain
>>>>>>M_freeMCB
free
>>>>>>E_joinLocal 21
Join of drive %c: failed, because the path '%s' is not local
>>>>>>E_identFloppies 22
Cannot identify one of or both floppies
>>>>>>E_swapDriverUnits 22
The devices don't share the same device driver.
\tTry the swap command "/s"
