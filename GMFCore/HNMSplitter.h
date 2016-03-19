//==========================================================================
//
// File: HNMSplitter.h
//
// Desc: Game Media Formats - Header file for HNM splitter filter
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

#ifndef __GMF_HNM_SPLITTER_H__
#define __GMF_HNM_SPLITTER_H__

#include "BaseParser.h"
#include "HNMSplitterConfig.h"

//==========================================================================
// HNM inner chunk parser class
// 
// This parser does actual parsing job. Note that the inner parser 
// maintains media sample properties and delivers samples.
//==========================================================================

class CHNMInnerChunkParser : public CBaseChunkParser 
{
	
	// Current media sample stuff
	CParserOutputPin	*m_pPin;	// Current output pin
	IMediaSample		*m_pSample;	// Current media sample
	REFERENCE_TIME		m_rtDelta;	// Sample's stream time delta
	LONGLONG			m_llDelta;	// Sample's media time delta
	
	// Junk data skipping stuff
	BOOL m_bShouldSkip;	// Should we skip the header junk in the AA subchunk?
	LONG m_lSkipLength;	// Length of data junk to be ignored

	// Audio & video format parameters (used by ParseChunkHeader() when 
	// calculating sample stream and media times)
	WORD	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

public:

	CHNMInnerChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		WORD nFramesPerSecond,			// Number of frames per second
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		HRESULT *phr					// Success or error code
	);
	~CHNMInnerChunkParser();

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
// HNM outer chunk parser class
//
// This parser just walks outer HNM chunks and use nested parser 
// to perform actual parsing of inner chunks
//==========================================================================

class CHNMOuterChunkParser : public CBaseChunkParser 
{
	
	CHNMInnerChunkParser m_InnerParser;
	
public:

	CHNMOuterChunkParser(
		CBaseParserFilter *pFilter,		// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		WORD nFramesPerSecond,			// Number of frames per second
		WORD nSampleSize,				// (Uncompressed) audio sample size in bytes
		DWORD nAvgBytesPerSec,			// Audio data rate
		HRESULT *phr					// Success or error code
	);
	~CHNMOuterChunkParser();

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
// HNM splitter filter class
//
// The filter uses CHNMOuterChunkParser helper class to perform 
// parsing of incoming data.
//==========================================================================

class CHNMSplitterFilter :	public CBaseParserFilter,
							public IConfigHNMSplitter
{

	DECLARE_FILETYPE

	// Audio & video format parameters (used by the inner parser when 
	// calculating sample stream and media times)
	WORD	m_nFramesPerSecond;		// Number of frames per second
	WORD	m_nSampleSize;			// (Uncompressed) audio sample size in bytes
	DWORD	m_nAvgBytesPerSec;		// Audio data rate

	// Stream durations
	DWORD m_nVideoFrames;	// Video stream duration in frames
	DWORD m_nAudioSamples;	// Audio stream duration in samples

	// External soundtrack (APC) options
	BOOL m_bUseExternalAPC;					// Should we use external APC file?
	OLECHAR m_szExternalAPCPath[MAX_PATH];	// Path (relative) to the APC files

	// External soundtrack (APC) stuff
	IBaseFilter *m_pAPCSource;				// APC source filter
	IBaseFilter *m_pAPCParser;				// APC parser filter
	CCritSec *m_pPassThruLock;				// Lock protecting pass-through streaming
	CPassThruInputPin *m_pPassThruInput;	// Pass-through input pin (for APC stream)
	CPassThruOutputPin *m_pPassThruOutput;	// Pass-through output pin (for APC stream)

	// Internally used APC source/parser shutdown method
	HRESULT ShutdownExternalAPC(void);

	// Utility function to get a pin with specified direction
	IPin* GetPinWithDirection(IBaseFilter *pFilter, PIN_DIRECTION PinDir);

	// Actual data parser
	CHNMOuterChunkParser *m_pParser;

	CHNMSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CHNMSplitterFilter();

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_IUNKNOWN;

	// Reveals IConfigHNMSplitter
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
	HRESULT GetDuration(
		LPCWSTR pPinName,
		const GUID *pCurrentFormat,
		LONGLONG *pDuration
	);

	// IConfigHNMSplitter methods
	STDMETHODIMP get_UseExternalAPC(BOOL *pbUseExternalAPC);
	STDMETHODIMP get_ExternalAPCPath(OLECHAR *szExternalAPCPath);
	STDMETHODIMP put_UseExternalAPC(BOOL bUseExternalAPC);
	STDMETHODIMP put_ExternalAPCPath(OLECHAR *szExternalAPCPath);

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
// HNM splitter property page class
//==========================================================================

class CHNMSplitterPage : public CBasePropertyPage
{

	CHNMSplitterPage(LPUNKNOWN pUnk);

	BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect(void);
	HRESULT OnActivate(void);
	HRESULT OnApplyChanges(void);

	// Utility (dialog controls handling) methods
	void SetDirty(void);
	BOOL GetUseExternalAPC(void);
	void EnableExternalAPCPath(BOOL bEnable);

	// Is it the first time set text in the edit control?
	BOOL m_bIsFirstSetText;

	// Configuration interface
	IConfigHNMSplitter *m_pConfig;

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// HNM splitter setup data
//==========================================================================

extern const WCHAR g_wszHNMSplitterName[];
extern const WCHAR g_wszHNMSplitterPageName[];
extern const AMOVIESETUP_FILTER g_sudHNMSplitter;

#endif
