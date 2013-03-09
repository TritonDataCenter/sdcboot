/*
// Program:  Mirror
// Written By:  Brian E. Reifsnyder
// Version:  0.2
// Copyright:  1999 under the terms of the GNU GPL
*/

#define NAME "Mirror"
#define VERSION "0.2"

/*
/////////////////////////////////////////////////////////////////////////////
//  INCLUDES
/////////////////////////////////////////////////////////////////////////////
*/

#include <bios.h>
#include <dos.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
/////////////////////////////////////////////////////////////////////////////
//  DEFINES
/////////////////////////////////////////////////////////////////////////////
*/

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

#define FIND 1
#define CREATE 2

#define FORCE 5
/*
/////////////////////////////////////////////////////////////////////////////
//  GLOBAL VARIABLES
/////////////////////////////////////////////////////////////////////////////
*/

unsigned char *sector_buffer;

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
int bad_boot_sector=FALSE;

unsigned long sectors_per_cluster;
long sectors_per_fat;
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

unsigned long partition_table_cylinders[8][24];
unsigned int partition_table_heads[8][24];
unsigned int partition_table_sectors[8][24];
int partition_table_number_of_partitions[8];

int hard_drive_index;
int hard_disk_installed[8];
int store_partition_table_flag=FALSE;

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

unsigned long beginning_of_mirror_image;
/* Buffers */
unsigned char *mirror_map;
unsigned char *mirror_buffer;

/*
/////////////////////////////////////////////////////////////////////////////
//  PROTOTYPES
/////////////////////////////////////////////////////////////////////////////
*/
unsigned long Decimal_Number(unsigned long hex1, unsigned long hex2, unsigned long hex3, unsigned long hex4);

void Allocate_Buffers();
void Clear_Sector_Buffer();
void Convert_Huge_To_Integers(unsigned long number);
void Convert_Logical_To_Physical(unsigned long sector);
void Deallocate_Buffers();
void Display_Help_Screen();
void Display_Invalid_Combination();
void Extract_Cylinder_and_Sector(unsigned long hex1, unsigned long hex2);
void Get_Drive_Parameters();
void Get_Physical_Floppy_Drive_Parameters();
void Get_Physical_Hard_Drive_Parameters();
void Read_Boot_Sector();
void Read_Partition_Table();
void Read_Physical_Sector(int drive, int head, long cyl, int sector);
void Read_Sector(unsigned long sector);
void Reset_Drive();
void Save_File_System();
void Save_Partition_Tables();
void Write_Physical_Sector(int drive, int head, long cyl, int sector);
void Write_Sector(unsigned long sector);

/*
/////////////////////////////////////////////////////////////////////////////
//  FUNCTIONS
/////////////////////////////////////////////////////////////////////////////
*/

/* Allocate Buffers */
void Allocate_Buffers()
{
  mirror_buffer=(unsigned char *)malloc(32768);
  mirror_map=(unsigned char *)malloc(5121);
  sector_buffer=(unsigned char *)malloc(512);

  if( (mirror_buffer==NULL) || (mirror_map==NULL) || (sector_buffer==NULL) )
    {
    printf("\nBuffer Allocation Failure...Program Terminated.\n");
    Deallocate_Buffers();
    exit(1);
    }
}

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

/* Convert Hexidecimal Number to a Decimal Number */
unsigned long Decimal_Number(unsigned long hex1, unsigned long hex2, unsigned long hex3, unsigned long hex4)
{
  return((hex1) + (hex2*256) + (hex3*65536) + (hex4*16777216));
}

/* Convert Huge number into 4 LMB integer values */
void Convert_Huge_To_Integers(unsigned long number)
{
  unsigned long temp=number;
  unsigned long mask=0x000000ff;

  integer1=temp & mask;

  temp=number;
  mask=0x0000ff00;
  integer2=(temp & mask) >> 8;

  temp=number;
  mask=0x00ff0000;
  integer3=(temp & mask) >> 16;

  temp=number;
  mask=0xff000000;
  integer4=(temp & mask) >> 24;
}


void Deallocate_Buffers()
{
  free(mirror_map);
  free(sector_buffer);
  free(mirror_buffer);
}

/* Extract Cylinder & Sector */
void Extract_Cylinder_and_Sector(unsigned long hex1, unsigned long hex2)
{
  unsigned long cylinder_and_sector = ( (256*hex2) + hex1 );

  g_sector = cylinder_and_sector % 64;

  g_cylinder = ( ( (cylinder_and_sector*4) & 768) + (cylinder_and_sector /256) );
}

