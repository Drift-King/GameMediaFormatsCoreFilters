//==========================================================================
//
// File: MVESpecs.h
//
// Desc: Game Media Formats - Definitions of MVE-related constants and structures
//
// Copyright (C) 2004 ANX Software.
//
// Based on the MVE specs by <don't-know-whom> (<don't-know-email>).
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

#ifndef __GMF_MVE_SPECS_H__
#define __GMF_MVE_SPECS_H__

#include <windows.h>
#include <mmsystem.h>

//==========================================================================
// Constants
//==========================================================================

#define MVE_FRAME_DELTA_DEFAULT (8 * 8341 * 10)

#define MVE_ID_MAGIC1		(0x001A)
#define MVE_ID_MAGIC2		(0x0100)
#define MVE_ID_MAGIC3		(0x1133)

#define MVE_IDSTR_MVE		"Interplay MVE File\x1A\0"
#define MVE_IDSTR_MAGIC1	"\x1A\x00"
#define MVE_IDSTR_MAGIC2	"\x00\x01"
#define MVE_IDSTR_MAGIC3	"\x33\x11"

#define MVE_CHUNK_AUDIOINFO	0
#define MVE_CHUNK_AUDIODATA	1
#define MVE_CHUNK_VIDEOINFO	2
#define MVE_CHUNK_VIDEODATA	3
#define MVE_CHUNK_SHUTDOWN	4
#define MVE_CHUNK_END		5

#define MVE_SUBCHUNK_STOP			(0x00)
#define MVE_SUBCHUNK_END			(0x01)
#define MVE_SUBCHUNK_TIMER			(0x02)
#define MVE_SUBCHUNK_AUDIOINFO		(0x03)
#define MVE_SUBCHUNK_AUDIOCMD		(0x04)
#define MVE_SUBCHUNK_VIDEOINFO		(0x05)
#define MVE_SUBCHUNK_VIDEOCMD		(0x07)
#define MVE_SUBCHUNK_AUDIODATA		(0x08)
#define MVE_SUBCHUNK_AUDIOSILENCE	(0x09)
#define MVE_SUBCHUNK_VIDEOMODE		(0x0A)
#define MVE_SUBCHUNK_PALETTE_GRAD	(0x0B)
#define MVE_SUBCHUNK_PALETTE		(0x0C)
#define MVE_SUBCHUNK_PALETTE_RLE	(0x0D)
#define MVE_SUBCHUNK_VIDEOMAP		(0x0F)
#define MVE_SUBCHUNK_VIDEODATA		(0x11)

#define MVE_AUDIO_STEREO		1
#define MVE_AUDIO_16BIT			2
#define MVE_AUDIO_COMPRESSED	4

#define MVE_CHUNKS_TO_SCAN		32

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagMVE_HEADER {
	CHAR	szID[20];		// ID string: "Interplay MVE File\x1A\0"
	WORD	wMagic1;		// |
	WORD	wMagic2;		// | -- Magic numbers
	WORD	wMagic3;		// |
} MVE_HEADER;

typedef struct tagMVE_CHUNK_HEADER {
	WORD	cbData;			// Data size
	BYTE	bType;			// Type
	BYTE	bSubtype;		// Subtype (version)
} MVE_CHUNK_HEADER;

typedef struct tagMVE_TIMER_DATA {
	DWORD	dwRate;			// Frame delta (microseconds)
	WORD	wSubdivision;	// Subdivision (delta multiplier)
} MVE_TIMER_DATA;

typedef struct tagMVE_VIDEO_INFO {
	WORD	wWidth;			// Width (in units of block width (8))
	WORD	wHeight;		// Height (in units of block height (8))
	WORD	nBuffers;		// Number of buffers
	WORD	wHiColor;		// Is it a hi-color movie?
} MVE_VIDEO_INFO;

typedef struct tagMVE_VIDEO_MODE_INFO {
	WORD	wWidth;			// Width
	WORD	wHeight;		// Height
	WORD	wFlags;			// Flags???
} MVE_VIDEO_MODE_INFO;

typedef struct tagMVE_VIDEO_CMD {
	WORD	iPaletteStart;	// Palette start index
	WORD	nPaletteEntries;// Number of palette entries to set
	WORD	wUnknown;		// ???
} MVE_VIDEO_CMD;

typedef struct tagMVE_PALETTE_GRADIENT {
	BYTE	bBaseRB;		// Base index for R x B gradient
	BYTE	nR_RB;			// Number of R rows
	BYTE	nB_RB;			// Number of B columns
	BYTE	bBaseRG;		// Base index for R x G gradient
	BYTE	nR_RG;			// Number of R rows
	BYTE	nG_RG;			// Number of G columns
} MVE_PALETTE_GRADIENT;

typedef struct tagMVE_PALETTE_HEADER {
	WORD	iStart;			// Start index
	WORD	nEntries;		// Number of entries
} MVE_PALETTE_HEADER;

typedef struct tagMVE_AUDIO_INFO {
	WORD	wUnknown;		// ???
	WORD	wFlags;			// Audio flags (channels, bits, compression)
	WORD	wSampleRate;	// Sample rate
	union {
		WORD	cbBufferV0;	// Audio buffer size
		DWORD	cbBufferV1;	// ----||----
	};
} MVE_AUDIO_INFO;

typedef struct tagMVE_AUDIO_HEADER {
	WORD	wSequenceIndex;	// Audio chunk sequence index
	WORD	wStreamMask;	// Audio stream mask
	WORD	cbPCMData;		// PCM data size
} MVE_AUDIO_HEADER;

typedef struct tagMVE_VIDEO_HEADER {
	WORD wFrameHot;			// ???
	WORD wFrameCold;		// ???
	WORD wXOffset;			// X offset of the frame
	WORD wYOffset;			// Y offset of the frame
	WORD wXSize;			// X size of the frame (in units of block width (8))
	WORD wYSize;			// Y size of the frame (in units of block width (8))
	WORD wFlags;			// Bit 0: swap the backbuffers
} MVE_VIDEO_HEADER;

#pragma pack()

#endif
