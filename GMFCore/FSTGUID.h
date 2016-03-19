//==========================================================================
//
// File: FSTGUID.h
//
// Desc: Game Media Formats - Definitions of FST-related GUIDs
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

#ifndef __GMF_FST_GUID_H__
#define __GMF_FST_GUID_H__

#include <initguid.h>

//
// FST media subtype
//
// {A39C8E46-D7CF-4056-8108-167DD56C1C0F}
DEFINE_GUID(MEDIASUBTYPE_FST, 
0xa39c8e46, 0xd7cf, 0x4056, 0x81, 0x8, 0x16, 0x7d, 0xd5, 0x6c, 0x1c, 0xf);

//
// FST splitter filter
//
// {6E1C0CAD-1046-4c5f-9D89-1B47E087028C}
DEFINE_GUID(CLSID_FSTSplitter, 
0x6e1c0cad, 0x1046, 0x4c5f, 0x9d, 0x89, 0x1b, 0x47, 0xe0, 0x87, 0x2, 0x8c);

//
// FST splitter property page
//
// {92A78201-A299-47ac-ACB7-60029770B6D8}
DEFINE_GUID(CLSID_FSTSplitterPage, 
0x92a78201, 0xa299, 0x47ac, 0xac, 0xb7, 0x60, 0x2, 0x97, 0x70, 0xb6, 0xd8);

//
// FST compressed video stream
//
// {905858E4-7DD1-468b-B282-9A3581F32B42}
DEFINE_GUID(MEDIASUBTYPE_FSTVideo, 
0x905858e4, 0x7dd1, 0x468b, 0xb2, 0x82, 0x9a, 0x35, 0x81, 0xf3, 0x2b, 0x42);

//
// FST compressed video stream format type
//
// {BD6C3BDF-2AD3-4220-A9A5-43E06F7E7C3A}
DEFINE_GUID(FORMAT_FSTVideo, 
0xbd6c3bdf, 0x2ad3, 0x4220, 0xa9, 0xa5, 0x43, 0xe0, 0x6f, 0x7e, 0x7c, 0x3a);

//
// FST video decompressor filter
//
// {6B2FE357-A66C-4a55-90D9-E8BD25C8EC57}
DEFINE_GUID(CLSID_FSTVideoDecompressor, 
0x6b2fe357, 0xa66c, 0x4a55, 0x90, 0xd9, 0xe8, 0xbd, 0x25, 0xc8, 0xec, 0x57);

//
// FST video decompressor property page
//
// {0394FD7D-F62B-4b2b-884D-A01DE3E43281}
DEFINE_GUID(CLSID_FSTVideoDecompressorPage, 
0x394fd7d, 0xf62b, 0x4b2b, 0x88, 0x4d, 0xa0, 0x1d, 0xe3, 0xe4, 0x32, 0x81);

#endif