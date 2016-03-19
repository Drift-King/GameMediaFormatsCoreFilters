//==========================================================================
//
// File: ROQVideoDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of ROQ video decompressor
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

#include "ROQGUID.h"
#include "ROQVideoDecompressor.h"
#include "resource.h"

#include <cguid.h>
#include <mmsystem.h>

//==========================================================================
// ROQ video decompressor setup data
//==========================================================================

const WCHAR g_wszROQVideoDecompressorName[] = L"ANX ROQ Video Decompressor";

const AMOVIESETUP_MEDIATYPE sudROQVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_ROQVideo
};

const DWORD BI_YV12 = mmioFOURCC('Y','V','1','2');

const AMOVIESETUP_MEDIATYPE sudYV12Type = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_YV12
};

const AMOVIESETUP_PIN sudROQVideoDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudROQVideoType	// Media types
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
		&sudYV12Type		// Media types
	}
};

const AMOVIESETUP_FILTER g_sudROQVideoDecompressor = {
	&CLSID_ROQVideoDecompressor,	// CLSID of filter
	g_wszROQVideoDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudROQVideoDecompressorPins		// Pin information
};

//==========================================================================
// CROQVideoDecompressor methods
//==========================================================================

CROQVideoDecompressor::CROQVideoDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("ROQ Video Decompressor"),
		pUnk,
		CLSID_ROQVideoDecompressor
	),
	m_pFormat(NULL),		// No format block at this time
	m_pPreviousFrame(NULL),	// No previous frame buffer at this time
	m_pCurrentFrame(NULL),	// No current frame buffer at this time
	m_wYStride(0),			// |
	m_wCStride(0)			// | -- No image strides at this time
{
	ASSERT(phr);
}

CROQVideoDecompressor::~CROQVideoDecompressor()
{
	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
	}

	// Free the previous frame buffer
	if (m_pPreviousFrame) {
		CoTaskMemFree(m_pPreviousFrame);
		m_pPreviousFrame = NULL;
	}

	// Free the current frame buffer
	if (m_pCurrentFrame) {
		CoTaskMemFree(m_pCurrentFrame);
		m_pCurrentFrame = NULL;
	}
}

CUnknown* WINAPI CROQVideoDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CROQVideoDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CROQVideoDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

HRESULT CROQVideoDecompressor::Transform(
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

	// Media sample starts with the chunk header
	ROQ_CHUNK_HEADER *pHeader = (ROQ_CHUNK_HEADER*)pbInBuffer;
	pbInBuffer += sizeof(ROQ_CHUNK_HEADER);

	// Fill in the codebooks if we have to
	if (pHeader->wID == ROQ_CHUNK_VIDEO_CODEBOOK) {

		// Extract codebook sizes
		WORD nCells = HIBYTE(pHeader->wArgument);
		if (nCells == 0)
			nCells = 256;
		WORD nQCells = LOBYTE(pHeader->wArgument);
		if ((nQCells == 0) && (nCells * sizeof(ROQ_CELL) < pHeader->cbSize))
			nQCells = 256;

		// Copy 2x2 codebook
		CopyMemory(m_Cells, pbInBuffer, nCells * sizeof(ROQ_CELL));
		pbInBuffer += nCells * sizeof(ROQ_CELL);

		// Copy 4x4 codebook
		CopyMemory(m_QCells, pbInBuffer, nQCells * sizeof(ROQ_QCELL));

		// We should not deliver any media sample
		return S_FALSE;

	}

	// Now it must be video frame chunk
	ASSERT(pHeader->wID == ROQ_CHUNK_VIDEO_FRAME);

	// Get the output sample's buffer
	BYTE *pbOutBuffer = NULL;
	hr = pOut->GetPointer(&pbOutBuffer);
	if (FAILED(hr))
		return hr;

	// Call the decoder function to decompress the frame
	ROQVideoDecodeFrame(
		pbInBuffer,
		pHeader->cbSize,
		pHeader->wArgument
	);

	// Calculate the frame size
	LONG lOutDataLength = (m_pFormat->wWidth * m_pFormat->wHeight * 12) / 8; // 12 bits per pixel

	// Downsample the upsampled frame to the output buffer
	DownsampleColorPlanes(m_pCurrentFrame, pbOutBuffer);
	
	// Swap frame buffers if it's not the first frame
	LONGLONG llStart = 0, llEnd = 0;
	hr = pIn->GetMediaTime(&llStart, &llEnd);
	if (FAILED(hr))
		return hr;
	if (llStart == 0)
		CopyMemory(m_pPreviousFrame, m_pCurrentFrame, lOutDataLength * 2);
	else {
		BYTE *pTemp = m_pPreviousFrame;
		m_pPreviousFrame = m_pCurrentFrame;
		m_pCurrentFrame = pTemp;
	}

	// Set the data length for the output sample. 
	// The data length is the uncompressed frame size
	hr = pOut->SetActualDataLength(lOutDataLength);
	if (FAILED(hr))
		return hr;

	// Each YUV frame is a sync point
	hr = pOut->SetSyncPoint(TRUE);
	if (FAILED(hr))
		return hr;

	// YUV sample should never be a preroll one
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

HRESULT CROQVideoDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	ROQ_VIDEO_FORMAT *pFormat = (ROQ_VIDEO_FORMAT*)mtIn->Format();

	// Check for proper video media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Video			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_ROQVideo	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_ROQVideo			) &&
		(pFormat->wBlockDimension			== 8					) &&
		(pFormat->wSubBlockDimension		== 4					)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CROQVideoDecompressor::SetMediaType(
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
		m_pFormat = (ROQ_VIDEO_FORMAT*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());

		// Set up image parameters (strides)
		m_wYStride = m_pFormat->wWidth;
		m_wCStride = m_pFormat->wWidth / 2;
	}

	return NOERROR;
}

