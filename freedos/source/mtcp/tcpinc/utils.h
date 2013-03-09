
/*

   mTCP Utils.H
   Copyright (C) 2006-2011 Michael B. Brutman (mbbrutman@gmail.com)
   mTCP web page: http://www.brutman.com/mTCP


   This file is part of mTCP.

   mTCP is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mTCP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mTCP.  If not, see <http://www.gnu.org/licenses/>.


   Description: Data structures for utility functions common to all
     of the applications.

   Changes:

   2011-05-27: Initial release as open source software

*/




#ifndef _UTILS_H
#define _UTILS_H


#include <dos.h>
#include <stdio.h>

#include CFG_H
#include "types.h"



// Macros to convert from host byte order to network byte order, and
// vice-versa.

#ifdef __TURBOC__

#define htons( x ) (((x)>>8) | (((x) & 0xFF)<<8) )
#define ntohs( x ) htons( x )


// Naive, but works
inline uint32_t htonl( uint32_t x ) {

  uint32_union_t t;

  t.l = x;

  uint8_t tc = t.c[0];
  t.c[0] = t.c[3];
  t.c[3] = tc;
  tc     = t.c[2];
  t.c[2] = t.c[1];
  t.c[1] = tc;

  return t.l;
};

#define ntohl( x ) htonl( x )


#else

extern uint16_t htons( uint16_t );
#pragma aux htons = \
  "xchg al, ah"     \
  parm [ax]         \
  modify [ax]       \
  value [ax];

#define ntohs( x ) htons( x )


extern uint32_t htonl( uint32_t );
#pragma aux htonl = \
  "xchg al, ah"     \
  "xchg bl, bh"     \
  "xchg ax, bx"     \
  parm [ax bx]      \
  modify [ax bx]    \
  value [ax bx];

#define ntohl( x ) htonl( x )

#endif


extern uint16_t dosVersion( void );
#pragma aux dosVersion = \
  "mov ah,0x30"          \
  "int 0x21"             \
  modify [ax]            \
  value [ax];



// Tracing support
//
// Tracing is conditional on a bit within a global variable.  Each class
// of tracepoint owns one bit in the global variable, providing for
// eight classes.
//
//   0x01 WARNINGS - used all over
//   0x02 GENERAL  - used in the APP part of an application
//   0x04 ARP      - used by ARP
//   0x08 IP       - used by IP/ICMP
//   0x10 UDP      - used by UDP
//   0x20 TCP      - used by TCP
//   0x40 DNS      - used by DNS
//   0x80 DUMP     - packet dumping for seriously large traces
//
// WARNINGS is special - it is both a stand-alone class and it can be
// used as an attribute on the other classes.  This allows one to turn
// on warnings for the entire app with just one bit, while avoiding a
// ton of noise.
//
// A program enables tracing by setting bits in the global variable.
// A program should allow the user to set tracing on and off, and
// probably to provide some control over what gets traced.  I generally
// use an environment variable, although command line options or even
// interactive controls in a program can be used.
//
// By default trace points go to STDERR.  Provide a logfile name if
// necessary.  (This is a good idea for most applications.)
//
// Tracing support is normally compiled in, but it can be supressed by
// defining NOTRACE.



void tprintf( char *fmt, ... );

extern FILE *TrcStream;
extern char  TrcSev;


#ifndef NOTRACE

#define TRACE_ON_WARN    (Utils::Debugging & 0x01)
#define TRACE_ON_GENERAL (Utils::Debugging & 0x02)
#define TRACE_ON_ARP     (Utils::Debugging & 0x04)
#define TRACE_ON_IP      (Utils::Debugging & 0x08)
#define TRACE_ON_UDP     (Utils::Debugging & 0x10)
#define TRACE_ON_TCP     (Utils::Debugging & 0x20)
#define TRACE_ON_DNS     (Utils::Debugging & 0x40)
#define TRACE_ON_DUMP    (Utils::Debugging & 0x80)


#define TRACE_WARN( x ) \
{ \
  if ( TRACE_ON_WARN ) { TrcSev = 'W'; tprintf x; }\
}

#define TRACE( x ) \
{ \
  if ( TRACE_ON_GENERAL ) { tprintf x; } \
}

#define TRACE_ARP( x ) \
{ \
  if ( TRACE_ON_ARP ) { tprintf x; } \
}
#define TRACE_ARP_WARN( x ) \
{ \
  if ( TRACE_ON_ARP || TRACE_ON_WARN ) { TrcSev = 'W'; tprintf x; }\
}


#define TRACE_IP( x ) \
{ \
  if ( TRACE_ON_IP ) { tprintf x; } \
}
#define TRACE_IP_WARN( x ) \
{ \
  if ( TRACE_ON_IP || TRACE_ON_WARN ) { TrcSev = 'W'; tprintf x; }\
}


#define TRACE_UDP( x ) \
{ \
  if ( TRACE_ON_UDP ) { tprintf x; } \
}
#define TRACE_UDP_WARN( x ) \
{ \
  if ( TRACE_ON_UDP || TRACE_ON_WARN ) { TrcSev = 'W'; tprintf x; }\
}


