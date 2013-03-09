/*
//  Program:  Free Unformat
//  Version:  0.8
//  Written By:  Brian E. Reifsnyder
//  Copyright 1999 under the terms of the GNU GPL by Brian E. Reifsnyder
//
//  Note:  This program is free and is without a warranty of any kind.
//
*/


/*
/////////////////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////////////////
*/

#include <bios.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
/////////////////////////////////////////////////////////////////////////////
// DEFINES
/////////////////////////////////////////////////////////////////////////////
*/

#define FOUND 1

#define LIST  1

#define TRUE 1
#define FALSE 0

#define NULL 0

#define PERCENTAGE 1

#define PRIMARY 0
#define EXTENDED 1

#define HARD 0
#define FLOPPY 1

#define FAT12 0
#define FAT16 1
#define FAT32 2

/*
/////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
/////////////////////////////////////////////////////////////////////////////
*/

int list_file_and_directory_names = 0;
int print_output = 0;
int test = 0;
int unformat_without_mirror_file = 0;


long sectors_per_fat;

/* Physical Drive Parameters */
int physical_drive;
int logical_drive;

unsigned long logical_sector_offset;

long total_cylinders;
long total_heads;
long total_sectors;
long floppy_drive_type;
int drive_type;
int media_descriptor;

/* Logical drive information */
long sectors_in_root_dir;
int type_of_fat;
unsigned long number_of_logical_sectors_on_drive;
long number_of_root_directory_entries;

/* Extracted cylinder & sector from partition table */
unsigned long g_cylinder;
unsigned long g_sector;

/* Partition Information */

int numeric_partition_type[24];

unsigned long starting_cylinder[24];
unsigned long starting_head[24];
unsigned long starting_sector[24];

unsigned long ending_cylinder[24];
unsigned long ending_head[24];
unsigned long ending_sector[24];

int relative_sectors_1[24];
int relative_sectors_2[24];
int relative_sectors_3[24];
int relative_sectors_4[24];

int partition_size_1[24];
int partition_size_2[24];
int partition_size_3[24];
int partition_size_4[24];

unsigned long partition_size[24];

int number_of_partitions;

/* "Logical Drive to Format" translation information */
int physical_drive_number;

unsigned long partition_starting_cylinder;
unsigned long partition_starting_head;
unsigned long partition_starting_sector;

unsigned long partition_ending_cylinder;
unsigned long partition_ending_head;
unsigned long partition_ending_sector;

unsigned long drive_maximum_cylinders;
unsigned long drive_maximum_heads;
unsigned long drive_maximum_sectors;

unsigned long total_logical_sectors;
unsigned long total_physical_sectors;
int number_of_sectors_per_cluster;

int partition_on_hard_disk;

/* Translated values from "void Convert_Logical_To_Physical" */
unsigned long translated_head;
unsigned long translated_cylinder;
unsigned long translated_sector;

/* Misc variables */

unsigned long integer1;
unsigned long integer2;
unsigned long integer3;
unsigned long integer4;

/* Buffers */
unsigned char mirror_map[5120];

unsigned char control_buffer[512];
unsigned char sector_buffer[512];

/*
/////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
/////////////////////////////////////////////////////////////////////////////
*/
int Compare_Sector_Buffers();

unsigned long Decimal_Number(unsigned long hex1, unsigned long hex2, unsigned long hex3, unsigned long hex4);

void Copy_Sector_Into_Control_Buffer();
void Display_Help_Screen();
void List_Names();
void Read_Mirror_Map();
void Restore_Partition_Tables(int list);
void Unformat_Drive();
void Verify_Drive_Mirror();


//////////////
void Clear_Sector_Buffer();
void Convert_Huge_To_Integers(unsigned long number);
void Convert_Logical_To_Physical(unsigned long sector);
void Extract_Cylinder_and_Sector(unsigned long hex1, unsigned long hex2);
void Get_Drive_Parameters();
void Get_Physical_Floppy_Drive_Parameters();
void Get_Physical_Hard_Drive_Parameters();
void Read_Partition_Table();
void Read_Physical_Sector(int drive, int head, long cyl, int sector);
void Read_Sector(unsigned long sector);
void Reset_Drive();
void Write_Physical_Sector(int drive, int head, long cyl, int sector);
void Write_Sector(unsigned long sector);


/*
/////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////
*/

/* Clear Sector Buffer */
void Clear_Sector_Buffer()
{
  long loop=0;

  do
    {
    sector_buffer[loop]=0;

    loop++;
    }while(loop<512);
}

