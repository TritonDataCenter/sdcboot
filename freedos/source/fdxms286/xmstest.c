/****************************************************************/
/*                                                              */
/*                          XMStest.c                           */
/*                                                              */
/*   verifikation module for (some) XMS functions               */
/*                                                              */
/*                      Copyright (c) 2001                      */
/*                      tom ehlert                              */
/*                      All Rights Reserved                     */
/*                                                              */
/* This file is part of DOS-C.                                  */
/*                                                              */
/* DOS-C is free software; you can redistribute it and/or       */
/* modify it under the terms of the GNU General Public License  */
/* as published by the Free Software Foundation; either version */
/* 2, or (at your option) any later version.                    */
/****************************************************************/

/*
    XMStest.c
    
    verifies functionality of an XMS handler
    
    compilable (at least) with TC 2.01 + TASM
    
    
    functions tested:
    
    XMSalloc()
    XMSfree() (random order)
    XMSmove(seg:offset --> HANDLE,
            HANDLE     --> seg:offset
            HANDLE     --> HANDLE
           ) 

    XMS memory is verified for correct content.
    
    XMSlock/unlock
    
    
    COMPILER supported : 
        TurboC 2.01, (TCPP 1.x ?) - TASM required 
        MS-C for 16-bit
    
*/


#include <stdio.h>
#include <dos.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>


#ifdef __TURBOC__
    #pragma warn -asc           /* suppress warning 'switching to assembly
*/
    #define _asm asm
#endif    


void (far *xmsPtr)();

unsigned XMSax,XMSbx,XMSdx;


int XMScall( unsigned rAX, unsigned rDX)
{
    if (xmsPtr == NULL)
        {
        _asm mov ax, 4300h
        _asm int 2fh                /* XMS installation check */

        _asm cmp al, 80h
        _asm jne detect_done

        _asm mov ax, 4310h               /* XMS get driver address */
        _asm int 2fh

        _asm mov word ptr [xmsPtr+2],es
        _asm mov word ptr [xmsPtr],bx
        
        
        printf("XMShandler located at %x:%x\n",
FP_SEG(xmsPtr),FP_OFF(xmsPtr));
        }
detect_done:

    if (xmsPtr == NULL)
        return 0;

    _asm    mov  ax,rAX
    _asm    mov  dx,rDX
    _asm    call dword ptr [xmsPtr]
    
    _asm    mov XMSax,ax
    _asm    mov XMSbx,bx
    _asm    mov XMSdx,dx

    return XMSax;    

}                

#define LENGTH(x) (sizeof(x)/sizeof(x[0]))

struct {
    unsigned handle;
    unsigned size;
    } hTable[100];


/*
    fill a 1 K buffer with (handlenum << 28) 
*/

void fillMem(long *buff,int hnum, int offsetK)
{
    long start = ((long)hnum << 28) | (offsetK << 10);
    int i;
    for (i = 0; i < 256; i++)
        buff[i] = start + i;
}    

long buff1[256];
long buff2[256];

