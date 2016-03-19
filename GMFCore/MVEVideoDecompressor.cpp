//==========================================================================
//
// File: MVEVideoDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of MVE video decompressor
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

#include "MVEGUID.h"
#include "MVEVideoDecompressor.h"
#include "resource.h"

#include <cguid.h>

//==========================================================================
// MVE video decompressor setup data
//==========================================================================

const WCHAR g_wszMVEVideoDecompressorName[] = L"ANX MVE Video Decompressor";

const AMOVIESETUP_MEDIATYPE sudMVEVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_MVEVideo
};

const AMOVIESETUP_MEDIATYPE sudOutputTypes[] = {
	{	// 8-bit palettized RGB
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB8
	},
	{	// 15-bit RGB (555)
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB555
	}
};

const AMOVIESETUP_PIN sudMVEVideoDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudMVEVideoType	// Media types
	},
	{	// Output pin
		L"Output",			// Pin name
		FALSE,				// Is it rendered
		TRUE,				// Is it an output
		TRUE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Input",			// Connects to pin
		2,					// Number of types
		sudOutputTypes		// Media types
	}
};

const AMOVIESETUP_FILTER g_sudMVEVideoDecompressor = {
	&CLSID_MVEVideoDecompressor,	// CLSID of filter
	g_wszMVEVideoDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudMVEVideoDecompressorPins		// Pin information
};

//==========================================================================
// CMVEVideoDecompressor methods
//==========================================================================

CMVEVideoDecompressor::CMVEVideoDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("MVE Video Decompressor"),
		pUnk,
		CLSID_MVEVideoDecompressor
	),
	m_pVideoMap(NULL),			// No video decoding map at this time
	m_cbVideoMap(0),			// ----||----
	m_pCurrentFrame(NULL),		// No frame buffers at this time
	m_pPreviousFrame(NULL),		// ----||----
	m_iPaletteStart(0),			// No palette at this time
	m_nPaletteEntries(0),		// ----||----
	m_pFormat(NULL),			// No format block at this time
	m_cbFormat(0),				// ----||----
	m_dwVideoWidth(0),			// ----||----
	m_dwVideoHeight(0),			// ----||----
	m_bVideoModeChanged(FALSE)	// ----||----
{
	ASSERT(phr);
}

CMVEVideoDecompressor::~CMVEVideoDecompressor()
{
	// Clean-up the decoder
	FreeVideoBuffers();

	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
		m_cbFormat = 0;
	}
}

CUnknown* WINAPI CMVEVideoDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CMVEVideoDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CMVEVideoDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

#define CHECKDATASIZE(n, s) if ((n) > (s)) return E_UNEXPECTED;

HRESULT CMVEVideoDecompressor::Transform(
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

	// Set up input data length
	LONG lInDataLength = pIn->GetActualDataLength();

	// Get the chunk type and subtype
	CHECKDATASIZE(2, lInDataLength);
	BYTE bType = *pbInBuffer++;
	BYTE bSubtype = *pbInBuffer++;
	lInDataLength -= 2;

	// Set up various chunk header pointers
	MVE_PALETTE_GRADIENT *pPalGradient = NULL;
	MVE_PALETTE_HEADER *pPalHeader = NULL;
	MVE_VIDEO_INFO *pVideoInfo = NULL;
	MVE_VIDEO_CMD *pVideoCmd = NULL;
	MVE_VIDEO_HEADER *pVideoHeader = NULL;

	BYTE *pbOutBuffer = NULL;
	LONG lOutDataLength = 0;
	WORD i;

	// Gradient palette stuff
	int curScaleR, curScaleGB;	/* current scale for R and GB component */
	int curBase;				/* base for current gradient block */
	int curIndexR, curIndexGB;	/* index within R and GB component of current gradient */

	// Parse chunk type
	switch (bType) {

		// Gradient palette
		case MVE_SUBCHUNK_PALETTE_GRAD:

			// Get palette gradient descriptor
			CHECKDATASIZE(sizeof(MVE_PALETTE_GRADIENT), lInDataLength);
			pPalGradient = (MVE_PALETTE_GRADIENT*)pbInBuffer;

			// TEMP: Experimental! No samples to test on...

			// Create R x B gradient
			curScaleR = 0;
			curBase = pPalGradient->bBaseRB;
			for (curIndexR=pPalGradient->nR_RB; curIndexR>0; --curIndexR)
			{
				/* components to set in palette entry */
				unsigned char r, g, b;

				curScaleGB = 0;
				r = curScaleR / (pPalGradient->nR_RB - 1);
				g = 0;

				for (curIndexGB = 0; curIndexGB < pPalGradient->nB_RB; curIndexGB++)
				{
					b = curScaleGB / (pPalGradient->nB_RB - 1) * 5 / 8;
					curScaleGB += 0x3f;

					m_Palette[curBase + curIndexGB].bRed = r << 2;
					m_Palette[curBase + curIndexGB].bGreen = g << 2;
					m_Palette[curBase + curIndexGB].bBlue = b << 2;
				}

				curScaleR += 0x3f;
				curBase += pPalGradient->nB_RB;
			}

			// Create R x G gradient
			curScaleR = 0;
			curBase = pPalGradient->bBaseRG;
			for (curIndexR=pPalGradient->nR_RG; curIndexR>0; --curIndexR)
			{
				/* components to set in palette entry */
				unsigned char r, g, b;

				curScaleGB = 0;
				r = curScaleR / (pPalGradient->nR_RG - 1);
				b = 0;

				for (curIndexGB = 0; curIndexGB < pPalGradient->nG_RG; curIndexGB++)
				{
					g = curScaleGB / (pPalGradient->nG_RG - 1) * 5 / 8;
					curScaleGB += 0x3f;

					m_Palette[curBase + curIndexGB].bRed = r << 2;
					m_Palette[curBase + curIndexGB].bGreen = g << 2;
					m_Palette[curBase + curIndexGB].bBlue = b << 2;
				}

				curScaleR += 0x3f;
				curBase += pPalGradient->nG_RG;
			} 

			// No need to deliver data any further
			hr = S_FALSE;

			break;

		// Normal palette
		case MVE_SUBCHUNK_PALETTE:

			// Get palette header
			CHECKDATASIZE(sizeof(MVE_PALETTE_HEADER), lInDataLength);
			pPalHeader = (MVE_PALETTE_HEADER*)pbInBuffer;
			pbInBuffer += sizeof(MVE_PALETTE_HEADER);
			lInDataLength -= sizeof(MVE_PALETTE_HEADER);

			// Sanity check of the palette range
			if (
				(pPalHeader->iStart > 255) ||
				(pPalHeader->iStart + pPalHeader->nEntries > 256)
			)
				return E_UNEXPECTED;

			// Read in palette entries
			CHECKDATASIZE(3 * pPalHeader->nEntries, lInDataLength);
			for (
				i = pPalHeader->iStart;
				i < pPalHeader->iStart + pPalHeader->nEntries;
				i++
			) {
				m_Palette[i].bRed	= (*pbInBuffer++) << 2;
				m_Palette[i].bGreen	= (*pbInBuffer++) << 2;
				m_Palette[i].bBlue	= (*pbInBuffer++) << 2;
			}
			m_iPaletteStart		= pPalHeader->iStart;
			m_nPaletteEntries	= pPalHeader->nEntries;

			// No need to deliver data any further
			hr = S_FALSE;

			break;

		// Compressed palette
		case MVE_SUBCHUNK_PALETTE_RLE:

			// TEMP: Experimental! No samples to test on...

			// Decompress palette
			for (i = 0; i < 32; i++) {

				// Read flags byte
				CHECKDATASIZE(1, lInDataLength);
				BYTE bFlags = *pbInBuffer++;
				lInDataLength--;

				// Test flags
				for (int k = 0; k < 8; k++) {

					// Read palette entry (if we have to)
					if (bFlags & 1) {
						CHECKDATASIZE(3, lInDataLength);
						m_Palette[i * 8 + k].bRed	= (*pbInBuffer++) << 2;
						m_Palette[i * 8 + k].bGreen	= (*pbInBuffer++) << 2;
						m_Palette[i * 8 + k].bBlue	= (*pbInBuffer++) << 2;
						lInDataLength -= 3;
					}

					// Advance to the next bit
					bFlags >>= 1;
				}
			}

			// No need to deliver data any further
			hr = S_FALSE;

			break;

		// Video info command
		case MVE_SUBCHUNK_VIDEOINFO:

			// TODO: Make a proper dimension change handling

			/*
			// Get video info
			CHECKDATASIZE(sizeof(MVE_VIDEO_INFO), lInDataLength);
			pVideoInfo = (MVE_VIDEO_INFO*)pbInBuffer;

			if (
				(pVideoInfo->wWidth * 8u	!= m_dwVideoWidth) ||
				(pVideoInfo->wHeight * 8u	!= m_dwVideoHeight)
			) {
				
				// Free old video buffers
				FreeVideoBuffers();

				// Set the new video parameters
				SetVideoParameters(pVideoInfo->wWidth, pVideoInfo->wHeight);

				// Allocate new video buffers
				hr = AllocateVideoBuffers();
				if (FAILED(hr))
					return hr;

				// Copy new video info to format block
				CopyMemory(m_pFormat, pVideoInfo, lInDataLength);

				m_bVideoModeChanged = TRUE;
				
			}
			*/

			// No need to deliver data any further
			hr = S_FALSE;

			break;

		// Video decoding map
		case MVE_SUBCHUNK_VIDEOMAP:

			// Copy the data to the map storage
			CHECKDATASIZE((LONG)m_cbVideoMap, lInDataLength);
			CopyMemory(m_pVideoMap, pbInBuffer, m_cbVideoMap);

			// No need to deliver data any further
			hr = S_FALSE;

			break;

		// Video data
		case MVE_SUBCHUNK_VIDEODATA:

			// Get video data header
			CHECKDATASIZE(sizeof(MVE_VIDEO_HEADER), lInDataLength);
			pVideoHeader = (MVE_VIDEO_HEADER*)pbInBuffer;
			pbInBuffer += sizeof(MVE_VIDEO_HEADER);
			lInDataLength -= sizeof(MVE_VIDEO_HEADER);

			// Swap the back buffers if we have to
			if (pVideoHeader->wFlags & 1) {
				BYTE *pTemp = m_pPreviousFrame;
				m_pPreviousFrame = m_pCurrentFrame;
				m_pCurrentFrame = pTemp;
			}

			// Call the decoder function to decompress the frame
			if (m_pFormat->wHiColor)
				DecodeFrame16(
					m_pCurrentFrame,
					m_pVideoMap,
					m_cbVideoMap,
					pbInBuffer,
					lInDataLength
				);
			else
				DecodeFrame8(
					m_pCurrentFrame,
					m_pVideoMap,
					m_cbVideoMap,
					pbInBuffer,
					lInDataLength
				);

			// No need to deliver data any further
			hr = S_FALSE;

			break;

		// Send buffer
		case MVE_SUBCHUNK_VIDEOCMD:

			// Get video command header
			CHECKDATASIZE(4, lInDataLength);
			pVideoCmd = (MVE_VIDEO_CMD*)pbInBuffer;

			// Check if we have to install some palette entries
			if (pVideoCmd->nPaletteEntries != 0) {
				m_iPaletteStart		= pVideoCmd->iPaletteStart;
				m_nPaletteEntries	= pVideoCmd->nPaletteEntries;
			}

			// Install a new palette (if we have to)
			if ((!m_pFormat->wHiColor) && (m_nPaletteEntries != 0)) {

				// Sanity check of the palette range
				if (m_iPaletteStart + m_nPaletteEntries > 256)
					return E_UNEXPECTED;

				// Get the output media type format
				CMediaType mt((AM_MEDIA_TYPE)m_pOutput->CurrentMediaType());
				VIDEOINFO *pvi = (VIDEOINFO*)mt.Format();

				// Fill in the output media type format palette
				for (
					i = m_iPaletteStart;
					i < m_iPaletteStart + m_nPaletteEntries;
					i++
				) {
					pvi->bmiColors[i].rgbRed		= m_Palette[i].bRed;
					pvi->bmiColors[i].rgbGreen		= m_Palette[i].bGreen;
					pvi->bmiColors[i].rgbBlue		= m_Palette[i].bBlue;
					pvi->bmiColors[i].rgbReserved	= 0;
				}

				// Set the changed media type for the output sample
				hr = pOut->SetMediaType(&mt);
				if (FAILED(hr))
					return hr;

				// Reset palette range
				m_iPaletteStart		= 0;
				m_nPaletteEntries	= 0;
			}

			// Get the output sample's buffer
			hr = pOut->GetPointer(&pbOutBuffer);
			if (FAILED(hr))
				return hr;

			// Copy image data to output sample
			lOutDataLength = m_dwVideoWidth * m_dwVideoHeight * ((m_pFormat->wHiColor) ? 2 : 1);
			CopyMemory(pbOutBuffer, m_pCurrentFrame, lOutDataLength);

			/*
			if (m_bVideoModeChanged) {

				// Get the output media type format
				CMediaType mt((AM_MEDIA_TYPE)m_pOutput->CurrentMediaType());
				VIDEOINFO *pvi = (VIDEOINFO*)mt.Format();

				// Set the new image dimensions
				pvi->rcSource.left		= 0;
				pvi->rcSource.top		= 0;
				pvi->rcSource.right		= m_dwVideoWidth;
				pvi->rcSource.bottom	= m_dwVideoHeight;

				// Set the changed media type for the output sample
				hr = pOut->SetMediaType(&mt);
				if (FAILED(hr))
					return hr;

				m_bVideoModeChanged = FALSE;
			}
			*/

			// Set the data length for the output sample
			hr = pOut->SetActualDataLength(lOutDataLength);
			if (FAILED(hr))
				return hr;

			// Each RGB frame is a sync point
			hr = pOut->SetSyncPoint(TRUE);
			if (FAILED(hr))
				return hr;

			// RGB sample should never be a preroll one
			hr = pOut->SetPreroll(FALSE);
			if (FAILED(hr))
				return hr;

			// This is where we deliver the video frame
			hr = S_OK;

			// We rely on the upstream filter (which is most likely 
			// a parser or splitter) in the matter of stream and media 
			// times setting. As to the discontinuity property, we should
			// not drop samples, so we just retain this property's value 
			// set by the upstream filter

			break;

		default:

			// No interest in other chunk types
			hr = S_FALSE;

			break;
	}

	return hr;
}

