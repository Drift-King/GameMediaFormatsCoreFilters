//==========================================================================
//
// File: MVEADPCMDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of MVE ADPCM decompressor
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

#include "MVEADPCMDecompressor.h"
#include "resource.h"

//==========================================================================
// MVE ADPCM decompression table
//==========================================================================

const SHORT CMVEADPCMDecompressor::g_iStepTable[] =
{
         0,      1,      2,      3,      4,      5,      6,      7,
         8,      9,     10,     11,     12,     13,     14,     15,
        16,     17,     18,     19,     20,     21,     22,     23,
        24,     25,     26,     27,     28,     29,     30,     31,
        32,     33,     34,     35,     36,     37,     38,     39,
        40,     41,     42,     43,     47,     51,     56,     61,
        66,     72,     79,     86,     94,    102,    112,    122,
       133,    145,    158,    173,    189,    206,    225,    245,
       267,    292,    318,    348,    379,    414,    452,    493,
       538,    587,    640,    699,    763,    832,    908,    991,
      1081,   1180,   1288,   1405,   1534,   1673,   1826,   1993,
      2175,   2373,   2590,   2826,   3084,   3365,   3672,   4008,
      4373,   4772,   5208,   5683,   6202,   6767,   7385,   8059,
      8794,   9597,  10472,  11428,  12471,  13609,  14851,  16206,
     17685,  19298,  21060,  22981,  25078,  27367,  29864,  32589,
    -29973, -26728, -23186, -19322, -15105, -10503,  -5481,     -1,
         1,      1,   5481,  10503,  15105,  19322,  23186,  26728,
     29973, -32589, -29864, -27367, -25078, -22981, -21060, -19298,
    -17685, -16206, -14851, -13609, -12471, -11428, -10472,  -9597,
     -8794,  -8059,  -7385,  -6767,  -6202,  -5683,  -5208,  -4772,
     -4373,  -4008,  -3672,  -3365,  -3084,  -2826,  -2590,  -2373,
     -2175,  -1993,  -1826,  -1673,  -1534,  -1405,  -1288,  -1180,
     -1081,   -991,   -908,   -832,   -763,   -699,   -640,   -587,
      -538,   -493,   -452,   -414,   -379,   -348,   -318,   -292,
      -267,   -245,   -225,   -206,   -189,   -173,   -158,   -145,
      -133,   -122,   -112,   -102,    -94,    -86,    -79,    -72,
       -66,    -61,    -56,    -51,    -47,    -43,    -42,    -41,
       -40,    -39,    -38,    -37,    -36,    -35,    -34,    -33,
       -32,    -31,    -30,    -29,    -28,    -27,    -26,    -25,
       -24,    -23,    -22,    -21,    -20,    -19,    -18,    -17,
       -16,    -15,    -14,    -13,    -12,    -11,    -10,     -9,
        -8,     -7,     -6,     -5,     -4,     -3,     -2,     -1
};

//==========================================================================
// MVE ADPCM decompressor setup data
//==========================================================================

// Global filter name
const WCHAR g_wszMVEADPCMDecompressorName[] = L"ANX MVE ADPCM Decompressor";

const AMOVIESETUP_MEDIATYPE sudMVEADPCMType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_MVEADPCM
};

const AMOVIESETUP_MEDIATYPE sudPCMType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

const AMOVIESETUP_PIN sudMVEADPCMDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudMVEADPCMType	// Media types
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

const AMOVIESETUP_FILTER g_sudMVEADPCMDecompressor = {
	&CLSID_MVEADPCMDecompressor,	// CLSID of filter
	g_wszMVEADPCMDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudMVEADPCMDecompressorPins		// Pin information
};

//==========================================================================
// CMVEADPCMDecompressor methods
//==========================================================================

CMVEADPCMDecompressor::CMVEADPCMDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("MVE ADPCM Decompressor"),
		pUnk,
		CLSID_MVEADPCMDecompressor
	),
	m_pFormat(NULL)	// No format block at this time
{
	ASSERT(phr);
}

CMVEADPCMDecompressor::~CMVEADPCMDecompressor()
{
	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
	}
}

CUnknown* WINAPI CMVEADPCMDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CMVEADPCMDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CMVEADPCMDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