/* Convert a logical sector to a physical drive location */
void Convert_Logical_To_Physical(unsigned long sector)
{
  unsigned long remaining;

  if(drive_type==HARD) sector=sector+logical_sector_offset;

  translated_cylinder=sector/(total_sectors*(total_heads+1));
  remaining=sector % (total_sectors*(total_heads+1));
  translated_head=remaining/(total_sectors);
  translated_sector=remaining % (total_sectors);
  translated_sector++;
}

/* Compare Sector Buffers ///////////// */
int Compare_Sector_Buffers()
{
  int answer=0;
  int compare=0;
  int loop = 0;

  do
    {
    compare=control_buffer[loop]-sector_buffer[loop];
    if(compare!=0) answer = 1;
    loop++;
    } while(loop<512);

  return(answer);
}

/* Copy sector into control buffer //// */
void Copy_Sector_Into_Control_Buffer()
{
  int loop=0;

  do
    {
    control_buffer[loop]=sector_buffer[loop];
    loop++;
    } while(loop<=512);
}

/* Convert Hexidecimal Number to a Decimal Number */
unsigned long Decimal_Number(unsigned long hex1, unsigned long hex2, unsigned long hex3, unsigned long hex4)
{
  return((hex1) + (hex2*256) + (hex3*65536) + (hex4*16777216));
}

/* Extract Cylinder & Sector */
void Extract_Cylinder_and_Sector(unsigned long hex1, unsigned long hex2)
{
  unsigned long cylinder_and_sector = ( (256*hex2) + hex1 );

  g_sector = cylinder_and_sector % 64;

  g_cylinder = ( ( (cylinder_and_sector*4) & 768) + (cylinder_and_sector /256) );
}

/* Get Drive Parameters */