HRESULT CMVEVideoDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	// Check for proper video media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Video			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_MVEVideo	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_MVEVideo			)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

void CMVEVideoDecompressor::SetVideoParameters(WORD wWidth, WORD wHeight)
{
	// Set up video map size and video resolution
	m_cbVideoMap	= wWidth * wHeight / 2;
	m_dwVideoWidth	= wWidth * 8;
	m_dwVideoHeight	= wHeight * 8;
}

HRESULT CMVEVideoDecompressor::SetMediaType(
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
		if (m_pFormat) {
			CoTaskMemFree(m_pFormat);
			m_cbFormat = 0;
		}
		m_pFormat = (MVE_VIDEO_INFO*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());
		m_cbFormat = pmt->FormatLength();

		// Set video parameters
		SetVideoParameters(m_pFormat->wWidth, m_pFormat->wHeight);
	}

	return NOERROR;
}

HRESULT CMVEVideoDecompressor::BreakConnect(PIN_DIRECTION dir)
{
	// Catch only the input pin disconnection
	if (dir == PINDIR_INPUT) {

		// Free the format block
		if (m_pFormat) {
			CoTaskMemFree(m_pFormat);
			m_pFormat = NULL;
		}

		// Reset format-specific parameters
		m_cbFormat		= 0;
		m_cbVideoMap	= 0;
		m_dwVideoWidth	= 0;
		m_dwVideoHeight	= 0;
	}

	return NOERROR;
}

HRESULT CMVEVideoDecompressor::CheckTransform(
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
		!IsEqualGUID(*mtOut->Type(),		MEDIATYPE_Video		) ||
		!IsEqualGUID(*mtOut->FormatType(),	FORMAT_VideoInfo	)
	)
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Get the media types' format blocks
	MVE_VIDEO_INFO	*pInFormat	= (MVE_VIDEO_INFO*)mtIn->Format();
	VIDEOINFO		*pOutFormat	= (VIDEOINFO*)mtOut->Format();

	// Check if the format length is enough
	if (mtOut->FormatLength() < sizeof(VIDEOINFO))
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check if the media subtype matches the bit count
	if (!IsEqualGUID(*mtOut->Subtype(), (pInFormat->wHiColor) ? MEDIASUBTYPE_RGB555 : MEDIASUBTYPE_RGB8))
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check the compatibility of the formats
	WORD wBitCount = (pInFormat->wHiColor) ? 16 : 8;
	DWORD cbFrame = pInFormat->wWidth * 8 * pInFormat->wHeight * 8 * wBitCount / 8;
	return (
		//(pOutFormat->AvgTimePerFrame			== UNITS / MVE_FRAME_DELTA_DEFAULT	) &&
		(pOutFormat->bmiHeader.biWidth			== (LONG)pInFormat->wWidth * 8		) &&
		(pOutFormat->bmiHeader.biHeight			== -(LONG)pInFormat->wHeight * 8	) &&
		(pOutFormat->bmiHeader.biPlanes			== 1								) &&
		(pOutFormat->bmiHeader.biBitCount		== wBitCount						) &&
		(pOutFormat->bmiHeader.biCompression	== BI_RGB							) &&
		(pOutFormat->bmiHeader.biSizeImage		== cbFrame							)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMVEVideoDecompressor::GetMediaType(
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

		pMediaType->InitMediaType(); // Is it needed ???

		// Allocate the video info block
		VIDEOINFO *pVideoInfo = (VIDEOINFO*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFO));
		if (pVideoInfo == NULL)
			return E_OUTOFMEMORY;

		// Set up frame delta
		REFERENCE_TIME rtFrameDelta = 0;
		if (m_cbFormat >= sizeof(MVE_VIDEO_INFO) + sizeof(MVE_VIDEO_MODE_INFO) + sizeof(MVE_TIMER_DATA)) {
			MVE_TIMER_DATA *pTimerData = (MVE_TIMER_DATA*)((BYTE*)m_pFormat + sizeof(MVE_VIDEO_INFO) + sizeof(MVE_VIDEO_MODE_INFO));
			rtFrameDelta = pTimerData->dwRate * pTimerData->wSubdivision * 10;
		}
		if (rtFrameDelta == 0)
			rtFrameDelta = MVE_FRAME_DELTA_DEFAULT;

		// Prepare the video info block
		ZeroMemory(pVideoInfo, sizeof(VIDEOINFO));
		SetRectEmpty(&pVideoInfo->rcSource);
		SetRectEmpty(&pVideoInfo->rcTarget);
		pVideoInfo->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
		pVideoInfo->bmiHeader.biWidth			= (LONG)m_dwVideoWidth;
		pVideoInfo->bmiHeader.biHeight			= -(LONG)m_dwVideoHeight;
		pVideoInfo->bmiHeader.biPlanes			= 1;
		pVideoInfo->bmiHeader.biBitCount		= (m_pFormat->wHiColor) ? 16 : 8;
		pVideoInfo->bmiHeader.biCompression		= BI_RGB;
		pVideoInfo->bmiHeader.biSizeImage		= GetBitmapSize(&pVideoInfo->bmiHeader);
		pVideoInfo->bmiHeader.biXPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biYPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biClrUsed			= (m_pFormat->wHiColor) ? 0 : 256;
		pVideoInfo->bmiHeader.biClrImportant	= 0;
		pVideoInfo->dwBitRate					= (DWORD)((LONGLONG)pVideoInfo->bmiHeader.biSizeImage * 8 * UNITS / rtFrameDelta);
		pVideoInfo->dwBitErrorRate				= 0;
		pVideoInfo->AvgTimePerFrame				= rtFrameDelta;

		// Set media type fields
		pMediaType->SetType(&MEDIATYPE_Video);
		// Work out the subtype GUID from the header info
		const GUID SubTypeGUID = GetBitmapSubtype(&pVideoInfo->bmiHeader);
		pMediaType->SetSubtype(&SubTypeGUID);
		pMediaType->SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);
		pMediaType->SetTemporalCompression(FALSE);
		pMediaType->SetFormatType(&FORMAT_VideoInfo);
	}

	return S_OK;
}

