//==========================================================================
//
// File: ROQADPCMDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of ROQ ADPCM decompressor
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

#include "ROQADPCMDecompressor.h"
#include "resource.h"

//==========================================================================
// ROQ ADPCM decompressor setup data
//==========================================================================

// Global filter name
const WCHAR g_wszROQADPCMDecompressorName[] = L"ANX ROQ ADPCM Decompressor";

const AMOVIESETUP_MEDIATYPE sudROQADPCMType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_ROQADPCM
};

const AMOVIESETUP_MEDIATYPE sudPCMType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

const AMOVIESETUP_PIN sudROQADPCMDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudROQADPCMType	// Media types
	},
	{	// Output pin
		L"Output",			// Pin name
		FALSE,				// Is it rendered
		TRUE,				// Is it an output
		TRUE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Input",			// Connects to pin
		1,					// Number of types
		&sudPCMType			// Media types
	}
};

const AMOVIESETUP_FILTER g_sudROQADPCMDecompressor = {
	&CLSID_ROQADPCMDecompressor,	// CLSID of filter
	g_wszROQADPCMDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudROQADPCMDecompressorPins		// Pin information
};

//==========================================================================
// CROQADPCMDecompressor methods
//==========================================================================

CROQADPCMDecompressor::CROQADPCMDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("ROQ ADPCM Decompressor"),
		pUnk,
		CLSID_ROQADPCMDecompressor
	),
	m_pFormat(NULL)	// No format block at this time
{
	ASSERT(phr);
}

CROQADPCMDecompressor::~CROQADPCMDecompressor()
{
	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
	}
}

void CROQADPCMDecompressor::Decompress(
	WORD wInitialPrediction,
	BYTE *pbInput,
	LONG lInputLength,
	SHORT *piOutput
)
{
	// Allocate the array for current sample values
	SHORT *piSample = new SHORT[m_pFormat->nChannels];
	if (piSample == NULL)
		return;

	// Initialize sample values
	if (m_pFormat->nChannels == 1)
		piSample[0] = (SHORT)wInitialPrediction;
	else if (m_pFormat->nChannels == 2) {
		piSample[0] = (SHORT)((WORD)HIBYTE(wInitialPrediction) << 8);
		piSample[1] = (SHORT)((WORD)LOBYTE(wInitialPrediction) << 8);
	} else
		ASSERT(false); // We support only mono or stereo sound

	// Walk the input buffer until its length is exhausted
	while (lInputLength > 0) {

		// Process a byte per channel for all channels
		for (WORD i = 0; i < m_pFormat->nChannels; i++) {

			// Calculate the predition error
			SHORT iPredictionError;
			if (*pbInput < 128)
				iPredictionError = (*pbInput) * (*pbInput);
			else
				iPredictionError = - (*pbInput - 128) * (*pbInput - 128);
			pbInput++;
			lInputLength--;

			// Calculate the current sample
			piSample[i] += iPredictionError;

			// Clip the sample value (if needed)
			if		(piSample[i] > 32767)	piSample[i] = 32767;
			else if	(piSample[i] < -32768)	piSample[i] = -32768;

			// Place the sample into the output buffer
			*piOutput = piSample[i];
			piOutput++;
		}
	}

	// Free the current sample values array
	delete[] piSample;
}

CUnknown* WINAPI CROQADPCMDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CROQADPCMDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CROQADPCMDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

HRESULT CROQADPCMDecompressor::Transform(
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

	// Decompress the input buffer to the output one
	Decompress(
		*((WORD*)pbInBuffer),
		pbInBuffer + sizeof(WORD),
		lInDataLength - sizeof(WORD),
		(SHORT*)pbOutBuffer
	);

	// Set the data length for the output sample
	LONG lOutDataLength = (lInDataLength - sizeof(WORD)) * 2;
	hr = pOut->SetActualDataLength(lOutDataLength);
	if (FAILED(hr))
		return hr;

	// Each PCM sample is a sync point
	hr = pOut->SetSyncPoint(TRUE);
	if (FAILED(hr))
		return hr;

	// PCM sample should never be a preroll one
	hr = pOut->SetPreroll(FALSE);
	if (FAILED(hr))
		return hr;

	// We rely on the upstream filter (which is most likely 
	// a parser or splitter) in the matter of stream and media 
	// times setting. As to the discontinuity property, we should
	// not drop samples, so we just retain this property's value 
	// set by the upstream filter

	return NOERROR;
}

