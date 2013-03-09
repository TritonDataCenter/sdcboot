/* Copyright (C) 1995,1996 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugarland, TX 77479
 * Copyright (C) Aug 5th 1991 Y.Shibata */

/*  Change Protect Mode Structure  */
typedef	struct {
	word32	page_table;	/*  Page Table Address  */
	word32	gdt_address;	/*  GDT Address         */
	word32	idt_address;	/*  IDT Address         */
	word16	ldt_selector;	/*  LDT Selector        */
	word16	tss_selector;	/*  TR  Selector        */
	word32	entry_eip;	/*  Protect Mode Entry Address  */
	word16	entry_cs;
	}	CLIENT;

typedef	struct {
	word32	offset32;
	word16	selector;
	}	far32;

typedef	struct {
	word16	offset;
	word16	segment;
	}	far16;

word16	vcpi_present(void);	/*  VCPI Installed Check         */
word32	vcpi_maxpage(void);	/*  VCPI Max Page Number	 */
word32	vcpi_capacity(void);	/*  VCPI Unallocated Page Count  */
word32	vcpi_alloc(void);	/*  VCPI Allocate Page           */
void	vcpi_free(word32);	/*  VCPI Deallocate Pgae         */
word16	vcpi_get_pic(void);	/*  VCPI Get 8259A INT Vector    */
void	vcpi_set_pic(word16);	/*  VCPI Set 8259A INT Vector    */
word16	vcpi_get_secpic(void);	/*  VCPI Get 8259A INT Vector slave */
void	vcpi_set_pics(word16,word16);	/*  VCPI Set 8259A INT Vectors master,slave */

word32	get_interface(void far *table,void *g);

void	ems_init(void);		/*  EMS page allocation 	*/
void	ems_free(void);		/*  Deallocated EMS Page	*/
