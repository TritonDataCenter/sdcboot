$Id: DEUTSCH.MSG 3.2 2002/11/28 06:20:29 ska Exp ska $
$Locker: ska $ $Name:  $	$State: Exp $

    message definition file for local messages for German
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
Aufruf: %s {option} [kommando]
   /#: in CDS-šbersicht die Statistik der JOIN-Laufwerke auffhren
   /!: wie /#; Flag korrigieren, falls abweichend
   /f: nicht druckbare Zeichen in Namen hexadezimal anzeigen
   /a: nach kommando CDS-šbersicht ausgeben
   /k: im kommando enthaltende Pfade anlegen
   /t: wie /k, bei SUBST nur testen, ob pfad existiert
   /_: in der CDS-šbersicht alle Attribute ausgeben
   /o=#: setzt explizit den Backslashoffset
   /x=??: Liste mit bei einer ::label:-Suche zu ignorierenden Laufwerken

   ohne kommando: Ausgabe der CDS-šbersicht
   kommando:
     -: alle SUBST und JOIN Referenzen aufl”sen
     --: auf alle Nicht-Netz-Laufwerke Kommando ? - anwenden
     lw [-]pfad: analog %s lw [-]pfad
     /u lw [-]pfad: lw mit pfad substituieren (SUBST)
     /j lw [-]pfad: lw in pfad einfgen (JOIN)
     /d lw | lw /d | lw - : SUBST/JOIN aufl”sen
     /s lw1 lw2: lw1 und lw2 austauschen
     /n [lw1 lw2]: Units desselben Ger„treiber austauschen (Std.: Floppies)
     /w lw: setzt ERRORLEVEL auf die Nummer des Laufwerksbuchstaben von lw
     /m [-]pfad: pfad anlegen, einschlieálich aller šberverzeichnisse
     /c [-]pfad: wie /m, anschlieáend hineinwechslen.
     lw:[+-=]{off,on,phys,net,join,subst,hidden}: Laufwerksflags direkt setzen
     /q lw:[+-]{phys,net,join,subst,hidden}: Laufwerksflag abfragen

>>>>>>E_SUBST_E 127
Weist einem Pfad eine Laufwerkbezeichnung zu.

SUBST [Laufwerk1: [Laufwerk2:]Pfad]
SUBST Laufwerk1: /D
 Laufwerk1:        Laufwerkbezeichnung, die dem Pfad zugewiesen werden soll.
 [Laufwerk2:]Pfad  Laufwerk und Pfad, die durch Laufwerk1: angesprochen
                   werden sollen.
 /D                Hebt die Zuordnung fr das (virtuelle) Laufwerk1 wieder auf.

SUBST ohne Parameter zeigt die mit SUBST erzeugten, virtuellen Laufwerke an.

>>>>>>E_JOIN_E 127
Fgt ein Laufwerk in ein Verzeichnis eines anderen Laufwerks ein.
JOIN [Laufwerk1: [Laufwerk2:]Pfad]
JOIN Laufwerk1: /D

 Laufwerk1:        Laufwerk, welches als Verzeichnis auf dem Laufwerk2 ercheint.
 [Laufwerk2:]Pfad  Laufwerk und Pfad, in welche Laufwerk1 eingefgt werden soll.
 /D                Hebt die Zuordnung fr das eingefgte Laufwerk1 wieder auf.

JOIN ohne Parameter zeigt die mit JOIN eingefgten Laufwerke an.

>>>>>>E_subst 20
Auf das Laufwerk %c: kann weder SUBST noch JOIN angewendet werden
>>>>>>E_mkDir 20
Kann Verzeichnis %s (physischer Pfad) nicht anlegen
>>>>>>E_swap 20
Kann die Laufwerke %s und %s nicht austauschen
>>>>>>E_number 20
Nummer '%s' fehlerhaft oder gr”áer als %u
>>>>>>E_criticPath 20
Kritischer Fehler w„hrend der Umwandlung von '%s'
>>>>>>E_nrJoin 20
Unbekannter Fehler: Nummer geJOINter Laufwerke nicht erreichbar
>>>>>>M_CDS_1
Anzahl eingefgter Laufwerke per CDS: %u  per Flag: %u  => 
>>>>>>M_CDS_2
scheint OK

>>>>>>M_CDS_3
unterschiedlich

>>>>>>M_CDS_4
Zurcksetzen des Flags 'Anzahl eingefgter Laufwerke' auf %u

>>>>>>E_setDisk 20
Kann nicht auf das Laufwerk %c: wechseln
>>>>>>E_chDir 20
Kann nicht ins Verzeichnis %s:%s wechseln
>>>>>>E_cds 20
CDS von Laufwerk %c: nicht lesbar
>>>>>>E_struc 20
Fehler beim Initialisieren interner Strukturen (evtl. zu alte DOS-Version)
>>>>>>E_drive 20
Laufwerk '%s' nicht verfgbar oder unbekannt
>>>>>>E_path 20
Pfad '%s' sytaktisch falsch oder Laufwerk spricht nicht an
>>>>>>E_local 20
Pfad '%s' ist nicht lokal
>>>>>>E_pathLen 20
Pfad '%s' zu lang
>>>>>>E_drvSetting 20
Fehlerhaftes Laufwerksflag '%s'
>>>>>>E_searchList 40
>>>>>>M_mcbChainWide
0x%04x %c 0x%04x
>>>>>>M_mcbChain
0x%04x %c
>>>>>>M_systemMCB
 sys:%s
>>>>>>M_codeMCB
Code
>>>>>>M_dataMCB
Daten
>>>>>>M_memoryMCB
Speicher
>>>>>>M_noneMCB
unbekannt
>>>>>>M_devMCB
Ger„tetreiber_(DEVICE)
>>>>>>M_extraDevMCB
Zusatzcode_fr_Treiber
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
INSTALL_transienter_Code
>>>>>>M_eocMCB
Ende_der_Kette
>>>>>>M_freeMCB
frei
>>>>>>E_joinLocal 21
Join des Laufwerks %c: ist miálungen, da der Pfad '%s' nicht lokal ist
>>>>>>E_identFloppies 22
Die Identifizierung eines oder beider Diskettenlaufwerke schlug fehl
>>>>>>E_swapDriverUnits 22
Die Laufwerke werden nicht vom selben Ger„tetreiber verwaltet.
\tVersuchen Sie das swap Kommando "/s"
