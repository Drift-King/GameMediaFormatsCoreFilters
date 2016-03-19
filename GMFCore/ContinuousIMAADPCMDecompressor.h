//==========================================================================
//
// File: ContinuousIMAADPCMDecompressor.h
//
// Desc: Game Media Formats - Header file for continuous IMA ADPCM 
//       decompressor. This filter decompresses only 4-bit IMA ADPCM 
//       multichannel-interleaved data to 16-bit signed samples. 
//       No other IMA ADPCM formats are supported
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

#ifndef __GMF_CONTINUOUS_IMA_ADPCM_DECOMPRESSOR_H__
#define __GMF_CONTINUOUS_IMA_ADPCM_DECOMPRESSOR_H__

#include <streams.h>

#include "ContinuousIMAADPCM.h"

//==========================================================================
// Continuous IMA ADPCM decompressor filter class
//==========================================================================

class CCIMAADPCMDecompressor :	public CTransformFilter,
								public ISpecifyPropertyPages
{

	// ---- IMA ADPCM decompression tables ----

	static const CHAR g_chIndexAdjust[];
	static const LONG g_lStepTable[];

	// Format block
	CIMAADPCMWAVEFORMAT *m_pFormat;

	// Current decompressor state (array of info blocks)
	CIMAADPCMINFO *m_pInfo;

	// Constructor
	CCIMAADPCMDecompressor(LPUNKNOWN pUnk, HRESULT *phr);

	// Destructor
	~CCIMAADPCMDecompressor();

	// Utility decompression method. Note that this method relies
	// on the pointers' validity and does not check them
	void DecompressNibble(BYTE bCode, LONG *plSample, CHAR *pchIndex);

	// Actual decompression method. Note that you should supply 
	// to this method the compressed data block containing the 
	// integral number of (uncompressed) samples. That means 
	// the compressed data block should have the length being
	// the multiple of ((nChannels * 2) / 4) bytes
	void Decompress(BYTE *pbInput, LONG lInputLength, SHORT *piOutput);

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
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);

	// ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);

};

//==========================================================================
// Continuous IMA ADPCM decompressor property page class
//==========================================================================

class CCIMAADPCMDecompressorPage : public CBasePropertyPage
{

	CCIMAADPCMDecompressorPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// Continuous IMA ADPCM decompressor setup data
//==========================================================================

extern const WCHAR g_wszCIMAADPCMDecompressorName[];
extern const WCHAR g_wszCIMAADPCMDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudCIMAADPCMDecompressor;

#endif