/* The physical drive parameters are necessary to allow this program  */
/* to bypass the DOS interrupts.                                      */
void Get_Drive_Parameters()
{
  int drives_found_flag;
  long hard_disk_installed[8];
  int increment_heads=FALSE;
  int logical_drive_found=FALSE;
  int partition_number_of_logical_drive;
  int pointer=0;
  int possible_logical_drive;
  int possible_physical_hard_disk;
  int sub_pointer=0;
  int result;

  /* Brief partition table variables */
  long partition_type_table[8][26];
  long maximum_partition_assigned[8];

  if(drive_type==FLOPPY)
    {
    physical_drive=logical_drive;

    Get_Physical_Floppy_Drive_Parameters();

    partition_starting_cylinder=0;
    partition_starting_head=0;
    partition_starting_sector=1;

    partition_ending_cylinder=total_cylinders;
    partition_ending_head=total_heads;
    partition_ending_sector=total_sectors;

    drive_maximum_cylinders=total_cylinders;

    drive_maximum_heads=1;

    drive_maximum_sectors=total_sectors;

    total_logical_sectors=(total_cylinders+1)*(total_heads+1)*total_sectors;
    total_physical_sectors=total_logical_sectors;
    number_of_sectors_per_cluster=1;
    }
  else
    {
    /* Get availability of physical hard drives and */
    /* assemble a brief partition table.            */

    /* Initialize partition_type_table */
    pointer=0;
    do
      {
      partition_type_table[0][pointer]=0;
      partition_type_table[1][pointer]=0;
      partition_type_table[2][pointer]=0;
      partition_type_table[3][pointer]=0;
      partition_type_table[4][pointer]=0;
      partition_type_table[5][pointer]=0;
      partition_type_table[6][pointer]=0;
      partition_type_table[7][pointer]=0;

      pointer++;
      }while(pointer<=26);

    /* Initialize maximum_partition_assigned */
    maximum_partition_assigned[0]=0;
    maximum_partition_assigned[1]=0;
    maximum_partition_assigned[2]=0;
    maximum_partition_assigned[3]=0;
    maximum_partition_assigned[4]=0;
    maximum_partition_assigned[5]=0;
    maximum_partition_assigned[6]=0;
    maximum_partition_assigned[7]=0;

    pointer=128;
    drives_found_flag=FALSE;

    do
      {
      result=biosdisk(2, pointer, 0, 0, 1, 1, sector_buffer);

      if(result==0)
	{
	drives_found_flag=TRUE;
	hard_disk_installed[pointer-128]=TRUE;

	physical_drive=pointer;
	Read_Partition_Table();

	sub_pointer=0;
	do
	  {
	  partition_type_table[pointer-128][sub_pointer]=numeric_partition_type[sub_pointer];
	  sub_pointer++;
	  }while(sub_pointer<=number_of_partitions);
	maximum_partition_assigned[pointer-128]=number_of_partitions;
	}
      else hard_disk_installed[pointer-128]=FALSE;

      pointer++;
      }while(pointer<136);

    /* If there aren't any physical hard drives available, exit program */

    if(drives_found_flag==FALSE)
      {
      printf("\nNo hard disks found...operation terminated.\n");
      exit(4);
      }

    /* Determine on which physical hard drive the logical drive is */
    /* and on which partition it is located.                       */

    /* Check on primary partitions */
    possible_logical_drive=1;
    possible_physical_hard_disk=128;

    do
      {
      if(hard_disk_installed[possible_physical_hard_disk-128]==TRUE)
	{
	pointer=0;

	do
	  {
	  if( (partition_type_table[possible_physical_hard_disk-128][pointer]==0x01) || (partition_type_table[possible_physical_hard_disk-128][pointer]==0x04) || (partition_type_table[possible_physical_hard_disk-128][pointer]==0x06) )
	    {
	    possible_logical_drive++;

	    if(logical_drive==possible_logical_drive)
	      {
	      physical_drive=possible_physical_hard_disk;
	      partition_number_of_logical_drive=pointer;
	      logical_drive_found=TRUE;
	      }
	    }
	  pointer++;
	  }while(pointer<4);
	}
      possible_physical_hard_disk++;
      }while(possible_physical_hard_disk<136);

    /* Check on extended partitions */
    possible_physical_hard_disk=128;

    if(logical_drive_found==FALSE)
      {
      do
	{
	if( (hard_disk_installed[possible_physical_hard_disk-128]==TRUE) && (maximum_partition_assigned[possible_physical_hard_disk-128]>=4) )
	  {
	  pointer=4;

	  do
	    {
	    if( (partition_type_table[possible_physical_hard_disk-128][pointer]==0x01) || (partition_type_table[possible_physical_hard_disk-128][pointer]==0x04) || (partition_type_table[possible_physical_hard_disk-128][pointer]==0x06) )
	      {
	      possible_logical_drive++;

	      if(logical_drive==possible_logical_drive)
		{
		physical_drive=possible_physical_hard_disk;
		partition_number_of_logical_drive=pointer;
		increment_heads=TRUE;
		logical_drive_found=TRUE;
		}
	      }
	    pointer++;
	    }while(pointer<=maximum_partition_assigned[possible_physical_hard_disk-128]);
	  }
	possible_physical_hard_disk++;
	}while(possible_physical_hard_disk<136);
      }

    /* Get a full partition table listing for that drive */

    if(logical_drive_found==TRUE)
      {
      Get_Physical_Hard_Drive_Parameters();
      Read_Partition_Table();
      }
    else
      {
      printf("\nDrive letter entered is out of range...operation terminated.\n");
      exit(4);
      }

    /* Load physical partition information */

    partition_starting_cylinder=starting_cylinder[partition_number_of_logical_drive];
    partition_starting_head=starting_head[partition_number_of_logical_drive];
    partition_starting_sector=starting_sector[partition_number_of_logical_drive];

    partition_ending_cylinder=ending_cylinder[partition_number_of_logical_drive];
    partition_ending_head=ending_head[partition_number_of_logical_drive];
    partition_ending_sector=ending_sector[partition_number_of_logical_drive];

    drive_maximum_cylinders=ending_cylinder[partition_number_of_logical_drive];
    drive_maximum_heads=total_heads;
    drive_maximum_sectors=total_sectors;

    total_logical_sectors=((partition_ending_cylinder-partition_starting_cylinder)+1)*(total_heads+1)*total_sectors;
    total_physical_sectors=total_logical_sectors;

    logical_sector_offset=((partition_starting_cylinder)*(total_heads+1)*(total_sectors));
    if(logical_sector_offset==0) logical_sector_offset=(partition_starting_head)*(total_sectors);
    if(increment_heads==TRUE) logical_sector_offset=logical_sector_offset+total_sectors;

    /* Compute cluster size */
    unsigned long size=(total_logical_sectors*512)/1000000;

    /* Sectors per cluster conversion table */
    if (size<=32767) number_of_sectors_per_cluster=64;
    if (size<=16383) number_of_sectors_per_cluster=32;
    if (size<=8191) number_of_sectors_per_cluster=16;

    /* May be illegal cluster sizes
    if (size<=256) number_of_sectors_per_cluster=8;
    if (size<=128) number_of_sectors_per_cluster=4;
    */
    }
}

