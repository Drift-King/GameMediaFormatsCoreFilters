//==========================================================================
//
// File: BasePlainParser.h
//
// Desc: Game Media Formats - Header file for generic plain parser filter
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

#ifndef __GMF_BASE_PLAIN_PARSER_H__
#define __GMF_BASE_PLAIN_PARSER_H__

#include "BaseParser.h"

//==========================================================================
// Base plain parser filter class
//
// Provides the generic framework for plain files parsing.
// Plain files are the files of the format following the simple 
// scheme: [header][data], so that we only need to parse header  
// and then deliver data to output without any special parsing.
// Note that the header may have variable length and the data may 
// require to be delivered in packets of fixed size.
// Plain parser has only one output pin
//==========================================================================

class CBasePlainParserFilter : public CBaseParserFilter
{

public:

	// ---- Parsing methods ----- (Implementations of PURE methods)

	HRESULT Initialize(IPin *pPin, IAsyncReader *pReader);
	HRESULT Shutdown(void);

	// Be sure to call the base-class methods in your derived class 
	// implementation as they handle the packetwise delivery stuff
	HRESULT InitializeParser(void);
	HRESULT ShutdownParser(void);
	HRESULT ResetParser(void);

	HRESULT CheckInputType(const CMediaType* pmt);
	HRESULT GetInputType(int iPosition, CMediaType *pMediaType);

protected:

	// ---- Input media types ----

	UINT m_nMediaTypes;					// Number of input media types
	const REGPINTYPES *m_pMediaType;	// Array of input media types

	// ---- Packetwise delivery stuff ----

	LONG m_lPacketSize;			// Size of the packet (1 for no packeting)
	LONG m_lPacketLength;		// Actual length of data in the packet storage
	BYTE *m_pbPacketStorage;	// Packet storage

	// Construction/destruction
	CBasePlainParserFilter(
		TCHAR *pName,					// Object name
		LPUNKNOWN pUnk,					// Controlling IUnknown
		REFCLSID clsid,					// CLSID of the filter
		const WCHAR *wszFilterName,		// Filter name
		UINT nMediaTypes,				// Number of input media types
		const REGPINTYPES *pMediaType,	// Array of input media types
		HRESULT *phr					// Success or error code
	);
	virtual ~CBasePlainParserFilter();

	HRESULT Receive(
		LONGLONG llStartPosition,
		BYTE *pbData,
		LONG lDataSize
	);

	// ---- Methods used internally by the above parsing methods ---- PURE

	// Parse header. This method is responsible for providing information 
	// about the output pin to be created. It should fill in the media 
	// type fields. This method must also set the input buffer size and 
	// alignment, the output pin's memory allocator properties, the 
	// default start/stop positions, the desired packet size and the 
	// output pin's parser seeking capabilities, number of supported time
	// formats and create the supported time formats array.
	// Note that typically it's more than sufficient to set the output 
	// buffer size equal to the input buffer size -- we're not going to 
	// perform sophisticated parsing in this class anyway. Also, you can 
	// force dummy parser seeking creation by setting its capabilities, 
	// number of time formats and time formats array to zero.
	// In the implementation of Receive() we assume that the output 
	// media sample's buffer is large enough to receive pre-processing 
	// data, input sample data (possibly plus one packet from the 
	// previous input media sample) and post-processing data. Otherwise 
	// we're not able to deliver a sample to the output pin. Be sure 
	// to reserve necessary output buffer size -- take into account
	// the input buffer size and pre-/post-processing data (the size of 
	// one additional packet is added by the base class Initialize())
	virtual HRESULT Initialize(
		IPin *pPin,
		IAsyncReader *pReader,
		CMediaType *pmt,
		ALLOCATOR_PROPERTIES *pap,
		DWORD *pdwCapabilities,
		int *pnTimeFormats,
		GUID **ppTimeFormats
	) PURE;

	// Get sample size in stream and media times
	virtual HRESULT GetSampleDelta(
		LONG lDataLength,				// Length of data stored in the sample
		REFERENCE_TIME *prtStreamDelta,	// Sample size in stream time
		LONGLONG *pllMediaDelta			// Sample size in media time
	) PURE;

	// Pre-process the sample which is ready to be filled.
	// This method is intended to supply sample information 
	// which should precede actual sample data. If the method 
	// returns an error code, the sample is not delivered. 
	// The base-class implementation of the method does nothing
	virtual HRESULT PreProcess(IMediaSample *pSample);

	// Post-process the sample which is ready to be delivered.
	// This method is a good place to set various sample properties 
	// (like syncpoint, preroll, discontinuity). If the method 
	// returns an error code, the sample is not delivered. The 
	// base-class implementation of the method does nothing
	virtual HRESULT PostProcess(IMediaSample *pSample);

};

#endif