#define TRACE_TCP( x ) \
{ \
  if ( TRACE_ON_TCP ) { tprintf x; } \
}
#define TRACE_TCP_WARN( x ) \
{ \
  if ( TRACE_ON_TCP || TRACE_ON_WARN ) { TrcSev = 'W'; tprintf x; }\
}


#define TRACE_DNS( x ) \
{ \
  if ( TRACE_ON_DNS ) { tprintf x; } \
}
#define TRACE_DNS_WARN( x ) \
{ \
  if ( TRACE_ON_DNS || TRACE_ON_WARN ) { TrcSev = 'W'; tprintf x; }\
}


#else

#define TRACE_ON_WARN    (0)
#define TRACE_ON_GENERAL (0)
#define TRACE_ON_ARP     (0)
#define TRACE_ON_IP      (0)
#define TRACE_ON_UDP     (0)
#define TRACE_ON_TCP     (0)
#define TRACE_ON_DNS     (0)

#define TRACE_WARN( x )
#define TRACE( x )
#define TRACE_ARP( x )
#define TRACE_ARP_WARN( x )
#define TRACE_IP( x )
#define TRACE_IP_WARN( x )
#define TRACE_UDP( x )
#define TRACE_UDP_WARN( x )
#define TRACE_TCP( x )
#define TRACE_TCP_WARN( x )
#define TRACE_DNS( x )
#define TRACE_DNS_WARN( x )

#endif





#ifdef SLEEP_CALLS

// Int 2F/1680.  2F is the multiplex interrupt.  If anything is installed on
// it, this should be safe.  At init time check to see if the vector is not
// all zeros - if it is not all zeros this is safe to use.  It doesn't mean
// that you will idle - something has to be installed, like an APM handler.
// But at least you are giving it a chance.
//
// This works under Windows XP DOS Boxes with SWSVPKT.

extern uint8_t mTCP_sleep( void );
#pragma aux mTCP_sleep = \
  "mov ax,0x1680"     \
  "int 0x2f"          \
  modify [ax]         \
  value  [al];




uint8_t mTCP_sleepCallEnabled = 0;

#endif




// Packet driving macros


#ifdef IP_FRAGMENTS_ON

#define PACKET_PROCESS_SINGLE \
  if ( Buffer_first != Buffer_next ) Packet_process_internal( );  \
  if ( Ip::fragsInReassembly ) Ip::purgeOverdue( );


// Use this one when dealing with lots of small packets and the receive
// buffer.

#define PACKET_PROCESS_MULT( n )                    \
{                                                   \
  uint8_t i=0;                                      \
  while ( i < n ) {                                 \
    if ( Buffer_first != Buffer_next ) {            \
      Packet_process_internal( );                   \
    }                                               \
    else {                                          \
      break;                                        \
    }                                               \
    i++;                                            \
  }                                                 \
  if ( Ip::fragsInReassembly ) Ip::purgeOverdue( ); \
}


#else

#define PACKET_PROCESS_SINGLE \
  if ( Buffer_first != Buffer_next ) Packet_process_internal( );

// Use this one when dealing with lots of small packets and the receive
// buffer.

#define PACKET_PROCESS_MULT( n )                   \
{                                                  \
  uint8_t i=0;                                     \
  while ( i < n ) {                                \
    if ( Buffer_first != Buffer_next ) {           \
      Packet_process_internal( );                  \
    }                                              \
    else {                                         \
      break;                                       \
    }                                              \
    i++;                                           \
  }                                                \
}


#endif



class Utils {

  public:

    static uint8_t  Debugging;
    static char     LogFile[80];

    static char     CfgFilename[80];
    static FILE    *CfgFile;

    // parseEnv and initStack return -1 on error
    static int8_t   parseEnv( void );

    static FILE    *openCfgFile( void );
    static void     closeCfgFile( void );


    static int8_t   initStack( uint8_t tcpSockets, uint8_t xmitBuffers );
    static void     endStack( void );

    static void     dumpStats( FILE *stream );

    static int8_t   getAppValue( const char *target, char *val, uint16_t valBufLen );


    static void      dumpBytes( unsigned char *, unsigned int );
    static uint32_t  timeDiff( DosTime_t startTime, DosTime_t endTime );
    static char     *getNextToken( char *input, char *target, uint16_t bufLen );

};



// Parameter Names

extern char Parm_PacketInt[];
extern char Parm_Hostname[];
extern char Parm_IpAddr[];
extern char Parm_Gateway[];
extern char Parm_Netmask[];
extern char Parm_Nameserver[];
extern char Parm_Mtu[];





#define trixterCpy( target, src, len ) \
asm push ds;          \
asm push si;          \
asm push es;          \
asm push di;          \
		      \
asm lds si, src;      \
asm les di, target;   \
		      \
asm cld;              \
		      \
asm mov cx, len;      \
asm shr cx, 1;        \
asm rep movsw;        \
asm adc cx, cx;       \
asm rep movsb;        \
		      \
asm pop di;           \
asm pop es;           \
asm pop si;           \
asm pop ds;


#endif
