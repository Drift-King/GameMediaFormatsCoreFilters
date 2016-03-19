//==========================================================================
//
// File: ROQSpecs.h
//
// Desc: Game Media Formats - Definitions of ROQ-related constants and structures
//
// Copyright (C) 2004 ANX Software.
//
// Based on the ROQ specs by Dr. Tim Ferguson (timf@csse.monash.edu.au).
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

#ifndef __GMF_ROQ_SPECS_H__
#define __GMF_ROQ_SPECS_H__

#include <windows.h>
#include <mmsystem.h>

//==========================================================================
// Constants
//==========================================================================

#define ROQ_CHUNKS_TO_SCAN			8
#define ROQ_SAMPLE_RATE				22050
#define ROQ_SAMPLE_BITS				16

#define ROQ_ID_ID					0x1084
#define ROQ_ID_SIZE					0xFFFFFFFF

#define ROQ_IDSTR_ID				"\x84\x10"
#define ROQ_IDSTR_SIZE				"\xFF\xFF\xFF\xFF"

#define ROQ_CHUNK_VIDEO_INFO		0x1001
#define ROQ_CHUNK_VIDEO_CODEBOOK	0x1002
#define ROQ_CHUNK_VIDEO_FRAME		0x1011
#define ROQ_CHUNK_SOUND_MONO		0x1020
#define ROQ_CHUNK_SOUND_STEREO		0x1021

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagROQ_CHUNK_HEADER {
	WORD	wID;		// Chunk ID
	DWORD	cbSize;		// Chunk size
	WORD	wArgument;	// Chunk argument
} ROQ_CHUNK_HEADER;

typedef struct tagROQ_VIDEO_INFO {
	WORD	wWidth;				// Video width
	WORD	wHeight;			// Video height
	WORD	wBlockDimension;	// Block dimension
	WORD	wSubBlockDimension;	// Sub-block dimension
} ROQ_VIDEO_INFO;

typedef struct tagROQ_VIDEO_FORMAT {
	WORD	nFramesPerSecond;	// Video frame rate
	WORD	wWidth;				// Video width
	WORD	wHeight;			// Video height
	WORD	wBlockDimension;	// Block dimension
	WORD	wSubBlockDimension;	// Sub-block dimension
} ROQ_VIDEO_FORMAT;

typedef struct tagROQADPCMWAVEFORMAT {
	WORD	nChannels;		// Number of channels
	DWORD	nSamplesPerSec;	// Sample rate
	WORD	wBitsPerSample;	// Sample resolution
} ROQADPCMWAVEFORMAT;

typedef struct tagROQ_CELL {
	BYTE y0, y1, y2, y3, u, v;
} ROQ_CELL;

typedef struct tagROQ_QCELL {
	BYTE idx[4];
} ROQ_QCELL;

#pragma pack()

#endif
