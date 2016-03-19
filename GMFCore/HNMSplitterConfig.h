//==========================================================================
//
// File: HNMSplitterConfig.h
//
// Desc: Game Media Formats - Interface to configure HNM splitter
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

#ifndef __GMF_HNM_SPLITTER_CONFIG_H__
#define __GMF_HNM_SPLITTER_CONFIG_H__

#include <windows.h>
#include <initguid.h>
#include <basetyps.h>

//
// HNM splitter configuration interface
//
// {5F547A7D-2B0F-406f-9E6F-109E36322E6C}
DEFINE_GUID(IID_IConfigHNMSplitter, 
0x5f547a7d, 0x2b0f, 0x406f, 0x9e, 0x6f, 0x10, 0x9e, 0x36, 0x32, 0x2e, 0x6c);

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(IConfigHNMSplitter, IUnknown)
{
	STDMETHOD(get_UseExternalAPC) (THIS_ BOOL *pbUseExternalAPC) PURE;
	STDMETHOD(get_ExternalAPCPath) (THIS_ OLECHAR *szExternalAPCPath) PURE;
	STDMETHOD(put_UseExternalAPC) (THIS_ BOOL bUseExternalAPC) PURE;
	STDMETHOD(put_ExternalAPCPath) (THIS_ OLECHAR *szExternalAPCPath) PURE;
};

#ifdef __cplusplus
}
#endif

#endif