HRESULT CROQVideoDecompressor::BreakConnect(PIN_DIRECTION dir)
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

HRESULT CROQVideoDecompressor::CheckTransform(
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
		!IsEqualGUID(*mtOut->Subtype(),		MEDIASUBTYPE_YV12	) ||
		!IsEqualGUID(*mtOut->FormatType(),	FORMAT_VideoInfo	)
	)
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Get the media types' format blocks
	ROQ_VIDEO_FORMAT	*pInFormat	= (ROQ_VIDEO_FORMAT*)mtIn->Format();
	VIDEOINFO			*pOutFormat	= (VIDEOINFO*)mtOut->Format();

	// Check if the format length is enough
	if (mtOut->FormatLength() < sizeof(VIDEOINFO))
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check the compatibility of the formats
	DWORD cbFrame = (pInFormat->wWidth * pInFormat->wHeight * 12) / 8; // 12 bits per pixel
	return (
		//(pOutFormat->AvgTimePerFrame			== UNITS / pInFormat->nFramesPerSecond	) &&
		(pOutFormat->bmiHeader.biWidth			== (LONG)pInFormat->wWidth				) &&
		(pOutFormat->bmiHeader.biHeight			== (LONG)pInFormat->wHeight				) &&
		(pOutFormat->bmiHeader.biPlanes			== 1									) &&
		(pOutFormat->bmiHeader.biBitCount		== 12									) &&
		(pOutFormat->bmiHeader.biCompression	== BI_YV12								) &&
		(pOutFormat->bmiHeader.biSizeImage		== cbFrame								)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CROQVideoDecompressor::GetMediaType(
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
		pVideoInfo->bmiHeader.biWidth			= (LONG)m_pFormat->wWidth;
		pVideoInfo->bmiHeader.biHeight			= (LONG)m_pFormat->wHeight;
		pVideoInfo->bmiHeader.biPlanes			= 1;
		pVideoInfo->bmiHeader.biBitCount		= 12;
		pVideoInfo->bmiHeader.biCompression		= BI_YV12;
		pVideoInfo->bmiHeader.biSizeImage		= GetBitmapSize(&pVideoInfo->bmiHeader);
		pVideoInfo->bmiHeader.biXPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biYPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biClrUsed			= 0;
		pVideoInfo->bmiHeader.biClrImportant	= 0;
		pVideoInfo->dwBitRate					= pVideoInfo->bmiHeader.biSizeImage * 8 * m_pFormat->nFramesPerSecond;
		pVideoInfo->dwBitErrorRate				= 0;
		pVideoInfo->AvgTimePerFrame				= UNITS / m_pFormat->nFramesPerSecond;

		// Set media type fields
		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetSubtype(&MEDIASUBTYPE_YV12);
		pMediaType->SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);
		pMediaType->SetTemporalCompression(FALSE);
		pMediaType->SetFormatType(&FORMAT_VideoInfo);
	}

	return S_OK;
}

