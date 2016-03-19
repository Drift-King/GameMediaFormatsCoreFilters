//==========================================================================
//
// File: BasePlainParser.cpp
//
// Desc: Game Media Formats - Implementation of generic plain parser filter
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

#include "BasePlainParser.h"

CBasePlainParserFilter::CBasePlainParserFilter(
	TCHAR *pName,
	LPUNKNOWN pUnk,
	REFCLSID clsid,
	const WCHAR *wszFilterName,
	UINT nMediaTypes,
	const REGPINTYPES *pMediaType,
	HRESULT *phr
) :
	CBaseParserFilter(pName, pUnk, clsid, wszFilterName, phr),
	m_nMediaTypes(nMediaTypes),		// Number of input media types
	m_pMediaType(pMediaType),		// Array of input media types
	m_lPacketSize(1),				// No packeting at this time
	m_lPacketLength(0),				// No packet data at this time
	m_pbPacketStorage(NULL)			// No packet storage at this time
{
	ASSERT(nMediaTypes > 0);
	ASSERT(pMediaType);
}

CBasePlainParserFilter::~CBasePlainParserFilter()
{
	// Free any resources allocated by Initialize() or InitializeParser()
	Shutdown();
}

HRESULT CBasePlainParserFilter::Initialize(IPin *pPin, IAsyncReader *pReader)
{
	// Check and validate the pointer
	CheckPointer(pReader, E_POINTER);
	ValidateReadPtr(pReader, sizeof(IAsyncReader));

	// Allocate media type
	CMediaType *pmt = new CMediaType();
	if (pmt == NULL)
		return E_OUTOFMEMORY;

	// Allocate the allocator properties
	ALLOCATOR_PROPERTIES *pap = (ALLOCATOR_PROPERTIES*)CoTaskMemAlloc(sizeof(ALLOCATOR_PROPERTIES));
	if (pap == NULL) {
		delete pmt;
		return E_OUTOFMEMORY;
	}

	// Parse file header
	DWORD dwCapabilities = 0;
	int nTimeFormats = 0;
	GUID *pTimeFormats = NULL;
	HRESULT hr = Initialize(
		pPin,
		pReader,
		pmt,
		pap,
		&dwCapabilities,
		&nTimeFormats,
		&pTimeFormats
	);
	if (FAILED(hr)) {
		delete pmt;
		CoTaskMemFree(pap);
		if (pTimeFormats)
			CoTaskMemFree(pTimeFormats);
		Shutdown();
		return hr;
	}

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	m_nOutputPins = 1; // The only output pin
	
	// Create output pins array
	ASSERT(m_ppOutputPin == NULL);
	m_ppOutputPin = new CParserOutputPin*[m_nOutputPins];
	if (m_ppOutputPin == NULL) {
		m_nOutputPins = 0;
		delete pmt;
		CoTaskMemFree(pap);
		if (pTimeFormats)
			CoTaskMemFree(pTimeFormats);
		Shutdown();
		return E_OUTOFMEMORY;
	}

	// Provide the additional space for previously collected packet
	pap->cbBuffer += m_lPacketSize;

	// Create the output pin according to info provided by Initialize()
	hr = NOERROR;
	m_ppOutputPin[0] = new CParserOutputPin(
		NAME("Plain Parser Output Pin"),
		this,
		&m_csFilter,
		pmt,
		pap,
		dwCapabilities,
		nTimeFormats,
		pTimeFormats,
		&hr,
		L"Output"
	);
	if (
		(FAILED(hr)) ||
		(m_ppOutputPin[0] == NULL)
	) {
		if (m_ppOutputPin[0]) {
			delete m_ppOutputPin[0];
			m_ppOutputPin[0] = NULL;
		} else {
			delete pmt;
			CoTaskMemFree(pap);
			if (pTimeFormats)
				CoTaskMemFree(pTimeFormats);
		}
		Shutdown();

		if (FAILED(hr))
			return hr;
		else
			return E_OUTOFMEMORY;
	}
	// Hold a reference on the pin
	m_ppOutputPin[0]->AddRef();

	// We've created a new pin -- so increment pin version
	IncrementPinVersion();

	return NOERROR;
}

HRESULT CBasePlainParserFilter::Shutdown(void)
{
	// Shutdown the parser
	HRESULT hr = ShutdownParser();
	if (FAILED(hr))
		return hr;

	// Scope for the locking
	{
		// Protect the filter data
		CAutoLock datalock(&m_csData);

		// Reset the packet size
		m_lPacketSize = 1;
	}

	// Call the base-class implementation
	return CBaseParserFilter::Shutdown();
}

HRESULT CBasePlainParserFilter::InitializeParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Reset the packet length and allocate packet storage
	m_lPacketLength = 0;
	if (m_pbPacketStorage)
		CoTaskMemFree(m_pbPacketStorage);
	m_pbPacketStorage = (BYTE*)CoTaskMemAlloc(m_lPacketSize);
	if (m_pbPacketStorage == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

HRESULT CBasePlainParserFilter::ShutdownParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Free the packet storage
	m_lPacketLength = 0;
	if (m_pbPacketStorage) {
		CoTaskMemFree(m_pbPacketStorage);
		m_pbPacketStorage = NULL;
	}

	return NOERROR;
}

HRESULT CBasePlainParserFilter::ResetParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Reset the packetwise delivery stuff
	m_lPacketLength	= 0;

	return NOERROR;
}

