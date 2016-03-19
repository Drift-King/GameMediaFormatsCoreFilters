//==========================================================================
//
// File: ROQADPCMDecompressor.h
//
// Desc: Game Media Formats - Header file for ROQ ADPCM decompressor
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

#ifndef __GMF_ROQ_ADPCM_DECOMPRESSOR_H__
#define __GMF_ROQ_ADPCM_DECOMPRESSOR_H__

#include <streams.h>

#include "ROQGUID.h"
#include "ROQSpecs.h"

//==========================================================================
// ROQ ADPCM decompressor filter class
//==========================================================================

class CROQADPCMDecompressor :	public CTransformFilter,
								public ISpecifyPropertyPages
{

	// Format block
	ROQADPCMWAVEFORMAT *m_pFormat;

	// Constructor/destructor
	CROQADPCMDecompressor(LPUNKNOWN pUnk, HRESULT *phr);
	~CROQADPCMDecompressor();

	// Decompression method
	void Decompress(
		WORD wInitialPrediction,
		BYTE *pbInput,
		LONG lInputLength,
		SHORT *piOutput
	);

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
// ROQ ADPCM decompressor property page class
//==========================================================================

class CROQADPCMDecompressorPage : public CBasePropertyPage
{

	CROQADPCMDecompressorPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// ROQ ADPCM decompressor setup data
//==========================================================================

extern const WCHAR g_wszROQADPCMDecompressorName[];
extern const WCHAR g_wszROQADPCMDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudROQADPCMDecompressor;

#endif