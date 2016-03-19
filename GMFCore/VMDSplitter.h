//==========================================================================
//
// File: VMDSplitter.h
//
// Desc: Game Media Formats - Header file for VMD splitter filter
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

#ifndef __GMF_VMD_SPLITTER_H__
#define __GMF_VMD_SPLITTER_H__

#include "BaseParser.h"
#include "VMDSpecs.h"

//==========================================================================
// VMD chunk parser class
//==========================================================================

class CVMDChunkParser : public CBaseChunkParser 
{
	
	// Current media sample stuff
	CParserOutputPin	*m_pPin;	// Current output pin
	IMediaSample		*m_pSample;	// Current media sample
	REFERENCE_TIME		m_rtDelta;	// Sample's stream time delta
	LONGLONG			m_llDelta;	// Sample's media time delta

	// Frame data and location information
	DWORD m_nFrames;				// Number of frames (both audio and video)
	DWORD *m_pOffsetTable;			// Table of frame offsets
	VMD_FRAME_ENTRY *m_pFrameTable;	// Table of frame entries
	
	// Audio & video format parameters (used by ParseChunkHeader() when 
	// calculating sample stream and media times)
	DWORD	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

public:

	CVMDChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		DWORD nFrames,					// Number of frames (both audio and video)
		DWORD *pOffsetTable,			// Table of frame offsets
		VMD_FRAME_ENTRY *pFrameTable,	// Table of frame entries
		DWORD nFramesPerSecond,			// Number of frames per second
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		HRESULT *phr					// Success or error code
	);
	~CVMDChunkParser();

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
// VMD splitter filter class
//
// The filter uses CVMDChunkParser helper class to perform 
// parsing of incoming data.
//==========================================================================

class CVMDSplitterFilter : public CBaseParserFilter
{

	DECLARE_FILETYPE

	// Frame data and location information
	DWORD m_nFrames;				// Number of frames (both audio and video)
	DWORD *m_pOffsetTable;			// Table of frame offsets
	VMD_FRAME_ENTRY *m_pFrameTable;	// Table of frame entries

	// Audio & video format parameters (used by the parser when 
	// calculating sample stream and media times)
	DWORD	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

	// Stream duration
	DWORD m_nVideoFrames;			// Video stream duration in frames

	// Actual data parser
	CVMDChunkParser *m_pParser;

	CVMDSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CVMDSplitterFilter();

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
// VMD splitter property page class
//==========================================================================

class CVMDSplitterPage : public CBasePropertyPage
{

	CVMDSplitterPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// VMD splitter setup data
//==========================================================================

extern const WCHAR g_wszVMDSplitterName[];
extern const WCHAR g_wszVMDSplitterPageName[];
extern const AMOVIESETUP_FILTER g_sudVMDSplitter;

#endif
