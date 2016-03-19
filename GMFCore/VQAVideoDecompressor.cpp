//==========================================================================
//
// File: VQAVideoDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of VQA video decompressor
//
// Copyright (C) 2004 ANX Software.
// Author: Valery V. Anisimovsky
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

#include <string.h>

#include "VQAGUID.h"
#include "VQAVideoDecompressor.h"
#include "FilterOptions.h"
#include "resource.h"

#include <cguid.h>

#define SWAPWORD(x) ((((x) & 0xFF) << 8) | ((x) >> 8))
#define LE_16(x) (*((WORD*)(x)))
#define BE_16(x) SWAPWORD(*((WORD*)(x)))

//==========================================================================
// VQA video decompressor setup data
//==========================================================================

const WCHAR g_wszVQAVideoDecompressorName[] = L"ANX VQA Video Decompressor";

const AMOVIESETUP_MEDIATYPE sudVQAVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_VQAVideo
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

const AMOVIESETUP_PIN sudVQAVideoDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudVQAVideoType	// Media types
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

const AMOVIESETUP_FILTER g_sudVQAVideoDecompressor = {
	&CLSID_VQAVideoDecompressor,	// CLSID of filter
	g_wszVQAVideoDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudVQAVideoDecompressorPins		// Pin information
};

//==========================================================================
// CVQAVideoDecompressor methods
//==========================================================================

CVQAVideoDecompressor::CVQAVideoDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("VQA Video Decompressor"),
		pUnk,
		CLSID_VQAVideoDecompressor
	),
	m_pCurrentFrame(NULL),				// No frame buffer at this time
	m_cbPalette(0),						// No palette data at this time
	m_pDecodeBuffer(NULL),				// No decode buffer at this time
	m_cbMaxDecodeBuffer(0),				// No decode buffer size at this time
	m_pCodebook(NULL),					// No current codebook at this time
	m_pNextCodebook(NULL),				// No next codebook at this time
	m_cbNextCodebook(0),				// No next codebook parts at this time
	m_nNextCodebookParts(0),			// No next codebook parts at this time
	m_bIsNextCodebookCompressed(FALSE),	// Default: non-compressed codebook
	m_bSolidColorMarker(0),				// No solid color marker at this time
	m_pFormat(NULL),					// No format block at this time
	m_pCodebookTable(NULL),				// No codebook descriptor table
	m_nCodebooks(0),					// No codebook descriptor table
	m_nBlocksX(0),						// |
	m_nBlocksY(0),						// |
	m_nBlocks(0),						// |
	m_cbBlock(0),						// |-- No image/block parameters at this time
	m_cbBlockStride(0),					// |
	m_cbImageXStride(0),				// |
	m_cbImageYStride(0)					// |
{
	ASSERT(phr);
}

CVQAVideoDecompressor::~CVQAVideoDecompressor()
{
	// Perform decoder clean-up
	Cleanup();

	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
	}
}

CUnknown* WINAPI CVQAVideoDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CVQAVideoDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CVQAVideoDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

void CVQAVideoDecompressor::FillBlock(BYTE *pbImage, WORD wIndex)
{
	// Sanity check of the index
	if (wIndex * m_cbBlock >= MAX_CODEBOOK_SIZE)
		return;

	BYTE *pbBlock = m_pCodebook + wIndex * m_cbBlock;
	for (WORD i = 0; i < m_pFormat->bBlockHeight; i++) {
		CopyMemory(pbImage, pbBlock, m_cbBlockStride);
		pbImage += m_cbImageXStride;
		pbBlock += m_cbBlockStride;
	}
}

void CVQAVideoDecompressor::FillBlockSolid(BYTE *pbImage, BYTE bColor)
{
	for (WORD i = 0; i < m_pFormat->bBlockHeight; i++) {
		FillMemory(pbImage, m_cbBlockStride, bColor);
		pbImage += m_cbImageXStride;
	}
}

