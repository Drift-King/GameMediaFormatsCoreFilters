//==========================================================================
//
// File: ContinuousIMAADPCMDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of continuous IMA ADPCM 
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA//
//==========================================================================

#include "ContinuousIMAADPCM.h"
#include "ContinuousIMAADPCMDecompressor.h"
#include "resource.h"

//==========================================================================
// IMA ADPCM decompression macros and tables
//==========================================================================

// Extract the nibble (upper nibble for TRUE, lower one for FALSE)
#define NIBBLE(b,n) ((n) ? (((b) & 0xF0) >> 4) : ((b) & 0x0F))

const CHAR CCIMAADPCMDecompressor::g_chIndexAdjust[]=
{ -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

const LONG CCIMAADPCMDecompressor::g_lStepTable[]=
{
	7,		8,		9,		10,		11,		12,		13,		14,		16,
	17,		19,		21,		23,		25,		28,		31,		34,		37,
	41,		45,		50,		55,		60,		66,		73,		80,		88,
	97,		107,	118,	130,	143,	157,	173,	190,	209,
	230,	253,	279,	307,	337,	371,	408,	449,	494,
	544,	598,	658,	724,	796,	876,	963,	1060,	1166,
	1282,	1411,	1552,	1707,	1878,	2066,	2272,	2499,	2749,
	3024,	3327,	3660,	4026,	4428,	4871,	5358,	5894,	6484,
	7132,	7845,	8630,	9493,	10442,	11487,	12635,	13899,	15289,
	16818,	18500,	20350,	22385,	24623,	27086,	29794,	32767
};

//==========================================================================
// Continuous IMA ADPCM decompressor setup data
//==========================================================================

// Global filter name
const WCHAR g_wszCIMAADPCMDecompressorName[] = L"ANX IMA ADPCM Decompressor";

const AMOVIESETUP_MEDIATYPE sudCIMAADPCMType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_CIMAADPCM
};

const AMOVIESETUP_MEDIATYPE sudPCMType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

const AMOVIESETUP_PIN sudCIMAADPCMDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudCIMAADPCMType	// Media types
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

const AMOVIESETUP_FILTER g_sudCIMAADPCMDecompressor = {
	&CLSID_CIMAADPCMDecompressor,	// CLSID of filter
	g_wszCIMAADPCMDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudCIMAADPCMDecompressorPins	// Pin information
};

//==========================================================================
// CCIMAADPCMDecompressor methods
//==========================================================================

CCIMAADPCMDecompressor::CCIMAADPCMDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("Continuous IMA ADPCM Decompressor"),
		pUnk,
		CLSID_CIMAADPCMDecompressor
	),
	m_pFormat(NULL),	// No format block at this time
	m_pInfo(NULL)		// No info array at this time
{
	ASSERT(phr);
}

CCIMAADPCMDecompressor::~CCIMAADPCMDecompressor()
{
	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
	}

	// Free the info blocks array
	if (m_pInfo) {
		CoTaskMemFree(m_pInfo);
		m_pInfo = NULL;
	}
}

void CCIMAADPCMDecompressor::DecompressNibble(
	BYTE bCode,
	LONG *plSample,
	CHAR *pchIndex
)
{
	// Compose sample delta
	LONG			lDelta  = g_lStepTable[*pchIndex] >> 3; 
	if (bCode & 1)	lDelta += g_lStepTable[*pchIndex] >> 2;
	if (bCode & 2)	lDelta += g_lStepTable[*pchIndex] >> 1;
	if (bCode & 4)	lDelta += g_lStepTable[*pchIndex];

	// Update the current sample value
	if (bCode & 8)
		*plSample -= lDelta;
	else
		*plSample += lDelta;

	// Clip the sample value (if needed)
	if		(*plSample > 32767)		*plSample = 32767;
    else if	(*plSample < -32768)	*plSample = -32768;

	// Update the current index value
	*pchIndex += g_chIndexAdjust[bCode];

	// Clip the index value (if needed)
	if		(*pchIndex < 0)		*pchIndex = 0;
    else if	(*pchIndex > 88)	*pchIndex = 88;
}

void CCIMAADPCMDecompressor::Decompress(
	BYTE *pbInput,
	LONG lInputLength,
	SHORT *piOutput
)
{
	// Set up the first nibble to be processed
	BOOL bIsCurrentNibbleHigh = m_pFormat->bIsHiNibbleFirst;

	// Walk the input buffer until its length is exhausted
	while (lInputLength > 0) {

		// Process a nibble per channel for all channels
		for (WORD i = 0; i < m_pFormat->nChannels; i++) {

			// Decompress the current nibble code
			DecompressNibble(
				NIBBLE(*pbInput, bIsCurrentNibbleHigh),
				&(m_pInfo[i].lSample),
				&(m_pInfo[i].chIndex)
			);

			// Place the sample into the output buffer
			*piOutput = (SHORT)m_pInfo[i].lSample;
			piOutput++;

			// Advance the current nibble
			bIsCurrentNibbleHigh = !bIsCurrentNibbleHigh;
			
			// If the next nibble is what the first nibble should be
			// then we have to advance to the next byte
			if (bIsCurrentNibbleHigh == m_pFormat->bIsHiNibbleFirst) {
				pbInput++;
				lInputLength--;
			}
		}
	}
}