HRESULT CROQVideoDecompressor::DecideBufferSize(
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
	LONG cbFrame = (m_pFormat->wWidth * m_pFormat->wHeight * 12) / 8; // 12 bits per pixel
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

HRESULT CROQVideoDecompressor::StartStreaming(void)
{
	// We should have the format block at this time
	if (m_pFormat == NULL)
		return E_UNEXPECTED;

	// Allocate frame buffers
	DWORD cbFrame = m_pFormat->wWidth * m_pFormat->wHeight * 3;
	m_pPreviousFrame = (BYTE*)CoTaskMemAlloc(cbFrame);
	if (m_pPreviousFrame == NULL)
		return E_OUTOFMEMORY;
	m_pCurrentFrame = (BYTE*)CoTaskMemAlloc(cbFrame);
	if (m_pCurrentFrame == NULL) {
		CoTaskMemFree(m_pPreviousFrame);
		return E_OUTOFMEMORY;
	}

	// Zero frame buffers memory and the codebooks
	ZeroMemory(m_pPreviousFrame, cbFrame);
	ZeroMemory(m_pCurrentFrame, cbFrame);
	ZeroMemory(m_Cells, 256 * sizeof(ROQ_CELL));
	ZeroMemory(m_QCells, 256 * sizeof(ROQ_QCELL));

	return NOERROR;
}

HRESULT CROQVideoDecompressor::StopStreaming(void)
{
	// Free the previous frame buffer
	if (m_pPreviousFrame) {
		CoTaskMemFree(m_pPreviousFrame);
		m_pPreviousFrame = NULL;
	}

	// Free the current frame buffer
	if (m_pCurrentFrame) {
		CoTaskMemFree(m_pCurrentFrame);
		m_pCurrentFrame = NULL;
	}

	return NOERROR;
}

STDMETHODIMP CROQVideoDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_ROQVideoDecompressorPage;

	return NOERROR;
}

//==========================================================================
// ROQ video decoder methods implementation.
// Source code taken from roqvideo.c
// by Dr. Tim Ferguson (timf@csse.monash.edu.au)
//==========================================================================

void CROQVideoDecompressor::ApplyVector2x2(int x, int y, ROQ_CELL *cell)
{
	unsigned char *yptr;

	yptr = GetYPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	*yptr++ = cell->y0;
	*yptr++ = cell->y1;
	yptr += (m_wYStride - 2);
	*yptr++ = cell->y2;
	*yptr++ = cell->y3;

	unsigned char *uptr;

	uptr = GetUPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	*uptr++ = cell->u;
	*uptr++ = cell->u;
	uptr += (m_wYStride - 2);
	*uptr++ = cell->u;
	*uptr++ = cell->u;

	unsigned char *vptr;

	vptr = GetVPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	*vptr++ = cell->v;
	*vptr++ = cell->v;
	vptr += (m_wYStride - 2);
	*vptr++ = cell->v;
	*vptr++ = cell->v;

	/*
	GetUPlane(m_pCurrentFrame)[(y/2) * (m_wCStride) + x/2] = cell->u;
	GetVPlane(m_pCurrentFrame)[(y/2) * (m_wCStride) + x/2] = cell->v;
	*/
}