#define VALIDATEREAD(p,n,t,e) if ((p) + (n) > (t)) return (e);

HRESULT CVQAVideoDecompressor::DecodeVPTR(BYTE *pbData, DWORD cbData, BYTE *pbImage)
{
	// There should be format block
	ASSERT(m_pFormat);

	// Set up the data threshold
	BYTE *pbDataThreshould = pbData + cbData;

	// Set up lo/hi value data pointers and their increment (for 8-bit video)
	BYTE *pbLoVal = pbData;
	BYTE *pbHiVal = (m_pFormat->wVersion == 1) ? (pbData + 1) : (pbData + m_nBlocks);
	int iIncrement = (m_pFormat->wVersion == 1) ? 2 : 1;
	
	// Command completion flag (for HiColor video)
	BOOL bIsCommandComplete = TRUE;

	// First block done flag (for VQA_CODE_PUT_ARRAY)
	BOOL bIsFirstBlockDone;

	// Command code, block index and count (for HiColor video)
	WORD wCode, wIndex, wCount;

	// Cycle through all blocks
	for (WORD y = 0; y < m_nBlocksY; y++) {
		for (WORD x = 0; x < m_nBlocksX; x++) {
		
			// HiColor video
			if (m_pFormat->nColors == 0) {

				if (bIsCommandComplete) {

					// Get the next value from the input buffer
					VALIDATEREAD(pbData, 2, pbDataThreshould, E_UNEXPECTED);
					WORD wValue = LE_16(pbData);
					pbData += 2;

					// Get command code
					wCode = (wValue >> 13) & 7;

					// Parse command code
					switch (wCode) {

						case VQA_CODE_SKIP:
							wCount = wValue & 0x1FFF;
							break;
						case VQA_CODE_REPEAT_SHORT:
							wIndex = wValue & 0xFF;
							wCount = (((wValue >> 8) & 0x1F) + 1) * 2;
							break;
						case VQA_CODE_PUT_ARRAY:
							wIndex = wValue & 0xFF;
							wCount = (((wValue >> 8) & 0x1F) + 1) * 2;
							bIsFirstBlockDone = FALSE;
							break;
						case VQA_CODE_PUT:
							wIndex = wValue & 0x1FFF;
							break;
						case VQA_CODE_REPEAT_LONG:
							wIndex = wValue & 0x1FFF;
							VALIDATEREAD(pbData, 1, pbDataThreshould, E_UNEXPECTED);
							wCount = *pbData++;
							break;
						default:
							// Unknown command code
							break;
					}

					// Reset command completion flag
					bIsCommandComplete = FALSE;
				}

				// Execute command code
				switch (wCode) {

					case VQA_CODE_SKIP:
						wCount--;
						bIsCommandComplete = (wCount == 0);
						break;
					case VQA_CODE_REPEAT_SHORT:
						FillBlock(pbImage, wIndex);
						wCount--;
						bIsCommandComplete = (wCount == 0);
						break;
					case VQA_CODE_PUT_ARRAY:
						if (bIsFirstBlockDone) {
							VALIDATEREAD(pbData, 1, pbDataThreshould, E_UNEXPECTED);
							FillBlock(pbImage, *pbData++);
							wCount--;
							bIsCommandComplete = (wCount == 0);
						} else {
							FillBlock(pbImage, wIndex);
							bIsFirstBlockDone = TRUE;
						}
						break;
					case VQA_CODE_PUT:
						FillBlock(pbImage, wIndex);
						bIsCommandComplete = TRUE;
						break;
					case VQA_CODE_REPEAT_LONG:
						FillBlock(pbImage, wIndex);
						wCount--;
						bIsCommandComplete = (wCount == 0);
						break;
					default:
						// Unknown command code
						break;
				}

			} else {

				// Validate read pointers
				VALIDATEREAD(pbHiVal, 1, pbDataThreshould, E_UNEXPECTED);
				VALIDATEREAD(pbLoVal, 1, pbDataThreshould, E_UNEXPECTED);

				// Parse values and fill the block
				if (*pbHiVal == m_bSolidColorMarker)
					FillBlockSolid(
						pbImage,
						(m_pFormat->wVersion == 1)
						? (~(*pbLoVal))
						: (*pbLoVal)
					);
				else
					FillBlock(
						pbImage,
						(((*pbHiVal) << 8) + (*pbLoVal)) >>
						((m_pFormat->wVersion == 1) ? 3 : 0)
					);

				// Advance pointers
				pbLoVal += iIncrement;
				pbHiVal += iIncrement;
			}
			pbImage += m_cbBlockStride;
		}
		pbImage += m_cbImageYStride;
	}

	return NOERROR;
}

