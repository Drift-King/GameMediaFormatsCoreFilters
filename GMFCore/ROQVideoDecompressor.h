//==========================================================================
//
// File: ROQVideoDecompressor.h
//
// Desc: Game Media Formats - Header file for ROQ video decompressor
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

#ifndef __GMF_ROQ_VIDEO_DECOMPRESSOR_H__
#define __GMF_ROQ_VIDEO_DECOMPRESSOR_H__

#include <streams.h>

#include "ROQSpecs.h"

//==========================================================================
// ROQ video decompressor filter class
//==========================================================================

class CROQVideoDecompressor :	public CTransformFilter,
								public ISpecifyPropertyPages
{

	// ---- ROQ video decoder stuff ----
	// Source code taken from roqvideo.c
	// by Dr. Tim Ferguson (timf@csse.monash.edu.au)

	// Previous frame data
	BYTE *m_pPreviousFrame, *m_pCurrentFrame;

	// Strides for Y and Cb/Cr planes
	WORD m_wYStride;
	WORD m_wCStride;

	// 2x2 and 4x4 codebooks
	ROQ_CELL m_Cells[256];
	ROQ_QCELL m_QCells[256];

	// Format block
	ROQ_VIDEO_FORMAT *m_pFormat;

	// Constructor/destructor
	CROQVideoDecompressor(LPUNKNOWN pUnk, HRESULT *phr);
	~CROQVideoDecompressor();

	// ---- ROQ video decoder methods ----
	// Source code taken from roqvideo.c 
	// by Dr. Tim Ferguson (timf@csse.monash.edu.au)
	void ApplyVector2x2(int x, int y, ROQ_CELL *cell);
	void ApplyVector4x4(int x, int y, ROQ_CELL *cell);
	void ApplyMotion4x4(int x, int y, unsigned char mv, char mean_x, char mean_y);
	void ApplyMotion8x8(int x, int y, unsigned char mv, char mean_x, char mean_y);
	void ROQVideoDecodeFrame(BYTE *pbData, DWORD dwSize, WORD wArgument);

	// Utility YUV plane pointers methods
	inline BYTE* GetYPlane(BYTE *pbImage) { return pbImage; };
	/* inline BYTE* GetUPlane(BYTE *pbImage) { return pbImage + 5 * m_pFormat->wWidth * m_pFormat->wHeight / 4; }; */
	inline BYTE* GetUPlane(BYTE *pbImage) { return pbImage + 2 * m_pFormat->wWidth * m_pFormat->wHeight; };
	inline BYTE* GetVPlane(BYTE *pbImage) { return pbImage + m_pFormat->wWidth * m_pFormat->wHeight; };

	// Utility method to downsample upsampled U/V planes
	// and put the result to the output buffer
	void DownsampleColorPlanes(BYTE *pbInImage, BYTE *pbOutImage);

public:

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_IUNKNOWN;

	// Reveals ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Transform filter methods
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
// ROQ video decompressor property page class
//==========================================================================

class CROQVideoDecompressorPage : public CBasePropertyPage
{

	CROQVideoDecompressorPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// ROQ video decompressor setup data
//==========================================================================

extern const WCHAR g_wszROQVideoDecompressorName[];
extern const WCHAR g_wszROQVideoDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudROQVideoDecompressor;

#endif