/* Get Physical Floppy Drive Parameters */
void Get_Physical_Floppy_Drive_Parameters()
{
  asm{
    mov ah, 0x08
    mov dl, BYTE PTR physical_drive
    int 0x013

    mov BYTE PTR total_sectors, cl
    mov BYTE PTR total_cylinders, ch
    mov BYTE PTR total_heads, dh
    mov BYTE PTR floppy_drive_type, bl
    }
}

/* Get Physical Hard Drive Parameters */
void Get_Physical_Hard_Drive_Parameters()
{
  asm{
    mov ah, 0x08
    mov dl, BYTE PTR physical_drive
    int 0x13

    mov bl,cl
    and bl,00111111B

    mov BYTE PTR total_sectors, bl

    mov bl,cl
    mov cl,ch
    shr bl,1
    shr bl,1
    shr bl,1
    shr bl,1
    shr bl,1
    shr bl,1

    mov ch,bl

    mov WORD PTR total_cylinders, cx
    mov BYTE PTR total_heads, dh
    }
}

/* List File & Directory Names //////// */
void List_Names()
{
  int loop=0;
  int sub_loop=0;

  char file_or_directory [12];

  file_or_directory [8] = 46;

  do
    {
    sub_loop = 0;
    do
      {
      file_or_directory[sub_loop]=sector_buffer[(loop+sub_loop)];
      sub_loop++;
      } while(sub_loop <=7);

    sub_loop++;

    do
      {
      file_or_directory[sub_loop]=sector_buffer[(loop+sub_loop-1)];
      sub_loop++;
      } while(sub_loop <=11);

    if( (file_or_directory[0]>=48) && (file_or_directory[0]<=122) )
      {
      printf("\n%.12s",file_or_directory);
      }

    loop = loop + 32;
    } while(loop <=480);
}

/* Help Routine  ////////////////////// */
void Display_Help_Screen()
{
  printf("\nUnformat Version 0.8               Restores a disk that has been formatted.\n\n");
  printf("Syntax:\n\n");
  printf("UNFORMAT drive: [/J]\n");
  printf("UNFORMAT drive: [/U] [/L] [/TEST] [/P]\n");
  printf("UNFORMAT /PARTN [/L]\n");
  printf("UNFORMAT /?\n\n");
  printf("  drive:    Drive letter to unformat.\n");
  printf("  /J        Verifies that the drive mirror files is synchronized with the disk.\n");
  printf("  /U      * Unformats without using MIRROR files.\n");
  printf("  /L        Lists all files and directories found, or, when used with the\n");
  printf("              /PART switch, displays current partition tables.\n");
  printf("  /TEST   * Displays information but does not write changes to disk.\n");
  printf("  /P      * Sends output messages to LPT1.\n");
  printf("  /PARTN    Restores disk partition tables.\n\n");
  printf("  /?        Displays this help screen.\n");
  printf("This program is copyright 1998 by Brian E. Reifsnyder under the terms of\n");
  printf("GNU General Public License and is without warranty of any kind.\n");
  printf("\n* Indicates functions not yet available.\n");
}

