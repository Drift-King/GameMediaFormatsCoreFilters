//==========================================================================
//
// File: WSADPCMDecompressor.h
//
// Desc: Game Media Formats - Header file for WS ADPCM decompressor
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

#ifndef __GMF_WS_ADPCM_DECOMPRESSOR_H__
#define __GMF_WS_ADPCM_DECOMPRESSOR_H__

#include <streams.h>

#include "WSADPCM.h"

//==========================================================================
// WS ADPCM decompressor filter class
//==========================================================================

class CWSADPCMDecompressor :	public CTransformFilter,
								public ISpecifyPropertyPages
{

	// ---- WS ADPDCM decompression tables ----

	static const CHAR g_chWSTable2bit[];
	static const CHAR g_chWSTable4bit[];

	// Format block
	WSADPCMWAVEFORMAT *m_pFormat;

	// Constructor/destructor
	CWSADPCMDecompressor(LPUNKNOWN pUnk, HRESULT *phr);
	~CWSADPCMDecompressor();

	// Utility method (sample clipping)
	static void Clip8BitSample(SHORT *iSample);

	// Decompression method
	void Decompress(WORD wInSize, BYTE *pbInput, WORD wOutSize, BYTE *pbOutput);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_IUNKNOWN;

	// Reveals ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Transform filter methods (implementations of pure methods)
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	HRESULT BreakConnect(PIN_DIRECTION dir);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);

	// ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);

};

//==========================================================================
// WS ADPCM decompressor property page class
//==========================================================================

class CWSADPCMDecompressorPage : public CBasePropertyPage
{

	CWSADPCMDecompressorPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// WS ADPCM decompressor setup data
//==========================================================================

extern const WCHAR g_wszWSADPCMDecompressorName[];
extern const WCHAR g_wszWSADPCMDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudWSADPCMDecompressor;

#endif