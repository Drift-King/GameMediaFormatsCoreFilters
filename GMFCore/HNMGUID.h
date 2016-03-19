//==========================================================================
//
// File: HNMGUID.h
//
// Desc: Game Media Formats - Definitions of HNM-related GUIDs
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

#ifndef __GMF_HNM_GUID_H__
#define __GMF_HNM_GUID_H__

#include <initguid.h>

//
// HNM media subtype
//
// {FD6A2AB7-77FA-4a89-9D54-C45C83A7EA9F}
DEFINE_GUID(MEDIASUBTYPE_HNM, 
0xfd6a2ab7, 0x77fa, 0x4a89, 0x9d, 0x54, 0xc4, 0x5c, 0x83, 0xa7, 0xea, 0x9f);

//
// HNM splitter filter
//
// {B7576D39-29B6-44fd-91D5-3464C5062241}
DEFINE_GUID(CLSID_HNMSplitter, 
0xb7576d39, 0x29b6, 0x44fd, 0x91, 0xd5, 0x34, 0x64, 0xc5, 0x6, 0x22, 0x41);

//
// HNM splitter property page
//
// {46F4B71B-2EA7-48d1-9864-78701A8E6232}
DEFINE_GUID(CLSID_HNMSplitterPage, 
0x46f4b71b, 0x2ea7, 0x48d1, 0x98, 0x64, 0x78, 0x70, 0x1a, 0x8e, 0x62, 0x32);

//
// HNM compressed video stream
//
// {AEEBD849-EBAC-48e3-8C0C-B5D4FE9C817E}
DEFINE_GUID(MEDIASUBTYPE_HNMVideo, 
0xaeebd849, 0xebac, 0x48e3, 0x8c, 0xc, 0xb5, 0xd4, 0xfe, 0x9c, 0x81, 0x7e);

//
// HNM compressed video stream format type
//
// {0C282979-A810-4816-A2F3-829D875004E2}
DEFINE_GUID(FORMAT_HNMVideo, 
0xc282979, 0xa810, 0x4816, 0xa2, 0xf3, 0x82, 0x9d, 0x87, 0x50, 0x4, 0xe2);

//
// HNM video decompressor filter
//
// {E8A09541-C47F-49d7-833F-00E2FD1F7569}
DEFINE_GUID(CLSID_HNMVideoDecompressor, 
0xe8a09541, 0xc47f, 0x49d7, 0x83, 0x3f, 0x0, 0xe2, 0xfd, 0x1f, 0x75, 0x69);

//
// HNM video decompressor property page
//
// {60E61631-D6ED-4e04-BC42-63D8B1AD5E48}
DEFINE_GUID(CLSID_HNMVideoDecompressorPage, 
0x60e61631, 0xd6ed, 0x4e04, 0xbc, 0x42, 0x63, 0xd8, 0xb1, 0xad, 0x5e, 0x48);

#endif