/* Read Partition Table */
void Read_Partition_Table()
{
  long index=0x1be;

  int exiting_primary=TRUE;
  int extended=FALSE;
  int partition_designation=PRIMARY;
  int pointer=0;
  int record_extended_info_flag=FALSE;

  int done_flag=FALSE;

  unsigned long extended_cylinder;
  unsigned long extended_head;
  unsigned long extended_sector;

  Read_Physical_Sector(physical_drive,0,0,1);

  do{
    if(pointer==4) partition_designation=EXTENDED;

    if((pointer>=4) && (extended==TRUE))
      {
      Read_Physical_Sector(physical_drive,extended_head,extended_cylinder,extended_sector);
      extended=FALSE;
      index=0x1be;

      if(exiting_primary==FALSE)
	{
	pointer--;
	}
      else
	{
	exiting_primary=FALSE;
	}
      }

    /* Determine Partition Type */
    numeric_partition_type[pointer]=sector_buffer[index+4];

    if(sector_buffer[index+4]==0x00)
      {
      if(partition_designation==EXTENDED)
	{
	number_of_partitions=pointer;
	done_flag=TRUE;
	}
      }
    if(sector_buffer[index+4]==0x05)
      {
      extended=TRUE;
      record_extended_info_flag=TRUE;
      }

    starting_head[pointer] = sector_buffer[index+1];
    ending_head[pointer] = sector_buffer[index+5];

    partition_size[pointer]=Decimal_Number(sector_buffer[index+12],sector_buffer[index+13],sector_buffer[index+14],sector_buffer[index+15])/2000;

    Extract_Cylinder_and_Sector(sector_buffer[index+2],sector_buffer[index+3]);

    starting_cylinder[pointer]=g_cylinder;
    starting_sector[pointer]=g_sector;

    if((extended==TRUE) && (record_extended_info_flag==TRUE))
      {
      extended_cylinder=starting_cylinder[pointer];
      extended_head=starting_head[pointer];
      extended_sector=starting_sector[pointer];
      record_extended_info_flag=FALSE;
      }

    Extract_Cylinder_and_Sector(sector_buffer[index+6],sector_buffer[index+7]);

    ending_cylinder[pointer]=g_cylinder;
    ending_sector[pointer]=g_sector;

    partition_size_1[pointer]=sector_buffer[index+12];
    partition_size_2[pointer]=sector_buffer[index+13];
    partition_size_3[pointer]=sector_buffer[index+14];
    partition_size_4[pointer]=sector_buffer[index+15];

    relative_sectors_1[pointer]=sector_buffer[index+8];
    relative_sectors_2[pointer]=sector_buffer[index+9];
    relative_sectors_3[pointer]=sector_buffer[index+10];
    relative_sectors_4[pointer]=sector_buffer[index+11];

    pointer++;
    number_of_partitions=pointer-1;

    if((extended==FALSE) && (pointer==4))
      {
      number_of_partitions=4;
      done_flag=TRUE;
      }

    index=index + 16;
    } while(done_flag==FALSE);
}

/* Read Physical Sector */
void Read_Physical_Sector(int drive, int head, long cyl, int sector)
{
  int result;

  result=biosdisk(2, drive, head, cyl, sector, 1, sector_buffer);

    if (result!=0)
      {
      printf("\nRead error...operation terminated.\n");
      exit(4);
      }
}

/* Read Sector From Disk */
void Read_Sector(unsigned long sector)
{
  Convert_Logical_To_Physical(sector);

  printf("Cylinder:  %5i  ",translated_cylinder);
  printf("Head:  %2i \n",translated_head);
  /* printf("Sector: %i \n",translated_sector); */

  /* Re-position cursor back to the beginning of the line */
  asm{
    /* Get current video display mode */
    mov ah,0x0f
    int 0x10

    /* Get cursor position */
    mov ah,0x03
    int 0x10

    /* Set cursor position to beginning of line */
    mov ah,0x02
    sub dh,1
    int 0x10
    }

  Read_Physical_Sector(physical_drive,translated_head,translated_cylinder,translated_sector);
}

/* Read Mirror Map Into Buffer //////// */
void Read_Mirror_Map()
{
  int flag = 0;

  long loop;
  long mirror_map_pointer;
  long index;
  long subloop;

  long current_sector;
  long mirror_map_start_sector;

  long source_sector;
  long destination_sector;

  printf("\n\nSearching for drive mirror...\n");

  /*Find "pointer sector" at end of drive */

  current_sector=total_logical_sectors-200;

  if(current_sector<0)
    {
    current_sector=0;
    }

  /*If drive is a floppy drive, change the following: */
  if(drive_type==FLOPPY)
    {
    current_sector=total_logical_sectors-10;
    total_logical_sectors--;
    }

  do
    {
    Read_Sector(current_sector);
    if((65==sector_buffer[4])&&(77==sector_buffer[5])&&(83==sector_buffer[6])&&(69==sector_buffer[7]))
      {
      mirror_map_start_sector=Decimal_Number(sector_buffer[0],sector_buffer[1],sector_buffer[2],sector_buffer[3]);
      flag=FOUND;
      }
    current_sector++;
    } while((current_sector<=total_logical_sectors) || (flag!=FOUND));

  loop=0;

  if(FOUND==flag)
    {
    /*Read Mirror Map into buffer */
    printf("\n\nLoading drive mirror...\n");

    do
      {
      Read_Sector(mirror_map_start_sector+loop);

      subloop=0;
      do
	{
	mirror_map[subloop+(loop*512)]=sector_buffer[subloop];
	subloop++;
	}while(subloop<512);

      loop++;
      }while(loop<=10);
    }

  else
    {
    printf("\nThe sector that points to the drive mirror has not been found.\n");
    printf("Operation Terminated.\n");
    exit(1);
    }

}