/* Help Routine */
void Display_Help_Screen()
{
  printf("\n%6s Version %s\n\n",NAME,VERSION);
  printf("Syntax:\n\n");
  printf("MIRROR [drive:]\n");
  printf("MIRROR [/PARTN]\n");
  printf("\n");
  printf("  drive:     Saves UNFORMAT information to drive#.\n");
  printf("  /PARTN     Saves the partition tables to a:partnsav.fil for future\n");
  printf("               recovery via UNFORMAT /PARTN.\n");
  printf("\n\n\n\n\n");
}

void Display_Invalid_Combination()
{
  printf("\nInvalid combination of options...please consult documentation.\n");
  printf("Operation Terminated.\n");
  Deallocate_Buffers();
  exit(4);
}

/* Get Drive Parameters */

/* The physical drive parameters are necessary to allow this program  */
/* to bypass the DOS interrupts.                                      */
void Get_Drive_Parameters()
{
  int drives_found_flag;
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

    drive_maximum_heads=total_heads;

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
      Deallocate_Buffers();
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
      Deallocate_Buffers();
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

/* Read Boot Sector /////////////////// */
void Read_Boot_Sector()
{
  long fat_16_total_sectors;
  long fat_12_total_sectors;

  Read_Sector(0);

  sectors_per_fat = sector_buffer[22];
  fat_16_total_sectors=Decimal_Number(sector_buffer[32],sector_buffer[33],sector_buffer[34],sector_buffer[35]);
  fat_12_total_sectors=Decimal_Number(sector_buffer[19],sector_buffer[20],0,0);
  number_of_root_directory_entries=Decimal_Number(sector_buffer[17],sector_buffer[18],0,0);

  if(fat_16_total_sectors==0)
    {
    type_of_fat=FAT12;
    number_of_logical_sectors_on_drive=fat_12_total_sectors;
    }
  else
    {
    type_of_fat=FAT16;
    number_of_logical_sectors_on_drive=fat_16_total_sectors;
    }
  if( (fat_12_total_sectors>0) && (fat_16_total_sectors>0) ) bad_boot_sector=TRUE;

  sectors_per_cluster=Decimal_Number(sector_buffer[13],0,0,0);
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

  int store_partition_counter=0;

  unsigned long extended_cylinder;
  unsigned long extended_head;
  unsigned long extended_sector;

  Read_Physical_Sector(physical_drive,0,0,1);

  if(store_partition_table_flag==TRUE)
    {
    partition_table_cylinders[hard_drive_index][store_partition_counter]=0;
    partition_table_heads[hard_drive_index][store_partition_counter]=0;
    partition_table_sectors[hard_drive_index][store_partition_counter]=1;
    partition_table_number_of_partitions[hard_drive_index]=store_partition_counter;
    store_partition_counter++;
    }

  do{
    if(pointer==4) partition_designation=EXTENDED;

    if((pointer>=4) && (extended==TRUE))
      {
      Read_Physical_Sector(physical_drive,extended_head,extended_cylinder,extended_sector);

      if(store_partition_table_flag==TRUE)
	{
	partition_table_cylinders[hard_drive_index][store_partition_counter]=extended_cylinder;
	partition_table_heads[hard_drive_index][store_partition_counter]=extended_head;
	partition_table_sectors[hard_drive_index][store_partition_counter]=extended_sector;
	partition_table_number_of_partitions[hard_drive_index]=store_partition_counter;
	store_partition_counter++;
	}

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
      Deallocate_Buffers();
      exit(4);
      }
}

/* Read Sector From Disk */
void Read_Sector(unsigned long sector)
{
  Convert_Logical_To_Physical(sector);
  Read_Physical_Sector(physical_drive,translated_head,translated_cylinder,translated_sector);
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

/* Save the old file system for possible recovery with unformat */
void Save_File_System()
{
  int end_of_root_directory_flag;

  long loop=512;
  unsigned long pointer=0;

  unsigned long offset_from_end=5;

  unsigned long destination_sector;
  unsigned long source_sector;

  unsigned long mirror_beginning;
  unsigned long mirror_map_beginning;
  unsigned long mirror_map_size;
  unsigned long mirror_size;
  unsigned long number_of_bytes_in_mirror_map;

  unsigned long beginning_of_fat;
  unsigned long beginning_of_root_directory;

  unsigned long end_of_fat;
  unsigned long end_of_root_directory;

  unsigned long cluster;
  unsigned long remainder;
  unsigned long fat_table_1_start_sector;
  unsigned long fat_table_2_start_sector;
  unsigned long total_clusters;
  unsigned long fat_table_offset_sector;

  unsigned int sector_flag[] = {
  'A','M','S','E','S','L','I','F','V','A','S',
  'R','O','R','I','M','E','S','A','E','P'};

  unsigned int mirror_map_header[] = {
  ':','\\','M','I','R','R','O','R','.','F','I','L'};

  if(drive_type==HARD) offset_from_end=20;

  /* Get the boot sector, compute the FAT size, compute the root dir size,*/
  /* and get the end of the logical drive. */
  Read_Boot_Sector();

  printf("\n\nSaving UNFORMAT information...\n");

  /* Compute the beginning sector of the mirror map and the size of */
  /* the mirror image.     */
  mirror_size=1+sectors_per_fat+66;

  mirror_map_size=(mirror_size/64)+2;

  mirror_beginning=(number_of_logical_sectors_on_drive-mirror_size)-offset_from_end;
  mirror_map_beginning=mirror_beginning-mirror_map_size;

  /* Compute the locations of the first FAT and the root directory */
  beginning_of_fat=1;
  end_of_fat=beginning_of_fat+sectors_per_fat-1;

  beginning_of_root_directory=(beginning_of_fat+sectors_per_fat-1)*2;
  end_of_root_directory=beginning_of_root_directory+(number_of_root_directory_entries/32);

  /* Reserve space in FAT tables so that the */
  /* mirror image is never overwritten       */
  fat_table_1_start_sector=1;
  fat_table_2_start_sector=fat_table_1_start_sector+sectors_per_fat;

  cluster=mirror_map_beginning/sectors_per_cluster;
  total_clusters=number_of_logical_sectors_on_drive/sectors_per_cluster;

  /* FAT16 has 256 entries per sector */
  /* FAT12 has 384 entries per sector */


  do
    {
    if(type_of_fat==FAT12)
      {
      printf("\nFAT 12 file systems are not supported at this time.\n");
      Deallocate_Buffers();
      exit(6);
      }

    if(type_of_fat==FAT16)
      {
      fat_table_offset_sector=(cluster+2)/256;
      remainder=((cluster+2) % 256);

      Read_Sector(fat_table_offset_sector+fat_table_1_start_sector);
      sector_buffer[(remainder*2)]=0xff;
      sector_buffer[(remainder*2)+1]=0xf6;

      Write_Sector(fat_table_offset_sector+fat_table_1_start_sector);
      Write_Sector(fat_table_offset_sector+fat_table_2_start_sector);
      }

    if(type_of_fat==FAT32)
      {
      printf("\nFAT 32 file systems are not supported at this time.\n");
      Deallocate_Buffers();
      exit(6);
      }

    printf("%u\n",cluster);

    cluster++;
    }while(cluster<=total_clusters);

  /* Clear the last sectors of a hard disk          */
  /* to ensure that UNFORMAT does not get confused. */
  pointer=mirror_map_beginning;

  Clear_Sector_Buffer();
  do
    {
    Write_Sector(pointer);
    pointer++;
    }while(pointer<number_of_logical_sectors_on_drive);

  /* Write the mirror map pointer to the last sectors of the logical drive. */
  Clear_Sector_Buffer();

  Convert_Huge_To_Integers(mirror_map_beginning);

  sector_buffer[0]=integer1;
  sector_buffer[1]=integer2;
  sector_buffer[2]=integer3;
  sector_buffer[3]=integer4;

  pointer=4;

  do                                           /* Add pointer sector flag */
    {
    sector_buffer[pointer]=sector_flag[pointer-4];
    pointer++;
    }while(pointer<=24);

  Write_Sector(number_of_logical_sectors_on_drive-offset_from_end);

  /* Create the mirror map and copy the file system to the mirror.  */
  Clear_Sector_Buffer();

  pointer=0;

  do                                           /* Clear mirror_map buffer */
    {
    mirror_map[pointer]=0;
    pointer++;
    }while(pointer<=5120);

  mirror_map[0]=logical_drive+65;

  pointer=1;

  do                                           /* Add mirror map header */
    {
    mirror_map[pointer]=mirror_map_header[pointer-1];
    pointer++;
    }while(pointer<=12);

					       /* Main mirror map creation */
					       /* and copying loop.        */
  pointer=84;
  source_sector=0;
  destination_sector=mirror_beginning;

  end_of_root_directory_flag=FALSE;
  number_of_bytes_in_mirror_map=0;

  do
    {

    if( (source_sector!=0) && (source_sector<beginning_of_fat) ) source_sector=beginning_of_fat;


    if( (source_sector>end_of_fat) && (source_sector<beginning_of_root_directory) ) source_sector=beginning_of_root_directory;

    /* Copy mirror image one sector at a time */
    Read_Sector(source_sector);

    Write_Sector(destination_sector);

    /* Enter mapping information into mirror map buffer */

    Convert_Huge_To_Integers(source_sector);

    mirror_map[pointer+0]=integer1;
    mirror_map[pointer+1]=integer2;
    mirror_map[pointer+2]=integer3;
    mirror_map[pointer+3]=integer4;

    Convert_Huge_To_Integers(destination_sector);

    mirror_map[pointer+4]=integer1;
    mirror_map[pointer+5]=integer2;
    mirror_map[pointer+6]=integer3;
    mirror_map[pointer+7]=integer4;

    source_sector++;
    destination_sector++;
    pointer=pointer+8;
    number_of_bytes_in_mirror_map=pointer;

    if(source_sector>end_of_root_directory) end_of_root_directory_flag=TRUE;

    }while(end_of_root_directory_flag==FALSE);

  /* Write trailer in mirror map */

  mirror_map[pointer+0]=0;
  mirror_map[pointer+1]=0;
  mirror_map[pointer+2]=0;
  mirror_map[pointer+3]=0;

  /* Write the mirror map onto the disk.   */

  pointer=0;
  destination_sector=mirror_map_beginning;

  do
    {
    loop=0;

    do                                         /* Load the sector buffer */
      {
      sector_buffer[loop]=mirror_map[pointer+loop];

      loop++;
      }while(loop<512);

    Write_Sector(destination_sector);          /* Write the mirror map   */
					       /* sector.                */
    destination_sector++;
    pointer=pointer+512;
    }while(pointer < number_of_bytes_in_mirror_map);
}

/* Save the partition tables to a floppy disk */
void Save_Partition_Tables()
{
/*
Map of the PARTNSAV.FIL for FreeDOS:

(I do not plan on using an MS-DOS compatible file.)

offset:00
FreeDOS partitio
n mirror file

offset:32-127 reserved for future expansion

offset:128(drive#)      129(lb offset)   130(mb offset) 131(hb offset)
       132(lb dest cyl) 133(hb dest cyl) 134(dest head) 135(dest sect.)

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
  unsigned long partition_sector_offset=1024;
  int drive_index;
  int partition_index;

  FILE *file_pointer;

  unsigned char partition_table_map[1024];

  unsigned int header[]= {'F','r','e','e','D','O','S',' ','p','a','r',
			  't','i','t','i','o','n',' ','m','i','r','r',
			  'o','r',' ','f','i','l','e'};

  store_partition_table_flag=TRUE;

  /* Get the sectors of the partition tables */
  index=0;

  do
    {
    if(hard_disk_installed[index]==TRUE)
      {
      hard_drive_index=index;
      physical_drive=index+128;
      Read_Partition_Table();
      }
    index++;
    }while(index<=7);

  store_partition_table_flag=FALSE;

  /* Clear partition_table_map[] */
  index=0;

  do
    {
    partition_table_map[index]=0;
    index++;
    }while(index<=1023);

  /* Create the map of the PARTNSAV.FIL file */
  index=0;

						 /* Add header */
  do
    {
    partition_table_map[index]=header[index];
    index++;
    }while(index<29);


  index=128;                                    /* Add the map */
  drive_index=0;

  do
    {
    if(hard_disk_installed[drive_index]==TRUE)
      {
      partition_index=0;

      do
	{
	/* Add the drive number */
	partition_table_map[index+0]=drive_index+128;

	/* Add the offset of the sector in the file */
	Convert_Huge_To_Integers(partition_sector_offset);

	partition_table_map[index+1]=integer1;
	partition_table_map[index+2]=integer2;

	/* Add the cylinder */
	Convert_Huge_To_Integers(partition_table_cylinders[drive_index][partition_index]);

	partition_table_map[index+3]=integer1;
	partition_table_map[index+4]=integer2;

	/* Add the head */
	Convert_Huge_To_Integers(partition_table_heads[drive_index][partition_index]);

	partition_table_map[index+5]=integer1;
	partition_table_map[index+6]=integer2;

	/* Add the sector */
	partition_table_map[index+7]=partition_table_sectors[drive_index][partition_index];

	partition_sector_offset=partition_sector_offset+512;
	index=index+8;
	partition_index++;
	}while(partition_index<=partition_table_number_of_partitions[drive_index]);
      }
    drive_index++;
    }while(drive_index<=7);

  /* Create the PARTNSAV.FIL */

  file_pointer=fopen("A:\PARTNSAV.FIL","wb");

  if(!file_pointer)
    {
    printf("\nError creating \"PARTNSAV.FIL\" on drive A:, disk may be full, write\n");
    printf("protected, damaged, or non-existant...operation terminated.\n");
    Deallocate_Buffers();
    exit(4);
    }

  /* Write the map to the PARTNSAV.FIL file */
  fwrite(partition_table_map,1024,1,file_pointer);

  /* Write the partition tables to the PARTNSAV.FIL file */


  drive_index=0;

  do
    {
    if(hard_disk_installed[drive_index]==TRUE)
      {
      partition_index=0;

      do
	{
	Read_Physical_Sector((drive_index+128),partition_table_heads[drive_index][partition_index],partition_table_cylinders[drive_index][partition_index],partition_table_sectors[drive_index][partition_index]);
	fwrite(sector_buffer,512,1,file_pointer);

	partition_index++;
	}while(partition_index<=partition_table_number_of_partitions[drive_index]);
      }
    drive_index++;
    }while(drive_index<=7);


  fclose(file_pointer);
}

/* Write Physical Sector */
void Write_Physical_Sector(int drive, int head, long cyl, int sector)
{
  int result;

  result=biosdisk(3, drive, head, cyl, sector, 1, sector_buffer);

  if (result!=0)
    {
    printf("\nDisk write error...operation terminated.\n");
    Deallocate_Buffers();
    exit(4);
    }
}

/* Write Sector To Disk */

void Write_Sector(unsigned long sector)
{
  Convert_Logical_To_Physical(sector);
  Write_Physical_Sector(physical_drive,translated_head,translated_cylinder,translated_sector);
}

/*
/////////////////////////////////////////////////////////////////////////////
//  MAIN ROUTINE
/////////////////////////////////////////////////////////////////////////////
*/
void main(int argc, char *argv[])
{
  Allocate_Buffers();

  /* if MIRROR is typed without any options */
  if(argc==1)
    {
    Display_Help_Screen();
    Deallocate_Buffers();
    exit(1);
    }

  /* if MIRROR /? is typed */
  if(argv[1][1]=='?')
    {
    Display_Help_Screen();
    Deallocate_Buffers();
    exit(0);
    }

  /* if MIRROR /PARTN is typed */
  if(argv[1][1]=='p' || argv[1][1]=='P')
    {
    logical_drive=2;
    Get_Drive_Parameters();
    Save_Partition_Tables();
    Deallocate_Buffers();
    exit(0);
    }

  /*if MIRROR is typed with a drive letter */
  if(argc>=2)
    {
    int index=3;

    char compare_character[1];
    char drive_letter[1];

    logical_drive=argv[1] [0];

    if(logical_drive>=97) logical_drive = logical_drive - 97;
    if(logical_drive>=65) logical_drive = logical_drive - 65;

    if( (logical_drive<0) || (logical_drive> 25) )
      {
      printf("\nDrive letter is out of range...Operation Terminated.\n");
      Deallocate_Buffers();
      exit(4);
      }

    drive_letter[0] = logical_drive+65;
    drive_letter[1] = 58;

    /* Default to a drive type of floppy unless... */
    drive_type=FLOPPY;

    /* ...the drive is a hard drive. */
    if(logical_drive>1)
      {
      drive_type=HARD;
      }

    /* Logical drives are designated as follows:              */
    /* drive   letter   descriptions                          */
    /*    0       A     First floppy drive                    */
    /*    1       B     Second floppy drive                   */
    /*    2       C     Primary partition of first hard disk  */
    /*    3       D     Primary partition of second hard disk */
    /*                   or first extended partition of first */
    /*                   hard disk.                           */
    /*    etc.    etc.  etc.                                  */

    /* When the physical drive # is established for hard disks */
    /* the physical hard disk # will become drive              */


    if(argc>=3)
      {
      index=3;
      do
	{
	compare_character[0]=argv[index-1][1];

	/* Determine which switches were entered */


	index++;
	}while(index<=argc);
      }

    /* Get drive parameters */
    Get_Drive_Parameters();

    /* Ensure that valid switch combinations were entered */

    /* Mirror drive */
    Save_File_System();
    }
  Deallocate_Buffers();
}