HRESULT CBasePlainParserFilter::Receive(
	LONGLONG llStartPosition,
	BYTE *pbData,
	LONG lDataSize
)
{
	// Check the pointer
	CheckPointer(pbData, E_POINTER);

	// If the total data size we have is less than packet size
	// we only need to apped new data to the packet storage
	if (m_lPacketLength + lDataSize < m_lPacketSize) {
		CopyMemory(m_pbPacketStorage + m_lPacketLength, pbData, lDataSize);
		m_lPacketLength += lDataSize;
		return NOERROR;
	}

	// If the output pin is not connected, just quit with no error
	if (!m_ppOutputPin[0]->IsConnected())
		return NOERROR;

	// Get an empty media sample
	IMediaSample *pSample = NULL;
	HRESULT hr = m_ppOutputPin[0]->GetDeliveryBuffer(&pSample, NULL, NULL, 0);
	if (FAILED(hr))
		return hr;

	// Set the actual data length to zero
	hr = pSample->SetActualDataLength(0);
	if (FAILED(hr)) {
		pSample->Release();
		return hr;
	}

	// Pre-process the sample
	hr = PreProcess(pSample);
	if (FAILED(hr)) {
		pSample->Release();
		return hr;
	}

	// Get the sample's buffer
	BYTE *pbBuffer = NULL;
	hr = pSample->GetPointer(&pbBuffer);
	if (FAILED(hr)) {
		pSample->Release();
		return hr;
	}

	// Get the sample's buffer size
	LONG lBufferSize = pSample->GetSize();

	// Get the current sample data length
	LONG lActualDataLength = pSample->GetActualDataLength();

	// Check if we can copy packet data to the buffer
	// If not, we're stuck with the sample and should reject it
	if (lActualDataLength + m_lPacketLength > lBufferSize) {
		pSample->Release();
		return S_FALSE;
	}

	// Copy collected packet data to the buffer.
	CopyMemory(pbBuffer + lActualDataLength, m_pbPacketStorage, m_lPacketLength);
	lActualDataLength += m_lPacketLength;

	// Determine how many bytes of incoming data we should copy
	// and how many bytes we should save into packet storage
	LONG	lDataToStore	= (m_lPacketLength + lDataSize) % m_lPacketSize,
			lDataToCopy		= lDataSize - lDataToStore;

	// Check if we can copy incoming data to the buffer
	// If not, we're stuck with the sample again and should reject it
	if (lActualDataLength + lDataToCopy > lBufferSize) {
		pSample->Release();
		return S_FALSE;
	}

	// Copy incoming data to the buffer
	CopyMemory(pbBuffer + lActualDataLength, pbData, lDataToCopy);
	lActualDataLength += lDataToCopy;

	// Set the data length
	hr = pSample->SetActualDataLength(lActualDataLength);
	if (FAILED(hr)) {
		pSample->Release();
		return hr;
	}

	// Store the remainder of the incoming data in the packet storage
	CopyMemory(m_pbPacketStorage, pbData + lDataToCopy, lDataToStore);
	m_lPacketLength = lDataToStore;

	// Calculate the size of the filled sample
	REFERENCE_TIME rtDelta = 0;
	LONGLONG llDelta = 0;
	GetSampleDelta(lActualDataLength, &rtDelta, &llDelta);

	// Post-process the sample (set various properties, etc)
	hr = PostProcess(pSample);
	if (FAILED(hr)) {
		pSample->Release();
		return hr;
	}

	// Deliver the sample
	hr = m_ppOutputPin[0]->Deliver(pSample, rtDelta, llDelta);
	if (FAILED(hr)) {
		pSample->Release();
		return hr;
	}

	// Finally release the sample
	pSample->Release();

	return NOERROR;
}

HRESULT CBasePlainParserFilter::PreProcess(IMediaSample *pSample)
{
	// Do nothing -- just return
	return NOERROR;
}

HRESULT CBasePlainParserFilter::PostProcess(IMediaSample *pSample)
{
	// Do nothing -- just return
	return NOERROR;
}

HRESULT CBasePlainParserFilter::CheckInputType(const CMediaType* pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	// Loop through available media types and compare to the given
	for (UINT i = 0; i < m_nMediaTypes; i++) {
		if (
			(
				IsEqualGUID(*m_pMediaType[i].clsMajorType, *pmt->Type()) ||
				IsEqualGUID(*m_pMediaType[i].clsMajorType, GUID_NULL)
			) &&
			(
				IsEqualGUID(*m_pMediaType[i].clsMinorType, *pmt->Subtype()) ||
				IsEqualGUID(*m_pMediaType[i].clsMinorType, GUID_NULL)
			)
		)
			return S_OK;
	}

	// None of the available media types matched the given
	return S_FALSE;
}

HRESULT CBasePlainParserFilter::GetInputType(int iPosition, CMediaType *pMediaType)
{
	// Check and validate the pointer
	CheckPointer(pMediaType, E_POINTER);
	ValidateWritePtr(pMediaType, sizeof(CMediaType));

	if (iPosition < 0)
		return E_INVALIDARG;
	else if (iPosition >= (int)m_nMediaTypes)
		return VFW_S_NO_MORE_ITEMS;
	else {
		pMediaType->InitMediaType();
		pMediaType->SetType(m_pMediaType[iPosition].clsMajorType);
		pMediaType->SetSubtype(m_pMediaType[iPosition].clsMinorType);
		pMediaType->SetVariableSize();
		pMediaType->SetTemporalCompression(FALSE);
		pMediaType->SetFormatType(&FORMAT_None);
	}

	return S_OK;
}