HRESULT CMVEVideoDecompressor::DecideBufferSize(
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

	// At this time we should have the format block
	if (!m_pFormat)
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

	// Set the properties: output buffer's size is frame size,
	// the buffers amount is the same as for the input pin and
	// we don't care about alignment and prefix
	LONG cbFrame = m_dwVideoWidth * m_dwVideoHeight * ((m_pFormat->wHiColor) ? 2 : 1);
	pProperties->cbBuffer = max(pProperties->cbBuffer, cbFrame);
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

HRESULT CMVEVideoDecompressor::AllocateVideoBuffers(void)
{
	// We should have the format block at this time
	if (m_pFormat == NULL)
		return E_UNEXPECTED;

	// Allocate video map buffer
	m_pVideoMap = (BYTE*)CoTaskMemAlloc(m_cbVideoMap);
	if (m_pVideoMap == NULL)
		return E_OUTOFMEMORY;

	// Allocate frame buffers. Due to some odd reason decoding crashes
	// when the frame buffers have proper size. Double size works well
	DWORD cbFrame = m_dwVideoWidth * m_dwVideoHeight * ((m_pFormat->wHiColor) ? 2 : 1) * 2;
	m_pPreviousFrame = (BYTE*)CoTaskMemAlloc(cbFrame);
	if (m_pPreviousFrame == NULL) {
		FreeVideoBuffers();
		return E_OUTOFMEMORY;
	}
	m_pCurrentFrame = (BYTE*)CoTaskMemAlloc(cbFrame);
	if (m_pCurrentFrame == NULL) {
		FreeVideoBuffers();
		return E_OUTOFMEMORY;
	}

	// Zero buffers memory
	ZeroMemory(m_pVideoMap, m_cbVideoMap);
	ZeroMemory(m_pPreviousFrame, cbFrame);
	ZeroMemory(m_pCurrentFrame, cbFrame);

	return NOERROR;
}

void CMVEVideoDecompressor::FreeVideoBuffers(void)
{
	// Free the video map
	if (m_pVideoMap) {
		CoTaskMemFree(m_pVideoMap);
		m_pVideoMap = NULL;
	}

	// Free the frame buffers
	if (m_pCurrentFrame) {
		CoTaskMemFree(m_pCurrentFrame);
		m_pCurrentFrame = NULL;
	}
	if (m_pPreviousFrame) {
		CoTaskMemFree(m_pPreviousFrame);
		m_pPreviousFrame = NULL;
	}
}

HRESULT CMVEVideoDecompressor::StartStreaming(void)
{
	// Allocate video buffers
	HRESULT hr = AllocateVideoBuffers();
	if (FAILED(hr))
		return hr;

	// Reset palette stuff
	ZeroMemory(m_Palette, 3 * 256);
	m_iPaletteStart		= 0;
	m_nPaletteEntries	= 0;
	lookup_initialized	= 0;
	m_bVideoModeChanged	= FALSE;

	return NOERROR;
}

HRESULT CMVEVideoDecompressor::StopStreaming(void)
{
	// Perform decoder clean-up
	FreeVideoBuffers();

	return NOERROR;
}

STDMETHODIMP CMVEVideoDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_MVEVideoDecompressorPage;

	return NOERROR;
}

//==========================================================================
// ---- MVE decoder methods ----
// Source code taken from libmve library
// by <don't-know-whom> (<don't-know-email>)
//==========================================================================

void CMVEVideoDecompressor::DecodeFrame8(
	unsigned char *pFrame,
	unsigned char *pMap,
	int mapRemain,
	unsigned char *pData,
	int dataRemain
)
{
	int i, j;
	int xb, yb;

	xb = m_pFormat->wWidth;
	yb = m_pFormat->wHeight;
	for (j=0; j<yb; j++)
	{
		for (i=0; i<xb/2; i++)
		{
			DispatchDecoder8(&pFrame, (*pMap) & 0xf, &pData, &dataRemain, &i, &j);
			/*
			if (
				(pFrame < m_pCurrentFrame) ||
				(pFrame >= m_pCurrentFrame + m_dwVideoWidth*m_dwVideoHeight)
			)
				return;
			*/

			DispatchDecoder8(&pFrame, (*pMap) >> 4, &pData, &dataRemain, &i, &j);
			/*
			if (
				(pFrame < m_pCurrentFrame) ||
				(pFrame >= m_pCurrentFrame + m_dwVideoWidth*m_dwVideoHeight)
			)
				return;
			*/

			++pMap;
			--mapRemain;
		}

		pFrame += 7*m_dwVideoWidth;
	}
}

void CMVEVideoDecompressor::RelClose(int i, int *x, int *y)
{
	int ma, mi;

	ma = i >> 4;
	mi = i & 0xf;

	*x = mi - 8;
	*y = ma - 8;
}

void CMVEVideoDecompressor::RelFar(int i, int sign, int *x, int *y)
{
	if (i < 56)
	{
		*x = sign * (8 + (i % 7));
		*y = sign *      (i / 7);
	}
	else
	{
		*x = sign * (-14 + (i - 56) % 29);
		*y = sign *   (8 + (i - 56) / 29);
	}
}

/* copies an 8x8 block from pSrc to pDest.
   pDest and pSrc are both m_dwVideoWidth bytes wide */
void CMVEVideoDecompressor::CopyFrame8(unsigned char *pDest, unsigned char *pSrc)
{
	int i;

	for (i=0; i<8; i++)
	{
		CopyMemory(pDest, pSrc, 8);
		pDest += m_dwVideoWidth;
		pSrc += m_dwVideoWidth;
	}
}

// Fill in the next eight bytes with p[0], p[1], p[2], or p[3],
// depending on the corresponding two-bit value in pat0 and pat1
void CMVEVideoDecompressor::PatternRow4Pixels8(
	unsigned char *pFrame,
	unsigned char pat0,
	unsigned char pat1,
	unsigned char *p
)
{
	unsigned short mask=0x0003;
	unsigned short shift=0;
	unsigned short pattern = (pat1 << 8) | pat0;

	while (mask != 0)
	{
		*pFrame++ = p[(mask & pattern) >> shift];
		mask <<= 2;
		shift += 2;
	}
}

// Fill in the next four 2x2 pixel blocks with p[0], p[1], p[2], or p[3],
// depending on the corresponding two-bit value in pat0.
void CMVEVideoDecompressor::PatternRow4Pixels2_8(
	unsigned char *pFrame,
	unsigned char pat0,
	unsigned char *p
)
{
	unsigned char mask=0x03;
	unsigned char shift=0;
	unsigned char pel;

	while (mask != 0)
	{
		pel = p[(mask & pat0) >> shift];
		pFrame[0] = pel;
		pFrame[1] = pel;
		pFrame[m_dwVideoWidth + 0] = pel;
		pFrame[m_dwVideoWidth + 1] = pel;
		pFrame += 2;
		mask <<= 2;
		shift += 2;
	}
}

