//==========================================================================
//
// File: FSTSplitter.h
//
// Desc: Game Media Formats - Header file for FST splitter filter
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

#ifndef __GMF_FST_SPLITTER_H__
#define __GMF_FST_SPLITTER_H__

#include "BaseParser.h"
#include "FSTSpecs.h"

//==========================================================================
// FST chunk parser class
//==========================================================================

class CFSTChunkParser : public CBaseChunkParser 
{
	
	// Current media sample stuff
	CParserOutputPin	*m_pPin;	// Current output pin
	IMediaSample		*m_pSample;	// Current media sample
	REFERENCE_TIME		m_rtDelta;	// Sample's stream time delta
	LONGLONG			m_llDelta;	// Sample's media time delta

	// Stream information
	DWORD m_nVideoFrames;			// Number of frames
	FST_FRAME_ENTRY *m_pFrameTable;	// Table of frame entries
	
	// Audio & video format parameters (used by ParseChunkHeader() when 
	// calculating sample stream and media times)
	DWORD	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

public:

	CFSTChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		DWORD nVideoFrames,				// Number of frames
		FST_FRAME_ENTRY *pFrameTable,	// Table of frame entries
		DWORD nFramesPerSecond,			// Number of frames per second
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		HRESULT *phr					// Success or error code
	);
	~CFSTChunkParser();

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
// FST splitter filter class
//
// The filter uses CFSTChunkParser helper class to perform 
// parsing of incoming data.
//==========================================================================

class CFSTSplitterFilter : public CBaseParserFilter
{

	DECLARE_FILETYPE

	// Audio & video format parameters (used by the parser when 
	// calculating sample stream and media times)
	DWORD	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

	// Stream duration
	DWORD m_nVideoFrames;			// Video stream duration in frames

	FST_FRAME_ENTRY *m_pFrameTable;	// Table of frame entries

	// Actual data parser
	CFSTChunkParser *m_pParser;

	CFSTSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CFSTSplitterFilter();

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
	HRESULT GetDuration(
		LPCWSTR pPinName,
		const GUID *pCurrentFormat,
		LONGLONG *pDuration
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
// FST splitter property page class
//==========================================================================

class CFSTSplitterPage : public CBasePropertyPage
{

	CFSTSplitterPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// FST splitter setup data
//==========================================================================

extern const WCHAR g_wszFSTSplitterName[];
extern const WCHAR g_wszFSTSplitterPageName[];
extern const AMOVIESETUP_FILTER g_sudFSTSplitter;

#endif
