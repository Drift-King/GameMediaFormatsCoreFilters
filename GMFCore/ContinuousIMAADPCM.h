//==========================================================================
//
// File: ContinuousIMAADPCM.h
//
// Desc: Game Media Formats - Definitions of continuous IMA ADPCM structures
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

#ifndef __GMF_CONTINUOUS_IMA_ADPCM_H__
#define __GMF_CONTINUOUS_IMA_ADPCM_H__

#include <windows.h>
#include <initguid.h>

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagCIMAADPCMINFO {
	LONG	lSample;			// Initial value for the sample
	CHAR	chIndex;			// Initial value for the step table index
} CIMAADPCMINFO;

typedef struct tagCIMAADPCMWAVEFORMAT {
	WORD	nChannels;			// Number of channels
	DWORD	nSamplesPerSec;		// Sample rate
	WORD	wBitsPerSample;		// Sample resolution
	BOOL	bIsHiNibbleFirst;	// Should the higher nibble be processed first?
	DWORD	dwReserved;			// Reserved for internal use
	CIMAADPCMINFO pInit[1];		// Array if CIMAADPCMINFO structs for all channels
} CIMAADPCMWAVEFORMAT;

// Reserved flags (nibbles interleaving type)
#define CIMAADPCM_INTERLEAVING_NORMAL	0	// Normal interleaving: LR LR ..
#define CIMAADPCM_INTERLEAVING_DOUBLE	1	// Doubled interleaving: LL RR ..
#define CIMAADPCM_INTERLEAVING_SPLIT	2	// No interleaving (split mode): LL..L RR..R

#pragma pack()

//==========================================================================
// GUIDs
//==========================================================================

//
// Continuous IMA ADPCM audio subtype
//
// {05AA7332-BA6A-4fe7-94FF-DC0E2CA33C54}
DEFINE_GUID(MEDIASUBTYPE_CIMAADPCM, 
0x5aa7332, 0xba6a, 0x4fe7, 0x94, 0xff, 0xdc, 0xe, 0x2c, 0xa3, 0x3c, 0x54);

//
// Continuous IMA ADPCM format type
//
// {5E19FF3F-9405-4028-A2B4-5FBC4179301E}
DEFINE_GUID(FORMAT_CIMAADPCM, 
0x5e19ff3f, 0x9405, 0x4028, 0xa2, 0xb4, 0x5f, 0xbc, 0x41, 0x79, 0x30, 0x1e);

//
// Continuous IMA ADPCM audio decompressor filter
//
// {293D40EE-78A4-4de6-A98F-8FFA0FE31579}
DEFINE_GUID(CLSID_CIMAADPCMDecompressor, 
0x293d40ee, 0x78a4, 0x4de6, 0xa9, 0x8f, 0x8f, 0xfa, 0xf, 0xe3, 0x15, 0x79);

//
// Continuous IMA ADPCM audio decompressor filter property page
//
// {1B25D0BB-83BE-4a43-BB19-D8B7B666FC6E}
DEFINE_GUID(CLSID_CIMAADPCMDecompressorPage, 
0x1b25d0bb, 0x83be, 0x4a43, 0xbb, 0x19, 0xd8, 0xb7, 0xb6, 0x66, 0xfc, 0x6e);

#endif