// Fill in the next four 2x1 pixel blocks with p[0], p[1], p[2], or p[3],
// depending on the corresponding two-bit value in pat.
void CMVEVideoDecompressor::PatternRow4Pixels2x1_8(
	unsigned char *pFrame,
	unsigned char pat,
	unsigned char *p
)
{
	unsigned char mask=0x03;
	unsigned char shift=0;
	unsigned char pel;

	while (mask != 0)
	{
		pel = p[(mask & pat) >> shift];
		pFrame[0] = pel;
		pFrame[1] = pel;
		pFrame += 2;
		mask <<= 2;
		shift += 2;
	}
}

// Fill in the next 4x4 pixel block with p[0], p[1], p[2], or p[3],
// depending on the corresponding two-bit value in pat0, pat1, pat2, and pat3.
void CMVEVideoDecompressor::PatternQuadrant4Pixels8(
	unsigned char *pFrame,
	unsigned char pat0,
	unsigned char pat1,
	unsigned char pat2,
	unsigned char pat3,
	unsigned char *p
)
{
	unsigned long mask = 0x00000003UL;
	int shift=0;
	int i;
	unsigned long pat = (pat3 << 24) | (pat2 << 16) | (pat1 << 8) | pat0;

	for (i=0; i<16; i++)
	{
		pFrame[i&3] = p[(pat & mask) >> shift];

		if ((i&3) == 3)
			pFrame += m_dwVideoWidth;

		mask <<= 2;
		shift += 2;
	}
}

// fills the next 8 pixels with either p[0] or p[1], depending on pattern
void CMVEVideoDecompressor::PatternRow2Pixels8(
	unsigned char *pFrame,
	unsigned char pat,
	unsigned char *p
)
{
	unsigned char mask=0x01;

	while (mask != 0)
	{
		*pFrame++ = p[(mask & pat) ? 1 : 0];
		mask <<= 1;
	}
}

// fills the next four 2 x 2 pixel boxes with either p[0] or p[1], depending on pattern
void CMVEVideoDecompressor::PatternRow2Pixels2_8(
	unsigned char *pFrame,
	unsigned char pat,
	unsigned char *p
)
{
	unsigned char pel;
	unsigned char mask=0x1;

	while (mask != 0x10)
	{
		pel = p[(mask & pat) ? 1 : 0];

		pFrame[0] = pel;              // upper-left
		pFrame[1] = pel;              // upper-right
		pFrame[m_dwVideoWidth + 0] = pel;    // lower-left
		pFrame[m_dwVideoWidth + 1] = pel;    // lower-right
		pFrame += 2;

		mask <<= 1;
	}
}

// fills pixels in the next 4 x 4 pixel boxes with either p[0] or p[1], depending on pat0 and pat1.
void CMVEVideoDecompressor::PatternQuadrant2Pixels8(
	unsigned char *pFrame,
	unsigned char pat0,
	unsigned char pat1,
	unsigned char *p
)
{
	unsigned char pel;
	unsigned short mask = 0x0001;
	int i, j;
	unsigned short pat = (pat1 << 8) | pat0;

	for (i=0; i<4; i++)
	{
		for (j=0; j<4; j++)
		{
			pel = p[(pat & mask) ? 1 : 0];

			pFrame[j + i * m_dwVideoWidth] = pel;

			mask <<= 1;
		}
	}
}

