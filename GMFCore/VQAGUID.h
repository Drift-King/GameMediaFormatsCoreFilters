//==========================================================================
//
// File: VQAGUID.h
//
// Desc: Game Media Formats - Definitions of VQA-related GUIDs
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

#ifndef __GMF_VQA_GUID_H__
#define __GMF_VQA_GUID_H__

#include <initguid.h>

//
// VQA media subtype
//
// {6FE58503-0CA3-4411-B8DF-0406C5F54F43}
DEFINE_GUID(MEDIASUBTYPE_VQA, 
0x6fe58503, 0xca3, 0x4411, 0xb8, 0xdf, 0x4, 0x6, 0xc5, 0xf5, 0x4f, 0x43);

//
// VQA splitter filter
//
// {D5E0C08B-1A0E-4c4c-930C-9CC0EDF306C1}
DEFINE_GUID(CLSID_VQASplitter, 
0xd5e0c08b, 0x1a0e, 0x4c4c, 0x93, 0xc, 0x9c, 0xc0, 0xed, 0xf3, 0x6, 0xc1);

//
// VQA splitter property page
//
// {743F8767-2C4C-42dc-B477-22265B346EEF}
DEFINE_GUID(CLSID_VQASplitterPage, 
0x743f8767, 0x2c4c, 0x42dc, 0xb4, 0x77, 0x22, 0x26, 0x5b, 0x34, 0x6e, 0xef);

//
// VQA compressed video stream
//
// {C4E931F3-1271-4091-8138-7FEF382BF6E8}
DEFINE_GUID(MEDIASUBTYPE_VQAVideo, 
0xc4e931f3, 0x1271, 0x4091, 0x81, 0x38, 0x7f, 0xef, 0x38, 0x2b, 0xf6, 0xe8);

//
// VQA compressed video stream format type
//
// {7D90C517-8A1D-46e7-A618-EA477147DA7A}
DEFINE_GUID(FORMAT_VQAVideo, 
0x7d90c517, 0x8a1d, 0x46e7, 0xa6, 0x18, 0xea, 0x47, 0x71, 0x47, 0xda, 0x7a);

//
// VQA video decompressor filter
//
// {05493C68-29B2-4d84-8E0A-751EE27F0875}
DEFINE_GUID(CLSID_VQAVideoDecompressor, 
0x5493c68, 0x29b2, 0x4d84, 0x8e, 0xa, 0x75, 0x1e, 0xe2, 0x7f, 0x8, 0x75);

//
// VQA video decompressor property page
//
// {57A97235-BDC4-4ace-AE3B-7C157A0F356F}
DEFINE_GUID(CLSID_VQAVideoDecompressorPage, 
0x57a97235, 0xbdc4, 0x4ace, 0xae, 0x3b, 0x7c, 0x15, 0x7a, 0xf, 0x35, 0x6f);

#endif