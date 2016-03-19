//==========================================================================
//
// File: VQASpecs.h
//
// Desc: Game Media Formats - Definitions of VQA-related constants and structures
//
// Copyright (C) 2004 ANX Software.
//
// Based on the VQA specs by Gordan Ugarkovic (ugordan@yahoo.com).
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

#ifndef __GMF_VQA_SPECS_H__
#define __GMF_VQA_SPECS_H__

#include <windows.h>
#include <mmsystem.h>

// Utility macro: convert DWORD from big-endian to little-endian
#define SWAPDWORD(x) (			\
	(((x) & 0xFF) << 24)	+	\
	(((x) >> 24) & 0xFF)	+	\
	(((x) >> 8) & 0xFF00)	+	\
	(((x) << 8) & 0xFF0000)		\
)

//==========================================================================
// Constants
//==========================================================================

// Chunk IDs
#define VQA_ID_FORM	((DWORD)mmioFOURCC('F','O','R','M'))	// "FORM"
#define VQA_ID_WVQA	((DWORD)mmioFOURCC('W','V','Q','A'))	// "WVQA"
#define VQA_ID_VQHD	((DWORD)mmioFOURCC('V','Q','H','D'))	// "VQHD"
#define VQA_ID_CINF	((DWORD)mmioFOURCC('C','I','N','F'))	// "CINF"
#define VQA_ID_CIND	((DWORD)mmioFOURCC('C','I','N','D'))	// "CIND"
#define VQA_ID_FINF	((DWORD)mmioFOURCC('F','I','N','F'))	// "FINF"
#define VQA_ID_SND0	((DWORD)mmioFOURCC('S','N','D','0'))	// "SND0"
#define VQA_ID_SND1	((DWORD)mmioFOURCC('S','N','D','1'))	// "SND1"
#define VQA_ID_SND2	((DWORD)mmioFOURCC('S','N','D','2'))	// "SND2"
#define VQA_ID_VQFR	((DWORD)mmioFOURCC('V','Q','F','R'))	// "VQFR"
#define VQA_ID_VQFL	((DWORD)mmioFOURCC('V','Q','F','L'))	// "VQFL"

// VQFR/VQFL Subchunk IDs
#define VQA_ID_CBFZ	((DWORD)mmioFOURCC('C','B','F','Z'))	// "CBFZ"
#define VQA_ID_CBF0	((DWORD)mmioFOURCC('C','B','F','0'))	// "CBF0"
#define VQA_ID_CBPZ	((DWORD)mmioFOURCC('C','B','P','Z'))	// "CBPZ"
#define VQA_ID_CBP0	((DWORD)mmioFOURCC('C','B','P','0'))	// "CBP0"
#define VQA_ID_CPLZ	((DWORD)mmioFOURCC('C','P','L','Z'))	// "CPLZ"
#define VQA_ID_CPL0	((DWORD)mmioFOURCC('C','P','L','0'))	// "CPL0"
#define VQA_ID_VPTR	((DWORD)mmioFOURCC('V','P','T','R'))	// "VPTR"
#define VQA_ID_VPTZ	((DWORD)mmioFOURCC('V','P','T','Z'))	// "VPTZ"
#define VQA_ID_VPRZ	((DWORD)mmioFOURCC('V','P','R','Z'))	// "VPRZ"

#define VQA_IDSTR_FORM	"FORM"
#define VQA_IDSTR_WVQA	"WVQA"

#define VQA_CODE_SKIP			0
#define VQA_CODE_REPEAT_SHORT	1
#define VQA_CODE_PUT_ARRAY		2
#define VQA_CODE_PUT			3
#define VQA_CODE_REPEAT_LONG	5

#define VQA_CHUNKS_TO_SCAN	16

#define VQA_PALETTE_MARKER 0x40000000

#define MAX_CODEBOOK_VECTORS 0xFF00
#define MAX_CODEBOOK_SIZE (MAX_CODEBOOK_VECTORS * 4 * 4 * 2)
#define MAX_PALETTE_SIZE (3 * 256)

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagVQA_FILE_HEADER {
	DWORD	dwFileID;	// == "FORM"
	DWORD	cbFileSize;	// File size
	DWORD	dwFormatID;	// == "WVQA"
} VQA_FILE_HEADER;

typedef struct tagVQA_CHUNK_HEADER {
	DWORD	dwID;		// Chunk ID
	DWORD	cbSize;		// Chunk size
} VQA_CHUNK_HEADER;

typedef struct tagVQA_INFO {
	WORD	wVersion;			// VQA version number
	WORD	wFlags;				// VQA flags
	WORD	nVideoFrames;		// Number of frames
	WORD	wVideoWidth;		// Movie width (pixels)
	WORD	wVideoHeight;		// Movie height (pixels)
	BYTE	bBlockWidth;		// Width of each image block (pixels)
	BYTE	bBlockHeight;		// Height of each image block (pixels)
	BYTE	nFramesPerSecond;	// Frame rate of the VQA
	BYTE	nCBParts;			// How many images use the same lookup table
	WORD	nColors;			// Maximum number of colors used in VQA
	WORD	nMaxBlocks;			// Maximum number of image blocks
	DWORD	dwUnknown1;			// Always 0 ???
	WORD	wUnknown2;			// Some kind of size ???
	WORD	wAudioSampleRate;	// Sound sampling frequency
	BYTE	nAudioChannels;		// Number of sound channels
	BYTE	nAudioBits;			// Sound resolution
	DWORD	dwUnknown3;			// Always 0 ???
	WORD	wUnknown4;			// 0 in old VQAs, 4 in HiColor ones ???
	DWORD	cbMaxCBFZSize;		// 0 in old VQAs, max. CBFZ size in HiColor
	DWORD	dwUnknown5;			// Always 0 ???
} VQA_INFO;

typedef struct tagVQA_CIND_ENTRY {
	WORD	iFrame;		// Frame index
	DWORD	cbSize;		// Full codebook size
} VQA_CIND_ENTRY;

#pragma pack()

#endif