/* Reset drive controller */
void Reset_Drive()
{
  asm{
    mov ah,0
    mov dl,BYTE PTR physical_drive
    int 0x13

    mov ah,0
    mov dl,BYTE PTR physical_drive
    int 0x13
    }
}

/* Restore Partition Tables /////////// */
void Restore_Partition_Tables(int list)
{
  if(list!=LIST)
    {
    /*  Re-create the partition tables on the hard disks from the
    /*  a:partnsav.fil. */


    /*
    Map of the PARTNSAV.FIL for FreeDOS:

    (I do not plan on using an MS-DOS compatible file.)

    offset:00
    FreeDOS partitio
    n mirror file

    offset:32-127 reserved for future expansion

    offset:128(drive#)      129(lb offset)    130(mb offset)    131(lb dest cyl)
	   132(mb dest cyl) 133(lb dest head) 134(mb dest head) 135(dest sect.)
    .
    .
    .
    offset:1024 stored sector
    offset:1536 stored sector
    offset:2048 stored sector
    .
    .
    .
    */

    long index=0;

    FILE *file_pointer;

    unsigned char partition_table_map[1024];

    int drive_number;
    unsigned long destination_cylinder;
    unsigned long destination_head;
    unsigned long destination_sector;

    /* Clear partition_table_map[] */
    index=0;

    do
      {
      partition_table_map[index]=0;
      index++;
      }while(index<=1023);

    /* Load the map of the PARTNSAV.FIL file */
    index=0;

    file_pointer=fopen("A:\\PARTNSAV.FIL","rb");
    if(file_pointer==NULL)
      {
      printf("\nError opening \"A:\\PARTNSAV.FIL\"...Operation terminated.\n");
      exit(1);
      }

    /* Read the map from the PARTNSAV.FIL file */
    test=fread(&partition_table_map,1024,1,file_pointer);
    if(test==0)
      {
      printf("\nError reading \"A:\\PARTNSAV.FIL\"...Operation terminated.\n");
      exit(1);
      }

    /* Write the partitions to the hard disk(s) */

    index=128;

    do
      {
      /* Read the drive number */
      drive_number=partition_table_map[index+0];

      /* Get the destination cylinder */
      destination_cylinder=Decimal_Number(partition_table_map[index+3],partition_table_map[index+4],0,0);

      /* Get the destination head */
      destination_head=Decimal_Number(partition_table_map[index+5],partition_table_map[index+6],0,0);

      /* Get the destination sector */
      destination_sector=partition_table_map[index+7];

      /* Read the partition sector from the partnsav.fil */
      fread(sector_buffer,512,1,file_pointer);

      /* Write the partition sector to the hard disk */
      Write_Physical_Sector(drive_number,destination_head,destination_cylinder,destination_sector);

      index=index+8;
      }while(0!=partition_table_map[index]);

    fclose(file_pointer);
    }
  else
    {
    /* List all of the partition tables */
    printf("\nThis feature is not implemented at this time.\n");
    }
  exit(0);
}

