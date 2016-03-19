//==========================================================================
//
// File: CINGUID.h
//
// Desc: Game Media Formats - Definitions of CIN-related GUIDs
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

#ifndef __GMF_CIN_GUID_H__
#define __GMF_CIN_GUID_H__

#include <initguid.h>

//
// CIN media subtype
//
// {53BDEEB0-EE8C-4e89-8960-6838E8DA623A}
DEFINE_GUID(MEDIASUBTYPE_CIN, 
0x53bdeeb0, 0xee8c, 0x4e89, 0x89, 0x60, 0x68, 0x38, 0xe8, 0xda, 0x62, 0x3a);

//
// CIN splitter filter
//
// {70BA359A-60F0-464f-9919-2694B2EBDB64}
DEFINE_GUID(CLSID_CINSplitter, 
0x70ba359a, 0x60f0, 0x464f, 0x99, 0x19, 0x26, 0x94, 0xb2, 0xeb, 0xdb, 0x64);

//
// CIN splitter property page
//
// {0F2D2969-D666-4be3-8F34-5E8A1BAB554C}
DEFINE_GUID(CLSID_CINSplitterPage, 
0xf2d2969, 0xd666, 0x4be3, 0x8f, 0x34, 0x5e, 0x8a, 0x1b, 0xab, 0x55, 0x4c);

//
// CIN compressed video stream
//
// {AC81DDEA-7F14-4b03-9BF1-409494F65519}
DEFINE_GUID(MEDIASUBTYPE_CINVideo, 
0xac81ddea, 0x7f14, 0x4b03, 0x9b, 0xf1, 0x40, 0x94, 0x94, 0xf6, 0x55, 0x19);

//
// CIN compressed video stream format type
//
// {FB0F9882-C8F8-4282-B499-FBA359827450}
DEFINE_GUID(FORMAT_CINVideo, 
0xfb0f9882, 0xc8f8, 0x4282, 0xb4, 0x99, 0xfb, 0xa3, 0x59, 0x82, 0x74, 0x50);

//
// CIN video decompressor filter
//
// {C13227A2-EEF1-48d9-B808-6DA5C8FCE9CC}
DEFINE_GUID(CLSID_CINVideoDecompressor, 
0xc13227a2, 0xeef1, 0x48d9, 0xb8, 0x8, 0x6d, 0xa5, 0xc8, 0xfc, 0xe9, 0xcc);

//
// CIN video decompressor property page
//
// {5484B500-9DEC-4b0d-9735-1BB75AC6F5C5}
DEFINE_GUID(CLSID_CINVideoDecompressorPage, 
0x5484b500, 0x9dec, 0x4b0d, 0x97, 0x35, 0x1b, 0xb7, 0x5a, 0xc6, 0xf5, 0xc5);

#endif