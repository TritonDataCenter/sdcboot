/* Copyright (C) 1995-1997 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugar Land, TX 77479
 * xms.h - parts copied from Kent Williams william@umaxc.weeg.uiowa.edu */

int xms_installed(void);
int xms_local_enable_a20(void);
int xms_local_disable_a20(void);
word32 xms_query_extended_memory(void);	/* Returns largest free block */
int xms_emb_allocate(word32 size);
int xms_emb_free(int16 handle);

/*returns 32 bit linear address of emb specified by handle, 0 otherwise. */
word32 xms_lock_emb(int16 handle);
int xms_unlock_emb(int16 handle);
