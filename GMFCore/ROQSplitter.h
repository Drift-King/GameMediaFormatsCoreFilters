//==========================================================================
//
// File: ROQSplitter.h
//
// Desc: Game Media Formats - Header file for ROQ splitter filter
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

#ifndef __GMF_ROQ_SPLITTER_H__
#define __GMF_ROQ_SPLITTER_H__

#include "BaseParser.h"
#include "ROQSplitterConfig.h"

//==========================================================================
// ROQ chunk parser class
//==========================================================================

class CROQChunkParser : public CBaseChunkParser 
{
	
	// Current media sample stuff
	CParserOutputPin	*m_pPin;	// Current output pin
	IMediaSample		*m_pSample;	// Current media sample
	REFERENCE_TIME		m_rtDelta;	// Sample's stream time delta
	LONGLONG			m_llDelta;	// Sample's media time delta

	// Audio & video format parameters (used by ParseChunkHeader() when 
	// calculating sample stream and media times)
	WORD	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

public:

	CROQChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		WORD nFramesPerSecond,			// Number of frames per second
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		HRESULT *phr					// Success or error code
	);
	~CROQChunkParser();

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
// ROQ splitter filter class
//
// The filter uses CROQChunkParser helper class to perform 
// parsing of incoming data.
//==========================================================================

class CROQSplitterFilter :	public CBaseParserFilter,
							public IConfigROQSplitter
{

	DECLARE_FILETYPE

	// Audio & video format parameters (used by the parser when 
	// calculating sample stream and media times)
	WORD	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

	// External soundtrack (MP3) options
	BOOL m_bUseExternalMP3;					// Should we use external MP3 file?
	OLECHAR m_szExternalMP3Path[MAX_PATH];	// Path (relative) to the MP3 files

	// External soundtrack (MP3) stuff
	IBaseFilter *m_pMP3Source;				// MP3 source filter
	IBaseFilter *m_pMPEGSplitter;			// MPEG-1 splitter filter
	IBaseFilter *m_pMP3Decoder;				// MP3 decoder filter
	CCritSec *m_pPassThruLock;				// Lock protecting pass-through streaming
	CPassThruInputPin *m_pPassThruInput;	// Pass-through input pin (for MP3 stream)
	CPassThruOutputPin *m_pPassThruOutput;	// Pass-through output pin (for MP3 stream)

	// Internally used MP3 source/splitter shutdown method
	HRESULT ShutdownExternalMP3(void);

	// Utility function to get a pin with specified direction
	IPin* GetPinWithDirection(IBaseFilter *pFilter, PIN_DIRECTION PinDir);

	// Actual data parser
	CROQChunkParser *m_pParser;

	CROQSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CROQSplitterFilter();

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_IUNKNOWN;

	// Reveals IConfigROQSplitter
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Pin access (overridden to handle pass-through pins)
	CBasePin *GetPin(int n);
	int GetPinCount(void);

	// Overridden to handle the pass-through pins
	STDMETHODIMP Stop(void);

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

	// IConfigROQSplitter methods
	STDMETHODIMP get_UseExternalMP3(BOOL *pbUseExternalMP3);
	STDMETHODIMP get_ExternalMP3Path(OLECHAR *szExternalMP3Path);
	STDMETHODIMP put_UseExternalMP3(BOOL bUseExternalMP3);
	STDMETHODIMP put_ExternalMP3Path(OLECHAR *szExternalMP3Path);

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
// ROQ splitter property page class
//==========================================================================

class CROQSplitterPage : public CBasePropertyPage
{

	CROQSplitterPage(LPUNKNOWN pUnk);

	BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect(void);
	HRESULT OnActivate(void);
	HRESULT OnApplyChanges(void);

	// Utility (dialog controls handling) methods
	void SetDirty(void);
	BOOL GetUseExternalMP3(void);
	void EnableExternalMP3Path(BOOL bEnable);

	// Is it the first time set text in the edit control?
	BOOL m_bIsFirstSetText;

	// Configuration interface
	IConfigROQSplitter *m_pConfig;

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// ROQ splitter setup data
//==========================================================================

extern const WCHAR g_wszROQSplitterName[];
extern const WCHAR g_wszROQSplitterPageName[];
extern const AMOVIESETUP_FILTER g_sudROQSplitter;

#endif
