//==========================================================================
//
// File: WSADPCM.h
//
// Desc: Game Media Formats - Definitions of WS ADPCM structures
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

#ifndef __GMF_WSADPCM_H__
#define __GMF_WSADPCM_H__

#include <windows.h>
#include <initguid.h>

//==========================================================================
// Structures
//==========================================================================

#pragma pack(1)

typedef struct tagWSADPCMWAVEFORMAT {
	WORD	nChannels;		// Number of channels
	DWORD	nSamplesPerSec;	// Sample rate
	WORD	wBitsPerSample;	// Sample resolution
} WSADPCMWAVEFORMAT;

typedef struct tagWSADPCMINFO {
	WORD wOutSize;			// Decompressed data size
	WORD wInSize;			// Compressed data size
} WSADPCMINFO;

#pragma pack()

//==========================================================================
// GUIDs
//==========================================================================

//
// WS ADPCM audio subtype
//
// {D996B1BA-28A2-4f09-80AB-DF538641EEA4}
DEFINE_GUID(MEDIASUBTYPE_WSADPCM, 
0xd996b1ba, 0x28a2, 0x4f09, 0x80, 0xab, 0xdf, 0x53, 0x86, 0x41, 0xee, 0xa4);

//
// WS ADPCM format type
//
// {7F6AE739-4B73-4c89-9CBB-20571B602954}
DEFINE_GUID(FORMAT_WSADPCM, 
0x7f6ae739, 0x4b73, 0x4c89, 0x9c, 0xbb, 0x20, 0x57, 0x1b, 0x60, 0x29, 0x54);

//
// WS ADPCM audio decompressor filter
//
// {679F5B04-D57B-4d74-818D-A4E6B3262DF7}
DEFINE_GUID(CLSID_WSADPCMDecompressor, 
0x679f5b04, 0xd57b, 0x4d74, 0x81, 0x8d, 0xa4, 0xe6, 0xb3, 0x26, 0x2d, 0xf7);

//
// WS ADPCM audio decompressor filter property page
//
// {215EB724-32D5-4f94-A643-656EBC52D47D}
DEFINE_GUID(CLSID_WSADPCMDecompressorPage, 
0x215eb724, 0x32d5, 0x4f94, 0xa6, 0x43, 0x65, 0x6e, 0xbc, 0x52, 0xd4, 0x7d);

#endif
