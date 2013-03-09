/* Copyright (C) 1995-1997 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
** Copyright (C) 1993 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
**
** This file is distributed under the terms listed in the document
** "copying.cws", available from CW Sandmann at the address above.
** A copy of "copying.cws" should accompany this file; if not, a copy
** should be available from where this file was obtained.  This file
** may not be distributed without a verbatim copy of "copying.cws".
**
** This file is distributed WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

typedef struct {
  word32 cb_address;
  word16 cb_sel;
  word8  cb_type;	/* 0 = DPMI, 1 = internal interrupt */
  word8  cb_filler;
  word32 reg_ptr;
  word16 reg_sel;
  word16 reg_filler;
} dpmisim_rmcb_struct;

#define num_rmcb 24	/* 16 + 8 extra since we use for HW ints */

extern dpmisim_rmcb_struct dpmisim_rmcb[num_rmcb];

extern dpmisim_rmcb0();
extern dpmisim_rmcb1();

extern word16 dpmisim_regs[25];
extern void dpmisim(void);

extern void do_raw_switch_ret(void);
extern void savestate_real(void);
extern void savestate_prot(void);

extern word16 DPMIsp;
extern word16 far init_size;
extern word8 far cpu_family;
extern void interrupt (* far oldint2f)();
extern void interrupt dpmiint2f(void);

extern void user_exception_return(void);
extern void ring0_iret(void);
extern void interrupt int23(void);
extern void interrupt int24(void);

extern void do_raw_switch(void);
