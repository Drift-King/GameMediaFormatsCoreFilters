//==========================================================================
//
// File: VQAVideoDecompressor.h
//
// Desc: Game Media Formats - Header file for VQA video decompressor
//
// Copyright (C) 2004 ANX Software.
// Author: Valery V. Anisimovsky
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

#ifndef __GMF_VQA_VIDEO_DECOMPRESSOR_H__
#define __GMF_VQA_VIDEO_DECOMPRESSOR_H__

#include <streams.h>

#include "VQASpecs.h"

//==========================================================================
// VQA video decompressor filter class
//==========================================================================

class CVQAVideoDecompressor :	public CTransformFilter,
								public ISpecifyPropertyPages
{
	// ---- Decoder data ----

	BYTE *m_pCurrentFrame;				// Current frame buffer
	BYTE m_pPalette[MAX_PALETTE_SIZE];	// Decompressed palette buffer
	int m_cbPalette;					// Size of palette data
	BYTE *m_pDecodeBuffer;				// Decompressed VPTR buffer
	int m_cbMaxDecodeBuffer;			// Size of decompressed VPTR buffer
	BYTE *m_pCodebook;					// Current codebook
	BYTE *m_pNextCodebook;				// Next codebook buffer
	DWORD m_cbNextCodebook;				// Size of the accumulated codebook parts
	BYTE m_nNextCodebookParts;			// Number of the accumulated codebook parts
	BOOL m_bIsNextCodebookCompressed;	// Is the next codebook compressed?
	BYTE m_bSolidColorMarker;			// Byte value indicating solid color block

	// Format block
	VQA_INFO *m_pFormat;				// Video info
	VQA_CIND_ENTRY *m_pCodebookTable;	// Full codebook descriptor table
	DWORD m_nCodebooks;					// Number of full codebook descriptors

	// Pre-calculated values
	DWORD m_nBlocksX;					// Number of blocks along X axis
	DWORD m_nBlocksY;					// Number of blocks along Y axis
	DWORD m_nBlocks;					// Total number of blocks in the image
	DWORD m_cbBlock;					// Size of the block
	DWORD m_cbBlockStride;				// Block stride
	DWORD m_cbImageXStride;				// Image stride along X axis
	DWORD m_cbImageYStride;				// Image stride along Y axis

	// Constructor/destructor
	CVQAVideoDecompressor(LPUNKNOWN pUnk, HRESULT *phr);
	~CVQAVideoDecompressor();

	// Internal clean-up method
	void Cleanup(void);

	// Video decoder methods
	HRESULT DecodeVPTR(BYTE *pbData, DWORD cbData, BYTE *pbImage);
	void FillBlock(BYTE *pbImage, WORD wIndex);
	void FillBlockSolid(BYTE *pbImage, BYTE bColor);
	void PutNewCodebook(void);

	// Format80 decoder source code taken from vqavideo.c
	// by Mike Melanson (melanson@pcisys.net)
	HRESULT DecodeFormat80(
		unsigned char *src,
		int src_size,
		unsigned char *dest,
		int& dest_size
	);

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
	HRESULT EndFlush(void);

	// ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);

};

//==========================================================================
// VQA video decompressor property page class
//==========================================================================

class CVQAVideoDecompressorPage : public CBasePropertyPage
{

	CVQAVideoDecompressorPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// VQA video decompressor setup data
//==========================================================================

extern const WCHAR g_wszVQAVideoDecompressorName[];
extern const WCHAR g_wszVQAVideoDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudVQAVideoDecompressor;

#endif
