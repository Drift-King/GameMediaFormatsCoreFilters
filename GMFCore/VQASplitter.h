//==========================================================================
//
// File: VQASplitter.h
//
// Desc: Game Media Formats - Header file for VQA splitter filter
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

#ifndef __GMF_VQA_SPLITTER_H__
#define __GMF_VQA_SPLITTER_H__

#include "BaseParser.h"
#include "VQASpecs.h"

//==========================================================================
// VQA chunk parser class
//==========================================================================

class CVQAChunkParser : public CBaseChunkParser 
{
	
	// Current media sample stuff
	CParserOutputPin	*m_pPin;	// Current output pin
	IMediaSample		*m_pSample;	// Current media sample
	REFERENCE_TIME		m_rtDelta;	// Sample's stream time delta
	LONGLONG			m_llDelta;	// Sample's media time delta

	// Audio & video format parameters (used by ParseChunkHeader() when 
	// calculating sample stream and media times)
	BYTE	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_wCompressionRatio;	// PCM or IMA ADPCM compression ratio
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

public:

	CVQAChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		BYTE nFramesPerSecond,			// Number of frames per second
		WORD wCompressionRatio,			// IMA ADPCM compression ratio
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		HRESULT *phr					// Success or error code
	);
	~CVQAChunkParser();

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
// VQA splitter filter class
//
// The filter uses CVQAChunkParser helper class to perform 
// parsing of incoming data.
//==========================================================================

class CVQASplitterFilter : public CBaseParserFilter
{

	DECLARE_FILETYPE

	// Audio & video format parameters (used by the parser when 
	// calculating sample stream and media times)
	BYTE	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_wCompressionRatio;	// PCM or IMA ADPCM compression ratio
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

	// Stream durations
	DWORD m_nVideoFrames;	// Video stream duration in frames

	// Seeking stuff
	DWORD *m_pFrameTable;				// Table of frame offsets
	DWORD m_cbFrameTable;				// Size of the frame table
	VQA_CIND_ENTRY *m_pCodebookTable;	// Full codebook descriptor table
	DWORD m_nCodebooks;					// Number of full codebook descriptors
	//BYTE m_nCBParts;					// Number of codebooks parts

	// Actual data parser
	CVQAChunkParser *m_pParser;

	CVQASplitterFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CVQASplitterFilter();

	/* TEMP: Seeking stuff (not working)
	// Utility method used for seeking
	HRESULT DeliverChunk(
		CParserOutputPin *pPin,	// Output pin to deliver chunk through
		LONGLONG llStart,		// File position to start scanning
		LONGLONG llStop,		// File position to stop scanning
		DWORD dwID,				// Chunk ID
		DWORD dwMask			// Chunk ID mask
	);
	*/

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
	HRESULT ConvertPosition(
		LONGLONG *pllTarget,
		LPCWSTR pPinName,
		const GUID *pSourceFormat,
		LONGLONG *pllSource,
		DWORD dwSourceFlags
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
// VQA splitter property page class
//==========================================================================

class CVQASplitterPage : public CBasePropertyPage
{

	CVQASplitterPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// VQA splitter setup data
//==========================================================================

extern const WCHAR g_wszVQASplitterName[];
extern const WCHAR g_wszVQASplitterPageName[];
extern const AMOVIESETUP_FILTER g_sudVQASplitter;

#endif