void CMVEVideoDecompressor::DispatchDecoder8(
	unsigned char **pFrame,
	unsigned char codeType,
	unsigned char **pData,
	int *pDataRemain,
	int *curXb,
	int *curYb
)
{
	unsigned char p[4];
	unsigned char pat[16];
	int i, j, k;
	int x, y;

	/* Data is processed in 8x8 pixel blocks.
	   There are 16 ways to encode each block.
	*/

	switch(codeType)
	{
	case 0x0:
		/* block is copied from block in current frame */
		CopyFrame8(*pFrame, m_pPreviousFrame + (*pFrame - m_pCurrentFrame));
	case 0x1:
		/* block is unchanged from two frames ago */
		*pFrame += 8;
		break;

	case 0x2:
		/* Block is copied from nearby (below and/or to the right) within the
		   new frame.  The offset within the buffer from which to grab the
		   patch of 8 pixels is given by grabbing a byte B from the data
		   stream, which is broken into a positive x and y offset according
		   to the following mapping:

		   if B < 56:
		   x = 8 + (B % 7)
		   y = B / 7
		   else
		   x = -14 + ((B - 56) % 29)
		   y =   8 + ((B - 56) / 29)
		*/
		RelFar(*(*pData)++, 1, &x, &y);
		CopyFrame8(*pFrame, *pFrame + x + y*m_dwVideoWidth);
		*pFrame += 8;
		--*pDataRemain;
		break;

	case 0x3:
		/* Block is copied from nearby (above and/or to the left) within the
		   new frame.

		   if B < 56:
		   x = -(8 + (B % 7))
		   y = -(B / 7)
		   else
		   x = -(-14 + ((B - 56) % 29))
		   y = -(  8 + ((B - 56) / 29))
		*/
		RelFar(*(*pData)++, -1, &x, &y);
		CopyFrame8(*pFrame, *pFrame + x + y*m_dwVideoWidth);
		*pFrame += 8;
		--*pDataRemain;
		break;

	case 0x4:
		/* Similar to 0x2 and 0x3, except this method copies from the
		   "current" frame, rather than the "new" frame, and instead of the
		   lopsided mapping they use, this one uses one which is symmetric
		   and centered around the top-left corner of the block.  This uses
		   only 1 byte still, though, so the range is decreased, since we
		   have to encode all directions in a single byte.  The byte we pull
		   from the data stream, I'll call B.  Call the highest 4 bits of B
		   BH and the lowest 4 bytes BL.  Then the offset from which to copy
		   the data is:

		   x = -8 + BL
		   y = -8 + BH
		*/
		RelClose(*(*pData)++, &x, &y);
		CopyFrame8(*pFrame, m_pPreviousFrame + (*pFrame - m_pCurrentFrame) + x + y*m_dwVideoWidth);
		*pFrame += 8;
		--*pDataRemain;
		break;

	case 0x5:
		/* Similar to 0x4, but instead of one byte for the offset, this uses
		   two bytes to encode a larger range, the first being the x offset
		   as a signed 8-bit value, and the second being the y offset as a
		   signed 8-bit value.
		*/
		x = (signed char)*(*pData)++;
		y = (signed char)*(*pData)++;
		CopyFrame8(*pFrame, m_pPreviousFrame + (*pFrame - m_pCurrentFrame) + x + y*m_dwVideoWidth);
		*pFrame += 8;
		*pDataRemain -= 2;
		break;

	case 0x6:
		/* I can't figure out how any file containing a block of this type
		   could still be playable, since it appears that it would leave the
		   internal bookkeeping in an inconsistent state in the BG player
		   code.  Ahh, well.  Perhaps it was a bug in the BG player code that
		   just didn't happen to be exposed by any of the included movies.
		   Anyway, this skips the next two blocks, doing nothing to them.
		   Note that if you've reached the end of a row, this means going on
		   to the next row.
		*/
		for (i=0; i<2; i++)
		{
			*pFrame += 16;
			if (++*curXb == m_pFormat->wWidth)
			{
				*pFrame += 7*m_dwVideoWidth;
				*curXb = 0;
				if (++*curYb == m_pFormat->wHeight)
					return;
			}
		}
		break;

	case 0x7:
		/* Ok, here's where it starts to get really...interesting.  This is,
		   incidentally, the part where they started using self-modifying
		   code.  So, most of the following encodings are "patterned" blocks,
		   where we are given a number of pixel values and then bitmapped
		   values to specify which pixel values belong to which squares.  For
		   this encoding, we are given the following in the data stream:

		   P0 P1

		   These are pixel values (i.e. 8-bit indices into the palette).  If
		   P0 <= P1, we then get 8 more bytes from the data stream, one for
		   each row in the block:

		   B0 B1 B2 B3 B4 B5 B6 B7

		   For each row, the leftmost pixel is represented by the low-order
		   bit, and the rightmost by the high-order bit.  Use your imagination
		   in between.  If a bit is set, the pixel value is P1 and if it is
		   unset, the pixel value is P0.

		   So, for example, if we had:

		   11 22 fe 83 83 83 83 83 83 fe

		   This would represent the following layout:

		   11 22 22 22 22 22 22 22     ; fe == 11111110
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   22 22 11 11 11 11 11 22     ; 83 == 10000011
		   11 22 22 22 22 22 22 22     ; fe == 11111110

		   If, on the other hand, P0 > P1, we get two more bytes from the
		   data stream:

		   B0 B1

		   Each of these bytes contains two 4-bit patterns. These patterns
		   work like the patterns above with 8 bytes, except each bit
		   represents a 2x2 pixel region.

		   B0 contains the pattern for the top two rows and B1 contains
		   the pattern for the bottom two rows.  Note that the low-order
		   nibble of each byte contains the pattern for the upper of the
		   two rows that that byte controls.

		   So if we had:

		   22 11 7e 83

		   The output would be:

		   11 11 22 22 22 22 22 22     ; e == 1 1 1 0
		   11 11 22 22 22 22 22 22     ;
		   22 22 22 22 22 22 11 11     ; 7 == 0 1 1 1
		   22 22 22 22 22 22 11 11     ;
		   11 11 11 11 11 11 22 22     ; 3 == 1 0 0 0
		   11 11 11 11 11 11 22 22     ;
		   22 22 22 22 11 11 11 11     ; 8 == 0 0 1 1
		   22 22 22 22 11 11 11 11     ;
		*/
		p[0] = *(*pData)++;
		p[1] = *(*pData)++;
		if (p[0] <= p[1])
		{
			for (i=0; i<8; i++)
			{
				PatternRow2Pixels8(*pFrame, *(*pData)++, p);
				*pFrame += m_dwVideoWidth;
			}
		}
		else
		{
			for (i=0; i<2; i++)
			{
				PatternRow2Pixels2_8(*pFrame, *(*pData) & 0xf, p);
				*pFrame += 2*m_dwVideoWidth;
				PatternRow2Pixels2_8(*pFrame, *(*pData)++ >> 4, p);
				*pFrame += 2*m_dwVideoWidth;
			}
		}
		*pFrame -= (8*m_dwVideoWidth - 8);
		break;

	case 0x8:
		/* Ok, this one is basically like encoding 0x7, only more
		   complicated.  Again, we start out by getting two bytes on the data
		   stream:

		   P0 P1

		   if P0 <= P1 then we get the following from the data stream:

		   B0 B1
		   P2 P3 B2 B3
		   P4 P5 B4 B5
		   P6 P7 B6 B7

		   P0 P1 and B0 B1 are used for the top-left corner, P2 P3 B2 B3 for
		   the bottom-left corner, P4 P5 B4 B5 for the top-right, P6 P7 B6 B7
		   for the bottom-right.  (So, each codes for a 4x4 pixel array.)
		   Since we have 16 bits in B0 B1, there is one bit for each pixel in
		   the array.  The convention for the bit-mapping is, again, left to
		   right and top to bottom.

		   So, basically, the top-left quarter of the block is an arbitrary
		   pattern with 2 pixels, the bottom-left a different arbitrary
		   pattern with 2 different pixels, and so on.

		   For example if the next 16 bytes were:

		   00 22 f9 9f  44 55 aa 55  11 33 cc 33  66 77 01 ef

		   We'd draw:

		   22 22 22 22 | 11 11 33 33     ; f = 1111, c = 1100
		   22 00 00 22 | 11 11 33 33     ; 9 = 1001, c = 1100
		   22 00 00 22 | 33 33 11 11     ; 9 = 1001, 3 = 0011
		   22 22 22 22 | 33 33 11 11     ; f = 1111, 3 = 0011
		   ------------+------------
		   44 55 44 55 | 66 66 66 66     ; a = 1010, 0 = 0000
		   44 55 44 55 | 77 66 66 66     ; a = 1010, 1 = 0001
		   55 44 55 44 | 66 77 77 77     ; 5 = 0101, e = 1110
		   55 44 55 44 | 77 77 77 77     ; 5 = 0101, f = 1111

		   I've added a dividing line in the above to clearly delineate the
		   quadrants.


		   Now, if P0 > P1 then we get 10 more bytes from the data stream:

		   B0 B1 B2 B3 P2 P3 B4 B5 B6 B7

		   Now, if P2 <= P3, then the first six bytes [P0 P1 B0 B1 B2 B3]
		   represent the left half of the block and the latter six bytes
		   [P2 P3 B4 B5 B6 B7] represent the right half.

		   For example:

		   22 00 01 37 f7 31   11 66 8c e6 73 31

		   yeilds:

		   22 22 22 22 | 11 11 11 66     ; 0: 0000 | 8: 1000
		   00 22 22 22 | 11 11 66 66     ; 1: 0001 | C: 1100
		   00 00 22 22 | 11 66 66 66     ; 3: 0011 | e: 1110
		   00 00 00 22 | 11 66 11 66     ; 7: 0111 | 6: 0101
		   00 00 00 00 | 66 66 66 11     ; f: 1111 | 7: 0111
		   00 00 00 22 | 66 66 11 11     ; 7: 0111 | 3: 0011
		   00 00 22 22 | 66 66 11 11     ; 3: 0011 | 3: 0011
		   00 22 22 22 | 66 11 11 11     ; 1: 0001 | 1: 0001


		   On the other hand, if P0 > P1 and P2 > P3, then
		   [P0 P1 B0 B1 B2 B3] represent the top half of the
		   block and [P2 P3 B4 B5 B6 B7] represent the bottom half.

		   For example:

		   22 00 cc 66 33 19   66 11 18 24 42 81

		   yeilds:

		   22 22 00 00 22 22 00 00     ; cc: 11001100
		   22 00 00 22 22 00 00 22     ; 66: 01100110
		   00 00 22 22 00 00 22 22     ; 33: 00110011
		   00 22 22 00 00 22 22 22     ; 19: 00011001
		   -----------------------
		   66 66 66 11 11 66 66 66     ; 18: 00011000
		   66 66 11 66 66 11 66 66     ; 24: 00100100
		   66 11 66 66 66 66 11 66     ; 42: 01000010
		   11 66 66 66 66 66 66 11     ; 81: 10000001
		*/
		if ( (*pData)[0] <= (*pData)[1])
		{
			// four quadrant case
			for (i=0; i<4; i++)
			{
				p[0] = *(*pData)++;
				p[1] = *(*pData)++;
				pat[0] = *(*pData)++;
				pat[1] = *(*pData)++;
				PatternQuadrant2Pixels8(*pFrame, pat[0], pat[1], p);

				// alternate between moving down and moving up and right
				if (i & 1)
					*pFrame += 4 - 4*m_dwVideoWidth; // up and right
				else
					*pFrame += 4*m_dwVideoWidth;     // down
			}
		}
		else if ( (*pData)[6] <= (*pData)[7])
		{
			// split horizontal
			for (i=0; i<4; i++)
			{
				if ((i & 1) == 0)
				{
					p[0] = *(*pData)++;
					p[1] = *(*pData)++;
				}
				pat[0] = *(*pData)++;
				pat[1] = *(*pData)++;
				PatternQuadrant2Pixels8(*pFrame, pat[0], pat[1], p);

				if (i & 1)
					*pFrame -= (4*m_dwVideoWidth - 4);
				else
					*pFrame += 4*m_dwVideoWidth;
			}
		}
		else
		{
			// split vertical
			for (i=0; i<8; i++)
			{
				if ((i & 3) == 0)
				{
					p[0] = *(*pData)++;
					p[1] = *(*pData)++;
				}
				PatternRow2Pixels8(*pFrame, *(*pData)++, p);
				*pFrame += m_dwVideoWidth;
			}
			*pFrame -= (8*m_dwVideoWidth - 8);
		}
		break;

	case 0x9:
		/* Similar to the previous 2 encodings, only more complicated.  And
		   it will get worse before it gets better.  No longer are we dealing
		   with patterns over two pixel values.  Now we are dealing with
		   patterns over 4 pixel values with 2 bits assigned to each pixel
		   (or block of pixels).

		   So, first on the data stream are our 4 pixel values:

		   P0 P1 P2 P3

		   Now, if P0 <= P1  AND  P2 <= P3, we get 16 bytes of pattern, each
		   2 bits representing a 1x1 pixel (00=P0, 01=P1, 10=P2, 11=P3).  The
		   ordering is again left to right and top to bottom.  The most
		   significant bits represent the left side at the top, and so on.

		   If P0 <= P1  AND  P2 > P3, we get 4 bytes of pattern, each 2 bits
		   representing a 2x2 pixel.  Ordering is left to right and top to
		   bottom.

		   if P0 > P1  AND  P2 <= P3, we get 8 bytes of pattern, each 2 bits
		   representing a 2x1 pixel (i.e. 2 pixels wide, and 1 high).

		   if P0 > P1  AND  P2 > P3, we get 8 bytes of pattern, each 2 bits
		   representing a 1x2 pixel (i.e. 1 pixel wide, and 2 high).
		*/
		if ( (*pData)[0] <= (*pData)[1])
		{
			if ( (*pData)[2] <= (*pData)[3])
			{
				p[0] = *(*pData)++;
				p[1] = *(*pData)++;
				p[2] = *(*pData)++;
				p[3] = *(*pData)++;

				for (i=0; i<8; i++)
				{
					pat[0] = *(*pData)++;
					pat[1] = *(*pData)++;
					PatternRow4Pixels8(*pFrame, pat[0], pat[1], p);
					*pFrame += m_dwVideoWidth;
				}

				*pFrame -= (8*m_dwVideoWidth - 8);
			}
			else
			{
				p[0] = *(*pData)++;
				p[1] = *(*pData)++;
				p[2] = *(*pData)++;
				p[3] = *(*pData)++;

				PatternRow4Pixels2_8(*pFrame, *(*pData)++, p);
				*pFrame += 2*m_dwVideoWidth;
				PatternRow4Pixels2_8(*pFrame, *(*pData)++, p);
				*pFrame += 2*m_dwVideoWidth;
				PatternRow4Pixels2_8(*pFrame, *(*pData)++, p);
				*pFrame += 2*m_dwVideoWidth;
				PatternRow4Pixels2_8(*pFrame, *(*pData)++, p);
				*pFrame -= (6*m_dwVideoWidth - 8);
			}
		}
		else
		{
			if ( (*pData)[2] <= (*pData)[3])
			{
				// draw 2x1 strips
				p[0] = *(*pData)++;
				p[1] = *(*pData)++;
				p[2] = *(*pData)++;
				p[3] = *(*pData)++;

				for (i=0; i<8; i++)
				{
					pat[0] = *(*pData)++;
					PatternRow4Pixels2x1_8(*pFrame, pat[0], p);
					*pFrame += m_dwVideoWidth;
				}

				*pFrame -= (8*m_dwVideoWidth - 8);
			}
			else
			{
				// draw 1x2 strips
				p[0] = *(*pData)++;
				p[1] = *(*pData)++;
				p[2] = *(*pData)++;
				p[3] = *(*pData)++;

				for (i=0; i<4; i++)
				{
					pat[0] = *(*pData)++;
					pat[1] = *(*pData)++;
					PatternRow4Pixels8(*pFrame, pat[0], pat[1], p);
					*pFrame += m_dwVideoWidth;
					PatternRow4Pixels8(*pFrame, pat[0], pat[1], p);
					*pFrame += m_dwVideoWidth;
				}

				*pFrame -= (8*m_dwVideoWidth - 8);
			}
		}
		break;

	case 0xa:
		/* Similar to the previous, only a little more complicated.

		We are still dealing with patterns over 4 pixel values with 2 bits
		assigned to each pixel (or block of pixels).

		So, first on the data stream are our 4 pixel values:

		P0 P1 P2 P3

		Now, if P0 <= P1, the block is divided into 4 quadrants, ordered
		(as with opcode 0x8) TL, BL, TR, BR.  In this case the next data
		in the data stream should be:

		B0  B1  B2  B3
		P4  P5  P6  P7  B4  B5  B6  B7
		P8  P9  P10 P11 B8  B9  B10 B11
		P12 P13 P14 P15 B12 B13 B14 B15

		Each 2 bits represent a 1x1 pixel (00=P0, 01=P1, 10=P2, 11=P3).
		The ordering is again left to right and top to bottom.  The most
		significant bits represent the right side at the top, and so on.

		If P0 > P1 then the next data on the data stream is:

		B0 B1 B2  B3  B4  B5  B6  B7
		P4 P5 P6 P7 B8 B9 B10 B11 B12 B13 B14 B15

		Now, in this case, if P4 <= P5,
		[P0 P1 P2 P3 B0 B1 B2 B3 B4 B5 B6 B7] represent the left half of
		the block and the other bytes represent the right half.  If P4 >
		P5, then [P0 P1 P2 P3 B0 B1 B2 B3 B4 B5 B6 B7] represent the top
		half of the block and the other bytes represent the bottom half.
		*/
		if ( (*pData)[0] <= (*pData)[1])
		{
			for (i=0; i<4; i++)
			{
				p[0] = *(*pData)++;
				p[1] = *(*pData)++;
				p[2] = *(*pData)++;
				p[3] = *(*pData)++;
				pat[0] = *(*pData)++;
				pat[1] = *(*pData)++;
				pat[2] = *(*pData)++;
				pat[3] = *(*pData)++;

				PatternQuadrant4Pixels8(*pFrame, pat[0], pat[1], pat[2], pat[3], p);

				if (i & 1)
					*pFrame -= (4*m_dwVideoWidth - 4);
				else
					*pFrame += 4*m_dwVideoWidth;
			}
		}
		else
		{
			if ( (*pData)[12] <= (*pData)[13])
			{
				// split vertical
				for (i=0; i<4; i++)
				{
					if ((i&1) == 0)
					{
						p[0] = *(*pData)++;
						p[1] = *(*pData)++;
						p[2] = *(*pData)++;
						p[3] = *(*pData)++;
					}

					pat[0] = *(*pData)++;
					pat[1] = *(*pData)++;
					pat[2] = *(*pData)++;
					pat[3] = *(*pData)++;

					PatternQuadrant4Pixels8(*pFrame, pat[0], pat[1], pat[2], pat[3], p);

					if (i & 1)
						*pFrame -= (4*m_dwVideoWidth - 4);
					else
						*pFrame += 4*m_dwVideoWidth;
				}
			}
			else
			{
				// split horizontal
				for (i=0; i<8; i++)
				{
					if ((i&3) == 0)
					{
						p[0] = *(*pData)++;
						p[1] = *(*pData)++;
						p[2] = *(*pData)++;
						p[3] = *(*pData)++;
					}

					pat[0] = *(*pData)++;
					pat[1] = *(*pData)++;
					PatternRow4Pixels8(*pFrame, pat[0], pat[1], p);
					*pFrame += m_dwVideoWidth;
				}

				*pFrame -= (8*m_dwVideoWidth - 8);
			}
		}
		break;

	case 0xb:
		/* In this encoding we get raw pixel data in the data stream -- 64
		   bytes of pixel data.  1 byte for each pixel, and in the standard
		   order (l->r, t->b).
		*/
		for (i=0; i<8; i++)
		{
			CopyMemory(*pFrame, *pData, 8);
			*pFrame += m_dwVideoWidth;
			*pData += 8;
			*pDataRemain -= 8;
		}
		*pFrame -= (8*m_dwVideoWidth - 8);
		break;

	case 0xc:
		/* In this encoding we get raw pixel data in the data stream -- 16
		   bytes of pixel data.  1 byte for each block of 2x2 pixels, and in
		   the standard order (l->r, t->b).
		*/
		for (i=0; i<4; i++)
		{
			for (j=0; j<2; j++)
			{
				for (k=0; k<4; k++)
				{
					(*pFrame)[2*k]   = (*pData)[k];
					(*pFrame)[2*k+1] = (*pData)[k];
				}
				*pFrame += m_dwVideoWidth;
			}
			*pData += 4;
			*pDataRemain -= 4;
		}
		*pFrame -= (8*m_dwVideoWidth - 8);
		break;

	case 0xd:
		/* In this encoding we get raw pixel data in the data stream -- 4
		   bytes of pixel data.  1 byte for each block of 4x4 pixels, and in
		   the standard order (l->r, t->b).
		*/
		for (i=0; i<2; i++)
		{
			for (j=0; j<4; j++)
			{
				for (k=0; k<4; k++)
				{
					(*pFrame)[k*m_dwVideoWidth+j] = (*pData)[0];
					(*pFrame)[k*m_dwVideoWidth+j+4] = (*pData)[1];
				}
			}
			*pFrame += 4*m_dwVideoWidth;
			*pData += 2;
			*pDataRemain -= 2;
		}
		*pFrame -= (8*m_dwVideoWidth - 8);
		break;

	case 0xe:
		/* This encoding represents a solid 8x8 frame.  We get 1 byte of pixel
		   data from the data stream.
		*/
		for (i=0; i<8; i++)
		{
			memset(*pFrame, **pData, 8);
			*pFrame += m_dwVideoWidth;
		}
		++*pData;
		--*pDataRemain;
		*pFrame -= (8*m_dwVideoWidth - 8);
		break;

	case 0xf:
		/* This encoding represents a "dithered" frame, which is
		   checkerboarded with alternate pixels of two colors.  We get 2
		   bytes of pixel data from the data stream, and these bytes are
		   alternated:

		   P0 P1 P0 P1 P0 P1 P0 P1
		   P1 P0 P1 P0 P1 P0 P1 P0
		   ...
		   P0 P1 P0 P1 P0 P1 P0 P1
		   P1 P0 P1 P0 P1 P0 P1 P0
		*/
		for (i=0; i<8; i++)
		{
			for (j=0; j<8; j++)
			{
				(*pFrame)[j] = (*pData)[(i+j)&1];
			}
			*pFrame += m_dwVideoWidth;
		}
		*pData += 2;
		*pDataRemain -= 2;
		*pFrame -= (8*m_dwVideoWidth - 8);
		break;

	default:
		break;
	}
}