void CROQVideoDecompressor::ApplyVector4x4(int x, int y, ROQ_CELL *cell)
{
	unsigned long row_inc, c_row_inc;
	register unsigned char y0, y1, u, v;
	unsigned char *yptr, *uptr, *vptr;

	yptr = GetYPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	/*
	uptr = GetUPlane(m_pCurrentFrame) + (y/2) * (m_wCStride) + x/2;
	vptr = GetVPlane(m_pCurrentFrame) + (y/2) * (m_wCStride) + x/2;
	*/

	row_inc = m_wYStride - 4;
	c_row_inc = (m_wCStride) - 2;
	*yptr++ = y0 = cell->y0;
	/*
	*uptr++ = u = cell->u;
	*vptr++ = v = cell->v;
	*/
	*yptr++ = y0;
	*yptr++ = y1 = cell->y1;
	/*
	*uptr++ = u;
	*vptr++ = v;
	*/
	*yptr++ = y1;

	yptr += row_inc;

	*yptr++ = y0;
	*yptr++ = y0;
	*yptr++ = y1;
	*yptr++ = y1;

	yptr += row_inc;
	/*
	uptr += c_row_inc;
	vptr += c_row_inc;
	*/

	*yptr++ = y0 = cell->y2;
	/*
	*uptr++ = u;
	*vptr++ = v;
	*/
	*yptr++ = y0;
	*yptr++ = y1 = cell->y3;
	/*
	*uptr++ = u;
	*vptr++ = v;
	*/
	*yptr++ = y1;

	yptr += row_inc;

	*yptr++ = y0;
	*yptr++ = y0;
	*yptr++ = y1;
	*yptr++ = y1;

	uptr = GetUPlane(m_pCurrentFrame) + (y * m_wYStride) + x;

	*uptr++ = u = cell->u;
	*uptr++ = u;
	*uptr++ = u;
	*uptr++ = u;

	uptr += row_inc;

	*uptr++ = u;
	*uptr++ = u;
	*uptr++ = u;
	*uptr++ = u;

	uptr += row_inc;

	*uptr++ = u;
	*uptr++ = u;
	*uptr++ = u;
	*uptr++ = u;

	uptr += row_inc;

	*uptr++ = u;
	*uptr++ = u;
	*uptr++ = u;
	*uptr++ = u;

	vptr = GetVPlane(m_pCurrentFrame) + (y * m_wYStride) + x;

	*vptr++ = v = cell->v;
	*vptr++ = v;
	*vptr++ = v;
	*vptr++ = v;

	vptr += row_inc;

	*vptr++ = v;
	*vptr++ = v;
	*vptr++ = v;
	*vptr++ = v;

	vptr += row_inc;

	*vptr++ = v;
	*vptr++ = v;
	*vptr++ = v;
	*vptr++ = v;

	vptr += row_inc;

	*vptr++ = v;
	*vptr++ = v;
	*vptr++ = v;
	*vptr++ = v;
}


#define uiclp(i) ((i) < 0 ? 0 : ((i) > 255 ? 255 : (i)))
#define avg2(a,b) uiclp((((int)(a)+(int)(b)+1)>>1))
#define avg4(a,b,c,d) uiclp((((int)(a)+(int)(b)+(int)(c)+(int)(d)+2)>>2))

