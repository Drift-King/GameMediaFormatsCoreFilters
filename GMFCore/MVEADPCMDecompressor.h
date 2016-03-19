//==========================================================================
//
// File: MVEADPCMDecompressor.h
//
// Desc: Game Media Formats - Header file for MVE ADPCM decompressor
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

#ifndef __GMF_MVE_ADPCM_DECOMPRESSOR_H__
#define __GMF_MVE_ADPCM_DECOMPRESSOR_H__

#include <streams.h>

#include "MVEGUID.h"
#include "MVESpecs.h"

//==========================================================================
// MVE ADPCM decompressor filter class
//==========================================================================

class CMVEADPCMDecompressor :	public CTransformFilter,
								public ISpecifyPropertyPages
{
	// Decoding step table
	static const SHORT g_iStepTable[];

	// Format block
	MVE_AUDIO_INFO *m_pFormat;

	// Constructor/destructor
	CMVEADPCMDecompressor(LPUNKNOWN pUnk, HRESULT *phr);
	~CMVEADPCMDecompressor();

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
// MVE ADPCM decompressor property page class
//==========================================================================

class CMVEADPCMDecompressorPage : public CBasePropertyPage
{

	CMVEADPCMDecompressorPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// MVE ADPCM decompressor setup data
//==========================================================================

extern const WCHAR g_wszMVEADPCMDecompressorName[];
extern const WCHAR g_wszMVEADPCMDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudMVEADPCMDecompressor;

#endif