CUnknown* WINAPI CCIMAADPCMDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CCIMAADPCMDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CCIMAADPCMDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

HRESULT CCIMAADPCMDecompressor::Transform(
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

	// Check if the input media type has changed
	AM_MEDIA_TYPE *pmt = NULL;
	hr = pIn->GetMediaType(&pmt);
	if ((hr == S_OK) && (pmt != NULL)) {

		// Get the format block
		CIMAADPCMWAVEFORMAT *pFormat = (CIMAADPCMWAVEFORMAT*)pmt->pbFormat;

		// Set the new initial parameters
		for (WORD i = 0; i < m_pFormat->nChannels; i++) {
			m_pInfo[i].lSample = pFormat->pInit[i].lSample;
			m_pInfo[i].chIndex = pFormat->pInit[i].chIndex;
		}

		// Delete media type
		DeleteMediaType(pmt);
	}

	// Decompress the input buffer to the output one
	Decompress(pbInBuffer, lInDataLength, (SHORT*)pbOutBuffer);

	// Set the data length for the output sample.
	// Output samples's data length is four times 
	// larger the input's one
	LONG lOutDataLength = lInDataLength * 4;
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

HRESULT CCIMAADPCMDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	// Check for proper audio media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Audio			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_CIMAADPCM	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_CIMAADPCM		) &&
		(((CIMAADPCMWAVEFORMAT*)mtIn->Format())->dwReserved		== CIMAADPCM_INTERLEAVING_NORMAL) &&
		(((CIMAADPCMWAVEFORMAT*)mtIn->Format())->wBitsPerSample	== 16)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CCIMAADPCMDecompressor::SetMediaType(
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
		m_pFormat = (CIMAADPCMWAVEFORMAT*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());
	}

	return NOERROR;
}

HRESULT CCIMAADPCMDecompressor::BreakConnect(PIN_DIRECTION dir)
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

HRESULT CCIMAADPCMDecompressor::CheckTransform(
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
	CIMAADPCMWAVEFORMAT	*pInFormat	= (CIMAADPCMWAVEFORMAT*)mtIn->Format();
	WAVEFORMATEX		*pOutFormat	= (WAVEFORMATEX*)mtOut->Format();

	// Compare the format blocks
	return (
		(pOutFormat->wFormatTag		== WAVE_FORMAT_PCM				) &&
		(pOutFormat->nChannels		== pInFormat->nChannels			) &&
		(pOutFormat->nSamplesPerSec	== pInFormat->nSamplesPerSec	) &&
		(pOutFormat->wBitsPerSample	== pInFormat->wBitsPerSample	)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CCIMAADPCMDecompressor::GetMediaType(
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

HRESULT CCIMAADPCMDecompressor::DecideBufferSize(
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

	// Set the properties: output buffer is four times larger 
	// then the input one, the buffers amount is the same and 
	// we don't care about alignment and prefix
	pProperties->cbBuffer = max(pProperties->cbBuffer, apInput.cbBuffer * 4);
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

HRESULT CCIMAADPCMDecompressor::StartStreaming(void)
{
	// We should have the format block at this time
	if (m_pFormat == NULL)
		return E_UNEXPECTED;

	// Allocate info blocks array
	m_pInfo = (CIMAADPCMINFO*)CoTaskMemAlloc(m_pFormat->nChannels * sizeof(CIMAADPCMINFO));
	if (m_pInfo == NULL)
		return E_OUTOFMEMORY;

	// Set the initial sample and index values
	CopyMemory(m_pInfo, m_pFormat->pInit, m_pFormat->nChannels * sizeof(CIMAADPCMINFO));

	return NOERROR;
}

HRESULT CCIMAADPCMDecompressor::StopStreaming(void)
{
	// Free the info blocks array
	if (m_pInfo) {
		CoTaskMemFree(m_pInfo);
		m_pInfo = NULL;
	}

	return NOERROR;
}

STDMETHODIMP CCIMAADPCMDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_CIMAADPCMDecompressorPage;

	return NOERROR;
}

//==========================================================================
// CCIMAADPCMDecompressorPage methods
//==========================================================================

const WCHAR g_wszCIMAADPCMDecompressorPageName[] = L"ANX IMA ADPCM Decompressor Property Page";

CCIMAADPCMDecompressorPage::CCIMAADPCMDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("Continuous IMA ADPCM Decompressor Property Page"),
		pUnk,
		IDD_CIMAADPCMDECOMPRESSORPAGE,
		IDS_TITLE_CIMAADPCMDECOMPRESSORPAGE
	)
{
}

CUnknown* WINAPI CCIMAADPCMDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CCIMAADPCMDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
