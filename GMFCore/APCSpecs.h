//==========================================================================
//
// File: APCSpecs.h
//
// Desc: Game Media Formats - Definitions of APC-related constants and structures
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

#ifndef __GMF_APC_SPECS_H__
#define __GMF_APC_SPECS_H__

#include <windows.h>
#include <mmsystem.h>

//==========================================================================
// Constants
//==========================================================================

#define APC_ID_CRYO		((DWORD)mmioFOURCC('C','R','Y','O'))	// "CRYO"
#define APC_ID_APC		((DWORD)mmioFOURCC('_','A','P','C'))	// "_APC"
#define APC_ID_VER		((DWORD)mmioFOURCC('1','.','2','0'))	// "1.20"

#define APC_IDSTR_CRYO	"CRYO"
#define APC_IDSTR_APC	"_APC"
#define APC_IDSTR_VER	"1.20"

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagAPC_HEADER {
	DWORD	dwID1;			// Format ID1: "CRYO"
	DWORD	dwID2;			// Format ID2: "_APC"
	DWORD	dwVersion;		// Version: always "1.20" ???
	DWORD	nSamples;		// Number of samples in the file
	DWORD	dwSampleRate;	// Sample rate
	LONG	lLeftSample;	// Initial value for the left sample
	LONG	lRightSample;	// Initial value for the right sample
	BOOL	bIsStereo;		// Is it a stereo file?
} APC_HEADER;

#pragma pack()

#endif
