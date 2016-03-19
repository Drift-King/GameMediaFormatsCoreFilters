//==========================================================================
//
// File: H263GUID.h
//
// Desc: Game Media Formats - Definitions of H263-related GUIDs
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


#ifndef __GMF_H263_GUID_H__
#define __GMF_H263_GUID_H__

#include <initguid.h>

//
// H263 stream & video subtype
//
// {33363248-0000-0010-8000-00AA00389B71}
DEFINE_GUID(MEDIASUBTYPE_H263,
0x33363248, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

//
// H263 parser filter
//
// {63EBD8E2-259E-4c1e-ACA3-FA6B9BFA3D7F}
DEFINE_GUID(CLSID_H263Parser, 
0x63ebd8e2, 0x259e, 0x4c1e, 0xac, 0xa3, 0xfa, 0x6b, 0x9b, 0xfa, 0x3d, 0x7f);

#endif