/* Unformat Drive ///////////////////// */
void Unformat_Drive()
{
  int loop;

  long mirror_map_pointer;
  long index;

  long current_sector;
  long mirror_map_start_sector;

  long source_sector;
  long destination_sector;


  Read_Mirror_Map();

  printf("\n\nUnformatting drive...\n");

  /*Re-create the boot sector */
  destination_sector=Decimal_Number(mirror_map[84],mirror_map[85],mirror_map[86],mirror_map[87]);
  source_sector=Decimal_Number(mirror_map[88],mirror_map[89],mirror_map[90],mirror_map[91]);

  Read_Sector(source_sector);
  Write_Sector(destination_sector);

  /* Get the size of the FAT tables from the boot sector */
  Read_Sector(0);
  sectors_per_fat=Decimal_Number(sector_buffer[22],sector_buffer[23],0,0);

  /*Re-create the FAT tables */
  mirror_map_pointer = 92;
  index=1;

  do
    {
    destination_sector=Decimal_Number(mirror_map[mirror_map_pointer],mirror_map[(mirror_map_pointer+1)],mirror_map[(mirror_map_pointer+2)],mirror_map[(mirror_map_pointer+3)]);
    source_sector=Decimal_Number(mirror_map[(mirror_map_pointer+4)],mirror_map[(mirror_map_pointer+5)],mirror_map[(mirror_map_pointer+6)],mirror_map[(mirror_map_pointer+7)]);

    Read_Sector(source_sector);
    Write_Sector(destination_sector);
    Write_Sector(destination_sector+sectors_per_fat);

    mirror_map_pointer=mirror_map_pointer + 8;
    index++;
    } while(index<=sectors_per_fat);

  /*Re-create the Root Directory */

  loop=1;

  /*If drive is a floppy drive, change the loop for a smaller root dir*/
  if(drive_type==FLOPPY) loop=18;

  do
    {
    destination_sector=Decimal_Number(mirror_map[mirror_map_pointer],mirror_map[(mirror_map_pointer+1)],mirror_map[(mirror_map_pointer+2)],mirror_map[(mirror_map_pointer+3)]);
    source_sector=Decimal_Number(mirror_map[(mirror_map_pointer+4)],mirror_map[(mirror_map_pointer+5)],mirror_map[(mirror_map_pointer+6)],mirror_map[(mirror_map_pointer+7)]);

    Read_Sector(source_sector);

    /* If user wants the file and directory names listed, list them. */
    if(1==list_file_and_directory_names) List_Names();

    Write_Sector(destination_sector);

    mirror_map_pointer=mirror_map_pointer + 8;

    loop++;
    } while(loop<=31);
  printf("\n\nUnformat operation complete!\n");
}


/* Verify Drive Mirror //////////////// */
void Verify_Drive_Mirror()
{
  long mirrored_sector;
  long system_sector;

  int loop;

  long mirror_map_pointer;
  long index;

  long current_sector;
  long mirror_map_start_sector;

  Read_Mirror_Map();

  printf("\n\nComparing drive mirror with file system...\n");

  /*Compare the boot sector */
  system_sector=Decimal_Number(mirror_map[84],mirror_map[85],mirror_map[86],mirror_map[87]);
  mirrored_sector=Decimal_Number(mirror_map[88],mirror_map[89],mirror_map[90],mirror_map[91]);

  Read_Sector(system_sector);

  Copy_Sector_Into_Control_Buffer();

  Read_Sector(mirrored_sector);

  if(Compare_Sector_Buffers()!=0)
    {
    printf("\n\nVerification failed at boot sector...operation terminated.\n\n");
    exit(1);
    }

  /*Compare the FAT tables */
  mirror_map_pointer = 92;
  index=1;

  do
    {
    system_sector=Decimal_Number(mirror_map[mirror_map_pointer],mirror_map[(mirror_map_pointer+1)],mirror_map[(mirror_map_pointer+2)],mirror_map[(mirror_map_pointer+3)]);
    mirrored_sector=Decimal_Number(mirror_map[(mirror_map_pointer+4)],mirror_map[(mirror_map_pointer+5)],mirror_map[(mirror_map_pointer+6)],mirror_map[(mirror_map_pointer+7)]);

    Read_Sector(system_sector);

    Copy_Sector_Into_Control_Buffer();

    Read_Sector(mirrored_sector);

    if(Compare_Sector_Buffers()!=0)
      {
      printf("\n\nVerification failed on 1st FAT table...operation terminated.\n\n");
      exit(1);
      }

    Read_Sector(system_sector+sectors_per_fat);

    if(Compare_Sector_Buffers()!=0)
      {
      printf("\n\nVerification failed on 2nd FAT table...operation terminated.\n\n");
      exit(1);
      }

    mirror_map_pointer=mirror_map_pointer + 8;
    index++;
    } while(index<=sectors_per_fat);

  /*Compare the Root Directory */

  loop=1;

  /*If drive is a floppy drive, change the loop for a smaller root dir*/
  if( (0==logical_drive) || (1==logical_drive) ) loop=18;


  do
    {
    system_sector=Decimal_Number(mirror_map[mirror_map_pointer],mirror_map[(mirror_map_pointer+1)],mirror_map[(mirror_map_pointer+2)],mirror_map[(mirror_map_pointer+3)]);
    mirrored_sector=Decimal_Number(mirror_map[(mirror_map_pointer+4)],mirror_map[(mirror_map_pointer+5)],mirror_map[(mirror_map_pointer+6)],mirror_map[(mirror_map_pointer+7)]);

    Read_Sector(system_sector);

    Copy_Sector_Into_Control_Buffer();

    Read_Sector(mirrored_sector);

    if(Compare_Sector_Buffers()!=0)
      {
      printf("\n\nVerification failed at root directory...operation terminated.\n\n");
      exit(1);
      }

    mirror_map_pointer=mirror_map_pointer + 8;

    loop++;
    } while(loop<=31);

  printf("\n\nMirror image has been verified.\n\n");
  exit(0);
}

