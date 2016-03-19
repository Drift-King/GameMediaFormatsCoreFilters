//==========================================================================
//
// File: CINVideoDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of CIN video decompressor
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

#include "CINGUID.h"
#include "CINVideoDecompressor.h"
#include "FilterOptions.h"
#include "resource.h"

#include <cguid.h>

//==========================================================================
// CIN video decompressor setup data
//==========================================================================

const WCHAR g_wszCINVideoDecompressorName[] = L"ANX CIN Video Decompressor";

const AMOVIESETUP_MEDIATYPE sudCINVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_CINVideo
};

const AMOVIESETUP_MEDIATYPE sudRGBType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_RGB8
};

const AMOVIESETUP_PIN sudCINVideoDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudCINVideoType	// Media types
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
		&sudRGBType			// Media types
	}
};

const AMOVIESETUP_FILTER g_sudCINVideoDecompressor = {
	&CLSID_CINVideoDecompressor,	// CLSID of filter
	g_wszCINVideoDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudCINVideoDecompressorPins		// Pin information
};

//==========================================================================
// CCINVideoDecompressor methods
//==========================================================================

CCINVideoDecompressor::CCINVideoDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("CIN Video Decompressor"),
		pUnk,
		CLSID_CINVideoDecompressor
	),
	m_pFormat(NULL)	// No format block at this time
{
	ASSERT(phr);
}

CCINVideoDecompressor::~CCINVideoDecompressor()
{
	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
	}
}

CUnknown* WINAPI CCINVideoDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CCINVideoDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CCINVideoDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

HRESULT CCINVideoDecompressor::Transform(
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

	// Check if the frame contains new palette
	BYTE *pbHuffmanData = NULL;
	if (*((DWORD*)pbInBuffer) == CIN_COMMAND_PALETTE) {

		// Get the palette pointer from the input data
		BYTE *pbPalette = pbInBuffer + 4;

		// Get the output media type format
		CMediaType mt((AM_MEDIA_TYPE)m_pOutput->CurrentMediaType());
		VIDEOINFO *pVideoInfo = (VIDEOINFO*)mt.Format();

		// Fill in the output media type format palette
		for (int i = 0; i < 256; i++) {
			pVideoInfo->bmiColors[i].rgbRed			= *pbPalette++;
			pVideoInfo->bmiColors[i].rgbGreen		= *pbPalette++;
			pVideoInfo->bmiColors[i].rgbBlue		= *pbPalette++;
			pVideoInfo->bmiColors[i].rgbReserved	= 0;
		}

		// Set the changed media type for the output sample
		hr = pOut->SetMediaType(&mt);
		if (FAILED(hr))
			return hr;

		// Set up Huffman data pointer
		pbHuffmanData = pbPalette;

	} else
		pbHuffmanData = pbInBuffer + 4;

	// Set up Huffman count
	LONG lHuffmanCount = pIn->GetActualDataLength();
	lHuffmanCount -= (LONG)(pbHuffmanData - pbInBuffer);

	// Get the output sample's buffer
	BYTE *pbOutBuffer = NULL;
	hr = pOut->GetPointer(&pbOutBuffer);
	if (FAILED(hr))
		return hr;

	// Call the decoder function to decompress the frame
	if (!HuffmanDecode(pbHuffmanData, lHuffmanCount, pbOutBuffer))
		return E_FAIL;

	// Set the data length for the output sample. 
	// The data length is the uncompressed frame size
	LONG lOutDataLength = m_pFormat->dwVideoWidth * m_pFormat->dwVideoHeight;
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

	// We rely on the upstream filter (which is most likely 
	// a parser or splitter) in the matter of stream and media 
	// times setting. As to the discontinuity property, we should
	// not drop samples, so we just retain this property's value 
	// set by the upstream filter

	return NOERROR;
}

