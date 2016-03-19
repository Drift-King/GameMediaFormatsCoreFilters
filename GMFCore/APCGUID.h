//==========================================================================
//
// File: APCGUID.h
//
// Desc: Game Media Formats - Definitions of APC-related GUIDs
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


#ifndef __GMF_APC_GUID_H__
#define __GMF_APC_GUID_H__

#include <initguid.h>

//
// APC media subtype
//
// {49506095-F0F0-402d-B53D-501261471C24}
DEFINE_GUID(MEDIASUBTYPE_APC, 
0x49506095, 0xf0f0, 0x402d, 0xb5, 0x3d, 0x50, 0x12, 0x61, 0x47, 0x1c, 0x24);

//
// APC parser filter
//
// {6A26E433-9CC4-4746-ABF3-C7C0B7B69538}
DEFINE_GUID(CLSID_APCParser, 
0x6a26e433, 0x9cc4, 0x4746, 0xab, 0xf3, 0xc7, 0xc0, 0xb7, 0xb6, 0x95, 0x38);

//
// APC parser property page
//
// {42884241-D999-44a3-8034-50D4FECD44EC}
DEFINE_GUID(CLSID_APCParserPage, 
0x42884241, 0xd999, 0x44a3, 0x80, 0x34, 0x50, 0xd4, 0xfe, 0xcd, 0x44, 0xec);

#endif