void CMVEVideoDecompressor::DecodeFrame16(
	unsigned char *pFrame,
	unsigned char *pMap,
	int mapRemain,
	unsigned char *pData,
	int dataRemain
)
{
    unsigned char *pOrig;
    unsigned char *pOffData, *pEnd;
    unsigned short offset;
    int length;
    int op;
    int i, j;
    int xb, yb;

	if (!lookup_initialized) {
		GenLoopkupTable();
	}

    xb = m_pFormat->wWidth;
    yb = m_pFormat->wHeight;

    offset = pData[0]|(pData[1]<<8);

    pOffData = pData + offset;
    pEnd = pData + offset;

    pData += 2;

    pOrig = pData;
    length = offset - 2; /*dataRemain-2;*/

    for (j=0; j<yb; j++)
    {
        for (i=0; i<xb/2; i++)
        {
            op = (*pMap) & 0xf;
            DispatchDecoder16((unsigned short **)&pFrame, op, &pData, &pOffData, &dataRemain, &i, &j);
			/*
			if (
				(pFrame < m_pCurrentFrame) ||
				(pFrame >= m_pCurrentFrame + m_dwVideoWidth*m_dwVideoHeight*2)
			)
				return;
			*/

			op = ((*pMap) >> 4) & 0xf;
            DispatchDecoder16((unsigned short **)&pFrame, op, &pData, &pOffData, &dataRemain, &i, &j);
			/*
			if (
				(pFrame < m_pCurrentFrame) ||
				(pFrame >= m_pCurrentFrame + m_dwVideoWidth*m_dwVideoHeight*2)
			)
				return;
			*/

            ++pMap;
            --mapRemain;
        }

        pFrame += 7*m_dwVideoWidth*2;
    }
}

