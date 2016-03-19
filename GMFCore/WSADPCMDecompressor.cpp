//==========================================================================
//
// File: WSADPCMDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of WS ADPCM decompressor
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

#include "WSADPCMDecompressor.h"
#include "resource.h"

//==========================================================================
// WS ADPCM decompression tables
//==========================================================================

const CHAR CWSADPCMDecompressor::g_chWSTable2bit[]=
{ -2, -1, 0, 1 };

const CHAR CWSADPCMDecompressor::g_chWSTable4bit[]=
{ -9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8 };

//==========================================================================
// WS ADPCM decompressor setup data
//==========================================================================

// Global filter name
const WCHAR g_wszWSADPCMDecompressorName[] = L"ANX WS ADPCM Decompressor";

const AMOVIESETUP_MEDIATYPE sudWSADPCMType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_WSADPCM
};

const AMOVIESETUP_MEDIATYPE sudPCMType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

const AMOVIESETUP_PIN sudWSADPCMDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudWSADPCMType		// Media types
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

const AMOVIESETUP_FILTER g_sudWSADPCMDecompressor = {
	&CLSID_WSADPCMDecompressor,	// CLSID of filter
	g_wszWSADPCMDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudWSADPCMDecompressorPins		// Pin information
};

//==========================================================================
// CWSADPCMDecompressor methods
//==========================================================================

CWSADPCMDecompressor::CWSADPCMDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("WS ADPCM Decompressor"),
		pUnk,
		CLSID_WSADPCMDecompressor
	),
	m_pFormat(NULL)	// No format block at this time
{
	ASSERT(phr);
}

CWSADPCMDecompressor::~CWSADPCMDecompressor()
{
	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
	}
}

void CWSADPCMDecompressor::Clip8BitSample(SHORT *piSample)
{
	if (*piSample > 0xFF)	*piSample = 0xFF;
	if (*piSample < 0)		*piSample = 0;
}

void CWSADPCMDecompressor::Decompress(
	WORD wInSize,
	BYTE *pbInput,
	WORD wOutSize,
	BYTE *pbOutput
)
{
	BYTE	bCode;
	CHAR	chCount;
	WORD	wIn;
	SHORT	iOut;

	// Initialize sample value
	iOut = 0x80;

	// Non-compressed block
	if (wInSize == wOutSize) {
		for (; wOutSize > 0; wOutSize--)
			*pbOutput++ = *pbInput++;
		return;
	}

	// Process data until it's exhausted
	while (wOutSize > 0) {

		wIn = *pbInput++;
		wIn <<= 2;
		bCode = HIBYTE(wIn);
		chCount = LOBYTE(wIn) >> 2;
		switch (bCode) {

			case 2:
				if (chCount & 0x20) {
					chCount <<= 3;
					iOut += chCount >> 3;
					*pbOutput++ = (BYTE)iOut;
					wOutSize--;
				} else {
					for (chCount++; chCount > 0; chCount--, wOutSize--)
						*pbOutput++ = *pbInput++;
					iOut = pbOutput[-1];
				}
				break;

			case 1:
				for (chCount++; chCount > 0; chCount--) {

					bCode = *pbInput++;

					iOut += g_chWSTable4bit[bCode & 0x0F];
					Clip8BitSample(&iOut);
					*pbOutput++ = (BYTE)iOut;

					iOut += g_chWSTable4bit[bCode >> 4];
					Clip8BitSample(&iOut);
					*pbOutput++ = (BYTE)iOut;

					wOutSize -= 2;
				}
				break;

			case 0:
				for (chCount++; chCount > 0; chCount--) {

					bCode = *pbInput++;

					iOut += g_chWSTable2bit[bCode & 0x03];
					Clip8BitSample(&iOut);
					*pbOutput++ = (BYTE)iOut;

					iOut += g_chWSTable2bit[(bCode >> 2) & 0x03];
					Clip8BitSample(&iOut);
					*pbOutput++ =(BYTE)iOut;

					iOut += g_chWSTable2bit[(bCode >> 4) & 0x03];
					Clip8BitSample(&iOut);
					*pbOutput++ = (BYTE)iOut;

					iOut += g_chWSTable2bit[(bCode >> 6) & 0x03];
					Clip8BitSample(&iOut);
					*pbOutput++ = (BYTE)iOut;

					wOutSize -= 4;
				}
				break;

			default:
				for (chCount++; chCount > 0; chCount--, wOutSize--)
					*pbOutput++ = (BYTE)iOut;
				break;
		}
	}
}

CUnknown* WINAPI CWSADPCMDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CWSADPCMDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CWSADPCMDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

HRESULT CWSADPCMDecompressor::Transform(
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
	WSADPCMINFO *pInfo = (WSADPCMINFO*)pbInBuffer;
	Decompress(
		pInfo->wInSize,
		pbInBuffer + sizeof(WSADPCMINFO),
		pInfo->wOutSize,
		pbOutBuffer
	);

	// Set the data length for the output sample
	hr = pOut->SetActualDataLength(pInfo->wOutSize);
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

HRESULT CWSADPCMDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	// Check for proper audio media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Audio			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_WSADPCM	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_WSADPCM			) &&
		(((WSADPCMWAVEFORMAT*)mtIn->Format())->nChannels		== 1) &&
		(((WSADPCMWAVEFORMAT*)mtIn->Format())->wBitsPerSample	== 8)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CWSADPCMDecompressor::SetMediaType(
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
		m_pFormat = (WSADPCMWAVEFORMAT*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());
	}

	return NOERROR;
}

HRESULT CWSADPCMDecompressor::BreakConnect(PIN_DIRECTION dir)
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

HRESULT CWSADPCMDecompressor::CheckTransform(
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
	WSADPCMWAVEFORMAT	*pInFormat	= (WSADPCMWAVEFORMAT*)mtIn->Format();
	WAVEFORMATEX		*pOutFormat	= (WAVEFORMATEX*)mtOut->Format();

	// Compare the format blocks
	return (
		(pOutFormat->wFormatTag		== WAVE_FORMAT_PCM				) &&
		(pOutFormat->nChannels		== pInFormat->nChannels			) &&
		(pOutFormat->nSamplesPerSec	== pInFormat->nSamplesPerSec	) &&
		(pOutFormat->wBitsPerSample	== pInFormat->wBitsPerSample	)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CWSADPCMDecompressor::GetMediaType(
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

HRESULT CWSADPCMDecompressor::DecideBufferSize(
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
	pProperties->cbBuffer = max(pProperties->cbBuffer, apInput.cbBuffer * 64);
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

STDMETHODIMP CWSADPCMDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_WSADPCMDecompressorPage;

	return NOERROR;
}

//==========================================================================
// CWSADPCMDecompressorPage methods
//==========================================================================

const WCHAR g_wszWSADPCMDecompressorPageName[] = L"ANX WS ADPCM Decompressor Property Page";

CWSADPCMDecompressorPage::CWSADPCMDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("WS ADPCM Decompressor Property Page"),
		pUnk,
		IDD_WSADPCMDECOMPRESSORPAGE,
		IDS_TITLE_WSADPCMDECOMPRESSORPAGE
	)
{
}

CUnknown* WINAPI CWSADPCMDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CWSADPCMDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
