//==========================================================================
//
// File: HNMVideoDecompressorConfig.h
//
// Desc: Game Media Formats - Interface to configure HNM video decompressor
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

#ifndef __GMF_HNM_VIDEO_DECOMPRESSOR_CONFIG_H__
#define __GMF_HNM_VIDEO_DECOMPRESSOR_CONFIG_H__

#include <windows.h>
#include <initguid.h>
#include <basetyps.h>

//
// HNM video decompressor configuration interface
//
// {39A41489-28F6-433e-9664-C4F535BEF327}
DEFINE_GUID(IID_IConfigHNMVideoDecompressor, 
0x39a41489, 0x28f6, 0x433e, 0x96, 0x64, 0xc4, 0xf5, 0x35, 0xbe, 0xf3, 0x27);

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(IConfigHNMVideoDecompressor, IUnknown)
{
	STDMETHOD(get_RequireConverter) (THIS_ BOOL *pbRequireConverter) PURE;
	STDMETHOD(put_RequireConverter) (THIS_ BOOL bRequireConverter) PURE;
};

#ifdef __cplusplus
}
#endif

#endif
