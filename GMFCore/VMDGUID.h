//==========================================================================
//
// File: VMDGUID.h
//
// Desc: Game Media Formats - Definitions of VMD-related GUIDs
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

#ifndef __GMF_VMD_GUID_H__
#define __GMF_VMD_GUID_H__

#include <initguid.h>

//
// VMD media subtype
//
// {4D77F693-E916-47df-BBF3-49EB99E1567E}
DEFINE_GUID(MEDIASUBTYPE_VMD, 
0x4d77f693, 0xe916, 0x47df, 0xbb, 0xf3, 0x49, 0xeb, 0x99, 0xe1, 0x56, 0x7e);

//
// VMD splitter filter
//
// {6014DCD0-C78C-4e29-93EC-8A0323F1AE27}
DEFINE_GUID(CLSID_VMDSplitter, 
0x6014dcd0, 0xc78c, 0x4e29, 0x93, 0xec, 0x8a, 0x3, 0x23, 0xf1, 0xae, 0x27);

//
// VMD splitter property page
//
// {B098366B-C50D-4c51-A468-C863C67F069D}
DEFINE_GUID(CLSID_VMDSplitterPage, 
0xb098366b, 0xc50d, 0x4c51, 0xa4, 0x68, 0xc8, 0x63, 0xc6, 0x7f, 0x6, 0x9d);

//
// VMD compressed video stream
//
// {9232E3BD-6116-404e-8E4F-5A2BC0306D46}
DEFINE_GUID(MEDIASUBTYPE_VMDVideo, 
0x9232e3bd, 0x6116, 0x404e, 0x8e, 0x4f, 0x5a, 0x2b, 0xc0, 0x30, 0x6d, 0x46);

//
// VMD compressed video stream format type
//
// {7BA0CA53-AA92-4588-A0C6-7CC6D19F879D}
DEFINE_GUID(FORMAT_VMDVideo, 
0x7ba0ca53, 0xaa92, 0x4588, 0xa0, 0xc6, 0x7c, 0xc6, 0xd1, 0x9f, 0x87, 0x9d);

//
// VMD video decompressor filter
//
// {7993EF0B-84D6-42f6-B433-4B5E9C27C590}
DEFINE_GUID(CLSID_VMDVideoDecompressor, 
0x7993ef0b, 0x84d6, 0x42f6, 0xb4, 0x33, 0x4b, 0x5e, 0x9c, 0x27, 0xc5, 0x90);

//
// VMD video decompressor property page
//
// {D11B3BB4-F15C-44d2-9F60-7F585BF222A4}
DEFINE_GUID(CLSID_VMDVideoDecompressorPage, 
0xd11b3bb4, 0xf15c, 0x44d2, 0x9f, 0x60, 0x7f, 0x58, 0x5b, 0xf2, 0x22, 0xa4);

//
// VMD compressed audio stream
//
// {7B5FD90C-FAC6-4215-AF50-8AB2EDDBDD15}
DEFINE_GUID(MEDIASUBTYPE_VMDAudio, 
0x7b5fd90c, 0xfac6, 0x4215, 0xaf, 0x50, 0x8a, 0xb2, 0xed, 0xdb, 0xdd, 0x15);

//
// VMD compressed audio stream format type
//
// {40C24A56-94D1-42ce-9DDD-7586E7C763D2}
DEFINE_GUID(FORMAT_VMDAudio, 
0x40c24a56, 0x94d1, 0x42ce, 0x9d, 0xdd, 0x75, 0x86, 0xe7, 0xc7, 0x63, 0xd2);

//
// VMD audio decompressor filter
//
// {05EBDE06-5E87-4b71-B8D2-64B5CBD07237}
DEFINE_GUID(CLSID_VMDAudioDecompressor, 
0x5ebde06, 0x5e87, 0x4b71, 0xb8, 0xd2, 0x64, 0xb5, 0xcb, 0xd0, 0x72, 0x37);

//
// VMD audio decompressor property page
//
// {434AAE7C-CD75-4bd8-AAD7-A17B8B7FD53A}
DEFINE_GUID(CLSID_VMDAudioDecompressorPage, 
0x434aae7c, 0xcd75, 0x4bd8, 0xaa, 0xd7, 0xa1, 0x7b, 0x8b, 0x7f, 0xd5, 0x3a);

#endif