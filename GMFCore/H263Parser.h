//==========================================================================
//
// File: H263Parser.h
//
// Desc: Game Media Formats - Header file for H263 parser filter
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

#ifndef __GMF_H263_PARSER_H__
#define __GMF_H263_PARSER_H__

#include "BasePlainParser.h"

//==========================================================================
// H263 parser filter class
//==========================================================================

class CH263ParserFilter : public CBasePlainParserFilter
{

	DECLARE_FILETYPE

	CH263ParserFilter(LPUNKNOWN pUnk, HRESULT *phr);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_FILETYPE_METHODS

protected:

	// ---- Parsing methods ----- (Implementations of PURE methods)

	HRESULT Initialize(
		IPin *pPin,
		IAsyncReader *pReader,
		CMediaType *pmt,
		ALLOCATOR_PROPERTIES *pap,
		DWORD *pdwCapabilities,
		int *pnTimeFormats,
		GUID **ppTimeFormats
	);

	HRESULT GetSampleDelta(
		LONG lDataLength,
		REFERENCE_TIME *prtStreamDelta,
		LONGLONG *pllMediaDelta
	);

};

//==========================================================================
// H263 parser setup data
//==========================================================================

extern const WCHAR g_wszH263ParserName[];
extern const AMOVIESETUP_FILTER g_sudH263Parser;

#endif