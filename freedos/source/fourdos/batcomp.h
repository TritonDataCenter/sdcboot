

//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  (1) The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  (2) The Software, or any portion of it, may not be compiled for use on any
//  operating system OTHER than FreeDOS without written permission from Rex Conn
//  <rconn@jpsoft.com>
//
//  (3) The Software, or any portion of it, may not be used in any commercial
//  product without written permission from Rex Conn <rconn@jpsoft.com>
//
//  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.


// Batch compiler data structures and parameters

#define BC_MAX_USER_KEY 4
#define BC_KK_LEN 4
#define BC_KEY_LEN 12
#define BC_COM_SIZE 30

// Offsets to generate key to encrypt user key.  The first four bytes are
// used to search for this string by the meta-key installer; the second
// four bytes are the actual offsets.  Starting with ' ' (32) these
// offsets generate the key 'maru' (a dog's name!).  In decimal the
// offsets are 77, -12, 17, 3.
#define KKTAG "\x07F\x008\x093\x0C7"
#define KKOFFSETS KKTAG "\x04D\x0F4\x011\x003"

typedef struct {
	unsigned short usSignature;				// compression signature
	unsigned short fEncrypted;				// encryption flag
	unsigned short usCount;					// uncompressed byte count
	unsigned char cCommon[BC_COM_SIZE];		// common character list
} CompHead;

typedef struct {
	unsigned short usSignature;				// compression signature
	unsigned short usCount;					// uncompressed byte count
	unsigned char cCommon[BC_COM_SIZE];		// common character list
} OldCompHead;
