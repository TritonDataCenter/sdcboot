#ifndef FTEERR_H_
#define FTEERR_H_

#define FTE_OK                     0
#define FTE_READ_ERR               1
#define FTE_WRITE_ERR              2
#define FTE_MEM_INSUFFICIENT       3
#define FTE_WRITE_ON_READONLY      4
#define FTE_INVALID_BYTESPERSECTOR 5
#define FTE_NOT_RESERVED           6
#define FTE_INSUFFICIENT_HANDLES   7
#define FTE_FILESYSTEM_BAD         8

int  GetFTEerror   (void);
void SetFTEerror   (int error);
void ClearFTEerror (void);

#endif
