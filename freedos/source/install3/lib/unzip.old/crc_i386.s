/*
 * crc_i386.S, optimized CRC calculation function for Zip and UnZip, not
 * copyrighted by Paul Kienitz and Christian Spieler.  Last revised 19 Jan 96.
 *
 * FLAT memory model assumed.  Calling interface:
 *   - args are pushed onto the stack from right to left,
 *   - return value is given in the EAX register,
 *   - all other registers (with exception of EFLAGS) are preserved. (With
 *     GNU C 2.7.x, %edx and %ecx are `scratch' registers, but preserving
 *     them nevertheless does only cost 4 single byte instructions.)
 *
 * This source generates the function
 * ulg crc32(ulg oldcrc, const uch *text, ulg textlen).
 *
 * The loop unroolling can be disabled by defining the macro NO_UNROLLED_LOOPS.
 * This results in shorter code at the expense of reduced performance.
 */

/* This file is NOT used in conjunction with zlib. */
#ifndef USE_ZLIB

/* Preprocess with -DNO_UNDERLINE if your C compiler does not prefix
 * external symbols with an underline character '_'.
 */
#if defined(NO_UNDERLINE) || defined(__ELF__)
#  define _crc32            crc32
#  define _get_crc_table    get_crc_table
#endif
/* Use 16-bytes alignment if your assembler supports it. Warning: gas
 * uses a log(x) parameter (.align 4 means 16-bytes alignment). On SVR4
 * the parameter is a number of bytes.
 */
#ifndef ALIGNMENT
#  define ALIGNMENT 4
#endif

#if defined(i386) || defined(_i386) || defined(_I386) || defined(__i386)

/* This version is for 386 Unix, OS/2, MSDOS in 32 bit mode (gcc & gas).
 * Warning: it uses the AT&T syntax: mov source,dest
 * This file is only optional. If you want to use the C version,
 * remove -DASM_CRC from CFLAGS in Makefile and set OBJA to an empty string.
 */

                .file   "crc_i386.S"

                .extern  _get_crc_table
                /* extern ulg near * __cdecl get_crc_table(void); */

#ifndef ALIGN_SMALL
#  if ALIGNMENT < 2
#    define ALIGN_SMALL  ALIGNMENT
#  else
#    define ALIGN_SMALL  2
#  endif
#endif

#if defined(NO_STD_STACKFRAME) && defined(USE_STD_STACKFRAME)
#  undef USE_STACKFRAME
#else
   /* The default is to use standard stack frame entry, because it
    * results in smaller code!
    */
#  ifndef USE_STD_STACKFRAME
#    define USE_STD_STACKFRAME
#  endif
#endif

#ifdef USE_STD_STACKFRAME
#  define _STD_ENTRY    pushl   %ebp ; movl   %esp,%ebp
#  define arg1  8(%ebp)
#  define arg2  12(%ebp)
#  define arg3  16(%ebp)
#  define _STD_LEAVE    popl    %ebp
#else /* !USE_STD_STACKFRAME */
#  define _STD_ENTRY
#  define arg1  24(%esp)
#  define arg2  28(%esp)
#  define arg3  32(%esp)
#  define _STD_LEAVE
#endif /* ?USE_STD_STACKFRAME */

/*
 * This is the loop body of the CRC32 cruncher.
 * registers modified:
 *   ebx  : crc value "c"
 *   esi  : pointer to next data byte "text++"
 * registers read:
 *   edi  : pointer to base of crc_table array
 * scratch registers:
 *   eax  : requires upper three bytes of eax = 0, uses al
 */
#define Do_CRC \
                lodsb                       ;/* al <-- *text++              */\
                xorb    %bl,%al             ;/* (c ^ *text++) & 0xFF        */\
                shrl    $8,%ebx             ;/* c = (c >> 8)                */\
                xorl    (%edi,%eax,4),%ebx  ;/*  ^table[(c^(*text++))&0xFF] */


                .text

                .globl  _crc32

_crc32:                         /* ulg crc32(ulg crc, uch *text, extent len) */
                _STD_ENTRY
                pushl   %edi
                pushl   %esi
                pushl   %ebx
                pushl   %edx
                pushl   %ecx

                movl    arg2,%esi            /* 2nd arg: uch *text           */
                testl   %esi,%esi
                jne     Crunch_it            /* > if (!text)                 */
                subl    %eax,%eax            /* >   return 0;                */
                jmp     fine                 /* >                            */
                .align  ALIGN_SMALL,0x90
Crunch_it:                                   /* > else {                     */
                call    _get_crc_table
                movl    %eax,%edi
                movl    arg1,%ebx            /* 1st arg: ulg crc             */
                subl    %eax,%eax            /* eax=0; al usable as dword */
                movl    arg3,%ecx            /* 3rd arg: extent textlen      */
                notl    %ebx                 /* >   c = ~crc;                */
                cld                          /* incr. idx regs on string ops */

#ifndef  NO_UNROLLED_LOOPS
                movl    %ecx,%edx            /* save textlen in edx          */
                shrl    $3,%ecx              /* ecx = textlen / 8            */
                andl    $7,%edx              /* edx = textlen % 8            */
                jecxz   No_Eights
/*  align loop head at start of 486 internal cache line !! */
                .align   ALIGNMENT,0x90
Next_Eight:
                 Do_CRC
                 Do_CRC
                 Do_CRC
                 Do_CRC
                 Do_CRC
                 Do_CRC
                 Do_CRC
                 Do_CRC
                loop    Next_Eight
No_Eights:
                movl    %edx,%ecx
#endif /* NO_UNROLLED_LOOPS */
                jecxz   bail                 /* > if (textlen)               */
/* align loop head at start of 486 internal cache line !! */
                .align   ALIGNMENT,0x90
loupe:                                       /* >   do {                     */
                 Do_CRC                      /*       c = CRC32(c, *text++); */
                loop    loupe                /* >   } while (--textlen);     */

bail:                                        /* > }                          */
                movl    %ebx,%eax
                notl    %eax                 /* > return ~c;                 */
fine:
                popl    %ecx
                popl    %edx
                popl    %ebx
                popl    %esi
                popl    %edi
                _STD_LEAVE
                ret

#else
 error: this asm version is for 386 only
#endif /* i386 || _i386 || _I386 || __i386 */

#endif /* !USE_ZLIB */