unsigned short CMVEVideoDecompressor::GETPIXEL(unsigned char **buf, int off)
{
	unsigned short val = (*buf)[0+off] | ((*buf)[1+off] << 8);
	return val;
}

unsigned short CMVEVideoDecompressor::GETPIXELI(unsigned char **buf, int off)
{
	unsigned short val = (*buf)[0+off] | ((*buf)[1+off] << 8);
	(*buf) += 2;
	return val;
}

void CMVEVideoDecompressor::GenLoopkupTable()
{
	int i;
	int x, y;

	for (i = 0; i < 256; i++) {
		RelClose(i, &x, &y);

		close_table[i*2+0] = x;
		close_table[i*2+1] = y;

		RelFar(i, 1, &x, &y);

		far_p_table[i*2+0] = x;
		far_p_table[i*2+1] = y;

		RelFar(i, -1, &x, &y);

		far_n_table[i*2+0] = x;
		far_n_table[i*2+1] = y;
	}

	lookup_initialized = 1;
}

void CMVEVideoDecompressor::CopyFrame16(unsigned short *pDest, unsigned short *pSrc)
{
    int i;

    for (i=0; i<8; i++)
    {
        CopyMemory(pDest, pSrc, 16);
        pDest += m_dwVideoWidth;
        pSrc += m_dwVideoWidth;
    }
}

void CMVEVideoDecompressor::PatternRow4Pixels16(
	unsigned short *pFrame,
	unsigned char pat0,
	unsigned char pat1,
	unsigned short *p
)
{
    unsigned short mask=0x0003;
    unsigned short shift=0;
    unsigned short pattern = (pat1 << 8) | pat0;

    while (mask != 0)
    {
        *pFrame++ = p[(mask & pattern) >> shift];
        mask <<= 2;
        shift += 2;
    }
}

void CMVEVideoDecompressor::PatternRow4Pixels2_16(
	unsigned short *pFrame,
	unsigned char pat0,
	unsigned short *p
)
{
    unsigned char mask=0x03;
    unsigned char shift=0;
    unsigned short pel;
	/* ORIGINAL VERSION IS BUGGY
	   int skip=1;

	   while (mask != 0)
	   {
	   pel = p[(mask & pat0) >> shift];
	   pFrame[0] = pel;
	   pFrame[2] = pel;
	   pFrame[m_dwVideoWidth + 0] = pel;
	   pFrame[m_dwVideoWidth + 2] = pel;
	   pFrame += skip;
	   skip = 4 - skip;
	   mask <<= 2;
	   shift += 2;
	   }
	*/
    while (mask != 0)
    {
        pel = p[(mask & pat0) >> shift];
        pFrame[0] = pel;
        pFrame[1] = pel;
        pFrame[m_dwVideoWidth + 0] = pel;
        pFrame[m_dwVideoWidth + 1] = pel;
        pFrame += 2;
        mask <<= 2;
        shift += 2;
    }
}

void CMVEVideoDecompressor::PatternRow4Pixels2x1_16(
	unsigned short *pFrame,
	unsigned char pat,
	unsigned short *p
)
{
    unsigned char mask=0x03;
    unsigned char shift=0;
    unsigned short pel;

    while (mask != 0)
    {
        pel = p[(mask & pat) >> shift];
        pFrame[0] = pel;
        pFrame[1] = pel;
        pFrame += 2;
        mask <<= 2;
        shift += 2;
    }
}

void CMVEVideoDecompressor::PatternQuadrant4Pixels16(
	unsigned short *pFrame,
	unsigned char pat0,
	unsigned char pat1,
	unsigned char pat2,
	unsigned char pat3,
	unsigned short *p
)
{
    unsigned long mask = 0x00000003UL;
    int shift=0;
    int i;
    unsigned long pat = (pat3 << 24) | (pat2 << 16) | (pat1 << 8) | pat0;

    for (i=0; i<16; i++)
    {
        pFrame[i&3] = p[(pat & mask) >> shift];

        if ((i&3) == 3)
            pFrame += m_dwVideoWidth;

        mask <<= 2;
        shift += 2;
    }
}

void CMVEVideoDecompressor::PatternRow2Pixels16(
	unsigned short *pFrame,
	unsigned char pat,
	unsigned short *p
)
{
    unsigned char mask=0x01;

    while (mask != 0)
    {
        *pFrame++ = p[(mask & pat) ? 1 : 0];
        mask <<= 1;
    }
}

void CMVEVideoDecompressor::PatternRow2Pixels2_16(
	unsigned short *pFrame,
	unsigned char pat,
	unsigned short *p
)
{
    unsigned short pel;
    unsigned char mask=0x1;

	/* ORIGINAL VERSION IS BUGGY
	   int skip=1;
	   while (mask != 0x10)
	   {
	   pel = p[(mask & pat) ? 1 : 0];
	   pFrame[0] = pel;
	   pFrame[2] = pel;
	   pFrame[m_dwVideoWidth + 0] = pel;
	   pFrame[m_dwVideoWidth + 2] = pel;
	   pFrame += skip;
	   skip = 4 - skip;
	   mask <<= 1;
	   }
	*/
	while (mask != 0x10) {
		pel = p[(mask & pat) ? 1 : 0];

		pFrame[0] = pel;
		pFrame[1] = pel;
		pFrame[m_dwVideoWidth + 0] = pel;
		pFrame[m_dwVideoWidth + 1] = pel;
		pFrame += 2;

		mask <<= 1;
	}
}

void CMVEVideoDecompressor::PatternQuadrant2Pixels16(
	unsigned short *pFrame,
	unsigned char pat0,
	unsigned char pat1,
	unsigned short *p
)
{
    unsigned short mask = 0x0001;
    int i;
    unsigned short pat = (pat1 << 8) | pat0;

    for (i=0; i<16; i++)
    {
        pFrame[i&3] = p[(pat & mask) ? 1 : 0];

        if ((i&3) == 3)
            pFrame += m_dwVideoWidth;

        mask <<= 1;
    }
}

