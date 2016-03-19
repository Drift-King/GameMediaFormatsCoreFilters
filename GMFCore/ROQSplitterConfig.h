//==========================================================================
//
// File: ROQSplitterConfig.h
//
// Desc: Game Media Formats - Interface to configure ROQ splitter
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

#ifndef __GMF_ROQ_SPLITTER_CONFIG_H__
#define __GMF_ROQ_SPLITTER_CONFIG_H__

#include <windows.h>
#include <initguid.h>
#include <basetyps.h>

//
// ROQ splitter configuration interface
//
// {3E7D5775-1B27-4799-A492-50EF837F162E}
DEFINE_GUID(IID_IConfigROQSplitter, 
0x3e7d5775, 0x1b27, 0x4799, 0xa4, 0x92, 0x50, 0xef, 0x83, 0x7f, 0x16, 0x2e);

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(IConfigROQSplitter, IUnknown)
{
	STDMETHOD(get_UseExternalMP3) (THIS_ BOOL *pbUseExternalMP3) PURE;
	STDMETHOD(get_ExternalMP3Path) (THIS_ OLECHAR *szExternalMP3Path) PURE;
	STDMETHOD(put_UseExternalMP3) (THIS_ BOOL bUseExternalMP3) PURE;
	STDMETHOD(put_ExternalMP3Path) (THIS_ OLECHAR *szExternalMP3Path) PURE;
};

#ifdef __cplusplus
}
#endif

#endif
