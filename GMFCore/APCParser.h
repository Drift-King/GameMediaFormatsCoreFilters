//==========================================================================
//
// File: APCParser.h
//
// Desc: Game Media Formats - Header file for APC parser filter
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

#ifndef __GMF_APC_PARSER_H__
#define __GMF_APC_PARSER_H__

#include "BasePlainParser.h"

//==========================================================================
// APC parser filter class
//==========================================================================

class CAPCParserFilter : public CBasePlainParserFilter
{

	DECLARE_FILETYPE

	// Wave format parameters (used by GetSampleDelta())
	WORD	m_nSampleSize;		// (Uncompressed) sample size in bytes
	DWORD	m_nAvgBytesPerSec;	// Data rate

	// Number of samples in the stream
	DWORD m_nSamples;

	CAPCParserFilter(LPUNKNOWN pUnk, HRESULT *phr);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	// Overridden to reset the stream-specific variables
	HRESULT Shutdown(void);

	// ---- Seeking methods -----

	HRESULT ConvertTimeFormat(
		LPCWSTR pPinName,
		LONGLONG *pTarget,
		const GUID *pTargetFormat,
		LONGLONG llSource,
		const GUID *pSourceFormat
	);
	HRESULT GetDuration(
		LPCWSTR pPinName,
		const GUID *pCurrentFormat,
		LONGLONG *pDuration
	);

	// ISpecifyPropertyPages method
	STDMETHODIMP GetPages(CAUUID *pPages);

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
// APC parser property page class
//==========================================================================

class CAPCParserPage : public CBasePropertyPage
{

	CAPCParserPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// APC parser setup data
//==========================================================================

extern const WCHAR g_wszAPCParserName[];
extern const WCHAR g_wszAPCParserPageName[];
extern const AMOVIESETUP_FILTER g_sudAPCParser;

#endif