void CROQVideoDecompressor::ApplyMotion4x4(int x, int y, unsigned char mv, char mean_x, char mean_y)
{
	int i, mx, my /*, hw */;
	unsigned char *pa, *pb;

	mx = x + 8 - (mv >> 4) - mean_x;
	my = y + 8 - (mv & 0xf) - mean_y;

	pa = GetYPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	pb = GetYPlane(m_pPreviousFrame) + (my * m_wYStride) + mx;
	for (i = 0; i < 4; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += m_wYStride;
		pb += m_wYStride;
	}

	pa = GetUPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	pb = GetUPlane(m_pPreviousFrame) + (my * m_wYStride) + mx;
	for (i = 0; i < 4; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += m_wYStride;
		pb += m_wYStride;
	}
	pa = GetVPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	pb = GetVPlane(m_pPreviousFrame) + (my * m_wYStride) + mx;
	for (i = 0; i < 4; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += m_wYStride;
		pb += m_wYStride;
	}

/*
#if 0
	pa = GetUPlane(m_pCurrentFrame) + (y/2) * (m_wCStride) + x/2;
	pb = GetUPlane(m_pPreviousFrame) + (my/2) * (m_wCStride) + (mx + 1)/2;
	for (i = 0; i < 2; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa += m_wCStride;
		pb += m_wCStride;
	}

	pa = GetVPlane(m_pCurrentFrame) + (y/2) * (m_wCStride) + x/2;
	pb = GetVPlane(m_pPreviousFrame) + (my/2) * (m_wCStride) + (mx + 1)/2;
	for (i = 0; i < 2; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa += m_wCStride;
		pb += m_wCStride;
	}
#else
    hw = m_wYStride/2;
    pa = GetUPlane(m_pCurrentFrame) + (y * m_wYStride)/4 + x/2;
    pb = GetUPlane(m_pPreviousFrame) + (my/2) * (m_wYStride/2) + (mx + 1)/2;

    for (i = 0; i < 2; i++) {
        switch (((my & 0x01) << 1) | (mx & 0x01)) {

			case 0:
	            pa[0] = pb[0];
				pa[1] = pb[1];
				pa[hw] = pb[hw];
				pa[hw+1] = pb[hw+1];
				break;

			case 1:
	            pa[0] = avg2(pb[0], pb[1]);
				pa[1] = avg2(pb[1], pb[2]);
				pa[hw] = avg2(pb[hw], pb[hw+1]);
				pa[hw+1] = avg2(pb[hw+1], pb[hw+2]);
				break;

			case 2:
	            pa[0] = avg2(pb[0], pb[hw]);
				pa[1] = avg2(pb[1], pb[hw+1]);
				pa[hw] = avg2(pb[hw], pb[hw*2]);
				pa[hw+1] = avg2(pb[hw+1], pb[(hw*2)+1]);
				break;

			case 3:
	            pa[0] = avg4(pb[0], pb[1], pb[hw], pb[hw+1]);
				pa[1] = avg4(pb[1], pb[2], pb[hw+1], pb[hw+2]);
				pa[hw] = avg4(pb[hw], pb[hw+1], pb[hw*2], pb[(hw*2)+1]);
				pa[hw+1] = avg4(pb[hw+1], pb[hw+2], pb[(hw*2)+1], pb[(hw*2)+1]);
				break;
        }

        pa = GetVPlane(m_pCurrentFrame) + (y * m_wYStride)/4 + x/2;
        pb = GetVPlane(m_pPreviousFrame) + (my/2) * (m_wYStride/2) + (mx + 1)/2;
    }
#endif
*/
}

