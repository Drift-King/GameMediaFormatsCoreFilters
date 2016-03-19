//==========================================================================
//
// File: MVESplitter.h
//
// Desc: Game Media Formats - Header file for MVE splitter filter
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

#ifndef __GMF_MVE_SPLITTER_H__
#define __GMF_MVE_SPLITTER_H__

#include "BaseParser.h"

//==========================================================================
// MVE inner chunk parser class
// 
// This parser does actual parsing job. Note that the inner parser 
// maintains media sample properties and delivers samples.
//==========================================================================

class CMVEInnerChunkParser : public CBaseChunkParser 
{
	
	// Current media sample stuff
	CParserOutputPin	*m_pPin;	// Current output pin
	IMediaSample		*m_pSample;	// Current media sample
	REFERENCE_TIME		m_rtDelta;	// Sample's stream time delta
	LONGLONG			m_llDelta;	// Sample's media time delta

	BYTE m_bCurrentSubchunkType;	// Current subchunk type
	BYTE m_bCurrentSubchunkSubtype;	// Current subchunk subtype
	
	// Audio & video format parameters (used by ParseChunkHeader() when 
	// calculating sample stream and media times)
	REFERENCE_TIME	m_rtFrameDelta;			// Frame duration (in media time units)
	WORD			m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD			m_nAvgBytesPerSec;		// Audio data rate
	WORD			m_wCompressionRatio;	// Audio compression ratio
	BOOL			m_bIs16Bit;				// Is audio 16-bit?

public:

	CMVEInnerChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		REFERENCE_TIME rtFrameDelta,	// Frame duration (in media time units)
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		WORD wCompressionRatio,			// Audio compression ratio
		BOOL bIs16Bit,					// Is audio 16-bit?
		HRESULT *phr					// Success or error code
	);
	~CMVEInnerChunkParser();

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
// MVE outer chunk parser class
//
// This parser just walks outer MVE chunks and use nested parser 
// to perform actual parsing of inner chunks
//==========================================================================

class CMVEOuterChunkParser : public CBaseChunkParser 
{
	
	CMVEInnerChunkParser m_InnerParser;
	
public:

	CMVEOuterChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		REFERENCE_TIME rtFrameDelta,	// Frame duration (in media time units)
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		WORD wCompressionRatio,			// Audio compression ratio
		BOOL bIs16Bit,					// Is audio 16-bit?
		HRESULT *phr					// Success or error code
	);
	~CMVEOuterChunkParser();

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
// MVE splitter filter class
//
// The filter uses CMVEOuterChunkParser helper class to perform 
// parsing of incoming data.
//==========================================================================

class CMVESplitterFilter :	public CBaseParserFilter
{

	DECLARE_FILETYPE

	// Audio & video format parameters (used by the inner parser when 
	// calculating sample stream and media times)
	REFERENCE_TIME	m_rtFrameDelta;			// Frame duration (in media time units)
	WORD			m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD			m_nAvgBytesPerSec;		// Audio data rate
	WORD			m_wCompressionRatio;	// Audio compression ratio
	BOOL			m_bIs16Bit;				// Is audio 16-bit?

	// Actual data parser
	CMVEOuterChunkParser *m_pParser;

	CMVESplitterFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CMVESplitterFilter();

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
// MVE splitter property page class
//==========================================================================

class CMVESplitterPage : public CBasePropertyPage
{

	CMVESplitterPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// MVE splitter setup data
//==========================================================================

extern const WCHAR g_wszMVESplitterName[];
extern const WCHAR g_wszMVESplitterPageName[];
extern const AMOVIESETUP_FILTER g_sudMVESplitter;

#endif
