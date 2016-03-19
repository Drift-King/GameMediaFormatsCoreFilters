//==========================================================================
//
// File: MVEGUID.h
//
// Desc: Game Media Formats - Definitions of MVE-related GUIDs
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

#ifndef __GMF_MVE_GUID_H__
#define __GMF_MVE_GUID_H__

#include <initguid.h>

//
// MVE media subtype
//
// {3840B376-092B-41a3-8F8A-E623AA663D59}
DEFINE_GUID(MEDIASUBTYPE_MVE, 
0x3840b376, 0x92b, 0x41a3, 0x8f, 0x8a, 0xe6, 0x23, 0xaa, 0x66, 0x3d, 0x59);

//
// MVE splitter filter
//
// {B360712D-B151-4bdb-9EA7-8A7948F78E8F}
DEFINE_GUID(CLSID_MVESplitter, 
0xb360712d, 0xb151, 0x4bdb, 0x9e, 0xa7, 0x8a, 0x79, 0x48, 0xf7, 0x8e, 0x8f);

//
// MVE splitter property page
//
// {87D6D855-E639-4fac-88E8-7D76D344AEC3}
DEFINE_GUID(CLSID_MVESplitterPage, 
0x87d6d855, 0xe639, 0x4fac, 0x88, 0xe8, 0x7d, 0x76, 0xd3, 0x44, 0xae, 0xc3);

//
// MVE compressed video stream
//
// {FDECAD1E-2B4E-470e-80B3-DC7B7CC421BC}
DEFINE_GUID(MEDIASUBTYPE_MVEVideo, 
0xfdecad1e, 0x2b4e, 0x470e, 0x80, 0xb3, 0xdc, 0x7b, 0x7c, 0xc4, 0x21, 0xbc);

//
// MVE compressed video stream format type
//
// {29F1AF33-E2E1-4eb7-8F61-D6D389AC4CFB}
DEFINE_GUID(FORMAT_MVEVideo, 
0x29f1af33, 0xe2e1, 0x4eb7, 0x8f, 0x61, 0xd6, 0xd3, 0x89, 0xac, 0x4c, 0xfb);

//
// MVE video decompressor filter
//
// {D64E5E96-C4E0-4a29-AAAC-586093DCA0C7}
DEFINE_GUID(CLSID_MVEVideoDecompressor, 
0xd64e5e96, 0xc4e0, 0x4a29, 0xaa, 0xac, 0x58, 0x60, 0x93, 0xdc, 0xa0, 0xc7);

//
// MVE video decompressor property page
//
// {B8AFA716-21D7-4294-B458-712426C920AE}
DEFINE_GUID(CLSID_MVEVideoDecompressorPage, 
0xb8afa716, 0x21d7, 0x4294, 0xb4, 0x58, 0x71, 0x24, 0x26, 0xc9, 0x20, 0xae);

//
// MVE ADPCM audio subtype
//
// {8A74745C-2DBF-43b8-A315-5269EB495148}
DEFINE_GUID(MEDIASUBTYPE_MVEADPCM, 
0x8a74745c, 0x2dbf, 0x43b8, 0xa3, 0x15, 0x52, 0x69, 0xeb, 0x49, 0x51, 0x48);

//
// MVE ADPCM format type
//
// {5544B6E8-CCAA-47ce-A8BF-A4CD886E7585}
DEFINE_GUID(FORMAT_MVEADPCM, 
0x5544b6e8, 0xccaa, 0x47ce, 0xa8, 0xbf, 0xa4, 0xcd, 0x88, 0x6e, 0x75, 0x85);

//
// MVE ADPCM audio decompressor filter
//
// {9CC4B8C2-36F5-48c6-9C33-FDD415619F3E}
DEFINE_GUID(CLSID_MVEADPCMDecompressor, 
0x9cc4b8c2, 0x36f5, 0x48c6, 0x9c, 0x33, 0xfd, 0xd4, 0x15, 0x61, 0x9f, 0x3e);

//
// MVE ADPCM audio decompressor filter property page
//
// {41AA9F7F-6B54-4ab7-851B-BE54813E1AAF}
DEFINE_GUID(CLSID_MVEADPCMDecompressorPage, 
0x41aa9f7f, 0x6b54, 0x4ab7, 0x85, 0x1b, 0xbe, 0x54, 0x81, 0x3e, 0x1a, 0xaf);

#endif