void CROQVideoDecompressor::ApplyMotion8x8(int x, int y, unsigned char mv, char mean_x, char mean_y)
{
	int mx, my, i/*, j, hw */;
	unsigned char *pa, *pb;

	mx = x + 8 - (mv >> 4) - mean_x;
	my = y + 8 - (mv & 0xf) - mean_y;

	pa = GetYPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	pb = GetYPlane(m_pPreviousFrame) + (my * m_wYStride) + mx;
	for (i = 0; i < 8; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa[4] = pb[4];
		pa[5] = pb[5];
		pa[6] = pb[6];
		pa[7] = pb[7];
		pa += m_wYStride;
		pb += m_wYStride;
	}

	pa = GetUPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	pb = GetUPlane(m_pPreviousFrame) + (my * m_wYStride) + mx;
	for (i = 0; i < 8; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa[4] = pb[4];
		pa[5] = pb[5];
		pa[6] = pb[6];
		pa[7] = pb[7];
		pa += m_wYStride;
		pb += m_wYStride;
	}
	pa = GetVPlane(m_pCurrentFrame) + (y * m_wYStride) + x;
	pb = GetVPlane(m_pPreviousFrame) + (my * m_wYStride) + mx;
	for (i = 0; i < 8; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa[4] = pb[4];
		pa[5] = pb[5];
		pa[6] = pb[6];
		pa[7] = pb[7];
		pa += m_wYStride;
		pb += m_wYStride;
	}

/*
#if 0
	pa = GetUPlane(m_pCurrentFrame) + (y/2) * (m_wCStride) + x/2;
	pb = GetUPlane(m_pPreviousFrame) + (my/2) * (m_wCStride) + (mx + 1)/2;
	for (i = 0; i < 4; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += m_wCStride;
		pb += m_wCStride;
	}

	pa = GetVPlane(m_pCurrentFrame) + (y/2) * (m_wCStride) + x/2;
	pb = GetVPlane(m_pPreviousFrame) + (my/2) * (m_wCStride) + (mx + 1)/2;
	for (i = 0; i < 4; i++) {
		pa[0] = pb[0];
		pa[1] = pb[1];
		pa[2] = pb[2];
		pa[3] = pb[3];
		pa += m_wCStride;
		pb += m_wCStride;
	}
#else
    hw = m_wCStride;
    pa = GetUPlane(m_pCurrentFrame) + (y * m_wYStride)/4 + x/2;
    pb = GetUPlane(m_pPreviousFrame) + (my/2) * (m_wYStride/2) + (mx + 1)/2;

    for (j = 0; j < 2; j++) {
        for (i = 0; i < 4; i++) {
            switch (((my & 0x01) << 1) | (mx & 0x01)) {

				case 0:
	                pa[0] = pb[0];
					pa[1] = pb[1];
					pa[2] = pb[2];
					pa[3] = pb[3];
					break;

				case 1:
	                pa[0] = avg2(pb[0], pb[1]);
					pa[1] = avg2(pb[1], pb[2]);
					pa[2] = avg2(pb[2], pb[3]);
					pa[3] = avg2(pb[3], pb[4]);
					break;
 
				case 2:
	                pa[0] = avg2(pb[0], pb[hw]);
					pa[1] = avg2(pb[1], pb[hw+1]);
					pa[2] = avg2(pb[2], pb[hw+2]);
					pa[3] = avg2(pb[3], pb[hw+3]);
					break;

				case 3:
	                pa[0] = avg4(pb[0], pb[1], pb[hw], pb[hw+1]);
					pa[1] = avg4(pb[1], pb[2], pb[hw+1], pb[hw+2]);
					pa[2] = avg4(pb[2], pb[3], pb[hw+2], pb[hw+3]);
					pa[3] = avg4(pb[3], pb[4], pb[hw+3], pb[hw+4]);
					break;
            }
            pa += m_wCStride;
            pb += m_wCStride;
        }

        pa = GetVPlane(m_pCurrentFrame) + (y * m_wYStride)/4 + x/2;
        pb = GetVPlane(m_pPreviousFrame) + (my/2) * (m_wYStride/2) + (mx + 1)/2;
    }
#endif
*/
}

#define RoQ_ID_MOT	0x00
#define RoQ_ID_FCC	0x01
#define RoQ_ID_SLD	0x02
#define RoQ_ID_CCC	0x03