void CVQAVideoDecompressor::PutNewCodebook(void)
{
	if (m_bIsNextCodebookCompressed) {

		// Decompress the codebook
		int cbCodebook = MAX_CODEBOOK_SIZE;
		DecodeFormat80(m_pNextCodebook, m_cbNextCodebook, m_pCodebook, cbCodebook);

	} else {

		// Swap the codebook pointers
		BYTE *pTemp = m_pCodebook;
		m_pCodebook = m_pNextCodebook;
		m_pNextCodebook = pTemp;

	}

	// Reset the accumulation stuff
	m_cbNextCodebook = 0;
	m_nNextCodebookParts = 0;
}

HRESULT CVQAVideoDecompressor::Transform(
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

	// Get the incoming data length
	LONG lInDataLength = pIn->GetActualDataLength();

	// Get the output sample's buffer
	BYTE *pbOutBuffer = NULL;
	hr = pOut->GetPointer(&pbOutBuffer);
	if (FAILED(hr))
		return hr;

	// Set the data length for the output sample. 
	// The data length is the uncompressed frame size
	LONG lOutDataLength = m_pFormat->wVideoWidth * m_pFormat->wVideoHeight * ((m_pFormat->nColors == 0) ? 2 : 1);
	hr = pOut->SetActualDataLength(lOutDataLength);
	if (FAILED(hr))
		return hr;

	// Check if we have accumulated the full codebook
	if (m_pFormat->nCBParts == 0) {

		if (m_pFormat->nColors != 0) {

			// Get the frame number
			LONGLONG llStart = 0, llEnd = 0;
			hr = pIn->GetMediaTime(&llStart, &llEnd);
			if (FAILED(hr))
				return hr;

			// Search the codebook table for an entry using our frame 
			DWORD i = 0;
			for (i = 0; i < m_nCodebooks; i++)
				if (m_pCodebookTable[i].iFrame == (DWORD)llStart)
					break;

			// Check if this frame should use new full codebook
			if (
				(i < m_nCodebooks) &&
				(m_pCodebookTable[i].cbSize != 0)
			)
				PutNewCodebook();
		}

	} else if (m_nNextCodebookParts == m_pFormat->nCBParts)
		PutNewCodebook();

	// Reset the flag indicating we've found and processed frame data
	BOOL bIsFramePrepared = FALSE;

	// Parse incoming data
	while (lInDataLength > 0) {

		// Check if we have enough data for chunk header
		if (lInDataLength < sizeof(VQA_CHUNK_HEADER))
			break;

		// Get the chunk header
		VQA_CHUNK_HEADER *pHeader = (VQA_CHUNK_HEADER*)pbInBuffer;
		pbInBuffer += sizeof(VQA_CHUNK_HEADER);
		lInDataLength -= sizeof(VQA_CHUNK_HEADER);

		DWORD cbChunk = SWAPDWORD(pHeader->cbSize);

		// Check that the chunk does not exceed incoming data threshold
		if ((DWORD)lInDataLength < cbChunk)
			break;

		int cbCodebook;
		int cbDecodeBuffer;

		// Check the chunk type
		switch (pHeader->dwID) {

			// Full codebook (compressed)
			case VQA_ID_CBFZ:

				// Decompress the codebook
				cbCodebook = MAX_CODEBOOK_SIZE;
				DecodeFormat80(pbInBuffer, cbChunk, m_pCodebook, cbCodebook);
				break;

			// Full codebook (non-compressed)
			case VQA_ID_CBF0:

				// Copy the codebook
				if (cbChunk > MAX_CODEBOOK_SIZE)
					return E_UNEXPECTED;
				CopyMemory(m_pCodebook, pbInBuffer, cbChunk);
				break;

			// Codebook part
			case VQA_ID_CBPZ:
			case VQA_ID_CBP0:

				m_bIsNextCodebookCompressed = pHeader->dwID == VQA_ID_CBPZ;

				// Sanity check of the codebook size
				if (m_cbNextCodebook + cbChunk > MAX_CODEBOOK_SIZE)
					return E_UNEXPECTED;

				// Append this part to the next codebook
				CopyMemory(m_pNextCodebook + m_cbNextCodebook, pbInBuffer, cbChunk);
				m_cbNextCodebook += cbChunk;
				m_nNextCodebookParts++;
				break;

			// Palette (compressed)
			case VQA_ID_CPLZ:

				// Decompress the palette
				m_cbPalette = MAX_PALETTE_SIZE;
				DecodeFormat80(pbInBuffer, cbChunk, m_pPalette, m_cbPalette);
				break;

			// Palette (non-compressed)
			case VQA_ID_CPL0:

				// Copy the palette data and set its size
				CopyMemory(m_pPalette, pbInBuffer, m_cbPalette = (int)cbChunk);
				break;

			// Vector pointer data (non-compressed)
			case VQA_ID_VPTR:

				// Decode frame data
				DecodeVPTR(
					pbInBuffer,
					cbChunk,
					(m_pFormat->nColors == 0) ? m_pCurrentFrame : pbOutBuffer
				);

				// Copy image data to output sample (if needed)
				if (m_pFormat->nColors == 0)
					CopyMemory(pbOutBuffer, m_pCurrentFrame, lOutDataLength);

				// We've processed the frame data, so set the flag
				bIsFramePrepared = TRUE;
				break;

			// Vector pointer data (compressed)
			case VQA_ID_VPTZ:
			case VQA_ID_VPRZ:

				// Decompress the data
				cbDecodeBuffer = m_cbMaxDecodeBuffer;
				hr = DecodeFormat80(pbInBuffer, cbChunk, m_pDecodeBuffer, cbDecodeBuffer);
				if (FAILED(hr))
					break;

				// Decode frame data
				DecodeVPTR(
					m_pDecodeBuffer,
					(DWORD)cbDecodeBuffer,
					(m_pFormat->nColors == 0) ? m_pCurrentFrame : pbOutBuffer
				);

				// Copy image data to output sample (if needed)
				if (m_pFormat->nColors == 0)
					CopyMemory(pbOutBuffer, m_pCurrentFrame, lOutDataLength);

				// We've processed the frame data, so set the flag
				bIsFramePrepared = TRUE;
				break;

			default:

				// Just skip over this chunk
				break;
		}

		// Align to even boundary
		if (cbChunk % 2)
			cbChunk++;

		// Advance data pointer and data size
		pbInBuffer += cbChunk;
		lInDataLength -= cbChunk;
	}

	// If there's no frame prepared, quit with no error
	if (!bIsFramePrepared)
		return S_FALSE;

	// If we've booked palette change previously, do it now
	if (m_cbPalette > 0) {

		// Sanity check of the palette size
		if (m_cbPalette > MAX_PALETTE_SIZE)
			return E_UNEXPECTED;

		// Get the output media type format
		CMediaType mt((AM_MEDIA_TYPE)m_pOutput->CurrentMediaType());
		VIDEOINFO *pVideoInfo = (VIDEOINFO*)mt.Format();

		// Fill in the output media type format palette
		BYTE *pbPalette = m_pPalette;
		for (int i = 0; i < m_cbPalette / 3; i++) {
			pVideoInfo->bmiColors[i].rgbRed			= (*pbPalette++) << 2;
			pVideoInfo->bmiColors[i].rgbGreen		= (*pbPalette++) << 2;
			pVideoInfo->bmiColors[i].rgbBlue		= (*pbPalette++) << 2;
			pVideoInfo->bmiColors[i].rgbReserved	= 0;
		}

		// Set the changed media type for the output sample
		hr = pOut->SetMediaType(&mt);
		if (FAILED(hr))
			return hr;

		// Reset the stored palette size
		m_cbPalette = 0;
	}

	// Each RGB frame is a sync point
	hr = pOut->SetSyncPoint(TRUE);
	if (FAILED(hr))
		return hr;

	// RGB sample should never be a preroll one
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

HRESULT CVQAVideoDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	// Check for proper video media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Video			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_VQAVideo	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_VQAVideo			) &&
		(((VQA_INFO*)mtIn->Format())->nFramesPerSecond	!= 0		) &&
		(((VQA_INFO*)mtIn->Format())->bBlockWidth		!= 0		) &&
		(((VQA_INFO*)mtIn->Format())->bBlockHeight		!= 0		)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CVQAVideoDecompressor::SetMediaType(
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
		m_pFormat = (VQA_INFO*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());

		// Set up full codebook table
		m_nCodebooks = (pmt->FormatLength() - sizeof(VQA_INFO)) / sizeof(VQA_CIND_ENTRY);
		if (m_nCodebooks > 0)
			m_pCodebookTable = (VQA_CIND_ENTRY*)((BYTE*)m_pFormat + sizeof(VQA_INFO));

		// Set up the solid color marker byte
		m_bSolidColorMarker = (
								(m_pFormat->bBlockHeight == 4) || 
								(m_pFormat->wVersion == 1)
							) ? 0xFF : 0x0F;

		// Set up block/image parameters
		WORD cbPixel = ((m_pFormat->nColors == 0) ? 2 : 1);
		m_nBlocksX = m_pFormat->wVideoWidth / m_pFormat->bBlockWidth;
		m_nBlocksY = m_pFormat->wVideoHeight / m_pFormat->bBlockHeight;
		m_nBlocks = m_nBlocksX * m_nBlocksY;
		m_cbBlock = m_pFormat->bBlockWidth * m_pFormat->bBlockHeight * cbPixel;
		m_cbBlockStride = m_pFormat->bBlockWidth * cbPixel;
		m_cbImageXStride = m_pFormat->wVideoWidth * cbPixel;
		m_cbImageYStride = m_cbImageXStride * (m_pFormat->bBlockHeight - 1);
	}

	return NOERROR;
}

