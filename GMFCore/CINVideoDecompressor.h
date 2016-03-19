//==========================================================================
//
// File: CINVideoDecompressor.h
//
// Desc: Game Media Formats - Header file for CIN video decompressor
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

#ifndef __GMF_CIN_VIDEO_DECOMPRESSOR_H__
#define __GMF_CIN_VIDEO_DECOMPRESSOR_H__

#include <streams.h>

#include "CINSpecs.h"

//==========================================================================
// CIN video decompressor filter class
//==========================================================================

class CCINVideoDecompressor :	public CTransformFilter,
								public ISpecifyPropertyPages
{

	// ---- Huffman decoder stuff ----
	// Source code taken from xcinplay program 
	// by Dr. Tim Ferguson (timf@csse.monash.edu.au)

	typedef struct tagCIN_HUFFMAN_NODE {
		int iCount;
		BYTE bUsed;
		int iChildren[2];
	} CIN_HUFFMAN_NODE;

	CIN_HUFFMAN_NODE m_HuffmanNodes[256][HUFFMAN_TOKENS * 2];
	int m_nHuffmanNodes[256];

	// Format block
	CIN_HEADER *m_pFormat;

	// Constructor/destructor
	CCINVideoDecompressor(LPUNKNOWN pUnk, HRESULT *phr);
	~CCINVideoDecompressor();

	// ---- Huffman decoder methods ----
	// Source code taken from xcinplay program 
	// by Dr. Tim Ferguson (timf@csse.monash.edu.au)
	BOOL HuffmanDecode(BYTE *data, long len, BYTE *image);
	int HuffmanSmallestNode(CIN_HUFFMAN_NODE *hnodes, int num_hnodes);
	void HuffmanBuildTree(int prev);

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

	// ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);

};

//==========================================================================
// CIN video decompressor property page class
//==========================================================================

class CCINVideoDecompressorPage : public CBasePropertyPage
{

	CCINVideoDecompressorPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// CIN video decompressor setup data
//==========================================================================

extern const WCHAR g_wszCINVideoDecompressorName[];
extern const WCHAR g_wszCINVideoDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudCINVideoDecompressor;

#endif
