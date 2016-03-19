//==========================================================================
//
// File: ADPCMInterleaver.h
//
// Desc: Game Media Formats - Header file for ADPCM interleaver. 
//       This filter performs 4-bit ADPCM multichannel data interleaving 
//       to be further processed by appropriate ADPCM decompressor which
//       expects its input data to have standard interleaving (LR LR ...)
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

#ifndef __GMF_ADPCM_INTERLEAVER_H__
#define __GMF_ADPCM_INTERLEAVER_H__

#include <initguid.h>
#include <streams.h>

//==========================================================================
// GUIDs
//==========================================================================

//
// ADPCM audio interleaver filter
//
// {77562712-FB85-4e28-A08A-46E579BD29D3}
DEFINE_GUID(CLSID_ADPCMInterleaver, 
0x77562712, 0xfb85, 0x4e28, 0xa0, 0x8a, 0x46, 0xe5, 0x79, 0xbd, 0x29, 0xd3);

//
// ADPCM audio interleaver filter property page
//
// {9D34E690-23E4-4bb2-BE3F-E677E7F59D7E}
DEFINE_GUID(CLSID_ADPCMInterleaverPage, 
0x9d34e690, 0x23e4, 0x4bb2, 0xbe, 0x3f, 0xe6, 0x77, 0xe7, 0xf5, 0x9d, 0x7e);

//==========================================================================
// ADPCM interleaver filter class
//==========================================================================

class CADPCMInterleaver :	public CTransformFilter,
							public ISpecifyPropertyPages
{

	// Interleaving parameters
	DWORD m_dwInterleavingType;	// Source interleaving type
	BOOL	m_bIsHiNibbleFirst;	// Should the higher nibble be processed first?

	// Constructor
	CADPCMInterleaver(LPUNKNOWN pUnk, HRESULT *phr);

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
// ADPCM interleaver property page class
//==========================================================================

class CADPCMInterleaverPage : public CBasePropertyPage
{

	CADPCMInterleaverPage(LPUNKNOWN pUnk);

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

//==========================================================================
// ADPCM interleaver setup data
//==========================================================================

extern const WCHAR g_wszADPCMInterleaverName[];
extern const WCHAR g_wszADPCMInterleaverPageName[];
extern const AMOVIESETUP_FILTER g_sudADPCMInterleaver;

#endif