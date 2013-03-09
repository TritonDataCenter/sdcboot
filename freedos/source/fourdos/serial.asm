

;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
;
;  (1) The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
;
;  (2) The Software, or any portion of it, may not be compiled for use on any
;  operating system OTHER than FreeDOS without written permission from Rex Conn
;  <rconn@jpsoft.com>
;
;  (3) The Software, or any portion of it, may not be used in any commercial
;  product without written permission from Rex Conn <rconn@jpsoft.com>
;
;  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.



// call DOS to get the volume label
char * QueryVolumeInfo( register char *arg, register char *volume_name, unsigned long *ulSerial )
{
	char szVolName[] = "C:\\*.*";
	FILESEARCH dir;
	struct {
		int info_level;
		unsigned long disk_serial_number;
		char disk_label[11];
		char filesystem_type[8];
	} bpb;

	if (( arg = gcdir( arg,0 )) == NULL )
		return NULL;

	// get the drive spec
	szVolName[0] = (char)_ctoupper( *arg );

	// get the serial number (local drives, DOS 4+ & OS/2 2.0 VDMs only)
	if ( QueryDriveRemote( szVolName[0] - 64 ))
		goto dos_3;

_asm {
	cmp	_osmajor, 4
	jb	dos_3
	mov	ax, 06900h	; int 21h 440Dh 66 call doesn't work in OS/2!
	xor	bh, bh
	mov	bl, byte ptr szVolName[0]
	sub	bl, 64
	xor	cx, cx
	lea	dx, word ptr bpb
	int	21h		; return serial # & volume info
	jc	dos_3
}
	*ulSerial = bpb.disk_serial_number;
dos_3:

	// get disk label (don't use the one in the BPB because it may not
	//   be the same as the one in the root directory!)
	if ( find_file( FIND_FIRST, szVolName, 0x108, &dir, NULL ) != NULL ) {

		// save volume name & remove a period preceding the extension
		sscanf( dir.name, "%12[^\001]", volume_name );
		if (( strlen( volume_name ) > 8 ) && ( volume_name[8] == '.' ))
			strcpy( volume_name+8, volume_name+9 );

	} else
		strcpy( volume_name, UNLABELED );	// disk isn't labeled

	return volume_name;
}


