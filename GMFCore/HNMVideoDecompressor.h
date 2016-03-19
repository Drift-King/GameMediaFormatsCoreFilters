//==========================================================================
//
// File: HNMVideoDecompressor.h
//
// Desc: Game Media Formats - Header file for HNM video decompressor
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

#ifndef __GMF_HNM_VIDEO_DECOMPRESSOR_H__
#define __GMF_HNM_VIDEO_DECOMPRESSOR_H__

#include <streams.h>

#include "HNMSpecs.h"
#include "HNMVideoDecompressorConfig.h"

//==========================================================================
// HNM video decompressor filter class
//==========================================================================

class CHNMVideoDecompressor :	public CTransformFilter,
								public IConfigHNMVideoDecompressor,
								public ISpecifyPropertyPages
{

	// Filter option: should we check for color space converter?
	// This's a minor hack: when rendering 16-bit HNM video stream 
	// in 16-bit video mode using video hardware not supporting 
	// RGB overlays, the playback is very choppy. The problem is that
	// VMR renderer (which is thrown into the graph by default) does 
	// not require color space converter to be added when the video 
	// mode color depth matches the video stream color depth.
	// This option (when on) forces usage of color space converter 
	// filter on the output pin: the filter just rejects any other 
	// filter except for color space converter
	BOOL m_bRequireConverter;

	// Critical section protecting option values
	CCritSec m_csOptions;

	// ---- HNM decoder DLL and the functions exported by it ----

	HMODULE m_hDecoderDLL;

	// Initialize decoder
	BOOL (__cdecl *HNMPI_Init)(
		DWORD dwBPP,	// Bits per pixel
		DWORD dwHeight,	// Frame height
		DWORD dwWidth,	// Frame width
		PVOID pHeader	// File header data
	);

	// Decode frame
	BOOL (__cdecl *HNMPI_DecodeFrame)(
		PVOID pEncodedFrameData,	// Compressed frame data
		PVOID pPrevFrameData,		// Previous (decompressed) frame data
		PVOID pNextFrameData		// Next (decompressed) frame data
	);

	// Cleanup decoder
	BOOL (__cdecl *HNMPI_Cleanup)(void);

	// Format block
	HNM_HEADER *m_pFormat;
	WORD m_nFramesPerSecond;

	// Previous frame buffer
	BYTE *m_pPreviousFrame;

	// Constructor
	CHNMVideoDecompressor(LPUNKNOWN pUnk, HRESULT *phr);

	// Destructor
	~CHNMVideoDecompressor();

	// Internal usage clean-up method
	void Cleanup(void);

public:

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_IUNKNOWN;

	// Reveals IConfigHNMVideoDecompressor and ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Transform filter methods
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	HRESULT CheckConnect(PIN_DIRECTION dir, IPin *pPin);
	HRESULT BreakConnect(PIN_DIRECTION dir);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);

	// IConfigHNMVideoDecompressor methods
	STDMETHODIMP get_RequireConverter(BOOL *pbRequireConverter);
	STDMETHODIMP put_RequireConverter(BOOL bRequireConverter);

	// ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);

};

//==========================================================================
// HNM video decompressor property page class
//==========================================================================

class CHNMVideoDecompressorPage : public CBasePropertyPage
{

	CHNMVideoDecompressorPage(LPUNKNOWN pUnk);

	BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect(void);
	HRESULT OnActivate(void);
	HRESULT OnApplyChanges(void);

	// Configuration interface
	IConfigHNMVideoDecompressor *m_pConfig;

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// HNM video decompressor setup data
//==========================================================================

extern const WCHAR g_wszHNMVideoDecompressorName[];
extern const WCHAR g_wszHNMVideoDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudHNMVideoDecompressor;

#endif