void CMVEVideoDecompressor::DispatchDecoder16(
	unsigned short **pFrame,
	unsigned char codeType,
	unsigned char **pData,
	unsigned char **pOffData,
	int *pDataRemain,
	int *curXb,
	int *curYb
)
{
    unsigned short p[4];
    unsigned char pat[16];
    int i, j, k;
    int x, y;
    unsigned short *pDstBak;

    pDstBak = *pFrame;

    switch(codeType)
    {
	case 0x0:
		CopyFrame16(*pFrame, (unsigned short *)m_pPreviousFrame + (*pFrame - (unsigned short *)m_pCurrentFrame));
	case 0x1:
		break;
	case 0x2: /*
				relFar(*(*pOffData)++, 1, &x, &y);
			  */

		k = *(*pOffData)++;
		x = far_p_table[k*2+0];
		y = far_p_table[k*2+1];

		CopyFrame16(*pFrame, *pFrame + x + y*m_dwVideoWidth);
		--*pDataRemain;
		break;
	case 0x3: /*
				relFar(*(*pOffData)++, -1, &x, &y);
			  */

		k = *(*pOffData)++;
		x = far_n_table[k*2+0];
		y = far_n_table[k*2+1];

		CopyFrame16(*pFrame, *pFrame + x + y*m_dwVideoWidth);
		--*pDataRemain;
		break;
	case 0x4: /*
				relClose(*(*pOffData)++, &x, &y);
			  */

		k = *(*pOffData)++;
		x = close_table[k*2+0];
		y = close_table[k*2+1];

		CopyFrame16(*pFrame, (unsigned short *)m_pPreviousFrame + (*pFrame - (unsigned short *)m_pCurrentFrame) + x + y*m_dwVideoWidth);
		--*pDataRemain;
		break;
	case 0x5:
		x = (char)*(*pData)++;
		y = (char)*(*pData)++;
		CopyFrame16(*pFrame, (unsigned short *)m_pPreviousFrame + (*pFrame - (unsigned short *)m_pCurrentFrame) + x + y*m_dwVideoWidth);
		*pDataRemain -= 2;
		break;
	case 0x6:
		// STUB: Encoding 6 not tested
		for (i=0; i<2; i++)
		{
			*pFrame += 16;
			if (++*curXb == m_pFormat->wWidth)
			{
				*pFrame += 7*m_dwVideoWidth;
				*curXb = 0;
				if (++*curYb == m_pFormat->wHeight)
					return;
			}
		}
		break;

	case 0x7:
		p[0] = GETPIXELI(pData, 0);
		p[1] = GETPIXELI(pData, 0);

		if (!((p[0]/*|p[1]*/)&0x8000))
		{
			for (i=0; i<8; i++)
			{
				PatternRow2Pixels16(*pFrame, *(*pData), p);
				(*pData)++;

				*pFrame += m_dwVideoWidth;
			}
		}
		else
		{
			for (i=0; i<2; i++)
			{
				PatternRow2Pixels2_16(*pFrame, *(*pData) & 0xf, p);
				*pFrame += 2*m_dwVideoWidth;
				PatternRow2Pixels2_16(*pFrame, *(*pData) >> 4, p);
				(*pData)++;

				*pFrame += 2*m_dwVideoWidth;
			}
		}
		break;

	case 0x8:
		p[0] = GETPIXEL(pData, 0);

		if (!(p[0] & 0x8000))
		{
			for (i=0; i<4; i++)
			{
				p[0] = GETPIXELI(pData, 0);
				p[1] = GETPIXELI(pData, 0);

				pat[0] = (*pData)[0];
				pat[1] = (*pData)[1];
				(*pData) += 2;

				PatternQuadrant2Pixels16(*pFrame, pat[0], pat[1], p);

				if (i & 1)
					*pFrame -= (4*m_dwVideoWidth - 4);
				else
					*pFrame += 4*m_dwVideoWidth;
			}


		} else {
			p[2] = GETPIXEL(pData, 8);

			if (!(p[2]&0x8000)) {
				for (i=0; i<4; i++)
				{
					if ((i & 1) == 0)
					{
						p[0] = GETPIXELI(pData, 0);
						p[1] = GETPIXELI(pData, 0);
					}
					pat[0] = *(*pData)++;
					pat[1] = *(*pData)++;
					PatternQuadrant2Pixels16(*pFrame, pat[0], pat[1], p);

					if (i & 1)
						*pFrame -= (4*m_dwVideoWidth - 4);
					else
						*pFrame += 4*m_dwVideoWidth;
				}
			} else {
				for (i=0; i<8; i++)
				{
					if ((i & 3) == 0)
					{
						p[0] = GETPIXELI(pData, 0);
						p[1] = GETPIXELI(pData, 0);
					}
					PatternRow2Pixels16(*pFrame, *(*pData), p);
					(*pData)++;

					*pFrame += m_dwVideoWidth;
				}
			}
		}
		break;

	case 0x9:
		p[0] = GETPIXELI(pData, 0);
		p[1] = GETPIXELI(pData, 0);
		p[2] = GETPIXELI(pData, 0);
		p[3] = GETPIXELI(pData, 0);

		*pDataRemain -= 8;

		if (!(p[0] & 0x8000))
		{
			if (!(p[2] & 0x8000))
			{

				for (i=0; i<8; i++)
				{
					pat[0] = (*pData)[0];
					pat[1] = (*pData)[1];
					(*pData) += 2;
					PatternRow4Pixels16(*pFrame, pat[0], pat[1], p);
					*pFrame += m_dwVideoWidth;
				}
				*pDataRemain -= 16;

			}
			else
			{
				PatternRow4Pixels2_16(*pFrame, (*pData)[0], p);
				*pFrame += 2*m_dwVideoWidth;
				PatternRow4Pixels2_16(*pFrame, (*pData)[1], p);
				*pFrame += 2*m_dwVideoWidth;
				PatternRow4Pixels2_16(*pFrame, (*pData)[2], p);
				*pFrame += 2*m_dwVideoWidth;
				PatternRow4Pixels2_16(*pFrame, (*pData)[3], p);

				(*pData) += 4;
				*pDataRemain -= 4;

			}
		}
		else
		{
			if (!(p[2] & 0x8000))
			{
				for (i=0; i<8; i++)
				{
					pat[0] = (*pData)[0];
					(*pData) += 1;
					PatternRow4Pixels2x1_16(*pFrame, pat[0], p);
					*pFrame += m_dwVideoWidth;
				}
				*pDataRemain -= 8;
			}
			else
			{
				for (i=0; i<4; i++)
				{
					pat[0] = (*pData)[0];
					pat[1] = (*pData)[1];

					(*pData) += 2;

					PatternRow4Pixels16(*pFrame, pat[0], pat[1], p);
					*pFrame += m_dwVideoWidth;
					PatternRow4Pixels16(*pFrame, pat[0], pat[1], p);
					*pFrame += m_dwVideoWidth;
				}
				*pDataRemain -= 8;
			}
		}
		break;

	case 0xa:
		p[0] = GETPIXEL(pData, 0);

		if (!(p[0] & 0x8000))
		{
			for (i=0; i<4; i++)
			{
				p[0] = GETPIXELI(pData, 0);
				p[1] = GETPIXELI(pData, 0);
				p[2] = GETPIXELI(pData, 0);
				p[3] = GETPIXELI(pData, 0);
				pat[0] = (*pData)[0];
				pat[1] = (*pData)[1];
				pat[2] = (*pData)[2];
				pat[3] = (*pData)[3];

				(*pData) += 4;

				PatternQuadrant4Pixels16(*pFrame, pat[0], pat[1], pat[2], pat[3], p);

				if (i & 1)
					*pFrame -= (4*m_dwVideoWidth - 4);
				else
					*pFrame += 4*m_dwVideoWidth;
			}
		}
		else
		{
			p[0] = GETPIXEL(pData, 16);

			if (!(p[0] & 0x8000))
			{
				for (i=0; i<4; i++)
				{
					if ((i&1) == 0)
					{
						p[0] = GETPIXELI(pData, 0);
						p[1] = GETPIXELI(pData, 0);
						p[2] = GETPIXELI(pData, 0);
						p[3] = GETPIXELI(pData, 0);
					}

					pat[0] = (*pData)[0];
					pat[1] = (*pData)[1];
					pat[2] = (*pData)[2];
					pat[3] = (*pData)[3];

					(*pData) += 4;

					PatternQuadrant4Pixels16(*pFrame, pat[0], pat[1], pat[2], pat[3], p);

					if (i & 1)
						*pFrame -= (4*m_dwVideoWidth - 4);
					else
						*pFrame += 4*m_dwVideoWidth;
				}
			}
			else
			{
				for (i=0; i<8; i++)
				{
					if ((i&3) == 0)
					{
						p[0] = GETPIXELI(pData, 0);
						p[1] = GETPIXELI(pData, 0);
						p[2] = GETPIXELI(pData, 0);
						p[3] = GETPIXELI(pData, 0);
					}

					pat[0] = (*pData)[0];
					pat[1] = (*pData)[1];
					PatternRow4Pixels16(*pFrame, pat[0], pat[1], p);
					*pFrame += m_dwVideoWidth;

					(*pData) += 2;
				}
			}
		}
		break;

	case 0xb:
		for (i=0; i<8; i++)
		{
			CopyMemory(*pFrame, *pData, 16);
			*pFrame += m_dwVideoWidth;
			*pData += 16;
			*pDataRemain -= 16;
		}
		break;

	case 0xc:
		for (i=0; i<4; i++)
		{
			p[0] = GETPIXEL(pData, 0);
			p[1] = GETPIXEL(pData, 2);
			p[2] = GETPIXEL(pData, 4);
			p[3] = GETPIXEL(pData, 6);

			for (j=0; j<2; j++)
			{
				for (k=0; k<4; k++)
				{
					(*pFrame)[j+2*k] = p[k];
					(*pFrame)[m_dwVideoWidth+j+2*k] = p[k];
				}
				*pFrame += m_dwVideoWidth;
			}
			*pData += 8;
			*pDataRemain -= 8;
		}
		break;

	case 0xd:
		for (i=0; i<2; i++)
		{
			p[0] = GETPIXEL(pData, 0);
			p[1] = GETPIXEL(pData, 2);

			for (j=0; j<4; j++)
			{
				for (k=0; k<4; k++)
				{
					(*pFrame)[k*m_dwVideoWidth+j] = p[0];
					(*pFrame)[k*m_dwVideoWidth+j+4] = p[1];
				}
			}

			*pFrame += 4*m_dwVideoWidth;

			*pData += 4;
			*pDataRemain -= 4;
		}
		break;

	case 0xe:
		p[0] = GETPIXEL(pData, 0);

		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				(*pFrame)[j] = p[0];
			}

			*pFrame += m_dwVideoWidth;
		}

		*pData += 2;
		*pDataRemain -= 2;

		break;

	case 0xf:
		p[0] = GETPIXEL(pData, 0);
		p[1] = GETPIXEL(pData, 1);

		for (i=0; i<8; i++)
		{
			for (j=0; j<8; j++)
			{
				(*pFrame)[j] = p[(i+j)&1];
			}
			*pFrame += m_dwVideoWidth;
		}

		*pData += 4;
		*pDataRemain -= 4;
		break;

	default:
		break;
    }

    *pFrame = pDstBak+8;
}

//==========================================================================
// CMVEVideoDecompressorPage methods
//==========================================================================

const WCHAR g_wszMVEVideoDecompressorPageName[] = L"ANX MVE Video Decompressor Property Page";

CMVEVideoDecompressorPage::CMVEVideoDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("MVE Video Decompressor Property Page"),
		pUnk,
		IDD_MVEVIDEODECOMPRESSORPAGE,
		IDS_TITLE_MVEVIDEODECOMPRESSORPAGE
	)
{
}

CUnknown* WINAPI CMVEVideoDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CMVEVideoDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