void CROQVideoDecompressor::ROQVideoDecodeFrame(
	BYTE *pbData,
	DWORD dwSize,
	WORD wArgument
)
{
	int k, vqflg = 0, vqflg_pos = -1;
	int vqid, xpos, ypos, xp, yp, x, y;
	DWORD bpos;
	ROQ_QCELL *qcell;

	bpos = xpos = ypos = 0;
	while (bpos < dwSize) {
		for (yp = ypos; yp < ypos + 16; yp += 8)
			for (xp = xpos; xp < xpos + 16; xp += 8) {
				if (vqflg_pos < 0) {
					vqflg = pbData[bpos++];
					vqflg |= (pbData[bpos++] << 8);
					vqflg_pos = 7;
				}
				vqid = (vqflg >> (vqflg_pos * 2)) & 0x3;
				vqflg_pos--;

				switch (vqid) {
					case RoQ_ID_MOT:
						// Skip the block
						/* ApplyMotion8x8(xp, yp, 0, 8, 8); */
						break;
					case RoQ_ID_FCC:
						ApplyMotion8x8(xp, yp, pbData[bpos++], wArgument >> 8, wArgument & 0xff);
						break;
					case RoQ_ID_SLD:
						qcell = m_QCells + pbData[bpos++];
						ApplyVector4x4(xp, yp, m_Cells + qcell->idx[0]);
						ApplyVector4x4(xp+4, yp, m_Cells + qcell->idx[1]);
						ApplyVector4x4(xp, yp+4, m_Cells + qcell->idx[2]);
						ApplyVector4x4(xp+4, yp+4, m_Cells + qcell->idx[3]);
						break;
					case RoQ_ID_CCC:
						for (k = 0; k < 4; k++) {
							x = xp; y = yp;
							if (k & 0x01) x += 4;
							if (k & 0x02) y += 4;

							if (vqflg_pos < 0) {
								vqflg = pbData[bpos++];
								vqflg |= (pbData[bpos++] << 8);
								vqflg_pos = 7;
							}
							vqid = (vqflg >> (vqflg_pos * 2)) & 0x3;
							vqflg_pos--;
							switch (vqid) {
								case RoQ_ID_MOT:
									// Skip the block
									/* ApplyMotion4x4(x, y, 0, 8, 8); */
									break;
								case RoQ_ID_FCC:
									ApplyMotion4x4(x, y, pbData[bpos++], wArgument >> 8, wArgument & 0xff);
									break;
								case RoQ_ID_SLD:
									qcell = m_QCells + pbData[bpos++];
									ApplyVector2x2(x, y, m_Cells + qcell->idx[0]);
									ApplyVector2x2(x+2, y, m_Cells + qcell->idx[1]);
									ApplyVector2x2(x, y+2, m_Cells + qcell->idx[2]);
									ApplyVector2x2(x+2, y+2, m_Cells + qcell->idx[3]);
									break;
								case RoQ_ID_CCC:
									ApplyVector2x2(x, y, m_Cells + pbData[bpos]);
									ApplyVector2x2(x+2, y, m_Cells + pbData[bpos+1]);
									ApplyVector2x2(x, y+2, m_Cells + pbData[bpos+2]);
									ApplyVector2x2(x+2, y+2, m_Cells + pbData[bpos+3]);
									bpos += 4;
									break;
							}
						}
						break;
					default:
						// Unknown VQ code
						break;
				}
			}

		xpos += 16;
		if (xpos >= m_pFormat->wWidth) {
			xpos -= m_pFormat->wWidth;
			ypos += 16;
		}
		if (ypos >= m_pFormat->wHeight)
			break;
	}
}

void CROQVideoDecompressor::DownsampleColorPlanes(BYTE *pbInImage, BYTE *pbOutImage)
{
	// Copy Y plane intact
	CopyMemory(pbOutImage, pbInImage, m_pFormat->wWidth * m_pFormat->wHeight);

	// Downsample V plane
	pbInImage += m_pFormat->wWidth * m_pFormat->wHeight;
	pbOutImage += m_pFormat->wWidth * m_pFormat->wHeight;
	for (int y = 0; y < m_pFormat->wHeight / 2; y++, pbInImage += m_wYStride)
		for (int x = 0; x < m_pFormat->wWidth / 2; x++, pbInImage += 2)
			*pbOutImage++ = avg4(
				pbInImage[0], 
				pbInImage[1], 
				pbInImage[m_wYStride], 
				pbInImage[m_wYStride + 1]
			);

	// Downsample U plane
	int y = 0;
	for (y = 0; y < m_pFormat->wHeight / 2; y++, pbInImage += m_wYStride)
		for (int x = 0; x < m_pFormat->wWidth / 2; x++, pbInImage += 2)
			*pbOutImage++ = avg4(
				pbInImage[0], 
				pbInImage[1], 
				pbInImage[m_wYStride], 
				pbInImage[m_wYStride + 1]
			);
}

//==========================================================================
// CROQVideoDecompressorPage methods
//==========================================================================

const WCHAR g_wszROQVideoDecompressorPageName[] = L"ANX ROQ Video Decompressor Property Page";

CROQVideoDecompressorPage::CROQVideoDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("ROQ Video Decompressor Property Page"),
		pUnk,
		IDD_ROQVIDEODECOMPRESSORPAGE,
		IDS_TITLE_ROQVIDEODECOMPRESSORPAGE
	)
{
}

CUnknown* WINAPI CROQVideoDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CROQVideoDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}

