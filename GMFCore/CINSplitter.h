//==========================================================================
//
// File: CINSplitter.h
//
// Desc: Game Media Formats - Header file for CIN splitter filter
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

#ifndef __GMF_CIN_SPLITTER_H__
#define __GMF_CIN_SPLITTER_H__

#include "BaseParser.h"

//==========================================================================
// CIN chunk parser class
//==========================================================================

class CCINChunkParser : public CBaseChunkParser 
{
	
	// Current media sample stuff
	CParserOutputPin	*m_pPin;	// Current output pin
	IMediaSample		*m_pSample;	// Current media sample
	REFERENCE_TIME		m_rtDelta;	// Sample's stream time delta
	LONGLONG			m_llDelta;	// Sample's media time delta

	enum ChunkType { 
		CCT_COMMAND,
		CCT_VIDEO,
		CCT_AUDIO
	};
	ChunkType m_ChunkType;

	BOOL m_bEndOfFile;		// Has the end-of-file chunk been encountered?
	BOOL m_bUseCorrection;	// Should we use audio chunk size correction?
	BOOL m_bDoCorrection;	// Does this chunk require a size correction?
	
	// Wave format parameters (used by ParseChunkHeader() when 
	// calculating sample stream and media times)
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

public:

	CCINChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		HRESULT *phr					// Success or error code
	);
	~CCINChunkParser();

	HRESULT ResetParser(void);

protected:

	HRESULT ParseChunkHeader(
		LONGLONG llStartPosition,
		BYTE *pbHeader,
		LONG *plDataSize
	);
	HRESULT ParseChunkData(
		LONGLONG llStartPosition,
		BYTE *pbData,
		LONG lDataSize
	);
	HRESULT ParseChunkEnd(void);

};

//==========================================================================
// CIN splitter filter class
//
// The filter uses CCINChunkParser helper class to perform 
// parsing of incoming data.
//==========================================================================

class CCINSplitterFilter : public CBaseParserFilter
{

	DECLARE_FILETYPE

	// Wave format parameters (used by the parser when 
	// calculating sample stream and media times)
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

	// Actual data parser
	CCINChunkParser *m_pParser;

	CCINSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CCINSplitterFilter();

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	// ---- Parsing methods -----

	HRESULT Initialize(IPin *pPin, IAsyncReader *pReader);
	HRESULT Shutdown(void);

	HRESULT InitializeParser(void);
	HRESULT ShutdownParser(void);
	HRESULT ResetParser(void);

	HRESULT CheckInputType(const CMediaType* pmt);
	HRESULT GetInputType(int iPosition, CMediaType *pMediaType);

	// ---- Seeking methods -----

	HRESULT ConvertTimeFormat(
		LPCWSTR pPinName,
		LONGLONG *pTarget,
		const GUID *pTargetFormat,
		LONGLONG llSource,
		const GUID *pSourceFormat
	);

	// ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);

	DECLARE_FILETYPE_METHODS

protected:

	HRESULT Receive(
		LONGLONG llStartPosition,
		BYTE *pbData,
		LONG lDataSize
	);

};

//==========================================================================
// CIN splitter property page class
//==========================================================================

class CCINSplitterPage : public CBasePropertyPage
{

	CCINSplitterPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// CIN splitter setup data
//==========================================================================

extern const WCHAR g_wszCINSplitterName[];
extern const WCHAR g_wszCINSplitterPageName[];
extern const AMOVIESETUP_FILTER g_sudCINSplitter;

#endif