HRESULT CROQADPCMDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	// Check for proper audio media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Audio			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_ROQADPCM	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_ROQADPCM			)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CROQADPCMDecompressor::SetMediaType(
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

		// Free the old format block and allocate a new one
		if (m_pFormat)
			CoTaskMemFree(m_pFormat);
		m_pFormat = (ROQADPCMWAVEFORMAT*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());
	}

	return NOERROR;
}

HRESULT CROQADPCMDecompressor::BreakConnect(PIN_DIRECTION dir)
{
	// Catch only the input pin disconnection
	if (dir == PINDIR_INPUT) {
		// Free the format block
		if (m_pFormat) {
			CoTaskMemFree(m_pFormat);
			m_pFormat = NULL;
		}
	}

	return NOERROR;
}

HRESULT CROQADPCMDecompressor::CheckTransform(
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

	// Check if the output format is acceptable
	if (
		!IsEqualGUID(*mtOut->Type(),		MEDIATYPE_Audio		) ||
		!IsEqualGUID(*mtOut->Subtype(),		MEDIASUBTYPE_PCM	) ||
		!IsEqualGUID(*mtOut->FormatType(),	FORMAT_WaveFormatEx	)
	) 
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Get the media types' format blocks
	ROQADPCMWAVEFORMAT	*pInFormat	= (ROQADPCMWAVEFORMAT*)mtIn->Format();
	WAVEFORMATEX		*pOutFormat	= (WAVEFORMATEX*)mtOut->Format();

	// Compare the format blocks
	return (
		(pOutFormat->wFormatTag		== WAVE_FORMAT_PCM				) &&
		(pOutFormat->nChannels		== pInFormat->nChannels			) &&
		(pOutFormat->nSamplesPerSec	== pInFormat->nSamplesPerSec	) &&
		(pOutFormat->wBitsPerSample	== pInFormat->wBitsPerSample	)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CROQADPCMDecompressor::GetMediaType(
	int iPosition,
	CMediaType *pMediaType
)
{
	CAutoLock lock(m_pLock);

	// Check and validate the pointer
	CheckPointer(pMediaType, E_POINTER);
	ValidateWritePtr(pMediaType, sizeof(CMediaType));

	// At this time we should have the format block
	if (!m_pFormat)
		return E_UNEXPECTED;

	if (iPosition < 0)
		return E_INVALIDARG;
	else if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;
	else {
		// Prepare the format block
		WAVEFORMATEX wfex		= {0};
		wfex.wFormatTag			= WAVE_FORMAT_PCM;
		wfex.nChannels			= m_pFormat->nChannels;
		wfex.nSamplesPerSec		= m_pFormat->nSamplesPerSec;
		wfex.wBitsPerSample		= m_pFormat->wBitsPerSample;
		wfex.nBlockAlign		= (wfex.nChannels * wfex.wBitsPerSample) / 8;
		wfex.nAvgBytesPerSec	= wfex.nSamplesPerSec * wfex.nBlockAlign;
		wfex.cbSize				= 0;

		// We support the only media type
		pMediaType->InitMediaType();						// Is it needed ???
		pMediaType->SetType(&MEDIATYPE_Audio);				// Audio stream
		pMediaType->SetSubtype(&MEDIASUBTYPE_PCM);			// PCM audio
		pMediaType->SetSampleSize(wfex.nBlockAlign);		// Sample size from the format
		pMediaType->SetTemporalCompression(FALSE);			// No temporal compression
		pMediaType->SetFormatType(&FORMAT_WaveFormatEx);	// WAVEFORMATEX
		pMediaType->SetFormat((BYTE*)&wfex, sizeof(wfex));
	}

	return S_OK;
}

HRESULT CROQADPCMDecompressor::DecideBufferSize(
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

	// Set the properties: output buffer is two times larger 
	// then the input one, the buffers amount is the same and 
	// we don't care about alignment and prefix
	pProperties->cbBuffer = max(pProperties->cbBuffer, apInput.cbBuffer * 2);
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

STDMETHODIMP CROQADPCMDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_ROQADPCMDecompressorPage;

	return NOERROR;
}

//==========================================================================
// CROQADPCMDecompressorPage methods
//==========================================================================

const WCHAR g_wszROQADPCMDecompressorPageName[] = L"ANX ROQ ADPCM Decompressor Property Page";

CROQADPCMDecompressorPage::CROQADPCMDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("ROQ ADPCM Decompressor Property Page"),
		pUnk,
		IDD_ROQADPCMDECOMPRESSORPAGE,
		IDS_TITLE_ROQADPCMDECOMPRESSORPAGE
	)
{
}

CUnknown* WINAPI CROQADPCMDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CROQADPCMDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
