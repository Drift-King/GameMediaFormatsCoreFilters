//==========================================================================
//
// File: HNMVideoDecompressor.cpp
//
// Desc: Game Media Formats - Implementation of HNM video decompressor
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

#include "HNMGUID.h"
#include "HNMVideoDecompressor.h"
#include "FilterOptions.h"
#include "resource.h"

#include <cguid.h>
#include <string.h>
#include <tchar.h>

//==========================================================================
// HNM video decompressor setup data
//==========================================================================

const WCHAR g_wszHNMVideoDecompressorName[] = L"ANX HNM Video Decompressor";

const AMOVIESETUP_MEDIATYPE sudHNMVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_HNMVideo
};

const AMOVIESETUP_MEDIATYPE sudRGBTypes[] = {
	{	// 15-bit RGB (555)
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB555
	},
	{	// 16-bit RGB (565)
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB565
	}
};

const AMOVIESETUP_PIN sudHNMVideoDecompressorPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudHNMVideoType	// Media types
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
		sudRGBTypes			// Media types
	}
};

const AMOVIESETUP_FILTER g_sudHNMVideoDecompressor = {
	&CLSID_HNMVideoDecompressor,	// CLSID of filter
	g_wszHNMVideoDecompressorName,	// Filter name
	MERIT_NORMAL,					// Filter merit
	2,								// Number of pins
	sudHNMVideoDecompressorPins		// Pin information
};

//==========================================================================
// CHNMVideoDecompressor methods
//==========================================================================

CHNMVideoDecompressor::CHNMVideoDecompressor(
	LPUNKNOWN pUnk,
	HRESULT *phr
) :
	CTransformFilter(
		NAME("HNM Video Decompressor"),
		pUnk,
		CLSID_HNMVideoDecompressor
	),
	m_hDecoderDLL(NULL),		// No decoder DLL at this time
	HNMPI_Init(NULL),			// |
	HNMPI_DecodeFrame(NULL),	// |-- No decoder functions at this time
	HNMPI_Cleanup(NULL),		// |
	m_pFormat(NULL),			// No format block at this time
	m_nFramesPerSecond(0),		// No frame rate at this time
	m_pPreviousFrame(NULL)		// No previous frame buffer at this time
{
	ASSERT(phr);

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Retrieve the bRequireConverter option from the registry
		m_bRequireConverter = TRUE; // Default value
		*phr = RegGetFilterOptionDWORD(
			g_wszHNMVideoDecompressorName,
			TEXT("Require Converter"),
			(DWORD*)&m_bRequireConverter
		);
	}
}

CHNMVideoDecompressor::~CHNMVideoDecompressor()
{
	// Perform decoder clean-up
	Cleanup();
	
	// Free the format block
	if (m_pFormat) {
		CoTaskMemFree(m_pFormat);
		m_pFormat = NULL;
	}

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Store the bRequireConverter option in the registry
		RegSetFilterOptionValue(
			g_wszHNMVideoDecompressorName,
			TEXT("Require Converter"),
			REG_DWORD,
			(LPBYTE)&m_bRequireConverter,
			sizeof(BOOL)
		);
	}
}

void CHNMVideoDecompressor::Cleanup(void)
{
	// Clean up the decoder
	if (HNMPI_Cleanup)
		HNMPI_Cleanup();

	// Free decoder DLL
	if (m_hDecoderDLL) {
		FreeLibrary(m_hDecoderDLL);
		m_hDecoderDLL = NULL;
	}

	// Reset the decoder DLL function pointers
	HNMPI_Init			= NULL;
	HNMPI_DecodeFrame	= NULL;
	HNMPI_Cleanup		= NULL;	

	// Free the previous frame buffer
	if (m_pPreviousFrame) {
		CoTaskMemFree(m_pPreviousFrame);
		m_pPreviousFrame = NULL;
	}
}