HRESULT CCINVideoDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	// Check for proper video media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Video			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_CINVideo	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_CINVideo			)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CCINVideoDecompressor::SetMediaType(
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
		m_pFormat = (CIN_HEADER*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());

		// Set up Huffman nodes and tree
		for (int i = 0; i < 256; i++) {
			for (int j = 0; j < HUFFMAN_TOKENS; j++)
				m_HuffmanNodes[i][j].iCount = (int)m_pFormat->bHuffmanTable[i][j];
			HuffmanBuildTree(i);
		}
	}

	return NOERROR;
}

HRESULT CCINVideoDecompressor::BreakConnect(PIN_DIRECTION dir)
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

HRESULT CCINVideoDecompressor::CheckTransform(
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
		!IsEqualGUID(*mtOut->Subtype(),		MEDIASUBTYPE_RGB8	) ||
		!IsEqualGUID(*mtOut->FormatType(),	FORMAT_VideoInfo	)
	)
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Get the media types' format blocks
	CIN_HEADER	*pInFormat	= (CIN_HEADER*)mtIn->Format();
	VIDEOINFO	*pOutFormat	= (VIDEOINFO*)mtOut->Format();

	// Check if the format length is enough
	if (mtOut->FormatLength() < sizeof(VIDEOINFO))
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check the compatibility of the formats
	DWORD cbFrame = pInFormat->dwVideoWidth * pInFormat->dwVideoHeight;
	return (
		//(pOutFormat->AvgTimePerFrame			== UNITS / CIN_FPS					) &&
		(pOutFormat->bmiHeader.biWidth			== (LONG)pInFormat->dwVideoWidth	) &&
		(pOutFormat->bmiHeader.biHeight			== -(LONG)pInFormat->dwVideoHeight	) &&
		(pOutFormat->bmiHeader.biPlanes			== 1								) &&
		(pOutFormat->bmiHeader.biBitCount		== 8								) &&
		(pOutFormat->bmiHeader.biCompression	== BI_RGB							) &&
		(pOutFormat->bmiHeader.biSizeImage		== cbFrame							)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CCINVideoDecompressor::GetMediaType(
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
		pVideoInfo->bmiHeader.biWidth			= (LONG)m_pFormat->dwVideoWidth;
		pVideoInfo->bmiHeader.biHeight			= -(LONG)m_pFormat->dwVideoHeight;
		pVideoInfo->bmiHeader.biPlanes			= 1;
		pVideoInfo->bmiHeader.biBitCount		= 8;
		pVideoInfo->bmiHeader.biCompression		= BI_RGB;
		pVideoInfo->bmiHeader.biSizeImage		= GetBitmapSize(&pVideoInfo->bmiHeader);
		pVideoInfo->bmiHeader.biXPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biYPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biClrUsed			= 256;
		pVideoInfo->bmiHeader.biClrImportant	= 0;
		pVideoInfo->dwBitRate					= pVideoInfo->bmiHeader.biSizeImage * 8 * CIN_FPS;
		pVideoInfo->dwBitErrorRate				= 0;
		pVideoInfo->AvgTimePerFrame				= UNITS / CIN_FPS;

		// Set media type fields
		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetSubtype(&MEDIASUBTYPE_RGB8);
		pMediaType->SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);
		pMediaType->SetTemporalCompression(FALSE);
		pMediaType->SetFormatType(&FORMAT_VideoInfo);
	}

	return S_OK;
}

HRESULT CCINVideoDecompressor::DecideBufferSize(
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
	LONG cbFrame = m_pFormat->dwVideoWidth * m_pFormat->dwVideoHeight;
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

STDMETHODIMP CCINVideoDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_CINVideoDecompressorPage;

	return NOERROR;
}

//==========================================================================
// Huffman decoder methods implementation.
// Source code taken from xcinplay program 
// by Dr. Tim Ferguson (timf@csse.monash.edu.au)
//==========================================================================

/* -------------------------------------------------------------------------
 *  Decodes input Huffman data using the Huffman table.
 *
 *  Input:   data = encoded data to be decoded.
 *           len = length of encoded data.
 *           image = a buffer for the decoded image data.
 */
