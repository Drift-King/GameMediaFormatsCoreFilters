//==========================================================================
//
// File: HNMSpecs.h
//
// Desc: Game Media Formats - Definitions of HNM-related constants and structures
//
// Copyright (C) 2004 ANX Software.
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

#ifndef __GMF_HNM_SPECS_H__
#define __GMF_HNM_SPECS_H__

#include <windows.h>
#include <mmsystem.h>

//==========================================================================
// Constants
//==========================================================================

#define HNM_FPS_DEFAULT 15

#define HNM_ID_HNM6			((DWORD)mmioFOURCC('H','N','M','6'))	// "HNM6"
#define HNM_ID_IX			( (WORD)mmioFOURCC('I','X', 0,  0 ))	// "IX"
#define HNM_ID_IV			( (WORD)mmioFOURCC('I','V', 0,  0 ))	// "IV"
#define HNM_ID_AA			( (WORD)mmioFOURCC('A','A', 0,  0 ))	// "AA"
#define HNM_ID_BB			( (WORD)mmioFOURCC('B','B', 0,  0 ))	// "BB"

#define HNM_IDSTR_HNM6		"HNM6"
#define HNM_IDSTR_RND		"Pascal URRO  R&D"
#define HNM_IDSTR_COPYRIGHT	"-Copyright CRYO-"

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagHNM_HEADER {
	DWORD	dwID;				// Format ID: "HNM6"
	WORD	wUnknown1;			// ??? Always zero
	BYTE	bSoundFlags;		// ??? 0x00 no sound, 0x80 stereo, 0x20 16bit
	BYTE	bBPP;				// ??? Bits per pixel
	WORD	wWidth;				// Video frame width
	WORD	wHeight;			// Video frame height
	DWORD	cbFile;				// File size
	DWORD	nFrames;			// Frame count
	DWORD	dwUnknown2;			// ???
	DWORD	cbMaxChunk;			// ??? Max chunk size (seems to be underestimated)
	DWORD	cbFrame;			// ??? Decompressed frame size
	CHAR	szRND[16];			// RND string: "Pascal URRO  R&D"
	CHAR	szCopyright[16];	// Copyright string: "-Copyright CRYO-"
} HNM_HEADER;

typedef struct tagHNM_BLOCK_HEADER {
	DWORD	cbBlock;	// Block size
	WORD	wBlockID;	// Block ID
	WORD	wUnknown;	// ???
} HNM_BLOCK_HEADER;

#pragma pack()

#endif
