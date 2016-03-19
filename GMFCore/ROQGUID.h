//==========================================================================
//
// File: ROQGUID.h
//
// Desc: Game Media Formats - Definitions of ROQ-related GUIDs
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

#ifndef __GMF_ROQ_GUID_H__
#define __GMF_ROQ_GUID_H__

#include <initguid.h>

//
// ROQ media subtype
//
// {B2A273DA-AF95-4e32-92C6-B9959249AF9A}
DEFINE_GUID(MEDIASUBTYPE_ROQ, 
0xb2a273da, 0xaf95, 0x4e32, 0x92, 0xc6, 0xb9, 0x95, 0x92, 0x49, 0xaf, 0x9a);

//
// ROQ splitter filter
//
// {60B6D30F-B30A-4552-A7D0-16A0502E4C94}
DEFINE_GUID(CLSID_ROQSplitter, 
0x60b6d30f, 0xb30a, 0x4552, 0xa7, 0xd0, 0x16, 0xa0, 0x50, 0x2e, 0x4c, 0x94);

//
// ROQ splitter property page
//
// {9C2602FA-1E84-4bda-A811-2D584265F78E}
DEFINE_GUID(CLSID_ROQSplitterPage, 
0x9c2602fa, 0x1e84, 0x4bda, 0xa8, 0x11, 0x2d, 0x58, 0x42, 0x65, 0xf7, 0x8e);

//
// ROQ compressed video stream
//
// {86235EF3-C9E8-4af2-95CD-82FE722A1352}
DEFINE_GUID(MEDIASUBTYPE_ROQVideo, 
0x86235ef3, 0xc9e8, 0x4af2, 0x95, 0xcd, 0x82, 0xfe, 0x72, 0x2a, 0x13, 0x52);

//
// ROQ compressed video stream format type
//
// {055361E4-0A20-413a-AA27-5874A15C0A79}
DEFINE_GUID(FORMAT_ROQVideo, 
0x55361e4, 0xa20, 0x413a, 0xaa, 0x27, 0x58, 0x74, 0xa1, 0x5c, 0xa, 0x79);

//
// ROQ video decompressor filter
//
// {756CA4AF-20D6-4522-B0D0-10F051642779}
DEFINE_GUID(CLSID_ROQVideoDecompressor, 
0x756ca4af, 0x20d6, 0x4522, 0xb0, 0xd0, 0x10, 0xf0, 0x51, 0x64, 0x27, 0x79);

//
// ROQ video decompressor property page
//
// {6CB1F233-03B5-47b9-BE6B-867323073FC1}
DEFINE_GUID(CLSID_ROQVideoDecompressorPage, 
0x6cb1f233, 0x3b5, 0x47b9, 0xbe, 0x6b, 0x86, 0x73, 0x23, 0x7, 0x3f, 0xc1);

//
// ROQ ADPCM audio subtype
//
// {4C7628BB-5CA2-44d6-82CE-59EC239DC27F}
DEFINE_GUID(MEDIASUBTYPE_ROQADPCM, 
0x4c7628bb, 0x5ca2, 0x44d6, 0x82, 0xce, 0x59, 0xec, 0x23, 0x9d, 0xc2, 0x7f);

//
// ROQ ADPCM format type
//
// {640E9FCC-8CDE-403c-8781-F13825C25C54}
DEFINE_GUID(FORMAT_ROQADPCM, 
0x640e9fcc, 0x8cde, 0x403c, 0x87, 0x81, 0xf1, 0x38, 0x25, 0xc2, 0x5c, 0x54);

//
// ROQ ADPCM audio decompressor filter
//
// {09730D25-1A98-4e21-A60C-F303F20462D6}
DEFINE_GUID(CLSID_ROQADPCMDecompressor, 
0x9730d25, 0x1a98, 0x4e21, 0xa6, 0xc, 0xf3, 0x3, 0xf2, 0x4, 0x62, 0xd6);

//
// ROQ ADPCM audio decompressor filter property page
//
// {C1BA7529-E97F-4831-8799-67D134D87337}
DEFINE_GUID(CLSID_ROQADPCMDecompressorPage, 
0xc1ba7529, 0xe97f, 0x4831, 0x87, 0x99, 0x67, 0xd1, 0x34, 0xd8, 0x73, 0x37);

//
// MPEG Layer-3 (MP3) audio decoder filter
// **** Cooked CLSID -- used for manual MP3 decoder addition ****
//
// {38BE3000-DBF4-11D0-860E-00A024CFEF6D}
DEFINE_GUID(CLSID_MP3Decoder, 
0x38BE3000, 0xDBF4, 0x11D0, 0x86, 0x0E, 0x00, 0xA0, 0x24, 0xCF, 0xEF, 0x6D);

#endif