BOOL CCINVideoDecompressor::HuffmanDecode(BYTE *data, long len, BYTE *image)
{
	CIN_HUFFMAN_NODE *hnodes;
	long i, dec_len;
	int prev;
	register BYTE v = 0;
	register int bit_pos, node_num, dat_pos;

	/* read count */
	dec_len  = (*data++ & 0xff);
	dec_len |= (*data++ & 0xff) << 8;
	dec_len |= (*data++ & 0xff) << 16;
	dec_len |= (*data++ & 0xff) << 24;

	prev = bit_pos = dat_pos = 0;
	for (i = 0; i < dec_len; i++) {

		node_num = m_nHuffmanNodes[prev];
		hnodes = m_HuffmanNodes[prev];

		while (node_num >= HUFFMAN_TOKENS) {

			if (!bit_pos) {
				if (dat_pos > len) {
					// Huffman decode error
					return FALSE;
				}
				bit_pos = 8;
				v = data[dat_pos++];
			}

			node_num = hnodes[node_num].iChildren[v & 0x01];
			v = v >> 1;
			bit_pos--;
		}

		*image++ = prev = node_num;
	}

	return TRUE;
}

/* -------------------------------------------------------------------------
 *  Find the lowest probability node in a Huffman table, and mark it as
 *  being assigned to a higher probability.
 *  Returns the node index of the lowest unused node, or -1 if all nodes
 *  are used.
 */
int CCINVideoDecompressor::HuffmanSmallestNode(CIN_HUFFMAN_NODE *hnodes, int num_hnodes)
{
	int i;
	int best, best_node;

	best = 99999999;
	best_node = -1;
	for (i = 0; i < num_hnodes; i++) {

		if (hnodes[i].bUsed) continue;
		if (!hnodes[i].iCount) continue;
		if (hnodes[i].iCount < best) {
			best = hnodes[i].iCount;
			best_node = i;
		}
	}

	if (best_node == -1) return -1;
	hnodes[best_node].bUsed = 1;
	return best_node;
}

/* -------------------------------------------------------------------------
 *  Build the Huffman tree using the generated/loaded probabilities histogram.
 *
 *  On completion:
 *   m_HuffmanNodes[prev][i < HUFFMAN_TOKENS] - are the nodes at the base of the tree.
 *   m_HuffmanNodes[prev][i >= HUFFMAN_TOKENS] - are used to construct the tree.
 *   m_nHuffmanNodes[prev] - contains the index to the root node of the tree.
 *     That is: m_HuffmanNodes[prev][m_nHuffmanNodes[prev]] is the root node.
 */
void CCINVideoDecompressor::HuffmanBuildTree(int prev)
{
	CIN_HUFFMAN_NODE *node, *hnodes;
	int num_hnodes, i;

	num_hnodes = HUFFMAN_TOKENS;
	hnodes = m_HuffmanNodes[prev];
	for (i = 0; i < HUFFMAN_TOKENS * 2; i++) hnodes[i].bUsed = 0;

	while (1) {

		node = &hnodes[num_hnodes];	/* next free node */

		/* pick two lowest counts */
		node->iChildren[0] = HuffmanSmallestNode(hnodes, num_hnodes);
		if (node->iChildren[0] == -1) break;	/* reached the root node */

		node->iChildren[1] = HuffmanSmallestNode(hnodes, num_hnodes);
		if (node->iChildren[1] == -1) break;	/* reached the root node */

		/* combine nodes probability for new node */
		node->iCount = hnodes[node->iChildren[0]].iCount + hnodes[node->iChildren[1]].iCount;
		num_hnodes++;
	}

	m_nHuffmanNodes[prev] = num_hnodes - 1;
}

//==========================================================================
// CCINVideoDecompressorPage methods
//==========================================================================

const WCHAR g_wszCINVideoDecompressorPageName[] = L"ANX CIN Video Decompressor Property Page";

CCINVideoDecompressorPage::CCINVideoDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("CIN Video Decompressor Property Page"),
		pUnk,
		IDD_CINVIDEODECOMPRESSORPAGE,
		IDS_TITLE_CINVIDEODECOMPRESSORPAGE
	)
{
}

CUnknown* WINAPI CCINVideoDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CCINVideoDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