HRESULT CMVEADPCMDecompressor::Transform(
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
	SHORT *piOutBuffer = NULL;
	hr = pOut->GetPointer((BYTE**)&piOutBuffer);
	if (FAILED(hr))
		return hr;

	// Set up the number of channels
	WORD nChannels = (m_pFormat->wFlags & MVE_AUDIO_STEREO) ? 2 : 1;

	// Allocate the array for current sample values
	SHORT *piSample = new SHORT[nChannels];
	if (piSample == NULL)
		return E_OUTOFMEMORY;

	LONG lOutDataLength = 0;

	// Initialize sample values
	for (WORD i = 0; i < nChannels; i++) {
		piSample[i] = *((SHORT*)pbInBuffer);
		pbInBuffer += 2;
		lInDataLength -= 2;
		*piOutBuffer++ = piSample[i];
		lOutDataLength += 2;
	}

	// Walk the input buffer until its length is exhausted
	while (lInDataLength > 0) {

		// Process a byte per channel for all channels
		int i = 0;
		for (i = 0; i < nChannels; i++) {

			// Calculate the current sample
			piSample[i] += g_iStepTable[*pbInBuffer++];
			lInDataLength--;

			// Clip the sample value (if needed)
			if		(piSample[i] > 32767)	piSample[i] = 32767;
			else if	(piSample[i] < -32768)	piSample[i] = -32768;

			// Place the sample into the output buffer
			*piOutBuffer++ = piSample[i];
			lOutDataLength += 2;
		}
	}

	// Free the current sample values array
	delete[] piSample;

	// Set the data length for the output sample
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

HRESULT CMVEADPCMDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	// Check for proper audio media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Audio			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_MVEADPCM	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_MVEADPCM			) &&
		(((MVE_AUDIO_INFO*)mtIn->Format())->wFlags & MVE_AUDIO_16BIT) &&
		(((MVE_AUDIO_INFO*)mtIn->Format())->wFlags & MVE_AUDIO_COMPRESSED)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMVEADPCMDecompressor::SetMediaType(
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
		m_pFormat = (MVE_AUDIO_INFO*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());
	}

	return NOERROR;
}

HRESULT CMVEADPCMDecompressor::BreakConnect(PIN_DIRECTION dir)
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

HRESULT CMVEADPCMDecompressor::CheckTransform(
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
	MVE_AUDIO_INFO	*pInFormat	= (MVE_AUDIO_INFO*)mtIn->Format();
	WAVEFORMATEX	*pOutFormat	= (WAVEFORMATEX*)mtOut->Format();

	// Compare the format blocks
	return (
		(pOutFormat->wFormatTag		== WAVE_FORMAT_PCM									) &&
		(pOutFormat->nChannels		== (pInFormat->wFlags & MVE_AUDIO_STEREO) ? 2 : 1	) &&
		(pOutFormat->nSamplesPerSec	== pInFormat->wSampleRate							) &&
		(pOutFormat->wBitsPerSample	== (pInFormat->wFlags & MVE_AUDIO_16BIT) ? 16 : 8	)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMVEADPCMDecompressor::GetMediaType(
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
		wfex.nChannels			= (m_pFormat->wFlags & MVE_AUDIO_STEREO) ? 2 : 1;
		wfex.nSamplesPerSec		= m_pFormat->wSampleRate;
		wfex.wBitsPerSample		= (m_pFormat->wFlags & MVE_AUDIO_16BIT) ? 16 : 8;
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

HRESULT CMVEADPCMDecompressor::DecideBufferSize(
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

STDMETHODIMP CMVEADPCMDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_MVEADPCMDecompressorPage;

	return NOERROR;
}

//==========================================================================
// CMVEADPCMDecompressorPage methods
//==========================================================================

const WCHAR g_wszMVEADPCMDecompressorPageName[] = L"ANX MVE ADPCM Decompressor Property Page";

CMVEADPCMDecompressorPage::CMVEADPCMDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("MVE ADPCM Decompressor Property Page"),
		pUnk,
		IDD_MVEADPCMDECOMPRESSORPAGE,
		IDS_TITLE_MVEADPCMDECOMPRESSORPAGE
	)
{
}

CUnknown* WINAPI CMVEADPCMDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CMVEADPCMDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
