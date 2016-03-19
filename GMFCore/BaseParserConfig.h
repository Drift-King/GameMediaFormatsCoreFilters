//==========================================================================
//
// File: BaseParserConfig.h
//
// Desc: Game Media Formats - Interface to configure base parser filter
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

#ifndef __GMF_BASE_PARSER_CONFIG_H__
#define __GMF_BASE_PARSER_CONFIG_H__

#include <windows.h>
#include <initguid.h>
#include <basetyps.h>

//
// Base parser property page
//
// {E2820088-F895-4c6c-90FD-48BE049A486C}
DEFINE_GUID(CLSID_BaseParserPage, 
0xe2820088, 0xf895, 0x4c6c, 0x90, 0xfd, 0x48, 0xbe, 0x4, 0x9a, 0x48, 0x6c);

//
// Base parser configuration interface
//
// {1F910BD6-A562-4262-BC6E-125F65F51F82}
DEFINE_GUID(IID_IConfigBaseParser, 
0x1f910bd6, 0xa562, 0x4262, 0xbc, 0x6e, 0x12, 0x5f, 0x65, 0xf5, 0x1f, 0x82);

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(IConfigBaseParser, IUnknown)
{
	// Pin information methods
	STDMETHOD(GetOutputPinCount) (THIS_ int *pnCount) PURE;
	STDMETHOD(GetOutputPinName) (THIS_ int iPin, LPWSTR wszName) PURE;

	// Option getting/setting methods
	STDMETHOD(get_SyncInput) (THIS_ BOOL *pbSyncInput) PURE;
	STDMETHOD(get_InputBuffersNumber) (THIS_ long *pnBuffers) PURE;
	STDMETHOD(get_OutputBuffersNumber) (THIS_ int iPin, long *pnBuffers) PURE;
	STDMETHOD(get_AutoQueue) (THIS_ int iPin, BOOL *pbAutoQueue) PURE;
	STDMETHOD(get_Queue) (THIS_ int iPin, BOOL *pbQueue) PURE;
	STDMETHOD(get_BatchSize) (THIS_ int iPin, LONG *plBatchSize) PURE;
	STDMETHOD(get_BatchExact) (THIS_ int iPin, BOOL *pbBatchExact) PURE;
	STDMETHOD(put_SyncInput) (THIS_ BOOL bSyncInput) PURE;
	STDMETHOD(put_InputBuffersNumber) (THIS_ long nBuffers) PURE;
	STDMETHOD(put_OutputBuffersNumber) (THIS_ int iPin, long nBuffers) PURE;
	STDMETHOD(put_AutoQueue) (THIS_ int iPin, BOOL bAutoQueue) PURE;
	STDMETHOD(put_Queue) (THIS_ int iPin, BOOL bQueue) PURE;
	STDMETHOD(put_BatchSize) (THIS_ int iPin, LONG lBatchSize) PURE;
	STDMETHOD(put_BatchExact) (THIS_ int iPin, BOOL bBatchExact) PURE;

	// Patterns and extensions registration methods
	STDMETHOD(GetFileTypeCount) (THIS_ int *pnCount) PURE;
	STDMETHOD(GetFileTypeName) (THIS_ int iType, LPTSTR szName) PURE;
	STDMETHOD(GetFileTypeExtensions) (THIS_ int iType, UINT *pnExtensions, LPCTSTR **ppExtension) PURE;
	STDMETHOD(RegisterFileTypePatterns) (THIS_ int iType, BOOL bRegister) PURE;
	STDMETHOD(RegisterFileTypeExtensions) (THIS_ int iType, BOOL bRegister) PURE;
};

#ifdef __cplusplus
}
#endif

#endif
