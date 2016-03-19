//==========================================================================
//
// File: ADPCMInterleaver.cpp
//
// Desc: Game Media Formats - Implementation of ADPCM interleaver. 
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA//
//==========================================================================

#include "ContinuousIMAADPCM.h"
#include "ADPCMInterleaver.h"
#include "resource.h"

// Extract the nibble (upper nibble for TRUE, lower one for FALSE)
#define GETNIBBLE(b,n) ((n) ? (((b) & 0xF0) >> 4) : ((b) & 0x0F))

// Put the nibbles together into a byte (keeping the first nibble 
// positioned as the flag indicates)
#define PUTNIBBLES(a, b, f) ((f) ? (((a) << 4) | (b)) : (((b) << 4) | (a)))

//==========================================================================
// ADPCM interleaver setup data
//==========================================================================

// Global filter name
const WCHAR g_wszADPCMInterleaverName[] = L"ANX ADPCM Interleaver";

const AMOVIESETUP_MEDIATYPE sudADPCMTypes[] = {
	{	// IMA ADPCM
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_CIMAADPCM
	}
};

const AMOVIESETUP_PIN sudADPCMInterleaverPins[] = {
	{	// Input pin
		L"Input",		// Pin name
		FALSE,			// Is it rendered
		FALSE,			// Is it an output
		FALSE,			// Allowed none
		FALSE,			// Allowed many
		&CLSID_NULL,	// Connects to filter
		L"Output",		// Connects to pin
		1,				// Number of types
		sudADPCMTypes	// Media types
	},
	{	// Output pin
		L"Output",		// Pin name
		FALSE,			// Is it rendered
		TRUE,			// Is it an output
		TRUE,			// Allowed none
		FALSE,			// Allowed many
		&CLSID_NULL,	// Connects to filter
		L"Input",		// Connects to pin
		1,				// Number of types
		sudADPCMTypes	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudADPCMInterleaver = {
	&CLSID_ADPCMInterleaver,	// CLSID of filter
	g_wszADPCMInterleaverName,	// Filter name
	MERIT_NORMAL,				// Filter merit
	2,							// Number of pins
	sudADPCMInterleaverPins		// Pin information
};

//==========================================================================
// CADPCMInterleaver methods
//==========================================================================

CADPCMInterleaver::CADPCMInterleaver(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("ADPCM Interleaver"),
		pUnk,
		CLSID_ADPCMInterleaver
	),
	m_dwInterleavingType(-1),	// No interleaving type at this time
	m_bIsHiNibbleFirst(TRUE)	// No nibble order at this time
{
	ASSERT(phr);
}

CUnknown* WINAPI CADPCMInterleaver::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CADPCMInterleaver(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CADPCMInterleaver::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	// Check and validate the pointer
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	// Expose ISpecifyPropertyPages (and the base-class interfaces)
	if (riid == IID_ISpecifyPropertyPages)
		return GetInterface((ISpecifyPropertyPages*)this, ppv);
	else
		return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CADPCMInterleaver::Transform(
	IMediaSample *pIn,
	IMediaSample *pOut
)
{
	// Check and validate the pointers
	CheckPointer(pIn, E_POINTER);
	ValidateReadPtr(pIn, sizeof(IMediaSample));
	CheckPointer(pOut, E_POINTER);
	ValidateReadPtr(pOut, sizeof(IMediaSample));

	// Get the input sample's buffer
	BYTE *pbInBuffer = NULL;
	HRESULT hr = pIn->GetPointer(&pbInBuffer);
	if (FAILED(hr))
		return hr;

	// Get the input sample's data length
	LONG lInDataLength = pIn->GetActualDataLength();

	// Get the output sample's buffer
	BYTE *pbOutBuffer = NULL;
	hr = pOut->GetPointer(&pbOutBuffer);
	if (FAILED(hr))
		return hr;

	// Set the data length for the output sample
	hr = pOut->SetActualDataLength(lInDataLength);
	if (FAILED(hr))
		return hr;

	// Set up the beginning of each channel data and the increment
	BYTE *pbLeftData = NULL, *pbRightData = NULL;
	LONG lIncrement = 0;
	switch (m_dwInterleavingType) {

		case CIMAADPCM_INTERLEAVING_DOUBLE:
			pbLeftData	= pbInBuffer;
			pbRightData	= pbInBuffer + 1;
			lIncrement	= 2;
			break;

		case CIMAADPCM_INTERLEAVING_SPLIT:
			pbLeftData	= pbInBuffer;
			pbRightData	= pbInBuffer + lInDataLength / 2;
			lIncrement	= 1;
			break;

		default:
			// Wrong choice
			ASSERT(false);
			break;
	}

	while (lInDataLength > 0) {

		// Get nibbles for the first stereo sample
		BYTE bLeftNibble = GETNIBBLE(*pbLeftData, m_bIsHiNibbleFirst);
		BYTE bRightNibble = GETNIBBLE(*pbRightData, m_bIsHiNibbleFirst);

		// Put the nibbles together into the output buffer
		*pbOutBuffer = PUTNIBBLES(bLeftNibble, bRightNibble, m_bIsHiNibbleFirst);

		// Advance the output buffer pointer and data size
		pbOutBuffer++;
		lInDataLength--;

		// Get nibbles for the second stereo sample
		bLeftNibble = GETNIBBLE(*pbLeftData, !m_bIsHiNibbleFirst);
		bRightNibble = GETNIBBLE(*pbRightData, !m_bIsHiNibbleFirst);

		// Put the nibbles together into the output buffer
		*pbOutBuffer = PUTNIBBLES(bLeftNibble, bRightNibble, m_bIsHiNibbleFirst);

		// Advance the output buffer pointer and data size
		pbOutBuffer++;
		lInDataLength--;

		// Finally, advance the left/right data pointers
		pbLeftData += lIncrement;
		pbRightData += lIncrement;
	}

	return NOERROR;
}

HRESULT CADPCMInterleaver::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	if (!IsEqualGUID(*mtIn->Type(), MEDIATYPE_Audio))
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check for proper audio media type
	if (
		(IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_CIMAADPCM	)) &&
		(IsEqualGUID(*mtIn->FormatType(),	FORMAT_CIMAADPCM		))
	) {
		CIMAADPCMWAVEFORMAT *pFormat = (CIMAADPCMWAVEFORMAT*)mtIn->Format();
		return (
			(pFormat->dwReserved	!= CIMAADPCM_INTERLEAVING_NORMAL) &&
			(pFormat->nChannels		== 2)
		) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CADPCMInterleaver::SetMediaType(
	PIN_DIRECTION direction,
	const CMediaType *pmt
)
{
	CAutoLock lock(m_pLock);

	// Catch only the input pin's media type
	if (direction == PINDIR_INPUT) {

		// Check and validate the pointer
		CheckPointer(pmt, E_POINTER);
		ValidateReadPtr(pmt, sizeof(CMediaType));

		// Get the required parameters from the format block
		if (IsEqualGUID(*pmt->FormatType(), FORMAT_CIMAADPCM)) {
			CIMAADPCMWAVEFORMAT* pFormat = (CIMAADPCMWAVEFORMAT*)pmt->Format();
			m_dwInterleavingType	= pFormat->dwReserved;
			m_bIsHiNibbleFirst		= pFormat->bIsHiNibbleFirst;
		}
	}

	return NOERROR;
}

HRESULT CADPCMInterleaver::BreakConnect(PIN_DIRECTION dir)
{
	// Catch only the input pin disconnection
	if (dir == PINDIR_INPUT) {
		// Reset the parameters
		m_dwInterleavingType	= -1;
		m_bIsHiNibbleFirst		= TRUE;
	}

	return NOERROR;
}

HRESULT CADPCMInterleaver::CheckTransform(
	const CMediaType *mtIn,
	const CMediaType *mtOut
)
{
	// Check and validate the pointers
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));
	CheckPointer(mtOut, E_POINTER);
	ValidateReadPtr(mtOut, sizeof(CMediaType));

	// Check if the input media type is acceptable
	HRESULT hr = CheckInputType(mtIn);
	if (hr != S_OK)
		return hr;

	// Check if the output format is the same as the input one
	if (
		!IsEqualGUID(*mtOut->Type(),			*mtIn->Type()				) ||
		!IsEqualGUID(*mtOut->Subtype(),			*mtIn->Subtype()			) ||
		!IsEqualGUID(*mtOut->FormatType(),		*mtIn->FormatType()			) ||
			(mtOut->FormatLength()			!=	mtIn->FormatLength()		) ||
			(mtOut->IsTemporalCompressed()	!=	mtIn->IsTemporalCompressed()) ||
			(mtOut->GetSampleSize()			!=	mtIn->GetSampleSize()		)
	) 
		return VFW_E_TYPE_NOT_ACCEPTED;

	if (IsEqualGUID(*mtOut->FormatType(), FORMAT_CIMAADPCM)) {

		// Get the media types' format blocks
		CIMAADPCMWAVEFORMAT	*pInFormat	= (CIMAADPCMWAVEFORMAT*)mtIn->Format();
		CIMAADPCMWAVEFORMAT	*pOutFormat	= (CIMAADPCMWAVEFORMAT*)mtOut->Format();

		// Compare the format blocks
		return (
			(pOutFormat->nChannels			== pInFormat->nChannels			) &&
			(pOutFormat->nSamplesPerSec		== pInFormat->nSamplesPerSec	) &&
			(pOutFormat->wBitsPerSample		== pInFormat->wBitsPerSample	) &&
			(pOutFormat->bIsHiNibbleFirst	== pInFormat->bIsHiNibbleFirst	) &&
			(pOutFormat->dwReserved			== CIMAADPCM_INTERLEAVING_NORMAL)
		) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CADPCMInterleaver::GetMediaType(
	int iPosition,
	CMediaType *pMediaType
)
{
	CAutoLock lock(m_pLock);

	// Check and validate the pointer
	CheckPointer(pMediaType, E_POINTER);
	ValidateWritePtr(pMediaType, sizeof(CMediaType));

	// Check if we have the input pin and it's connected.
	// Otherwise there's no way to proceed any further
	if (
		(m_pInput == NULL) ||
		(!m_pInput->IsConnected())
	)
		return E_UNEXPECTED;

	if (iPosition < 0)
		return E_INVALIDARG;
	else if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;
	else {

		// Get media type from the input pin
		CMediaType& mt = m_pInput->CurrentMediaType(); 

		// Set the output media type to the input one
		*pMediaType = mt;

		// Change the interleaving type for the output format
		if (IsEqualGUID(*pMediaType->FormatType(), FORMAT_CIMAADPCM))
			((CIMAADPCMWAVEFORMAT*)pMediaType->Format())->dwReserved = CIMAADPCM_INTERLEAVING_NORMAL;
	}

	return S_OK;
}

HRESULT CADPCMInterleaver::DecideBufferSize(
	IMemAllocator *pAlloc,
	ALLOCATOR_PROPERTIES *pProperties
)
{
	CAutoLock lock(m_pLock);

	ASSERT(pAlloc);
	ASSERT(pProperties);

	// Check if we have the input pin and it's connected.
	// Otherwise there's no way to proceed any further
	if (
		(m_pInput == NULL) ||
		(!m_pInput->IsConnected())
	)
		return E_UNEXPECTED;

	// Get the input pin's allocator
	IMemAllocator *pInputAlloc = NULL;
	HRESULT hr = m_pInput->GetAllocator(&pInputAlloc);
	if (FAILED(hr))
		return hr;

	// Get the input pin's allocator properties
	ALLOCATOR_PROPERTIES apInput;
	hr = pInputAlloc->GetProperties(&apInput);
	if (FAILED(hr)) {
		pInputAlloc->Release();
		return hr;
	}

	// Release the input pin's allocator
	pInputAlloc->Release();

	// Set the properties: output buffer size is the same as
	// for the input one, the buffers amount is the same and 
	// we don't care about alignment and prefix
	pProperties->cbBuffer = max(pProperties->cbBuffer, apInput.cbBuffer);
	pProperties->cBuffers = max(pProperties->cBuffers, apInput.cBuffers);
	ALLOCATOR_PROPERTIES apActual;
	hr = pAlloc->SetProperties(pProperties, &apActual);
	if (FAILED(hr))
		return hr;

	// Check if the allocator is suitable
	return (
		(apActual.cBuffers < pProperties->cBuffers) ||
		(apActual.cbBuffer < pProperties->cbBuffer)
	) ? E_FAIL : NOERROR;
}

STDMETHODIMP CADPCMInterleaver::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_ADPCMInterleaverPage;

	return NOERROR;
}

//==========================================================================
// CADPCMInterleaverPage methods
//==========================================================================

const WCHAR g_wszADPCMInterleaverPageName[] = L"ANX ADPCM Interleaver Property Page";

CADPCMInterleaverPage::CADPCMInterleaverPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("ADPCM Interleaver Property Page"),
		pUnk,
		IDD_ADPCMINTERLEAVERPAGE,
		IDS_TITLE_ADPCMINTERLEAVERPAGE
	)
{
}

CUnknown* WINAPI CADPCMInterleaverPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CADPCMInterleaverPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
