//==========================================================================
//
// File: FSTSpecs.h
//
// Desc: Game Media Formats - Definitions of FST-related constants and structures
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

#ifndef __GMF_FST_SPECS_H__
#define __GMF_FST_SPECS_H__

#include <windows.h>
#include <mmsystem.h>

//==========================================================================
// Constants
//==========================================================================

#define FST_ID_2TSF	((DWORD)mmioFOURCC('2','T','S','F'))	// "2TSF"

#define FST_IDSTR_2TSF	"2TSF"

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagFST_HEADER {
	DWORD	dwID;				// "2TSF"
	DWORD	dwVideoWidth;		// Video width
	DWORD	dwVideoHeight;		// Video height
	DWORD	dwUnknown1;			// ???
	DWORD	nVideoFrames;		// Number of frames
	DWORD	nFramesPerSecond;	// Video frame rate
	DWORD	dwAudioSampleRate;	// Audio sample rate
	WORD	nAudioBits;			// Audio bit resolution
	WORD	wAudioChannels;		// ??? 0 -- mono, 1 -- stereo
} FST_HEADER;

typedef struct tagFST_FRAME_ENTRY {
	DWORD	cbImage;			// Size of image data
	WORD	cbSound;			// Size of sound data
} FST_FRAME_ENTRY;

#pragma pack()

#endif
