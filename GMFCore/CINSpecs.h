//==========================================================================
//
// File: CINSpecs.h
//
// Desc: Game Media Formats - Definitions of CIN-related constants and structures
//
// Copyright (C) 2004 ANX Software.
//
// Based on the CIN specs by Dr. Tim Ferguson (timf@csse.monash.edu.au).
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

#ifndef __GMF_CIN_SPECS_H__
#define __GMF_CIN_SPECS_H__

#include <windows.h>

//==========================================================================
// Constants
//==========================================================================

#define CIN_FPS 14

#define CIN_COMMAND_PALETTE	1
#define CIN_COMMAND_EOF		2

#define HUFFMAN_TOKENS 256

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagCIN_HEADER {
	DWORD	dwVideoWidth;		// Video frame width
	DWORD	dwVideoHeight;		// Video frame height
	DWORD	dwAudioSampleRate;	// Audio sample rate
	DWORD	dwAudioSampleWidth;	// Audio sample width (bytes per sample)
	DWORD	nAudioChannels;		// Number of audio channels
	BYTE	bHuffmanTable[256][HUFFMAN_TOKENS];	// Huffman table
} CIN_HEADER;

#pragma pack()

#endif