/* Write Physical Sector */
void Write_Physical_Sector(int drive, int head, long cyl, int sector)
{
  int result;

  result=biosdisk(3, drive, head, cyl, sector, 1, sector_buffer);

  if (result!=0)
    {
    printf("\nDisk write error...operation terminated.\n");
    exit(4);
    }
}

/* Write Sector To Disk */
void Write_Sector(unsigned long sector)
{
  Convert_Logical_To_Physical(sector);

  printf("Cylinder:  %5i  ",translated_cylinder);
  printf("Head:  %2i      \n",translated_head);
  /* printf("Sector: %i \n",translated_sector); */

  /* Re-position cursor back to the beginning of the line */
  asm{
    /* Get current video display mode */
    mov ah,0x0f
    int 0x10

    /* Get cursor position */
    mov ah,0x03
    int 0x10

    /* Set cursor position to beginning of line */
    mov ah,0x02
    sub dh,1
    int 0x10
    }

  Write_Physical_Sector(physical_drive,translated_head,translated_cylinder,translated_sector);
}


/*
/////////////////////////////////////////////////////////////////////////////
// MAIN ROUTINE
/////////////////////////////////////////////////////////////////////////////
*/

void main(int argc, char *argv[])
{
  int true=1;

  /* if UNFORMAT is typed without any options */
  if(argc==1)
    {
    Display_Help_Screen();
    exit(1);
    }

  /* if UNFORMAT is typed with more than one option and
  // the first option is "/?" or "/PARTN" or "/partn" */
  if(argc>=2)
    {

    /* if "UNFORMAT /?" is entered  */
    true=strcmp("/?",argv[1]);
    if(0==true)
      {
      Display_Help_Screen();
      exit(0);
      }


    /* if "UNFORMAT /PARTN" or "UNFORMAT /partn" is entered */
    if(0==stricmp("/PARTN",argv[1]))
      {
      if(argc==3)
	{
	/*if "UNFORMAT /PARTN /L" is entered */
	if(0==stricmp("/L",argv[2]))
	  {
	  Restore_Partition_Tables(LIST);
	  exit(0);
	  }

	printf("\nSyntax error, type \"UNFORMAT /?\" for help.\n");
	exit(1);
	}
      else
	{
	Restore_Partition_Tables(FALSE);
	exit(0);
	}
      }
    }

  /*if UNFORMAT is typed with a drive letter */
  if(argc>=2)
    {
    char drive_letter[1];
    char *input;

    logical_drive=argv[1] [0];

    if(logical_drive>=97) logical_drive = logical_drive - 97;
    if(logical_drive>=65) logical_drive = logical_drive - 65;

    if( (logical_drive<0) || (logical_drive> 25) )
      {
      printf("\nSyntax error, type \"UNFORMAT /?\" for help.\n");
      exit(1);
      }

    drive_letter[0] = logical_drive+65;
    drive_letter[1] = 58;

    if(logical_drive>=2) drive_type=HARD;
    else drive_type=FLOPPY;

    Get_Drive_Parameters();

    true=1;
    true=stricmp("/j",argv[2]);
    if(0==true) Verify_Drive_Mirror();

    true=0;
    if(0==true)
      {
      if(argc>=3)
	{
	int switch_to_test = (argc-1);

	do
	  {
	  true=1;
	  true=stricmp("/u",argv[switch_to_test]);
	  if(0==true) unformat_without_mirror_file = 1;


	  true=1;
	  true=stricmp("/l",argv[switch_to_test]);
	  if(0==true) list_file_and_directory_names = 1;

	  true=1;
	  true=stricmp("/test",argv[switch_to_test]);
	  if(0==true) test = 1;

	  true=1;
	  true=stricmp("/p",argv[switch_to_test]);
	  if(0==true) print_output = 1;

	  switch_to_test--;
	  } while (switch_to_test>=2);

	}

      Unformat_Drive();
      }
    }
}