XMSmove(unsigned dhandle, long doffset,
        unsigned shandle, long soffset,
        long length)
{
    struct {
        long length;
        short shandle;
        long soffset;
        short dhandle;
        long  doffset;
        } C;
   C.length =   length;  
   C.shandle = shandle;  
   C.soffset=  soffset; 
   C.dhandle=  dhandle; 
   C.doffset=  doffset; 

    _asm    mov  ax,0x0b00;
    _asm    lea si,C;
    _asm    call dword ptr [xmsPtr]
    
    _asm    mov XMSax,ax
    _asm    mov XMSbx,bx


    if (XMSax != 1)
        {
        printf("E %02x while moving (%u,%lx) <-- (%u,%lx), len %u\n",
            (XMSbx & 0xff),
            dhandle, 
            doffset,
            shandle,
            soffset,
            length);  
        }            
    return (XMSbx & 0xff);
            
}
main()
{
    int numhandles;
    unsigned maxfree;
    unsigned long totalallocated = 0;
    unsigned i,offset;
    unsigned freeMemAtStart,freeMemAtEnd;
    
    unsigned startClock = clock();
    
    XMScall(0,0);           /* get version number */
    
    printf("version info interface %04x revision %04x exists %04x\n",
XMSax,XMSbx,XMSdx);
    
    XMScall(0x0800,0);  /* query free extended memory */

    freeMemAtStart = XMSax;
            
    printf("total free memory is %u\n",XMSdx);
    printf("max free memory block is %u\n",freeMemAtStart);


    for (numhandles = 0; numhandles < LENGTH(hTable); numhandles++)
        {    
        XMScall(0x0800,0);  /* query free extended memory */
        
        maxfree = XMSax;
        
        if (maxfree == 0)
            break;
            
        maxfree = maxfree/2 + 20;

        printf("free %u, allocating %5uk of memory -",XMSax,maxfree);
        
        XMScall(0x0900,maxfree);  /* allocate extended memory */
        
        
        if (XMSax == 0)
            break;

        totalallocated += maxfree;

        hTable[numhandles].handle = XMSdx;
        hTable[numhandles].size   = maxfree;
        
        
        
                         
        printf(" sucess - handle %x",hTable[numhandles].handle);

        XMScall(0x0c00,hTable[numhandles].handle);
        if (XMSax != 1)
            printf("$ERROR:\acan't lock - %02x", XMSbx & 0xff);
        else
            {
            printf(" phys = %04x:%04x ",XMSdx,XMSbx);
            XMScall(0x0d00,hTable[numhandles].handle);
            }
            
        
        printf("\n");
        
        }            
        
    printf("\ndone - total allocated %lu\n",totalallocated);


    /* now we have nearly every XMS memory allocated, fill it */

    for (i = 0; i < numhandles; i++)
        {
        printf("filling - handle %x, size %5uK%c",hTable[i].handle,hTable[i].size,isatty(fileno(stdout))?'\r':'\n');

        for (offset = 0; offset < hTable[i].size; offset++)
            {
            fillMem(buff1,i,offset);

            XMSmove(hTable[i].handle, ((long)offset << 10), 
                    0,                (long)(void far*)&buff1,
                    1024);
            }                
        }
    printf("\n");

    /* now its filled, verify content */

    for (i = 0; i < numhandles; i++)
        {
        printf("verify handle %x, size %5uK%c",hTable[i].handle,hTable[i].size,isatty(fileno(stdout))?'\r':'\n');
        
        for (offset = 0; offset < hTable[i].size; offset++)
            {
            fillMem(buff1,i,offset);

            XMSmove(
                    0,                (long)(void far*)&buff2,
                    hTable[i].handle, ((long)offset << 10), 
                    1024);
            }                

        if (memcmp(buff1, buff2, 1024))
            {
            printf("problems retrieving contents of handle %u = %x at offset %uK\n",
                        i,hTable[i].handle,offset);
            }

            
        }
    printf("\n");


    /* now shift down all memory for all handles by 1 K */

    for (i = 0; i < numhandles; i++)
        {
        printf("shifting - handle %x, size %5uK%c",hTable[i].handle,hTable[i].size - 1,isatty(fileno(stdout))?'\r':'\n');
        
        for (offset = 0; offset < hTable[i].size - 1; offset++)
            {
            fillMem(buff1,i,offset);

            XMSmove(hTable[i].handle, ((long)offset << 10), 
                    hTable[i].handle, ((long)(offset+1) << 10), 
                    1024);
            }                
        }
    printf("\n");

    
    /* now its shifted, verify content again */

    for (i = 0; i < numhandles; i++)
        {
        printf("verify handle %x, size %5uK%c",hTable[i].handle,hTable[i].size,isatty(fileno(stdout))?'\r':'\n');
        
        for (offset = 0; offset < hTable[i].size - 1; offset++)
            {
            fillMem(buff1,i,offset+1);

            XMSmove(
                    0,                (long)(void far*)&buff2,
                    hTable[i].handle, ((long)offset << 10), 
                    1024);
            }                

        if (memcmp(buff1, buff2, 1024))
            {
            printf("problems after shift: handle %u = %x at offset %uK\n",
                        i,hTable[i].handle,offset);
            }

            
        }
    printf("\n");
    


        

    /* on exit, we MUST release the memory again */
    /* and we do that in random order !          */
    
    srand((unsigned) time(NULL));
    
    for (i = 0; i < numhandles; )
        {
        int randomindex = rand() % numhandles;
        
        if (hTable[randomindex].handle)
            {
            printf("free - handle %x\n",hTable[randomindex].handle);
            XMScall(0x0a00,hTable[randomindex].handle);    
            hTable[randomindex].handle = 0;
            i++;
            }
        }
    printf("\n");
    
    XMScall(0x0800,0);  /* query free extended memory */

    freeMemAtEnd = XMSax;

    if (freeMemAtEnd != freeMemAtStart)
        printf("$ERROR$\a:free memory end %u != start %u\n",freeMemAtEnd,freeMemAtStart);


    printf("total time (clocks)= %u\n",clock() - startClock );
    
    return( 0 );
}        


