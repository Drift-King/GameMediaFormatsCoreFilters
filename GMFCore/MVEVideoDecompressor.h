//==========================================================================
//
// File: MVEVideoDecompressor.h
//
// Desc: Game Media Formats - Header file for MVE video decompressor
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

#ifndef __GMF_MVE_VIDEO_DECOMPRESSOR_H__
#define __GMF_MVE_VIDEO_DECOMPRESSOR_H__

#include <streams.h>

#include "MVESpecs.h"

//==========================================================================
// MVE video decompressor filter class
//==========================================================================

class CMVEVideoDecompressor :	public CTransformFilter,
								public ISpecifyPropertyPages
{

	// Video decoder buffers
	BYTE *m_pVideoMap;		// Latest video decoding map
	DWORD m_cbVideoMap;		// Video map size
	BYTE *m_pCurrentFrame;	// Current frame buffer
	BYTE *m_pPreviousFrame;	// Previous frame buffer

	// Current palette
	struct tagRGBTriple {
		BYTE bRed;
		BYTE bGreen;
		BYTE bBlue;
	} m_Palette[256];
	WORD m_iPaletteStart;
	WORD m_nPaletteEntries;

	// Format block
	MVE_VIDEO_INFO *m_pFormat;
	DWORD m_cbFormat;
	DWORD m_dwVideoWidth, m_dwVideoHeight;
	BOOL m_bVideoModeChanged;

	// Constructor/destructor
	CMVEVideoDecompressor(LPUNKNOWN pUnk, HRESULT *phr);
	~CMVEVideoDecompressor();

	// Internal video buffers setup/allocation/freeing methods
	void SetVideoParameters(WORD wWidth, WORD wHeight);
	HRESULT AllocateVideoBuffers(void);
	void FreeVideoBuffers(void);

	// ---- MVE decoder methods ----
	// Source code taken from libmve library
	// by <don't-know-whom> (<don't-know-email>)

	int close_table[512];
	int far_p_table[512];
	int far_n_table[512];
	int lookup_initialized;
	void GenLoopkupTable();
	void RelClose(int i, int *x, int *y);
	void RelFar(int i, int sign, int *x, int *y);

	void DecodeFrame8(
		unsigned char *pFrame,
		unsigned char *pMap,
		int mapRemain,
		unsigned char *pData,
		int dataRemain
	);
	void CopyFrame8(unsigned char *pDest, unsigned char *pSrc);
	void PatternRow4Pixels8(
		unsigned char *pFrame,
		unsigned char pat0,
		unsigned char pat1,
		unsigned char *p
	);
	void PatternRow4Pixels2_8(
		unsigned char *pFrame,
		unsigned char pat0,
		unsigned char *p
	);
	void PatternRow4Pixels2x1_8(
		unsigned char *pFrame,
		unsigned char pat,
		unsigned char *p
	);
	void PatternQuadrant4Pixels8(
		unsigned char *pFrame,
		unsigned char pat0,
		unsigned char pat1,
		unsigned char pat2,
		unsigned char pat3,
		unsigned char *p
	);
	void PatternRow2Pixels8(
		unsigned char *pFrame,
		unsigned char pat,
		unsigned char *p
	);
	void PatternRow2Pixels2_8(
		unsigned char *pFrame,
		unsigned char pat,
		unsigned char *p
	);
	void PatternQuadrant2Pixels8(
		unsigned char *pFrame,
		unsigned char pat0,
		unsigned char pat1,
		unsigned char *p
	);
	void DispatchDecoder8(
		unsigned char **pFrame,
		unsigned char codeType,
		unsigned char **pData,
		int *pDataRemain,
		int *curXb,
		int *curYb
	);

	void DecodeFrame16(
		unsigned char *pFrame,
		unsigned char *pMap,
		int mapRemain,
		unsigned char *pData,
		int dataRemain
	);
	unsigned short GETPIXEL(unsigned char **buf, int off);
	unsigned short GETPIXELI(unsigned char **buf, int off);
	void CopyFrame16(unsigned short *pDest, unsigned short *pSrc);
	void PatternRow4Pixels16(
		unsigned short *pFrame,
		unsigned char pat0,
		unsigned char pat1,
		unsigned short *p
	);
	void PatternRow4Pixels2_16(
		unsigned short *pFrame,
		unsigned char pat0,
		unsigned short *p
	);
	void PatternRow4Pixels2x1_16(
		unsigned short *pFrame,
		unsigned char pat,
		unsigned short *p
	);
	void PatternQuadrant4Pixels16(
		unsigned short *pFrame,
		unsigned char pat0,
		unsigned char pat1,
		unsigned char pat2,
		unsigned char pat3,
		unsigned short *p
	);
	void PatternRow2Pixels16(
		unsigned short *pFrame,
		unsigned char pat,
		unsigned short *p
	);
	void PatternRow2Pixels2_16(
		unsigned short *pFrame,
		unsigned char pat,
		unsigned short *p
	);
	void PatternQuadrant2Pixels16(
		unsigned short *pFrame,
		unsigned char pat0,
		unsigned char pat1,
		unsigned short *p
	);
	void DispatchDecoder16(
		unsigned short **pFrame,
		unsigned char codeType,
		unsigned char **pData,
		unsigned char **pOffData,
		int *pDataRemain,
		int *curXb,
		int *curYb
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

	// ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);

};

//==========================================================================
// MVE video decompressor property page class
//==========================================================================

class CMVEVideoDecompressorPage : public CBasePropertyPage
{

	CMVEVideoDecompressorPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// MVE video decompressor setup data
//==========================================================================

extern const WCHAR g_wszMVEVideoDecompressorName[];
extern const WCHAR g_wszMVEVideoDecompressorPageName[];
extern const AMOVIESETUP_FILTER g_sudMVEVideoDecompressor;

#endif
