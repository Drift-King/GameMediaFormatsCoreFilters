//==========================================================================
//
// File: VMDSpecs.h
//
// Desc: Game Media Formats - Definitions of VMD-related constants and structures
//
// Copyright (C) 2004 ANX Software.
//
// Based on the VMD specs by Mike Melanson (melanson@pcisys.net) and 
// Vladimir "VAG" Gneushev (vagsoft@mail.ru)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License 
// along with this program; if not, write to the Free Software 
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//==========================================================================

#ifndef __GMF_VMD_SPECS_H__
#define __GMF_VMD_SPECS_H__

#include <windows.h>

//==========================================================================
// Constants
//==========================================================================

#define VMD_ID_HEADERSIZE		0x032E

#define VMD_IDSTR_HEADERSIZE	"\x2E\x03"

#define VMD_DEFAULT_FPS 10

#define VMD_FRAME_AUDIO	1
#define VMD_FRAME_VIDEO	2

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagVMD_HEADER {
	WORD	cbHeader;			// Length of header, this should be 0x32E (814)
	WORD	wHandle;			// Placeholder for VMD handle
	WORD	wUnknown1;			// ???
	WORD	nBlocks;			// Number of blocks in table of contents
	WORD	wVideoTop;			// Top corner coordinate of video frame
	WORD	wVideoLeft;			// Left corner coordinate of video frame
	WORD	wVideoWidth;		// Width of video frame
	WORD	wVideoHeight;		// Height of video frame
	WORD	wVideoFlags;		// Video flags
	WORD	nFramesPerBlock;	// Frames per block
	DWORD	dwDataOffset;		// Absolute file offset of multimedia data
	DWORD	dwUnknown2;			// ???
	struct tagRGBTriple {
		BYTE bRed, bGreen, bBlue;
	} Palette[256];				// RGB palette entries, 3 bytes/entry in R-G-B order
	DWORD	cbFrameBuffer;		// Recommended size (bytes) of data frame load buffer
	DWORD	cbVideoBuffer;		// Recommended size (bytes) of unpack buffer for video decoding
	WORD	wAudioSampleRate;	// Audio sample rate
	WORD	cbAudioFrameLength;	// Audio frame length
	WORD	nAudioBuffers;		// Number of sound buffers
	BYTE	bUnknown3;			// ???
	BYTE	bAudioFlags;		// Audio flag
	DWORD	dwTableOffset;		// Absolute file offset of table of contents 
} VMD_HEADER;

typedef struct tagVMD_BLOCK_ENTRY {
	WORD	wUnknown;			// ???
	DWORD	dwOffset;			// Offset to the block data
} VMD_BLOCK_ENTRY;

typedef struct tagVMD_FRAME_ENTRY {
	BYTE	bType;				// Data type
	BYTE	bUnknown1;			// ???
	DWORD	cbFrameData;		// Frame data length

	union {

	struct tagAudioFrame {
		BYTE	bAudioFlags;	// Audio flags
		BYTE	bUnknown2[9];	// ???
	};

	struct tagVideoFrame {
		WORD	wImageLeft;		// Left coordinate of video frame
		WORD	wImageTop;		// Top coordinate of video frame
		WORD	wImageRight;	// Right coordinate of video frame
		WORD	wImageBottom;	// Bottom coordinate of video frame
		BYTE	bUnknown3;		// ???
		BYTE	bPaletteFlag;	// Bit 1 (byte[15] & 0x02) indicates a new palette
	};

	};
} VMD_FRAME_ENTRY;

#pragma pack()

#endif