HRESULT CVQAVideoDecompressor::BreakConnect(PIN_DIRECTION dir)
{
	// Catch only the input pin disconnection
	if (dir == PINDIR_INPUT) {

		// Perform decoder clean-up
		Cleanup();

		// Free the format block
		if (m_pFormat) {
			CoTaskMemFree(m_pFormat);
			m_pFormat = NULL;
		}

		// Reset codebook descriptor table
		if (m_pCodebookTable) {
			m_pCodebookTable = NULL;
			m_nCodebooks = 0;
		}

		// Reset format-specific parameters
		m_bSolidColorMarker = 0;
		m_nBlocksX = 0;
		m_nBlocksY = 0;
		m_nBlocks = 0;
		m_cbBlock = 0;
		m_cbBlockStride = 0;
		m_cbImageXStride = 0;
		m_cbImageYStride = 0;
	}

	return NOERROR;
}

HRESULT CVQAVideoDecompressor::CheckTransform(
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
	VQA_INFO	*pInFormat	= (VQA_INFO*)mtIn->Format();
	VIDEOINFO	*pOutFormat	= (VIDEOINFO*)mtOut->Format();

	// Check if the format length is enough
	if (mtOut->FormatLength() < sizeof(VIDEOINFO))
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check if the media subtype matches the bit count
	if (!IsEqualGUID(*mtOut->Subtype(), (pInFormat->nColors == 0) ? MEDIASUBTYPE_RGB555 : MEDIASUBTYPE_RGB8))
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check the compatibility of the formats
	WORD wBitCount = (pInFormat->nColors == 0) ? 16 : 8;
	DWORD cbFrame = pInFormat->wVideoWidth * pInFormat->wVideoHeight * wBitCount / 8;
	return (
		//(pOutFormat->AvgTimePerFrame			== UNITS / pInFormat->nFramesPerSecond	) &&
		(pOutFormat->bmiHeader.biWidth			== (LONG)pInFormat->wVideoWidth		) &&
		(pOutFormat->bmiHeader.biHeight			== -(LONG)pInFormat->wVideoHeight	) &&
		(pOutFormat->bmiHeader.biPlanes			== 1								) &&
		(pOutFormat->bmiHeader.biBitCount		== wBitCount						) &&
		(pOutFormat->bmiHeader.biCompression	== BI_RGB							) &&
		(pOutFormat->bmiHeader.biSizeImage		== cbFrame							)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CVQAVideoDecompressor::GetMediaType(
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

		// Prepare the video info block
		ZeroMemory(pVideoInfo, sizeof(VIDEOINFO));
		SetRectEmpty(&pVideoInfo->rcSource);
		SetRectEmpty(&pVideoInfo->rcTarget);
		pVideoInfo->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
		pVideoInfo->bmiHeader.biWidth			= (LONG)m_pFormat->wVideoWidth;
		pVideoInfo->bmiHeader.biHeight			= -(LONG)m_pFormat->wVideoHeight;
		pVideoInfo->bmiHeader.biPlanes			= 1;
		pVideoInfo->bmiHeader.biBitCount		= (m_pFormat->nColors == 0) ? 16 : 8;
		pVideoInfo->bmiHeader.biCompression		= BI_RGB;
		pVideoInfo->bmiHeader.biSizeImage		= GetBitmapSize(&pVideoInfo->bmiHeader);
		pVideoInfo->bmiHeader.biXPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biYPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biClrUsed			= m_pFormat->nColors;
		pVideoInfo->bmiHeader.biClrImportant	= 0;
		pVideoInfo->dwBitRate					= pVideoInfo->bmiHeader.biSizeImage * 8 * m_pFormat->nFramesPerSecond;
		pVideoInfo->dwBitErrorRate				= 0;
		pVideoInfo->AvgTimePerFrame				= UNITS / m_pFormat->nFramesPerSecond;

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

HRESULT CVQAVideoDecompressor::DecideBufferSize(
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
	LONG cbFrame = m_pFormat->wVideoWidth * m_pFormat->wVideoHeight * ((m_pFormat->nColors == 0) ? 2 : 1);
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

void CVQAVideoDecompressor::Cleanup(void)
{
	if (m_pCurrentFrame) {
		CoTaskMemFree(m_pCurrentFrame);
		m_pCurrentFrame = NULL;
	}

	m_cbPalette = 0;

	if (m_pDecodeBuffer) {
		CoTaskMemFree(m_pDecodeBuffer);
		m_pDecodeBuffer = NULL;
		m_cbMaxDecodeBuffer = 0;
	}

	if (m_pCodebook) {
		CoTaskMemFree(m_pCodebook);
		m_pCodebook = NULL;
	}

	if (m_pNextCodebook) {
		CoTaskMemFree(m_pNextCodebook);
		m_pNextCodebook = NULL;
		m_cbNextCodebook = 0;
		m_nNextCodebookParts = 0;
		m_bIsNextCodebookCompressed = FALSE;
	}
}

HRESULT CVQAVideoDecompressor::StartStreaming(void)
{
	// We should have the format block at this time
	if (m_pFormat == NULL)
		return E_UNEXPECTED;

	// Allocate frame buffer (for HiColor video only)
	if (m_pFormat->nColors == 0) {
		DWORD cbFrame = m_pFormat->wVideoWidth * m_pFormat->wVideoHeight * ((m_pFormat->nColors == 0) ? 2 : 1);
		m_pCurrentFrame = (BYTE*)CoTaskMemAlloc(cbFrame);
		if (m_pCurrentFrame == NULL) {
			Cleanup();
			return E_OUTOFMEMORY;
		}
		ZeroMemory(m_pCurrentFrame, cbFrame);
	}

	// Reset palette data size
	m_cbPalette = 0;

	// Allocate decode buffer
	m_cbMaxDecodeBuffer = 3 * (m_pFormat->wVideoWidth / m_pFormat->bBlockWidth) *
						(m_pFormat->wVideoHeight / m_pFormat->bBlockHeight); // Maximum 3 bytes per block
	m_pDecodeBuffer = (BYTE*)CoTaskMemAlloc((ULONG)m_cbMaxDecodeBuffer);
	if (m_pDecodeBuffer == NULL) {
		Cleanup();
		return E_OUTOFMEMORY;
	}

	// Allocate codebooks
	m_pCodebook = (BYTE*)CoTaskMemAlloc(MAX_CODEBOOK_SIZE);
	if (m_pCodebook == NULL) {
		Cleanup();
		return E_OUTOFMEMORY;
	}
	m_pNextCodebook = (BYTE*)CoTaskMemAlloc(MAX_CODEBOOK_SIZE);
	if (m_pNextCodebook == NULL) {
		Cleanup();
		return E_OUTOFMEMORY;
	}
	m_cbNextCodebook = 0;
	m_nNextCodebookParts = 0;
	m_bIsNextCodebookCompressed = FALSE;

	// Zero palette and the codebooks
	ZeroMemory(m_pPalette, MAX_PALETTE_SIZE);
	ZeroMemory(m_pCodebook, MAX_CODEBOOK_SIZE);
	ZeroMemory(m_pNextCodebook, MAX_CODEBOOK_SIZE);

	return NOERROR;
}

HRESULT CVQAVideoDecompressor::StopStreaming(void)
{
	// Perform decoder clean-up
	Cleanup();

	return NOERROR;
}

HRESULT CVQAVideoDecompressor::EndFlush(void)
{
	// Reset codebook accumulation stuff so that after flush we will 
	// be able to receive codebook parts from the scratch
	m_cbNextCodebook = 0;
	m_nNextCodebookParts = 0;

	// Call the base-class method
	return CTransformFilter::EndFlush();
}

STDMETHODIMP CVQAVideoDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_VQAVideoDecompressorPage;

	return NOERROR;
}

//==========================================================================
// Format80 decoder source code taken from vqavideo.c
// by Mike Melanson (melanson@pcisys.net)
//==========================================================================

#define CHECK_COUNT() if (dest_index + count > dest_size) return VFW_E_BUFFER_OVERFLOW;

HRESULT CVQAVideoDecompressor::DecodeFormat80(
	unsigned char *src,
	int src_size,
	unsigned char *dest,
	int& dest_size
)
{
    int src_index = 0;
    int dest_index = 0;
    int count;
    int src_pos;
    unsigned char color;
    int i;

	// Set up the data threshold
	unsigned char *src_threshold = src + src_size;
	unsigned char *dest_threshold = dest + dest_size;

	// Set up modification flag
	VALIDATEREAD(src, 1, src_threshold, E_UNEXPECTED);
	int mod = src[0] == 0x00;
	if (mod)
		src_index++;

    while (src_index < src_size) {

        /* 0x80 means that frame is finished */
        if (src[src_index] == 0x80) {
			dest_size = dest_index;
            return NOERROR;
		}

        if (dest_index >= dest_size)
            return VFW_E_BUFFER_OVERFLOW;

        if (src[src_index] == 0xFF) {

            src_index++;
			VALIDATEREAD(src + src_index, 2, src_threshold, E_UNEXPECTED);
            count = LE_16(&src[src_index]);
            src_index += 2;
			VALIDATEREAD(src + src_index, 2, src_threshold, E_UNEXPECTED);
            src_pos = (mod) ? (dest_index - LE_16(&src[src_index])) : LE_16(&src[src_index]);
            src_index += 2;
            CHECK_COUNT();
			VALIDATEREAD(dest + src_pos, count, dest_threshold, E_UNEXPECTED);
			if (src_pos < 0)
				return E_UNEXPECTED;
            for (i = 0; i < count; i++)
                dest[dest_index + i] = dest[src_pos + i];
            dest_index += count;

        } else if (src[src_index] == 0xFE) {

            src_index++;
			VALIDATEREAD(src + src_index, 2, src_threshold, E_UNEXPECTED);
            count = LE_16(&src[src_index]);
            src_index += 2;
			VALIDATEREAD(src + src_index, 1, src_threshold, E_UNEXPECTED);
            color = src[src_index++];
            CHECK_COUNT();
            memset(&dest[dest_index], color, count);
            dest_index += count;

        } else if ((src[src_index] & 0xC0) == 0xC0) {

            count = (src[src_index++] & 0x3F) + 3;
			VALIDATEREAD(src + src_index, 2, src_threshold, E_UNEXPECTED);
            src_pos = (mod) ? (dest_index - LE_16(&src[src_index])) : LE_16(&src[src_index]);
            src_index += 2;
            CHECK_COUNT();
			VALIDATEREAD(dest + src_pos, count, dest_threshold, E_UNEXPECTED);
			if (src_pos < 0)
				return E_UNEXPECTED;
            for (i = 0; i < count; i++)
                dest[dest_index + i] = dest[src_pos + i];
            dest_index += count;

        } else if (src[src_index] > 0x80) {

            count = src[src_index++] & 0x3F;
            CHECK_COUNT();
			VALIDATEREAD(src + src_index, count, src_threshold, E_UNEXPECTED);
            memcpy(&dest[dest_index], &src[src_index], count);
            src_index += count;
            dest_index += count;

        } else {

            count = ((src[src_index] & 0x70) >> 4) + 3;
			VALIDATEREAD(src + src_index, 2, src_threshold, E_UNEXPECTED);
            src_pos = BE_16(&src[src_index]) & 0x0FFF;
            src_index += 2;
            CHECK_COUNT();
			VALIDATEREAD(dest + dest_index - src_pos, count, dest_threshold, E_UNEXPECTED);
			if (dest_index - src_pos < 0)
				return E_UNEXPECTED;
            for (i = 0; i < count; i++)
                dest[dest_index + i] = dest[dest_index - src_pos + i];
            dest_index += count;
        }
    }

	dest_size = dest_index;
	return NOERROR;
}

//==========================================================================
// CVQAVideoDecompressorPage methods
//==========================================================================

const WCHAR g_wszVQAVideoDecompressorPageName[] = L"ANX VQA Video Decompressor Property Page";

CVQAVideoDecompressorPage::CVQAVideoDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("VQA Video Decompressor Property Page"),
		pUnk,
		IDD_VQAVIDEODECOMPRESSORPAGE,
		IDS_TITLE_VQAVIDEODECOMPRESSORPAGE
	)
{
}

CUnknown* WINAPI CVQAVideoDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CVQAVideoDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