CUnknown* WINAPI CHNMVideoDecompressor::CreateInstance(
	LPUNKNOWN pUnk,
	HRESULT *phr
)
{
	CUnknown* pObject = new CHNMVideoDecompressor(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CHNMVideoDecompressor::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	// Check and validate the pointer
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	// Expose IConfigHNMVideoDecompressor, ISpecifyPropertyPages 
	// (and the base-class interfaces)
	if (riid == IID_IConfigHNMVideoDecompressor)
		return GetInterface((IConfigHNMVideoDecompressor*)this, ppv);
	else if (riid == IID_ISpecifyPropertyPages)
		return GetInterface((ISpecifyPropertyPages*)this, ppv);
	else
		return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CHNMVideoDecompressor::Transform(
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

	// Get the output sample's buffer
	BYTE *pbOutBuffer = NULL;
	hr = pOut->GetPointer(&pbOutBuffer);
	if (FAILED(hr))
		return hr;

	// Call the decoder function to decompress the frame
	if (!HNMPI_DecodeFrame(pbInBuffer, m_pPreviousFrame, pbOutBuffer))
		return E_FAIL;

	// Copy current frame data to the previous frame buffer
	LONG lOutDataLength = m_pFormat->wWidth * m_pFormat->wHeight * 2;
	CopyMemory(m_pPreviousFrame, pbOutBuffer, lOutDataLength);

	// Set the data length for the output sample. 
	// The data length is the uncompressed frame size
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

HRESULT CHNMVideoDecompressor::CheckInputType(const CMediaType *mtIn)
{
	// Check and validate the pointer
	CheckPointer(mtIn, E_POINTER);
	ValidateReadPtr(mtIn, sizeof(CMediaType));

	// Check for proper video media type
	return (
		IsEqualGUID(*mtIn->Type(),			MEDIATYPE_Video			) &&
		IsEqualGUID(*mtIn->Subtype(),		MEDIASUBTYPE_HNMVideo	) &&
		IsEqualGUID(*mtIn->FormatType(),	FORMAT_HNMVideo			)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CHNMVideoDecompressor::SetMediaType(
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
		m_pFormat = (HNM_HEADER*)CoTaskMemAlloc(pmt->FormatLength());
		if (m_pFormat == NULL)
			return E_OUTOFMEMORY;

		// Copy format block to the allocated storage
		CopyMemory(m_pFormat, pmt->Format(), pmt->FormatLength());

		// Get the frame rate and put the zero value back
		m_nFramesPerSecond = m_pFormat->wUnknown1;
		m_pFormat->wUnknown1 = 0; // TEMP: Is it really so?
	}

	return NOERROR;
}

HRESULT CHNMVideoDecompressor::CheckConnect(PIN_DIRECTION dir, IPin *pPin)
{
	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	// If the option says, check that our output pin connects to 
	// the color space converter
	if (
		(m_bRequireConverter) &&
		(dir == PINDIR_OUTPUT)
	) {
		// Query the connecting pin info
		PIN_INFO pininfo;
		HRESULT hr = pPin->QueryPinInfo(&pininfo);
		if (FAILED(hr))
			return hr;

		// The connecting pin should be owned by a filter
		if (pininfo.pFilter == NULL)
			return E_UNEXPECTED;

		// Get the downstream filter's CLSID
		CLSID clsid;
		hr = pininfo.pFilter->GetClassID(&clsid);
		pininfo.pFilter->Release();
		if (FAILED(hr))
			return hr;

		// Check if the downstream filter is the color space converter
		return (IsEqualCLSID(clsid, CLSID_Colour) ? NOERROR : E_FAIL);
	}

	return NOERROR;
}

HRESULT CHNMVideoDecompressor::BreakConnect(PIN_DIRECTION dir)
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

		m_nFramesPerSecond = 0;
	}

	return NOERROR;
}

HRESULT CHNMVideoDecompressor::CheckTransform(
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
	HNM_HEADER	*pInFormat	= (HNM_HEADER*)mtIn->Format();
	VIDEOINFO	*pOutFormat	= (VIDEOINFO*)mtOut->Format();

	// Check if the format length is enough
	if (mtOut->FormatLength() < sizeof(VIDEOINFO))
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check if the bitfields are set correctly
	if (memcmp(
			pOutFormat->TrueColorInfo.dwBitMasks, 
			(pInFormat->bBPP == 15) ? bits555 : bits565, 
			3 * sizeof(DWORD)
		)
	)
		return VFW_E_TYPE_NOT_ACCEPTED;

	// Check the compatibility of the formats
	DWORD cbFrame = pInFormat->wWidth * pInFormat->wHeight * 2;
	return (
		//(pOutFormat->AvgTimePerFrame			== UNITS / pInFormat->wUnknown1	) &&
		(pOutFormat->bmiHeader.biWidth			== (LONG)pInFormat->wWidth	) &&
		(pOutFormat->bmiHeader.biHeight			== -(LONG)pInFormat->wHeight) &&
		(pOutFormat->bmiHeader.biPlanes			== 1						) &&
		(pOutFormat->bmiHeader.biBitCount		== 16						) &&
		(pOutFormat->bmiHeader.biCompression	== BI_BITFIELDS				) &&
		(pOutFormat->bmiHeader.biSizeImage		== cbFrame					)
	) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CHNMVideoDecompressor::GetMediaType(
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
		pVideoInfo->bmiHeader.biHeight			= -(LONG)m_pFormat->wHeight;
		pVideoInfo->bmiHeader.biPlanes			= 1;
		pVideoInfo->bmiHeader.biBitCount		= 16;
		pVideoInfo->bmiHeader.biCompression		= BI_BITFIELDS;
		pVideoInfo->bmiHeader.biSizeImage		= GetBitmapSize(&pVideoInfo->bmiHeader);
		pVideoInfo->bmiHeader.biXPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biYPelsPerMeter	= 0;
		pVideoInfo->bmiHeader.biClrUsed			= 0;
		pVideoInfo->bmiHeader.biClrImportant	= 0;
		pVideoInfo->dwBitRate					= pVideoInfo->bmiHeader.biSizeImage * 8 * m_nFramesPerSecond;
		pVideoInfo->dwBitErrorRate				= 0;
		pVideoInfo->AvgTimePerFrame				= UNITS / m_nFramesPerSecond;

		// Set the bitmasks
		if (m_pFormat->bBPP == 15) {
			for (int i = 0; i < 3; i++)
				pVideoInfo->TrueColorInfo.dwBitMasks[i] = bits555[i];
		} else if (m_pFormat->bBPP == 16) {
			for (int i = 0; i < 3; i++)
				pVideoInfo->TrueColorInfo.dwBitMasks[i] = bits565[i];
		} else // This should never happen
			ASSERT(false);

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

HRESULT CHNMVideoDecompressor::DecideBufferSize(
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
	LONG cbFrame = m_pFormat->wWidth * m_pFormat->wHeight * 2;
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

HRESULT CHNMVideoDecompressor::StartStreaming(void)
{
	// We should have the format block at this time
	if (m_pFormat == NULL)
		return E_UNEXPECTED;

	// Work out the filename of the decoder DLL
	TCHAR szDecoderName[MAX_PATH];
	wsprintf(szDecoderName, "CM6_%dx16.dll", m_pFormat->wWidth);

	// Get the full pathname of this module
	TCHAR szTemp[MAX_PATH];
	if (GetModuleFileName(g_hInst, szTemp, sizeof(szTemp) / sizeof(TCHAR)) == 0)
		return E_FAIL;

	// Compose the decoder DLL pathname
	TCHAR *pNameStart = _tcsrchr(szTemp, '\\');
	if (pNameStart == NULL)
		pNameStart = szTemp;	// Start right from the beginning
	else
		pNameStart++;			// Advance to the filename start
	lstrcpy(pNameStart, szDecoderName);

	// Load decoder DLL
	m_hDecoderDLL = LoadLibrary(szTemp);
	if (m_hDecoderDLL == NULL)
		return E_FAIL;

	// Retrieve decoder functions pointers
	HNMPI_Init			= (BOOL (__cdecl *) (DWORD, DWORD, DWORD, PVOID))GetProcAddress(m_hDecoderDLL, "HNMPI_Init");
	HNMPI_DecodeFrame	= (BOOL (__cdecl *) (PVOID, PVOID, PVOID))GetProcAddress(m_hDecoderDLL, "HNMPI_DecodeFrame");
	HNMPI_Cleanup		= (BOOL (__cdecl *) (void))GetProcAddress(m_hDecoderDLL, "HNMPI_Cleanup");
	if (
		(HNMPI_Init			== NULL) ||
		(HNMPI_DecodeFrame	== NULL) ||
		(HNMPI_Cleanup		== NULL)
	) {
		Cleanup();
		return E_FAIL;
	}

	// Allocate a previous frame buffer
	DWORD cbFrame = m_pFormat->wWidth * m_pFormat->wHeight * 2;
	m_pPreviousFrame = (BYTE*)CoTaskMemAlloc(cbFrame);
	if (m_pPreviousFrame == NULL) {
		Cleanup();
		return E_OUTOFMEMORY;
	}

	// Initialize the decoder
	if (
		!HNMPI_Init(
			(DWORD)m_pFormat->bBPP,
			(DWORD)m_pFormat->wHeight,
			(DWORD)m_pFormat->wWidth,
			m_pFormat
		)
	) {
		Cleanup();
		return E_FAIL;
	}

	return NOERROR;
}

HRESULT CHNMVideoDecompressor::StopStreaming(void)
{
	// Perform decoder clean-up
	Cleanup();

	return NOERROR;
}

STDMETHODIMP CHNMVideoDecompressor::get_RequireConverter(BOOL *pbRequireConverter)
{
	// Check and validate the pointer
	CheckPointer(pbRequireConverter, E_POINTER);
	ValidateWritePtr(pbRequireConverter, sizeof(BOOL));

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	*pbRequireConverter = m_bRequireConverter;

	return NOERROR;
}

STDMETHODIMP CHNMVideoDecompressor::put_RequireConverter(BOOL bRequireConverter)
{
	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	m_bRequireConverter = bRequireConverter;

	return NOERROR;
}

STDMETHODIMP CHNMVideoDecompressor::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_HNMVideoDecompressorPage;

	return NOERROR;
}

//==========================================================================
// CHNMVideoDecompressorPage methods
//==========================================================================

const WCHAR g_wszHNMVideoDecompressorPageName[] = L"ANX HNM Video Decompressor Property Page";

CHNMVideoDecompressorPage::CHNMVideoDecompressorPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("HNM Video Decompressor Property Page"),
		pUnk,
		IDD_HNMVIDEODECOMPRESSORPAGE,
		IDS_TITLE_HNMVIDEODECOMPRESSORPAGE
	),
	m_pConfig(NULL) // No config interface at this time
{
}

CUnknown* WINAPI CHNMVideoDecompressorPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CHNMVideoDecompressorPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}

BOOL CHNMVideoDecompressorPage::OnReceiveMessage(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMsg) {

		case WM_COMMAND:

			// If we clicked on the checkbox then set the dirty flag
			if (LOWORD(wParam) == IDC_REQUIRECONVERTER) {
				m_bDirty = TRUE;
				// Notify the page site of the dirty state
				if (m_pPageSite)
					m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
			}

			return (LRESULT)1;
	}

	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CHNMVideoDecompressorPage::OnConnect(IUnknown *pUnknown)
{
	// At this time there should be no config interface
	ASSERT(m_pConfig == NULL);

	// Retrieve the config interface from the filter
	HRESULT hr = pUnknown->QueryInterface(
		IID_IConfigHNMVideoDecompressor,
		(void**)&m_pConfig
	);
	if (FAILED(hr))
		return hr;

	ASSERT(m_pConfig);

	return NOERROR;
}

HRESULT CHNMVideoDecompressorPage::OnDisconnect()
{
	// At this time we should have the config interface
	if (m_pConfig == NULL)
		return E_UNEXPECTED;

	// Release the config interface
	m_pConfig->Release();
	m_pConfig = NULL;

	return NOERROR;
}

HRESULT CHNMVideoDecompressorPage::OnActivate()
{
	// At this time we should have the config interface
	if (m_pConfig == NULL)
		return E_UNEXPECTED;

	// Get the option value from the filter
	BOOL bRequireConverter;
	HRESULT hr = m_pConfig->get_RequireConverter(&bRequireConverter);
	if (FAILED(hr))
		return hr;

	// Set the checkbox appropriately
	CheckDlgButton(m_Dlg, IDC_REQUIRECONVERTER, (bRequireConverter) ? BST_CHECKED : BST_UNCHECKED);

	m_bDirty = FALSE;

	return NOERROR;
}

HRESULT CHNMVideoDecompressorPage::OnApplyChanges()
{
	if (m_bDirty) {

		// Get the checkbox state 
		BOOL bRequireConverter = IsDlgButtonChecked(m_Dlg, IDC_REQUIRECONVERTER) == BST_CHECKED;

		// At this time we should have the config interface
		if (m_pConfig == NULL)
			return E_UNEXPECTED;

		// Put the option on the filter
		HRESULT hr = m_pConfig->put_RequireConverter(bRequireConverter);
		if (FAILED(hr))
			return hr;

		m_bDirty = FALSE;
	}

	return NOERROR;
}
