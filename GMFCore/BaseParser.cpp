//==========================================================================
//
// File: BaseParser.cpp
//
// Desc: Game Media Formats - Implementation of generic parser filter
//
// Copyright (C) 2004 ANX Software.
//
// Portions (C) 1992-2001 Microsoft Corporation.
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

#include "BaseParser.h"
#include "FilterOptions.h"
#include "resource.h"

#include <commctrl.h>

//==========================================================================
// CParserPullPin methods
//==========================================================================

CParserPullPin::CParserPullPin(CParserInputPin *pParentPin) : 
	m_pParentPin(pParentPin)
{
	ASSERT(pParentPin);
}

HRESULT CParserPullPin::Receive(IMediaSample *pSample)
{
	// Delegate Receive to the containing input pin
	return m_pParentPin->Receive(pSample);
}

HRESULT CParserPullPin::BeginFlush(void)
{
	// Delegate BeginFlush to the containing input pin
	return m_pParentPin->BeginFlush();
}

HRESULT CParserPullPin::EndFlush(void)
{
	// Delegate EndFlush to the containing input pin
	return m_pParentPin->EndFlush();
}

HRESULT CParserPullPin::EndOfStream(void)
{
	// Delegate EndOfStream to the containing input pin
	return m_pParentPin->EndOfStream();
}

void CParserPullPin::OnError(HRESULT hr)
{
	// Do nothing...
}

//==========================================================================
// CParserInputPin methods
//==========================================================================

CParserInputPin::CParserInputPin(
	TCHAR *pObjectName,
	CBaseParserFilter *pFilter,
	CCritSec *pFilterLock,
	CCritSec *pReceiveLock,
	HRESULT *phr,
	LPCWSTR pName
) :
	CBaseInputPin(pObjectName, pFilter, pFilterLock, phr, pName),
	m_pReceiveLock(pReceiveLock),
	m_PullPin(this)
{
	ASSERT(pFilter);
	ASSERT(pFilterLock);
	ASSERT(pReceiveLock);
	ASSERT(phr);
}

HRESULT CParserInputPin::CheckConnect(IPin *pPin)
{
	// Check and validate the pointer
	CheckPointer(pPin, E_POINTER);
	ValidateReadPtr(pPin, sizeof(IPin));

	// Perform base-class checking
	HRESULT hr = CBaseInputPin::CheckConnect(pPin);
	if (FAILED(hr))
		return hr;

	// Try to connect pullpin to the upstream pin
	IUnknown *pUnk = NULL;
	hr = pPin->QueryInterface(IID_IUnknown, (void**)&pUnk);
	if (FAILED(hr))
		return hr;
	CBaseParserFilter *pFilter = (CBaseParserFilter*)m_pFilter;
	hr = m_PullPin.Connect(pUnk, m_pAllocator, pFilter->IsSyncInput());
	pUnk->Release();
	if (FAILED(hr))
		return hr;
	
	// Provide the filter with IAsyncReader to perform its
	// checks and output pins creation
	IAsyncReader *pReader = m_PullPin.GetReader();
	ASSERT(pReader);
	hr = pFilter->Initialize(pPin, pReader);
	pReader->Release();
	if (FAILED(hr)) {
		m_PullPin.Disconnect();
		return hr;
	}

	// Get the input allocator properties from the parent filter
	ALLOCATOR_PROPERTIES apRequest;
	hr = pFilter->GetInputAllocatorProperties(&apRequest);
	if (FAILED(hr)) {
		pFilter->Shutdown();
		m_PullPin.Disconnect();
		return hr;
	}

	// Request an allocator with the desired properties
	hr = m_PullPin.DecideAllocator(NULL, &apRequest);
	if (FAILED(hr)) {
		pFilter->Shutdown();
		m_PullPin.Disconnect();
		return hr;
	}

	// Make sure we obtained the allocator we wanted
	if (!m_PullPin.m_pAlloc) {
		pFilter->Shutdown();
		m_PullPin.Disconnect();
		return E_FAIL;
	}
	ALLOCATOR_PROPERTIES apActual;
	hr = m_PullPin.m_pAlloc->GetProperties(&apActual);
	if (FAILED(hr)) {
		pFilter->Shutdown();
		m_PullPin.Disconnect();
		return hr;
	}
	if (
		(apActual.cBuffers	< apRequest.cBuffers)	||
		(apActual.cbBuffer	< apRequest.cbBuffer)	||
		(apActual.cbAlign	% apRequest.cbAlign)	||
		(apActual.cbPrefix	!= apRequest.cbPrefix)
	) {
		pFilter->Shutdown();
		m_PullPin.Disconnect();
		return E_FAIL;
	}

	// Set the default start/stop positions (we must do it AFTER 
	// deciding on the pullpin's allocator)
	hr = pFilter->SetDefaultPositions();
	if (FAILED(hr)) {
		pFilter->Shutdown();
		m_PullPin.Disconnect();
		return hr;
	}

	return NOERROR;
}

HRESULT CParserInputPin::BreakConnect(void)
{
	// Delete the output pins
	HRESULT hr = ((CBaseParserFilter*)m_pFilter)->Shutdown();
	if (FAILED(hr))
		return hr;

	// Disconnect the pullpin
	hr = m_PullPin.Disconnect();
	if (FAILED(hr))
		return hr;

	// Let the base class break its connection
	return CBaseInputPin::BreakConnect();
}

HRESULT CParserInputPin::Active(void)
{
	// Perform the base-class activation
	HRESULT hr = CBaseInputPin::Active();
	if (FAILED(hr))
		return hr;

	// Initialize parser
	CBaseParserFilter *pFilter = (CBaseParserFilter*)m_pFilter;
	hr = pFilter->InitializeParser();
	if (FAILED(hr))
		return hr;

	// Activate the pullpin
	hr = m_PullPin.Active();
	if (FAILED(hr)) {
		pFilter->ShutdownParser();
		return hr;
	}

	return NOERROR;
}

HRESULT CParserInputPin::Inactive(void)
{
	// Deactivate the pullpin
	HRESULT hr = m_PullPin.Inactive();
	if (FAILED(hr))
		return hr;

	// Shutdown parser
	CBaseParserFilter *pFilter = (CBaseParserFilter*)m_pFilter;
	hr = pFilter->ShutdownParser();
	if (FAILED(hr))
		return hr;

	// Set the default start/stop positions, so that next time
	// we'll run the playback it'll start from the file beginning
	hr = pFilter->SetDefaultPositions();
	if (FAILED(hr))
		return hr;

	// Instead of the base-class deactivation just set the flags.
	// We should not call the base-class implementation as it 
	// tries to decommit allocator while we have none
	m_bRunTimeError = FALSE;
	m_bFlushing = FALSE;
	
	return NOERROR;
}

HRESULT CParserInputPin::CheckMediaType(const CMediaType* pmt)
{
	// Delegate the call to the parent filter
	return ((CBaseParserFilter*)m_pFilter)->CheckInputType(pmt);
}

HRESULT CParserInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Delegate the call to the parent filter
	return ((CBaseParserFilter*)m_pFilter)->GetInputType(iPosition, pMediaType);
}

STDMETHODIMP CParserInputPin::Receive(IMediaSample *pSample)
{
	CAutoLock lock(m_pReceiveLock);

	// Check whether it's all right with pSample in the base-class 
	// implementation and return if not
	HRESULT hr = CBaseInputPin::Receive(pSample);
	if (hr != S_OK)
		return hr;

	// Delegate the call on to the parent filter
	return ((CBaseParserFilter*)m_pFilter)->Receive(pSample);
}

HRESULT CParserInputPin::Seek(LONGLONG llStart, LONGLONG llStop)
{
	// Hold the filter lock so that the pullpin allocator is kept 
	// intact inside the method
	CAutoLock lock(m_pLock);

	// Convert file positions to media time and seek the pullpin
	REFERENCE_TIME rtStart, rtStop;
	rtStart	= llStart * UNITS;
	if (llStop < MAXLONGLONG)
		rtStop = llStop * UNITS;
	else
		rtStop = MAXLONGLONG;
	HRESULT hr = m_PullPin.Seek(rtStart, rtStop);
	if (FAILED(hr))
		return hr;

	// Get the alignment of the pullpin's allocator
	ALLOCATOR_PROPERTIES apActual;
	ASSERT(m_PullPin.m_pAlloc);
	hr = m_PullPin.m_pAlloc->GetProperties(&apActual);
	if (FAILED(hr))
		return hr;

	// Set the adjustment length
	((CBaseParserFilter*)m_pFilter)->SetAdjustment(m_PullPin.AlignDown(llStart, apActual.cbAlign));

	return NOERROR;
}

STDMETHODIMP CParserInputPin::BeginFlush(void)
{
	CAutoLock lock(m_pLock);

	// Perform the base-class BeginFlush
	HRESULT hr = CBaseInputPin::BeginFlush();
	if (FAILED(hr))
		return hr;

	// Delegate the call on to the parent filter
	return ((CBaseParserFilter*)m_pFilter)->BeginFlush();
}

STDMETHODIMP CParserInputPin::EndFlush(void)
{
	CAutoLock lock(m_pLock);

	// Delegate the call to the parent filter
	HRESULT hr = ((CBaseParserFilter*)m_pFilter)->EndFlush();
	if (FAILED(hr))
		return hr;

	// Perform the base-class EndFlush
	return CBaseInputPin::EndFlush();
}

STDMETHODIMP CParserInputPin::NewSegment(
	REFERENCE_TIME tStart, 
	REFERENCE_TIME tStop, 
	double dRate
)
{
	CAutoLock lock(m_pReceiveLock);

	// Delegate the call to the parent filter
	HRESULT hr = ((CBaseParserFilter*)m_pFilter)->NewSegment(tStart, tStop, dRate);
	if (FAILED(hr))
		return hr;

	// Perform the base-class NewSegment
	return CBaseInputPin::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CParserInputPin::EndOfStream(void)
{
	CAutoLock lock(m_pReceiveLock);

	// Check if we can accept end of stream message
	HRESULT hr = CheckStreaming();
	if (hr != S_OK)
		return hr;

	// Delegate the call to the parent filter
	hr = ((CBaseParserFilter*)m_pFilter)->EndOfStream();
	if (FAILED(hr))
		return hr;
	
	// Perform the base-class EndOfStream
	return CBaseInputPin::EndOfStream();
}

//==========================================================================
// CParserOutputPin methods
//==========================================================================

CParserOutputPin::CParserOutputPin(
	TCHAR *pObjectName,
	CBaseParserFilter *pFilter,
	CCritSec *pFilterLock,
	CMediaType *pmt,
	ALLOCATOR_PROPERTIES *pap,
	DWORD dwCapabilities,
	int nTimeFormats,
	GUID *pTimeFormats,
	HRESULT *phr,
	LPCWSTR pName
) :
	CBaseOutputPin(pObjectName, pFilter, pFilterLock, phr, pName),
	m_rtStreamTime(0),
	m_llMediaTime(0),
	m_bIsDiscontinuity(TRUE),
	m_pMediaType(pmt),
	m_pAllocatorProperties(pap),
	m_pOutputQueue(NULL)
{
	ASSERT(pFilter);
	ASSERT(pFilterLock);
	ASSERT(pmt);
	ASSERT(pap);
	ASSERT(phr);

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// The option path
		TCHAR szOption[MAX_PATH];

		// Retrieve the options from the registry
		m_bAutoQueue = FALSE; // Default value
		wsprintf(szOption, TEXT("%ls: Auto Queue"), pName);
		HRESULT hr = RegGetFilterOptionDWORD(
			pFilter->Name(),
			szOption,
			(DWORD*)&m_bAutoQueue
		);
		if (FAILED(hr))
			*phr = hr;
		m_bQueue = TRUE; // Default value
		wsprintf(szOption, TEXT("%ls: Queue"), pName);
		hr = RegGetFilterOptionDWORD(
			pFilter->Name(),
			szOption,
			(DWORD*)&m_bQueue
		);
		if (FAILED(hr))
			*phr = hr;
		m_lBatchSize = 1; // Default value
		wsprintf(szOption, TEXT("%ls: Batch Size"), pName);
		hr = RegGetFilterOptionDWORD(
			pFilter->Name(),
			szOption,
			(DWORD*)&m_lBatchSize
		);
		if (FAILED(hr))
			*phr = hr;
		m_bBatchExact = FALSE; // Default value
		wsprintf(szOption, TEXT("%ls: Batch Exact"), pName);
		hr = RegGetFilterOptionDWORD(
			pFilter->Name(),
			szOption,
			(DWORD*)&m_bBatchExact
		);
		if (FAILED(hr))
			*phr = hr;
		wsprintf(szOption, TEXT("%ls: Number of Buffers"), pName);
		hr = RegGetFilterOptionDWORD(
			pFilter->Name(),
			szOption,
			(DWORD*)&(pap->cBuffers)
		);
		if (FAILED(hr))
			*phr = hr;
	}

	// Create a parser seeking object
	m_pParserSeeking = new CParserSeeking(
		NAME("Parser Seeking"),
		NULL,
		pFilter,
		this,
		dwCapabilities,
		nTimeFormats,
		pTimeFormats
	);
	if (m_pParserSeeking == NULL)
		*phr = E_OUTOFMEMORY;
	
	// Hold a reference on the seeking object
	if (m_pParserSeeking)
		m_pParserSeeking->AddRef();
}

CParserOutputPin::~CParserOutputPin()
{
	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// The option path
		TCHAR szOption[MAX_PATH];

		CBaseParserFilter *pFilter = (CBaseParserFilter*)m_pFilter;

		// Store the options in the registry
		wsprintf(szOption, TEXT("%ls: Auto Queue"), m_pName);
		RegSetFilterOptionValue(
			pFilter->Name(),
			szOption,
			REG_DWORD,
			(LPBYTE)&m_bAutoQueue,
			sizeof(BOOL)
		);
		wsprintf(szOption, TEXT("%ls: Queue"), m_pName);
		RegSetFilterOptionValue(
			pFilter->Name(),
			szOption,
			REG_DWORD,
			(LPBYTE)&m_bQueue,
			sizeof(BOOL)
		);
		wsprintf(szOption, TEXT("%ls: Batch Size"), m_pName);
		RegSetFilterOptionValue(
			pFilter->Name(),
			szOption,
			REG_DWORD,
			(LPBYTE)&m_lBatchSize,
			sizeof(LONG)
		);
		wsprintf(szOption, TEXT("%ls: Batch Exact"), m_pName);
		RegSetFilterOptionValue(
			pFilter->Name(),
			szOption,
			REG_DWORD,
			(LPBYTE)&m_bBatchExact,
			sizeof(BOOL)
		);
		wsprintf(szOption, TEXT("%ls: Number of Buffers"), m_pName);
		RegSetFilterOptionValue(
			pFilter->Name(),
			szOption,
			REG_DWORD,
			(LPBYTE)&(m_pAllocatorProperties->cBuffers),
			sizeof(long)
		);
	}

	// Free the media type
	if (m_pMediaType) {
		delete m_pMediaType;
		m_pMediaType = NULL;
	}

	// Free the allocator properties
	if (m_pAllocatorProperties) {
		CoTaskMemFree(m_pAllocatorProperties);
		m_pAllocatorProperties = NULL;
	}

	// Release the parser seeking object
	if (m_pParserSeeking) {
		m_pParserSeeking->Release();
		m_pParserSeeking = NULL;
	}

	// Output queue should already have been destroyed
	ASSERT(m_pOutputQueue == NULL);
}

STDMETHODIMP CParserOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	// Check and validate the pointer
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	// Expose IMediaSeeking (and the base-class interfaces)
	if (riid == IID_IMediaSeeking) {
		if (m_pParserSeeking == NULL)
			return E_NOINTERFACE;
		return m_pParserSeeking->QueryInterface(riid, ppv);
	} else
		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CParserOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
	// If we've got a quality sink, pass the message to it
	if (m_pQSink != NULL)
		return m_pQSink->Notify(m_pFilter, q);

	// Otherwise pass the quality message to the parent filter
	return ((CBaseParserFilter*)m_pFilter)->NotifyQuality(m_pName, q);
}

STDMETHODIMP_(ULONG) CParserOutputPin::NonDelegatingAddRef(void)
{
	// Call the base-class implementation (it does not do correct 
	// refcounting but refcounts parent filter)
	CBaseOutputPin::NonDelegatingAddRef();

	// Call the CUnknown implementation (it does correct refcounting).
	// In case of DEBUG build we're doing double refcounting here, 
	// but that's no problem since Release() does double dereferencing
	return CUnknown::NonDelegatingAddRef();
}

STDMETHODIMP_(ULONG) CParserOutputPin::NonDelegatingRelease(void)
{
	// Call the base-class implementation (it does not do correct 
	// dereferencing but dereferences parent filter)
	CBaseOutputPin::NonDelegatingRelease();

	// Call the CUnknown implementation (it does correct dereferencing
	// and deletes the object when it's due time)
	return CUnknown::NonDelegatingRelease();
}

HRESULT CParserOutputPin::CheckMediaType(const CMediaType *pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	CAutoLock lock(m_pLock);

	// Check if the proposed type strictly matches the assigned one
	return (*pmt == *m_pMediaType) ? S_OK : S_FALSE;
}

HRESULT CParserOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Check and validate the pointer
	CheckPointer(pMediaType, E_POINTER);
	ValidateWritePtr(pMediaType, sizeof(CMediaType));

	CAutoLock lock(m_pLock);

	// Support only one media type
	if (iPosition < 0)
		return E_INVALIDARG;
	else if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;
	else {
		// Return our media type
		*pMediaType = *m_pMediaType;
	}

	return S_OK;
}

HRESULT CParserOutputPin::DecideBufferSize(
	IMemAllocator *pMemAllocator, 
	ALLOCATOR_PROPERTIES *pRequest
)
{
	CAutoLock lock(m_pLock);

	ASSERT(pMemAllocator);
	ASSERT(pRequest);

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Set the properties honoring peer pin's request
		// Note that we don't care about alignment and prefix here
		pRequest->cBuffers = max(pRequest->cBuffers, m_pAllocatorProperties->cBuffers);
		pRequest->cbBuffer = max(pRequest->cbBuffer, m_pAllocatorProperties->cbBuffer);
	}

	// Ask the allocator to reserve requested buffers
	ALLOCATOR_PROPERTIES apActual;
	HRESULT hr = pMemAllocator->SetProperties(pRequest, &apActual);
	if (FAILED(hr))
		return hr;

	// Check if the allocator is suitable
	if (
		(apActual.cBuffers < pRequest->cBuffers) ||
		(apActual.cbBuffer < pRequest->cbBuffer)
	)
		return E_FAIL;

	return NOERROR;
}

HRESULT CParserOutputPin::Active(void)
{
	CAutoLock lock(m_pLock);

	HRESULT hr = NOERROR;

	// Make sure that the pin is connected
	if (!IsConnected())
		return NOERROR;

	// Create the output queue if we have to
	if (m_pOutputQueue == NULL) {

		{
			// Protect the filter options
			CAutoLock optionlock(&m_csOptions);

			m_pOutputQueue = new COutputQueue(
				m_Connected,
				&hr,
				m_bAutoQueue,
				m_bQueue,
				m_lBatchSize,
				m_bBatchExact
			);
		}
		if (m_pOutputQueue == NULL)
			return E_OUTOFMEMORY;

		// Make sure that the constructor did not return any error
		if (FAILED(hr)) {
			delete m_pOutputQueue;
			m_pOutputQueue = NULL;
			return hr;
		}
	}

	// Initialize the stream time to zero
	m_rtStreamTime = 0;

	// The first sample is discontinuity
	m_bIsDiscontinuity = TRUE;

	// Deliver NewSegment() downstream at the beginning of the stream.
	// If this fails, proceed with no error
	if (m_pParserSeeking) {

		// Retrieve the current segment parameters from the seeker
		REFERENCE_TIME rtStart = 0, rtStop = 0;
		double dRate = 1.0;
		hr = m_pParserSeeking->GetSegmentParameters(&rtStart, &rtStop, &dRate);
		if (SUCCEEDED(hr)) {
			
			// Set the segment parameters on the pin
			NewSegment(rtStart, rtStop, dRate);

			// Deliver NewSegment() downstream
			DeliverNewSegment(rtStart, rtStop, dRate);
		}
	}

	// Pass the call on to the base class
	return CBaseOutputPin::Active();
}

HRESULT CParserOutputPin::Inactive(void)
{
	CAutoLock lock(m_pLock);

	// Delete the output queue associated with the pin
	if (m_pOutputQueue)
	{
		delete m_pOutputQueue;
		m_pOutputQueue = NULL;
	}

	// Reset the stream and media times to zeros
	m_rtStreamTime	= 0;
	m_llMediaTime	= 0;

	// Reset the seeker
	if (m_pParserSeeking)
		m_pParserSeeking->Reset();

	// Pass the call on to the base class
	return CBaseOutputPin::Inactive();
}

HRESULT CParserOutputPin::Deliver(
	IMediaSample *pMediaSample,
	REFERENCE_TIME rtDelta,
	LONGLONG llDelta
)
{
	if (m_pParserSeeking) {
		// Get the current rate (that's a thread-safe way as the seeker 
		// holds its critical section when setting/getting rate)
		double dRate = 1.0;
		if (SUCCEEDED(m_pParserSeeking->GetRate(&dRate))) {
			// Adjust time delta according to the current rate
			if (dRate > 0)
				rtDelta /= dRate;
		}
	}

	// Set the stream time
	REFERENCE_TIME rtStop = m_rtStreamTime + rtDelta;
	HRESULT hr = pMediaSample->SetTime(&m_rtStreamTime, &rtStop);
	if (FAILED(hr))
		return hr;

	// Update the stream time
	m_rtStreamTime = rtStop;

	// Set the media time
	LONGLONG llStop = m_llMediaTime + llDelta;
	hr = pMediaSample->SetMediaTime(&m_llMediaTime, &llStop);
	if (FAILED(hr))
		return hr;

	// Update the media time
	m_llMediaTime = llStop;

	// Deliver the sample
	hr = Deliver(pMediaSample);
	if (FAILED(hr))
		return hr;

	// Advance the seeker's position (we ignore the possible failure of 
	// the method call because the failure doesn't hamper the delivery)
	if (m_pParserSeeking)
		m_pParserSeeking->AdvanceCurrentPosition(llDelta);

	// If the last sample was a discontinuity, the new one is not
	m_bIsDiscontinuity = FALSE;

	return NOERROR;
}

HRESULT CParserOutputPin::Deliver(IMediaSample *pMediaSample)
{
	// Make sure that we have an output queue
	if (m_pOutputQueue == NULL)
		return NOERROR;

	pMediaSample->AddRef();
	return m_pOutputQueue->Receive(pMediaSample);
}

HRESULT CParserOutputPin::DeliverEndOfStream(void)
{
	// Make sure that we have an output queue
	if (m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->EOS();
	return NOERROR;
}

HRESULT CParserOutputPin::DeliverBeginFlush(void)
{
	// Make sure that we have an output queue
	if (m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->BeginFlush();
	return NOERROR;
}

HRESULT CParserOutputPin::DeliverEndFlush(void)
{
	// Make sure that we have an output queue
	if (m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->EndFlush();
	return NOERROR;
}

HRESULT CParserOutputPin::DeliverNewSegment(
	REFERENCE_TIME tStart,
	REFERENCE_TIME tStop,
	double dRate
)
{
	// Make sure that we have an output queue
	if (m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->NewSegment(tStart, tStop, dRate);
	return NOERROR;
}

HRESULT CParserOutputPin::DisconnectBoth(void)
{
	CAutoLock lock(m_pLock);

	// See if the filter is active
	if (!IsStopped())
		return VFW_E_NOT_STOPPED;

	// First disconnect the peer pin (so that it Release()'s us)
	if (m_Connected) {
		HRESULT hr = m_Connected->Disconnect();
		if (FAILED(hr))
			return hr;
	}

	// Now disconnect ourselves
	return Disconnect();
}

HRESULT CParserOutputPin::GetDeliveryBuffer(
	IMediaSample **ppSample,
	REFERENCE_TIME *pStartTime,
	REFERENCE_TIME *pEndTime,
	DWORD dwFlags
)
{
	// Call the base class method (which actually retrieves the buffer)
	HRESULT hr = CBaseOutputPin::GetDeliveryBuffer(ppSample, pStartTime, pEndTime, dwFlags);
	if (FAILED(hr))
		return hr;

	// If bTemporalCompression is TRUE, set syncpoint to TRUE 
	// only for the first sample 
	BOOL bSyncPoint;
	if (m_pMediaType->IsTemporalCompressed())
		bSyncPoint = (m_rtStreamTime == 0);
	else // All samples are sync points
		bSyncPoint = TRUE;

	// Set the syncpoint property
	hr = (*ppSample)->SetSyncPoint(bSyncPoint);
	if (FAILED(hr)) {
		(*ppSample)->Release();
		return hr;
	}

	// No preroll samples by default
	hr = (*ppSample)->SetPreroll(FALSE);
	if (FAILED(hr)) {
		(*ppSample)->Release();
		return hr;
	}

	// Set the discontinuity property
	hr = (*ppSample)->SetDiscontinuity(m_bIsDiscontinuity);
	if (FAILED(hr)) {
		(*ppSample)->Release();
		return hr;
	}

	return NOERROR;
}

long CParserOutputPin::get_BuffersNumber(void)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	return m_pAllocatorProperties->cBuffers;
}

BOOL CParserOutputPin::get_AutoQueue(void)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	return m_bAutoQueue;
}

BOOL CParserOutputPin::get_Queue(void)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	return m_bQueue;
}

LONG CParserOutputPin::get_BatchSize(void)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	return m_lBatchSize;
}

BOOL CParserOutputPin::get_BatchExact(void)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	return m_bBatchExact;
}

void CParserOutputPin::put_BuffersNumber(long nBuffers)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	m_pAllocatorProperties->cBuffers = nBuffers;
}

void CParserOutputPin::put_AutoQueue(BOOL bAutoQueue)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	m_bAutoQueue = bAutoQueue;
}

void CParserOutputPin::put_Queue(BOOL bQueue)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	m_bQueue = bQueue;
}

void CParserOutputPin::put_BatchSize(LONG lBatchSize)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	m_lBatchSize = lBatchSize;
}

void CParserOutputPin::put_BatchExact(BOOL bBatchExact)
{
	// Protect the pin options
	CAutoLock optionlock(&m_csOptions);

	m_bBatchExact = bBatchExact;
}

//==========================================================================
// CBaseChunkParser methods
//==========================================================================

CBaseChunkParser::CBaseChunkParser(
	TCHAR *pName,
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	LONG lHeaderSize,
	LONG lAlignment,
	HRESULT *phr
) :
	CBaseObject(pName),
	m_pFilter(pFilter),
	m_ppOutputPin(ppOutputPin),
	m_lHeaderSize(lHeaderSize),
	m_lAlignment(lAlignment),
	m_State(CPS_CHUNK_COMPLETE),
	m_lHeaderLength(0),
	m_lChunkDataSize(0)
{
	ASSERT(pFilter);
	ASSERT(ppOutputPin);
	ASSERT(lHeaderSize >= 0);
	ASSERT(lAlignment > 0);
	ASSERT(phr);

	// Allocate space for header storage
	m_pbHeader = (BYTE*)CoTaskMemAlloc(lHeaderSize);
	*phr = (m_pbHeader == NULL) ? E_OUTOFMEMORY : NOERROR;
}

CBaseChunkParser::~CBaseChunkParser()
{
	// Free header storage
	if (m_pbHeader) {
		CoTaskMemFree(m_pbHeader);
		m_pbHeader = NULL;
	}

	// Reset and shutdown parser
	ResetParser();
}

HRESULT CBaseChunkParser::ResetParser(void)
{
	CAutoLock lock(&m_csLock);

	// Reset the parser state and incomplete data lengths
	m_State				= CPS_CHUNK_COMPLETE;
	m_lHeaderLength		= 0;
	m_lChunkDataSize	= 0;

	return NOERROR;
}

HRESULT CBaseChunkParser::Receive(
	LONGLONG llStartPosition,
	BYTE *pbData,
	LONG lDataSize
)
{
	CAutoLock lock(&m_csLock);

	// Check the pointer
	CheckPointer(pbData, E_POINTER);

	HRESULT hr = NOERROR;
	LONG lLength, lRemainder, lCorrection;

	// Process data buffer until the data is exhausted
	while (lDataSize > 0) {

		// Check if we're flushing and if so return
		if (m_pFilter->IsFlushing()) {
			// No need to reset us here, as it will be done 
			// in EndFlush(), so just reject the data
			return S_FALSE;
		}

		switch (m_State) {

			// The previous chunk is done -- start a new one
			case CPS_CHUNK_COMPLETE:

				// Correct the position according to the alignment value
				lRemainder = (LONG)(llStartPosition % m_lAlignment);
				if (lRemainder)
					lCorrection = m_lAlignment - lRemainder;
				else
					lCorrection = 0;
				if (lCorrection > lDataSize)
					lCorrection = lDataSize;

				// Advance position, data pointer and remained data size
				llStartPosition	+= lCorrection;
				pbData			+= lCorrection;
				lDataSize		-= lCorrection;

				// Break out if no data left, so that the state remains unchanged
				if (!lDataSize)
					break;

				// Start receiving header
				m_lHeaderLength = min(lDataSize, m_lHeaderSize);
				if (m_lHeaderLength < m_lHeaderSize) {
					// We've got incomplete header: copy what we have to 
					// the header buffer and change state
					CopyMemory(m_pbHeader, pbData, m_lHeaderLength);
					m_State = CPS_HEADER_PARTIAL;
				} else {
					// We've got complete header: parse it and change state
					m_lChunkDataSize = 0;
					hr = ParseChunkHeader(llStartPosition, pbData, &m_lChunkDataSize);
					m_State = CPS_HEADER_COMPLETE;
					if (FAILED(hr))
						return hr;
					if (m_lChunkDataSize < 0)
						return E_UNEXPECTED;
				}

				// Advance data pointer and remained data size
				llStartPosition += m_lHeaderLength;
				pbData			+= m_lHeaderLength;
				lDataSize		-= m_lHeaderLength;

				break;

			// We've got incomplete header: try to receive it completely
			case CPS_HEADER_PARTIAL:

				// Copy available data to header storage
				lLength = min(lDataSize, m_lHeaderSize - m_lHeaderLength);
				CopyMemory(m_pbHeader + m_lHeaderLength, pbData, lLength);
				llStartPosition	+= lLength;
				m_lHeaderLength	+= lLength;
				if (m_lHeaderLength == m_lHeaderSize) {
					// We've completed the header: parse it and change state
					m_lChunkDataSize = 0;
					hr = ParseChunkHeader(llStartPosition - m_lHeaderSize, m_pbHeader, &m_lChunkDataSize);
					m_State = CPS_HEADER_COMPLETE;
					if (FAILED(hr))
						return hr;
					if (m_lChunkDataSize < 0)
						return E_UNEXPECTED;
				}

				// Advance data pointer and remained data size
				pbData		+= lLength;
				lDataSize	-= lLength;

				break;

			// Header complete: we're receiving chunk data
			case CPS_HEADER_COMPLETE:
				
				// Parse available data
				lLength = min(lDataSize, m_lChunkDataSize);
				hr = ParseChunkData(llStartPosition, pbData, lLength);
				if (FAILED(hr))
					return hr;

				// Advance data pointer and remained data size
				llStartPosition		+= lLength;
				pbData				+= lLength;
				lDataSize			-= lLength;
				m_lChunkDataSize	-= lLength;
				
				// If the chunk data size is exhausted, change state
				if (!m_lChunkDataSize) {
					ParseChunkEnd();
					m_State = CPS_CHUNK_COMPLETE;
				}

				break;

			default:

				// This should never happen
				ASSERT(false);
		}
	}

	return NOERROR;
}

//==========================================================================
// CBaseParserFilter methods
//==========================================================================

CBaseParserFilter::CBaseParserFilter(
	TCHAR *pName,
	LPUNKNOWN pUnk,
	REFCLSID clsid,
	const WCHAR *wszFilterName,
	HRESULT *phr
) :
	CBaseFilter(pName, pUnk, &m_csFilter, clsid),
	m_wszFilterName(wszFilterName),
	m_wszAuthorName(NULL),			// |
	m_wszTitle(NULL),				// |
	m_wszRating(NULL),				// |
	m_wszDescription(NULL),			// |
	m_wszCopyright(NULL),			// |
	m_wszBaseURL(NULL),				// |
	m_wszLogoURL(NULL),				// |-- No information at this time
	m_wszLogoIconURL(NULL),			// |
	m_wszWatermarkURL(NULL),		// |
	m_wszMoreInfoURL(NULL),			// |
	m_wszMoreInfoBannerImage(NULL),	// |
	m_wszMoreInfoBannerURL(NULL),	// |
	m_wszMoreInfoText(NULL),		// |
	m_llStartPosition(0),			// Default is file beginning
	m_llStopPosition(MAXLONGLONG),	// Default is file end
	m_llAdjustment(0),				// Default is zero adjustment
	m_llDefaultStart(0),			// Default is file beginning
	m_llDefaultStop(MAXLONGLONG),	// Default is file end
	m_cbInputBuffer(0),				// We cannot guess at this time
	m_cbInputAlign(1),				// We cannot guess at this time
	m_InputPin(
		NAME("Parser Input Pin"),
		this,
		&m_csFilter,
		&m_csReceive,
		phr,
		L"Input"
	),
	m_nOutputPins(0),				// No output pins at this time
	m_ppOutputPin(NULL)				// No output pins at this time
{
	ASSERT(wszFilterName);
	ASSERT(phr);

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Retrieve the options from the registry
		m_bSyncInput = FALSE; // Default value
		HRESULT hr = RegGetFilterOptionDWORD(
			m_wszFilterName,
			TEXT("Synchronous Input"),
			(DWORD*)&m_bSyncInput
		);
		if (FAILED(hr))
			*phr = hr;
		m_nInputBuffers = 1; // Default value
		hr = RegGetFilterOptionDWORD(
			m_wszFilterName,
			TEXT("Number of Input Buffers"),
			(DWORD*)&m_nInputBuffers
		);
		if (FAILED(hr))
			*phr = hr;
	}
}

CBaseParserFilter::~CBaseParserFilter()
{
	// Delete output pins created by Initialize()
	Shutdown();

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Store the options in the registry
		RegSetFilterOptionValue(
			m_wszFilterName,
			TEXT("Synchronous Input"),
			REG_DWORD,
			(LPBYTE)&m_bSyncInput,
			sizeof(BOOL)
		);
		RegSetFilterOptionValue(
			m_wszFilterName,
			TEXT("Number of Input Buffers"),
			REG_DWORD,
			(LPBYTE)&m_nInputBuffers,
			sizeof(long)
		);
	}
}

STDMETHODIMP CBaseParserFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	// Check and validate the pointer
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	// Expose IAMMediaContent, IConfigBaseParser, ISpecifyPropertyPages 
	// (and the base-class interfaces)
	if (riid == IID_IAMMediaContent)
		return GetInterface((IAMMediaContent*)this, ppv);
	else if (riid == IID_IConfigBaseParser)
		return GetInterface((IConfigBaseParser*)this, ppv);
	else if (riid == IID_ISpecifyPropertyPages)
		return GetInterface((ISpecifyPropertyPages*)this, ppv);
	else
		return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CBaseParserFilter::GetTypeInfoCount(UINT * pctinfo)
{
	return m_basedisp.GetTypeInfoCount(pctinfo);
}

STDMETHODIMP CBaseParserFilter::GetTypeInfo(
	UINT itinfo,
	LCID lcid,
	ITypeInfo ** pptinfo
)
{
	return m_basedisp.GetTypeInfo(IID_IAMMediaContent, itinfo, lcid, pptinfo);
}

STDMETHODIMP CBaseParserFilter::GetIDsOfNames(
	REFIID riid,
	OLECHAR  ** rgszNames,
	UINT cNames,
	LCID lcid,
	DISPID * rgdispid
)
{
	return m_basedisp.GetIDsOfNames(
		IID_IAMMediaContent,
		rgszNames,
		cNames,
		lcid,
		rgdispid
	);
}

STDMETHODIMP CBaseParserFilter::Invoke(
	DISPID dispidMember,
	REFIID riid,
	LCID lcid,
	WORD wFlags,
	DISPPARAMS *pdispparams,
	VARIANT *pvarResult,
	EXCEPINFO *pexcepinfo,
	UINT *puArgErr
)
{
	if (riid != IID_NULL)
		return DISP_E_UNKNOWNINTERFACE;

	ITypeInfo *pti;
	HRESULT hr = GetTypeInfo(0, lcid, &pti);
	if (FAILED(hr))
		return hr;

	hr = pti->Invoke(
		(IAMMediaContent*)this,
		dispidMember,
		wFlags,
		pdispparams,
		pvarResult,
		pexcepinfo,
		puArgErr
	);

	pti->Release();
	return hr;
}

STDMETHODIMP CBaseParserFilter::get_AuthorName(BSTR *pbstrAuthorName)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszAuthorName == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrAuthorName = SysAllocString(m_wszAuthorName);
	if (*pbstrAuthorName == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_Title(BSTR *pbstrTitle)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszTitle == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrTitle = SysAllocString(m_wszTitle);
	if (*pbstrTitle == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_Rating(BSTR *pbstrRating)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszRating == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrRating = SysAllocString(m_wszRating);
	if (*pbstrRating == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_Description(BSTR *pbstrDescription)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszDescription == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrDescription = SysAllocString(m_wszDescription);
	if (*pbstrDescription == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_Copyright(BSTR *pbstrCopyright)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszCopyright == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrCopyright = SysAllocString(m_wszCopyright);
	if (*pbstrCopyright == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_BaseURL(BSTR *pbstrBaseURL)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszBaseURL == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrBaseURL = SysAllocString(m_wszBaseURL);
	if (*pbstrBaseURL == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_LogoURL(BSTR *pbstrLogoURL)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszLogoURL == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrLogoURL = SysAllocString(m_wszLogoURL);
	if (*pbstrLogoURL == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_LogoIconURL(BSTR *pbstrLogoIconURL)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszLogoIconURL == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrLogoIconURL = SysAllocString(m_wszLogoIconURL);
	if (*pbstrLogoIconURL == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_WatermarkURL(BSTR *pbstrWatermarkURL)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszWatermarkURL == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrWatermarkURL = SysAllocString(m_wszWatermarkURL);
	if (*pbstrWatermarkURL == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_MoreInfoURL(BSTR *pbstrMoreInfoURL)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszMoreInfoURL == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrMoreInfoURL = SysAllocString(m_wszMoreInfoURL);
	if (*pbstrMoreInfoURL == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_MoreInfoBannerImage(BSTR *pbstrMoreInfoBannerImage)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszMoreInfoBannerImage == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrMoreInfoBannerImage = SysAllocString(m_wszMoreInfoBannerImage);
	if (*pbstrMoreInfoBannerImage == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_MoreInfoBannerURL(BSTR *pbstrMoreInfoBannerURL)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszMoreInfoBannerURL == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrMoreInfoBannerURL = SysAllocString(m_wszMoreInfoBannerURL);
	if (*pbstrMoreInfoBannerURL == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_MoreInfoText(BSTR *pbstrMoreInfoText)
{
	// Protect media content information
	CAutoLock infolock(&m_csInfo);

	// If the string is NULL, report that we cannot find the info
	if (m_wszMoreInfoText == NULL)
		return VFW_E_NOT_FOUND;

	// Allocate BSTR and copy the string into it
	*pbstrMoreInfoText = SysAllocString(m_wszMoreInfoText);
	if (*pbstrMoreInfoText == NULL)
		return E_OUTOFMEMORY;

	return NOERROR;
}

HRESULT CBaseParserFilter::Shutdown(void)
{
	// Shutdown the parser
	HRESULT hr = ShutdownParser();
	if (FAILED(hr))
		return hr;

	// Scope for the locking
	{
		// Protect the output pins state
		CAutoLock pinlock(&m_csPins);

		// Delete output pins (if any)
		if (m_nOutputPins > 0) {

			ASSERT(m_ppOutputPin);

			for (int i = 0; i < m_nOutputPins; i++) {

				if (m_ppOutputPin[i]) {

					// Disconnect the pin
					hr = m_ppOutputPin[i]->DisconnectBoth();
					if (FAILED(hr))
						return hr;

					// Release the pin
					m_ppOutputPin[i]->Release();

					// We've deleted pin -- so increment pin version
					IncrementPinVersion();
				}
			}

			// Delete output pins array
			delete[] m_ppOutputPin;
			m_ppOutputPin = NULL;

			m_nOutputPins = 0;
		}
	}

	// Scope for the locking
	{
		// Protect media content information
		CAutoLock infolock(&m_csInfo);

		// Reset the media content strings
		if (m_wszAuthorName) {
			CoTaskMemFree(m_wszAuthorName);
			m_wszAuthorName = NULL;
		}
		if (m_wszTitle) {
			CoTaskMemFree(m_wszTitle);
			m_wszTitle = NULL;
		}
		if (m_wszRating) {
			CoTaskMemFree(m_wszRating);
			m_wszRating = NULL;
		}
		if (m_wszDescription) {
			CoTaskMemFree(m_wszDescription);
			m_wszDescription = NULL;
		}
		if (m_wszCopyright) {
			CoTaskMemFree(m_wszCopyright);
			m_wszCopyright = NULL;
		}
		if (m_wszBaseURL) {
			CoTaskMemFree(m_wszBaseURL);
			m_wszBaseURL = NULL;
		}
		if (m_wszLogoURL) {
			CoTaskMemFree(m_wszLogoURL);
			m_wszLogoURL = NULL;
		}
		if (m_wszLogoIconURL) {
			CoTaskMemFree(m_wszLogoIconURL);
			m_wszLogoIconURL = NULL;
		}
		if (m_wszWatermarkURL) {
			CoTaskMemFree(m_wszWatermarkURL);
			m_wszWatermarkURL = NULL;
		}
		if (m_wszMoreInfoURL) {
			CoTaskMemFree(m_wszMoreInfoURL);
			m_wszMoreInfoURL = NULL;
		}
		if (m_wszMoreInfoBannerImage) {
			CoTaskMemFree(m_wszMoreInfoBannerImage);
			m_wszMoreInfoBannerImage = NULL;
		}
		if (m_wszMoreInfoBannerURL) {
			CoTaskMemFree(m_wszMoreInfoBannerURL);
			m_wszMoreInfoBannerURL = NULL;
		}
		if (m_wszMoreInfoText) {
			CoTaskMemFree(m_wszMoreInfoText);
			m_wszMoreInfoText = NULL;
		}
	}

	// Scope for the locking
	{
		// Protect the filter data
		CAutoLock datalock(&m_csData);
		
		// Reset the file position variables (no real need to do that)
		m_llStartPosition	= 0;
		m_llStopPosition	= MAXLONGLONG;
		m_llAdjustment		= 0;
		m_llDefaultStart	= 0;
		m_llDefaultStop		= MAXLONGLONG;

		// Reset the input pin properties
		m_cbInputBuffer		= 0;
		m_cbInputAlign		= 1;
	}
	
	return NOERROR;
}

HRESULT CBaseParserFilter::SetDefaultPositions(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Set the positions to the default values
	m_llStartPosition	= m_llDefaultStart;
	m_llStopPosition	= m_llDefaultStop;

	// Seek to the default positions
	return m_InputPin.Seek(m_llDefaultStart, m_llDefaultStop);
}

int CBaseParserFilter::GetPinCount(void)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// We've got one input and m_nOutputPins output pins
	return 1 + m_nOutputPins;
}

CBasePin* CBaseParserFilter::GetPin(int n)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	if (n < 0) {
		// Invalid index
		return NULL;
	} else if (n == 0) {
		// Input pin
		return &m_InputPin;
	} else if (n <= m_nOutputPins) {
		// Output pin
		return m_ppOutputPin[n-1];
	} else {
		// No more pins
		return NULL;
	}
}

BOOL CBaseParserFilter::IsFlushing(void)
{
	return m_InputPin.IsFlushing();
}

void CBaseParserFilter::SetAdjustment(LONGLONG llAdjustment)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	m_llAdjustment = llAdjustment;
}

HRESULT CBaseParserFilter::NotifyQuality(LPCWSTR pPinName, Quality q)
{
	// Call actual handler
	HRESULT hr = AlterQuality(pPinName, q);
	if (hr == S_OK) {
		// Return success code
		return NOERROR;
	} else if (hr == S_FALSE) {
		// Pass the message upstream
		return m_InputPin.PassNotify(q);
	} else {
		// Return error code
		return hr;
	}
}

HRESULT CBaseParserFilter::AlterQuality(LPCWSTR pPinName, Quality q)
{
	// Return S_FALSE indicating that we did not process the message
	return S_FALSE;
}

HRESULT CBaseParserFilter::SetPositions(
	LPCWSTR pPinName,
	const GUID *pCurrentFormat,
	LONGLONG *pllCurrent,
	DWORD dwCurrentFlags,
	LONGLONG *pllStop,
	DWORD dwStopFlags
)
{
	// Scope for the locking
	{
		// Protect the output pins state
		CAutoLock pinlock(&m_csPins);

		// If we got called then we should have at least one output pin
		ASSERT(m_ppOutputPin[0]);

		// Accept requests only from the first output pin
		if (lstrcmpW(pPinName, m_ppOutputPin[0]->Name()))
			return E_NOTIMPL;
	}

	// Convert the current position to file position
	LONGLONG llCurrent = 0;
	HRESULT hr = ConvertPosition(
		&llCurrent,
		pPinName,
		pCurrentFormat,
		pllCurrent,
		dwCurrentFlags
	);
	if (FAILED(hr))
		return hr;

	// Convert the stop position to file position
	LONGLONG llStop = 0;
	hr = ConvertPosition(
		&llStop,
		pPinName,
		pCurrentFormat,
		pllStop,
		dwStopFlags
	);
	if (FAILED(hr))
		return hr;

	// Sanity check of the stop position
	if (llStop <= llCurrent)
		llStop = m_llDefaultStop;

	// Ask the input pin to perform file seek
	hr = m_InputPin.Seek(llCurrent, llStop);
	if (FAILED(hr))
		return hr;

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Set the file positions
	m_llStartPosition	= llCurrent;
	m_llStopPosition	= llStop;

	return NOERROR;
}

HRESULT CBaseParserFilter::SetRate(
	LPCWSTR pPinName,
	REFERENCE_TIME rtCurrent,
	REFERENCE_TIME rtStop,
	double dRate
)
{
	// Scope for the locking
	{
		// Protect the output pins state
		CAutoLock pinlock(&m_csPins);

		// If we got called then we should have at least one output pin
		ASSERT(m_ppOutputPin[0]);

		// Accept requests only from the first output pin
		if (lstrcmpW(pPinName, m_ppOutputPin[0]->Name()))
			return E_NOTIMPL;
	}

	// We cannot support negative (reverse) plaback rate as the 
	// pullpin cannot pull data in the reverse direction
	if (dRate <= 0.)
		return E_INVALIDARG;

	// Deliver NewSegment() downstream and set rate for the 
	// filter's output pins
	return NewSegment(rtCurrent, rtStop, dRate);
}

HRESULT CBaseParserFilter::GetPreroll(
	LPCWSTR pPinName,
	const GUID *pCurrentFormat,
	LONGLONG *pllPreroll
)
{
	// By default there's no preroll
	*pllPreroll = 0;

	return NOERROR;
}

HRESULT CBaseParserFilter::Receive(IMediaSample *pSample)
{
	// Get data start and stop times
	REFERENCE_TIME rtStart, rtStop;
	HRESULT hr = pSample->GetTime(&rtStart, &rtStop);
	if (FAILED(hr))
		return hr;

	// Get the start and stop positions from reference times
	LONGLONG llStart, llStop;
	llStart	= rtStart / UNITS;
	llStop	= rtStop / UNITS;

	// Scope for the locking
	{
		// Protect the filter data
		CAutoLock datalock(&m_csData);

		// Adjust the start and stop positions
		llStart	+= m_llAdjustment;
		llStop	+= m_llAdjustment;
	}

	// Get data length
	LONG lDataSize = pSample->GetActualDataLength();

	// This should always be true
	ASSERT(llStop == llStart + lDataSize);

	// Get data buffer
	BYTE *pbData;
	hr = pSample->GetPointer(&pbData);
	if (FAILED(hr))
		return hr;

	LONGLONG llDataStart, llDataStop;

	// Scope for the locking
	{
		// Protect the filter data
		CAutoLock datalock(&m_csData);

		// Work out the data range we can receive
		llDataStart	= max(llStart, m_llStartPosition);
		llDataStop	= min(llStop, m_llStopPosition);
	}

	// If the range has zero or negative length return with no error
	if (llDataStart >= llDataStop)
		return NOERROR;

	// Delegate actual parsing to another Receive()
	return Receive(
		llDataStart,
		pbData + llDataStart - llStart,
		(LONG)(llDataStop - llDataStart)
	);
}

STDMETHODIMP CBaseParserFilter::Stop(void)
{
	CAutoLock lock1(&m_csFilter);

	if (m_State == State_Stopped)
		return NOERROR;

	// Decommit the input pin before locking or we can deadlock
	m_InputPin.Inactive();

	// Synchronize with Receive() calls
	CAutoLock lock2(&m_csReceive);

	// Scope for the locking
	{
		// Protect the output pins state
		CAutoLock pinlock(&m_csPins);

		// Decommit the output pins
		for (int i = 0; i < m_nOutputPins; i++) {
			if (m_ppOutputPin[i])
				m_ppOutputPin[i]->Inactive();
		}
	}

	m_State = State_Stopped;

	return NOERROR;
}

HRESULT CBaseParserFilter::BeginFlush(void)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Loop through all output pins and deliver BeginFlush() downstream
	for (int i = 0; i < m_nOutputPins; i++) {
		ASSERT(m_ppOutputPin[i]);
		HRESULT hr = m_ppOutputPin[i]->DeliverBeginFlush();
		if (FAILED(hr))
			return hr;
	}

	return NOERROR;
}

HRESULT CBaseParserFilter::EndFlush(void)
{
	// We should wait until all ongoing sample processing is done,
	// so try to get receive lock -- when Receive() finishes, it'll
	// free its lock and we'll proceed
	CAutoLock lock(&m_csReceive);

	// Now that we're out of any Receive() calls and we're sure not to 
	// enter one until we're done, so we can reset the parser so that 
	// it's ready to accept new data (e.g. after seeking)
	HRESULT hr = ResetParser();
	if (FAILED(hr))
		return hr;

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Loop through all output pins and deliver EndFlush() downstream
	for (int i = 0; i < m_nOutputPins; i++) {
		ASSERT(m_ppOutputPin[i]);
		hr = m_ppOutputPin[i]->DeliverEndFlush();
		if (FAILED(hr))
			return hr;
	}

	return NOERROR;
}

HRESULT CBaseParserFilter::NewSegment(
	REFERENCE_TIME tStart, 
	REFERENCE_TIME tStop, 
	double dRate
)
{
	// If we are called from the input pin's NewSegment(), we've already 
	// locked this critical section, but if we're called from SetRate(),
	// we should lock it
	CAutoLock lock(&m_csReceive);

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Loop through all output pins and deliver NewSegment() downstream
	for (int i = 0; i < m_nOutputPins; i++) {

		ASSERT(m_ppOutputPin[i]);

		// Set the rate on the output pin itself
		HRESULT hr = m_ppOutputPin[i]->NewSegment(tStart, tStop, dRate);
		if (FAILED(hr))
			return hr;

		// Deliver NewSegment() downstream
		hr = m_ppOutputPin[i]->DeliverNewSegment(tStart, tStop, dRate);
		if (FAILED(hr))
			return hr;
	}

	return NOERROR;
}

HRESULT CBaseParserFilter::EndOfStream(void)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Loop through all output pins and deliver EndOfStream() downstream
	for (int i = 0; i < m_nOutputPins; i++) {
		ASSERT(m_ppOutputPin[i]);
		HRESULT hr = m_ppOutputPin[i]->DeliverEndOfStream();
		if (FAILED(hr))
			return hr;
	}

	return NOERROR;
}

BOOL CBaseParserFilter::IsSyncInput(void)
{
	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	return m_bSyncInput;
}

HRESULT CBaseParserFilter::GetInputAllocatorProperties(ALLOCATOR_PROPERTIES *pRequest)
{
	// Check and validate the pointer
	CheckPointer(pRequest, E_POINTER);
	ValidateWritePtr(pRequest, sizeof(ALLOCATOR_PROPERTIES));

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	// Fill the properties by the stored values
	pRequest->cbAlign	= m_cbInputAlign;
	pRequest->cBuffers	= m_nInputBuffers;
	pRequest->cbBuffer	= m_cbInputBuffer;
	pRequest->cbPrefix	= 0;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::GetOutputPinCount(int *pnCount)
{
	// Check and validate the pointer
	CheckPointer(pnCount, E_POINTER);
	ValidateWritePtr(pnCount, sizeof(int));

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	*pnCount = m_nOutputPins;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::GetOutputPinName(int iPin, LPWSTR wszName)
{
	// Check the pointer
	CheckPointer(wszName, E_POINTER);

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	lstrcpyW(wszName, m_ppOutputPin[iPin]->Name());

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_SyncInput(BOOL *pbSyncInput)
{
	// Check and validate the pointer
	CheckPointer(pbSyncInput, E_POINTER);
	ValidateWritePtr(pbSyncInput, sizeof(BOOL));

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	*pbSyncInput = m_bSyncInput;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_InputBuffersNumber(long *pnBuffers)
{
	// Check and validate the pointer
	CheckPointer(pnBuffers, E_POINTER);
	ValidateWritePtr(pnBuffers, sizeof(long));

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	*pnBuffers = m_nInputBuffers;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_OutputBuffersNumber(int iPin, long *pnBuffers)
{
	// Check and validate the pointer
	CheckPointer(pnBuffers, E_POINTER);
	ValidateWritePtr(pnBuffers, sizeof(long));

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	*pnBuffers = m_ppOutputPin[iPin]->get_BuffersNumber();

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_AutoQueue(int iPin, BOOL *pbAutoQueue)
{
	// Check and validate the pointer
	CheckPointer(pbAutoQueue, E_POINTER);
	ValidateWritePtr(pbAutoQueue, sizeof(BOOL));

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	*pbAutoQueue = m_ppOutputPin[iPin]->get_AutoQueue();

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_Queue(int iPin, BOOL *pbQueue)
{
	// Check and validate the pointer
	CheckPointer(pbQueue, E_POINTER);
	ValidateWritePtr(pbQueue, sizeof(BOOL));

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	*pbQueue = m_ppOutputPin[iPin]->get_Queue();

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_BatchSize(int iPin, LONG *plBatchSize)
{
	// Check and validate the pointer
	CheckPointer(plBatchSize, E_POINTER);
	ValidateWritePtr(plBatchSize, sizeof(LONG));

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	*plBatchSize = m_ppOutputPin[iPin]->get_BatchSize();

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::get_BatchExact(int iPin, BOOL *pbBatchExact)
{
	// Check and validate the pointer
	CheckPointer(pbBatchExact, E_POINTER);
	ValidateWritePtr(pbBatchExact, sizeof(BOOL));

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	*pbBatchExact = m_ppOutputPin[iPin]->get_BatchExact();

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::put_SyncInput(BOOL bSyncInput)
{
	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	m_bSyncInput = bSyncInput;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::put_InputBuffersNumber(long nBuffers)
{
	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	m_nInputBuffers = nBuffers;

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::put_OutputBuffersNumber(int iPin, long nBuffers)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	m_ppOutputPin[iPin]->put_BuffersNumber(nBuffers);

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::put_AutoQueue(int iPin, BOOL bAutoQueue)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	m_ppOutputPin[iPin]->put_AutoQueue(bAutoQueue);

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::put_Queue(int iPin, BOOL bQueue)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	m_ppOutputPin[iPin]->put_Queue(bQueue);

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::put_BatchSize(int iPin, LONG lBatchSize)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	m_ppOutputPin[iPin]->put_BatchSize(lBatchSize);

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::put_BatchExact(int iPin, BOOL bBatchExact)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Check if the pin index is acceptable
	if ((iPin < 0) || (iPin >= m_nOutputPins))
		return E_INVALIDARG;

	m_ppOutputPin[iPin]->put_BatchExact(bBatchExact);

	return NOERROR;
}

STDMETHODIMP CBaseParserFilter::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_BaseParserPage;

	return NOERROR;
}

HRESULT CBaseParserFilter::RegisterPatterns(
	const GMF_FILETYPE_REGINFO *pRegInfo,
	BOOL bRegister
)
{
	// Convert media type GUID to string
	OLECHAR szMajorType[CHARS_IN_GUID];
	if (!StringFromGUID2(*pRegInfo->clsMajorType, szMajorType, CHARS_IN_GUID))
		return E_FAIL;

	// Convert media subtype GUID to string
	OLECHAR szMinorType[CHARS_IN_GUID];
	if (!StringFromGUID2(*pRegInfo->clsMinorType, szMinorType, CHARS_IN_GUID))
		return E_FAIL;

	// Compose registry path
	TCHAR szPath[MAX_PATH];
	wsprintf(
		szPath, 
		TEXT("Media Type\\%ls\\%ls"), 
		szMajorType,
		szMinorType
	);

	// Convert source filter CLSID to string
	OLECHAR szSrcFlt[CHARS_IN_GUID];
	if (!StringFromGUID2(*pRegInfo->clsSourceFilter, szSrcFlt, CHARS_IN_GUID))
		return E_FAIL;

	// Convert CLSID string from OLECHAR to TCHAR
	TCHAR szSourceFilter[MAX_PATH];
	wsprintf(
		szSourceFilter, 
		TEXT("%ls"), 
		szSrcFlt
	);

	if (bRegister) {

		// Delete the key if it exists
		RegDeleteKey(HKEY_CLASSES_ROOT, szPath);

		// Create the key
		HKEY hk;
		DWORD dwDisposition;
		LONG lr = RegCreateKeyEx(
			HKEY_CLASSES_ROOT,
			szPath,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_SET_VALUE,
			NULL,
			&hk,
			&dwDisposition
		);
		if (lr != ERROR_SUCCESS)
			return AmHresultFromWin32(lr);

		// Set the source filter CLSID
		lr = RegSetValueEx(
			hk,
			TEXT("Source Filter"),
			0,
			REG_SZ,
			(LPBYTE)szSourceFilter,
			sizeof(TCHAR) * (lstrlen(szSourceFilter) + 1)
		);
		if (lr != ERROR_SUCCESS) {
			RegCloseKey(hk);
			RegDeleteKey(HKEY_CLASSES_ROOT, szPath);
			return AmHresultFromWin32(lr);
		}

		for (UINT iEntry = 0; iEntry < pRegInfo->nEntries; iEntry++) {

			// First walk through all the patterns to find out 
			// how large string we should allocate
			ULONG ulLength;
			ulLength = 26 * pRegInfo->pEntry[iEntry].nPatterns; // 2 LONGs (2*11), 4 commas
			UINT iPattern;
			for (
				iPattern = 0; 
				iPattern < pRegInfo->pEntry[iEntry].nPatterns; 
				iPattern++
			)
				ulLength += 2 * pRegInfo->pEntry[iEntry].pPattern[iPattern].cbPattern;
			
			// Now compose the entry string
			TCHAR *pszEntry = (TCHAR*)CoTaskMemAlloc(ulLength);
			if (pszEntry == NULL) {
				RegCloseKey(hk);
				RegDeleteKey(HKEY_CLASSES_ROOT, szPath);
				return E_OUTOFMEMORY;
			}
			lstrcpy(pszEntry, "");
			for (
				iPattern = 0; 
				iPattern < pRegInfo->pEntry[iEntry].nPatterns; 
				iPattern++
			) {

				// Compose the heading of the pattern
				TCHAR szHead[26];
				wsprintf(
					szHead, 
					TEXT("%ld,%ld,,"), 
					pRegInfo->pEntry[iEntry].pPattern[iPattern].lOffset,
					pRegInfo->pEntry[iEntry].pPattern[iPattern].cbPattern
				);
				lstrcat(pszEntry, szHead);
				
				// Append hexadecimal digits of pattern value to the entry string
				for (
					LONG i = 0; 
					i < pRegInfo->pEntry[iEntry].pPattern[iPattern].cbPattern; 
					i++
				) {
					TCHAR szByte[3];
					wsprintf(szByte, TEXT("%02.2X"), pRegInfo->pEntry[iEntry].pPattern[iPattern].szValue[i]);
					lstrcat(pszEntry, szByte);
				}
				
				// Append closing comma if needed
				if (iPattern < pRegInfo->pEntry[iEntry].nPatterns - 1)
					lstrcat(pszEntry, ",");
			}

			// Append the entry string to registry
			TCHAR szNumber[11];
			wsprintf(szNumber, TEXT("%lu"), iEntry);
			lr = RegSetValueEx(
				hk,
				(LPCTSTR)szNumber,
				0,
				REG_SZ,
				(LPBYTE)pszEntry,
				sizeof(TCHAR) * (lstrlen(pszEntry) + 1)
			);
			CoTaskMemFree(pszEntry);
			if (lr != ERROR_SUCCESS) {
				RegCloseKey(hk);
				RegDeleteKey(HKEY_CLASSES_ROOT, szPath);
				return AmHresultFromWin32(lr);
			}
		}

		// Finally close the key
		RegCloseKey(hk);

	} else { // !bRegister

		// Try to open the key
		HKEY hk;
		LONG lr = RegOpenKeyEx(
			HKEY_CLASSES_ROOT,
			szPath,
			0,
			KEY_READ,
			&hk
		);
		// Delete the key if it exists
		if (lr == ERROR_SUCCESS) {
			RegCloseKey(hk);
			// Delete the key specified by its path
			lr = RegDeleteKey(HKEY_CLASSES_ROOT, szPath);
			if (lr != ERROR_SUCCESS)
				return AmHresultFromWin32(lr);
		}
	}

	return NOERROR;
}

HRESULT CBaseParserFilter::RegisterExtensions(
	const GMF_FILETYPE_REGINFO *pRegInfo,
	BOOL bRegister
)
{
	// Convert media type GUID to string
	OLECHAR szMajorType[CHARS_IN_GUID];
	if (!StringFromGUID2(*pRegInfo->clsMajorType, szMajorType, CHARS_IN_GUID))
		return E_FAIL;

	// Convert media subtype GUID to string
	OLECHAR szMinorType[CHARS_IN_GUID];
	if (!StringFromGUID2(*pRegInfo->clsMinorType, szMinorType, CHARS_IN_GUID))
		return E_FAIL;

	// Convert source filter CLSID to string
	OLECHAR szSourceFilter[CHARS_IN_GUID];
	if (!StringFromGUID2(*pRegInfo->clsSourceFilter, szSourceFilter, CHARS_IN_GUID))
		return E_FAIL;

	// Loop through the extensions
	for (UINT i = 0; i < pRegInfo->nExtensions; i++) {

		// Compose registry path
		TCHAR szPath[MAX_PATH];
		wsprintf(
			szPath,
			TEXT("Media Type\\Extensions\\%s"),
			pRegInfo->pExtension[i]
		);

		if (bRegister) {

			// Delete the key if it exists
			RegDeleteKey(HKEY_CLASSES_ROOT, szPath);

			// Create the key
			HKEY hk;
			DWORD dwDisposition;
			LONG lr = RegCreateKeyEx(
				HKEY_CLASSES_ROOT,
				szPath,
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_SET_VALUE,
				NULL,
				&hk,
				&dwDisposition
			);
			if (lr != ERROR_SUCCESS)
				return AmHresultFromWin32(lr);

			// Convert major type CLSID string from OLECHAR to TCHAR
			TCHAR szValue[MAX_PATH];
			wsprintf(
				szValue, 
				TEXT("%ls"), 
				szMajorType
			);

			// Set the media type
			lr = RegSetValueEx(
				hk,
				TEXT("Media Type"),
				0,
				REG_SZ,
				(LPBYTE)szValue,
				sizeof(TCHAR) * (lstrlen(szValue) + 1)
			);
			if (lr != ERROR_SUCCESS) {
				RegCloseKey(hk);
				RegDeleteKey(HKEY_CLASSES_ROOT, szPath);
				return AmHresultFromWin32(lr);
			}

			// Convert minor type CLSID string from OLECHAR to TCHAR
			wsprintf(
				szValue, 
				TEXT("%ls"), 
				szMinorType
			);

			// Set the media subtype
			lr = RegSetValueEx(
				hk,
				TEXT("Subtype"),
				0,
				REG_SZ,
				(LPBYTE)szValue,
				sizeof(TCHAR) * (lstrlen(szValue) + 1)
			);
			if (lr != ERROR_SUCCESS) {
				RegCloseKey(hk);
				RegDeleteKey(HKEY_CLASSES_ROOT, szPath);
				return AmHresultFromWin32(lr);
			}

			// Convert source filter CLSID string from OLECHAR to TCHAR
			wsprintf(
				szValue, 
				TEXT("%ls"), 
				szSourceFilter
			);

			// Set the source filter
			lr = RegSetValueEx(
				hk,
				TEXT("Source Filter"),
				0,
				REG_SZ,
				(LPBYTE)szValue,
				sizeof(TCHAR) * (lstrlen(szValue) + 1)
			);
			if (lr != ERROR_SUCCESS) {
				RegCloseKey(hk);
				RegDeleteKey(HKEY_CLASSES_ROOT, szPath);
				return AmHresultFromWin32(lr);
			}

			// Finally close the key
			RegCloseKey(hk);

		} else { // !bRegister

			// Try to open the key
			HKEY hk;
			LONG lr = RegOpenKeyEx(
				HKEY_CLASSES_ROOT,
				szPath,
				0,
				KEY_READ,
				&hk
			);
			// Delete the key if it exists
			if (lr == ERROR_SUCCESS) {
				RegCloseKey(hk);
				// Delete the key specified by its path
				lr = RegDeleteKey(HKEY_CLASSES_ROOT, szPath);
				if (lr != ERROR_SUCCESS)
					return AmHresultFromWin32(lr);
			}
		}
	}

	return NOERROR;
}

//==========================================================================
// CBaseParserPage methods
//==========================================================================

const WCHAR g_wszBaseParserPageName[] = L"ANX Base Parser Property Page";

CBaseParserPage::CBaseParserPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("Base Parser Property Page"),
		pUnk,
		IDD_BASEPARSERPAGE,
		IDS_TITLE_BASEPARSERPAGE
	),
	m_pConfig(NULL),			// No config interface at this time
	m_nOutputPins(0),			// No output pins at this time
	m_pOutputPinOptions(NULL)	// No array at this time
{
	InitCommonControls();
}

CUnknown* WINAPI CBaseParserPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CBaseParserPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}

void CBaseParserPage::SetDirty()
{
	// Set the dirty flag
	m_bDirty = TRUE;

	// Notify the page site of the dirty state
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

UINT CBaseParserPage::GetTrackBarPos(int nTrackBarID, int nTextID)
{
	// Get the position
	UINT nPos = (UINT)SendDlgItemMessage(m_Dlg, nTrackBarID, TBM_GETPOS, 0, 0);

	// Update the trackbar's text
	SetDlgItemInt(m_Dlg, nTextID, nPos, FALSE);

	return nPos;
}

int CBaseParserPage::GetComboBoxCurSel(int nID)
{
	// Get the current selection in the combo box
	int iSel = (int)SendDlgItemMessage(m_Dlg, nID, CB_GETCURSEL, 0, 0);
	if (iSel == CB_ERR)
		return -1;

	return iSel;
}

void CBaseParserPage::UpdateOutputPinGroup(int iSel)
{
	// We should not get called if there're no output opins
	if (m_pOutputPinOptions == NULL)
		return;

	// Get the options from the output pin options array
	// and set the controls in the output pin group accordingly

	SendDlgItemMessage(
		m_Dlg,
		IDC_OUTPUTBUFFERS,
		TBM_SETPOS,
		TRUE,
		m_pOutputPinOptions[iSel].nOutputBuffers
	);
	GetTrackBarPos(IDC_OUTPUTBUFFERS, IDC_OUTPUTBUFFERSTEXT);
	CheckDlgButton(
		m_Dlg,
		IDC_AUTOQUEUE,
		(m_pOutputPinOptions[iSel].bAutoQueue) ? BST_CHECKED : BST_UNCHECKED
	);
	EnableWindow(
		GetDlgItem(m_Dlg, IDC_QUEUE),
		!(m_pOutputPinOptions[iSel].bAutoQueue)
	);
	CheckDlgButton(
		m_Dlg,
		IDC_QUEUE,
		(m_pOutputPinOptions[iSel].bQueue) ? BST_CHECKED : BST_UNCHECKED
	);
	SendDlgItemMessage(
		m_Dlg,
		IDC_BATCHSIZE,
		TBM_SETPOS,
		TRUE,
		m_pOutputPinOptions[iSel].lBatchSize
	);
	GetTrackBarPos(IDC_BATCHSIZE, IDC_BATCHSIZETEXT);
	CheckDlgButton(
		m_Dlg,
		IDC_BATCHEXACT,
		(m_pOutputPinOptions[iSel].bBatchExact) ? BST_CHECKED : BST_UNCHECKED
	);
}

void CBaseParserPage::UpdateFileTypeExtensions(int iSel)
{
	// Reset the extensions control
	SendDlgItemMessage(m_Dlg, IDC_EXTENSIONS, WM_SETTEXT, 0, (LPARAM)TEXT(""));

	// Get the extensions
	UINT nExtensions = 0;
	LPCTSTR *pExtensions = NULL;
	HRESULT hr = m_pConfig->GetFileTypeExtensions(iSel, &nExtensions, &pExtensions);
	if (FAILED(hr))
		return;

	// Compose the single string out of the obtained extensions
	TCHAR szExtensions[256];
	lstrcpy(szExtensions, TEXT(""));
	for (UINT i = 0; i < nExtensions; i++) {
		lstrcat(szExtensions, pExtensions[i]);
		if (i != nExtensions - 1)
			lstrcat(szExtensions, TEXT(" "));
	}

	// Set the extensions control text
	SendDlgItemMessage(m_Dlg, IDC_EXTENSIONS, WM_SETTEXT, 0, (LPARAM)szExtensions);
}

BOOL CBaseParserPage::OnReceiveMessage(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMsg) {

		case WM_HSCROLL:

			switch (GetDlgCtrlID((HWND)lParam)) {

				case IDC_INPUTBUFFERS:
					SetDirty();
					GetTrackBarPos(IDC_INPUTBUFFERS, IDC_INPUTBUFFERSTEXT);
					break;

				case IDC_OUTPUTBUFFERS:
					SetDirty();
					m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].nOutputBuffers = 
						(long)GetTrackBarPos(IDC_OUTPUTBUFFERS, IDC_OUTPUTBUFFERSTEXT);
					break;

				case IDC_BATCHSIZE:
					SetDirty();
					m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].lBatchSize = 
						(LONG)GetTrackBarPos(IDC_BATCHSIZE, IDC_BATCHSIZETEXT);
					break;
			}

			return (LRESULT)1;

		case WM_COMMAND:

			switch (LOWORD(wParam)) {

				case IDC_SYNCINPUT:
					SetDirty();
					break;

				case IDC_PINNAME:
					// Update the output pin's group of controls
					if (HIWORD(wParam) == CBN_SELCHANGE)
						UpdateOutputPinGroup(GetComboBoxCurSel(IDC_PINNAME));
					break;

				case IDC_AUTOQUEUE:
					SetDirty();
					EnableWindow(GetDlgItem(m_Dlg, IDC_QUEUE), !GetCheckBox(IDC_AUTOQUEUE));
					m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].bAutoQueue = GetCheckBox(IDC_AUTOQUEUE);
					break;

				case IDC_QUEUE:
					SetDirty();
					m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].bQueue = GetCheckBox(IDC_QUEUE);
					break;

				case IDC_BATCHEXACT:
					SetDirty();
					m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].bBatchExact = GetCheckBox(IDC_BATCHEXACT);
					break;

				case IDC_DEFAULTS:
					CheckDlgButton(m_Dlg, IDC_SYNCINPUT, BST_UNCHECKED);
					SendDlgItemMessage(m_Dlg, IDC_INPUTBUFFERS, TBM_SETPOS, TRUE, 1);
					GetTrackBarPos(IDC_INPUTBUFFERS, IDC_INPUTBUFFERSTEXT);
					if (m_pOutputPinOptions != NULL) {
						SendDlgItemMessage(m_Dlg, IDC_OUTPUTBUFFERS, TBM_SETPOS, TRUE, 1);
						GetTrackBarPos(IDC_OUTPUTBUFFERS, IDC_OUTPUTBUFFERSTEXT);
						m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].nOutputBuffers = 1;
						CheckDlgButton(m_Dlg, IDC_AUTOQUEUE, BST_UNCHECKED);
						m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].bAutoQueue = FALSE;
						EnableWindow(GetDlgItem(m_Dlg, IDC_QUEUE), TRUE);
						CheckDlgButton(m_Dlg, IDC_QUEUE, BST_CHECKED);
						m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].bQueue = TRUE;
						SendDlgItemMessage(m_Dlg, IDC_BATCHSIZE, TBM_SETPOS, TRUE, 1);
						GetTrackBarPos(IDC_BATCHSIZE, IDC_BATCHSIZETEXT);
						m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].lBatchSize = 1;
						CheckDlgButton(m_Dlg, IDC_BATCHEXACT, BST_UNCHECKED);
						m_pOutputPinOptions[GetComboBoxCurSel(IDC_PINNAME)].bBatchExact = FALSE;
					}
					SetDirty();
					break;

				case IDC_TYPENAME:
					// Update the format's extensions list
					if (HIWORD(wParam) == CBN_SELCHANGE)
						UpdateFileTypeExtensions(GetComboBoxCurSel(IDC_TYPENAME));
					break;

				case IDC_PATTERNS_REGISTER:
					m_pConfig->RegisterFileTypePatterns(GetComboBoxCurSel(IDC_TYPENAME), TRUE);
					break;

				case IDC_PATTERNS_UNREGISTER:
					m_pConfig->RegisterFileTypePatterns(GetComboBoxCurSel(IDC_TYPENAME), FALSE);
					break;

				case IDC_EXTENSIONS_REGISTER:
					m_pConfig->RegisterFileTypeExtensions(GetComboBoxCurSel(IDC_TYPENAME), TRUE);
					break;

				case IDC_EXTENSIONS_UNREGISTER:
					m_pConfig->RegisterFileTypeExtensions(GetComboBoxCurSel(IDC_TYPENAME), FALSE);
					break;
			}

			return (LRESULT)1;
	}

	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CBaseParserPage::OnConnect(IUnknown *pUnknown)
{
	// At this time there should be no config interface
	ASSERT(m_pConfig == NULL);

	// Retrieve the config interface from the filter
	HRESULT hr = pUnknown->QueryInterface(
		IID_IConfigBaseParser,
		(void**)&m_pConfig
	);
	if (FAILED(hr))
		return hr;

	ASSERT(m_pConfig);

	return NOERROR;
}

HRESULT CBaseParserPage::OnDisconnect()
{
	// At this time we should have the config interface
	if (m_pConfig == NULL)
		return E_UNEXPECTED;

	// Release the config interface
	m_pConfig->Release();
	m_pConfig = NULL;

	return NOERROR;
}

void CBaseParserPage::FreeOutputPinOptions(void)
{
	// Free the output pin options array
	if (m_pOutputPinOptions) {
		CoTaskMemFree(m_pOutputPinOptions);
		m_pOutputPinOptions = NULL;
	}

	// Reset the output pins count
	m_nOutputPins = 0;
}

HRESULT CBaseParserPage::OnActivate()
{
	// At this time we should have the config interface
	if (m_pConfig == NULL)
		return E_UNEXPECTED;

	// Set ranges and tics on the trackbars
	SendDlgItemMessage(m_Dlg, IDC_INPUTBUFFERS, TBM_SETRANGE, TRUE, MAKELONG(1, 64));
	SendDlgItemMessage(m_Dlg, IDC_INPUTBUFFERS, TBM_SETTICFREQ, 4, 0);
	SendDlgItemMessage(m_Dlg, IDC_OUTPUTBUFFERS, TBM_SETRANGE, TRUE, MAKELONG(1, 64));
	SendDlgItemMessage(m_Dlg, IDC_OUTPUTBUFFERS, TBM_SETTICFREQ, 4, 0);
	SendDlgItemMessage(m_Dlg, IDC_BATCHSIZE, TBM_SETRANGE, TRUE, MAKELONG(1, 64));
	SendDlgItemMessage(m_Dlg, IDC_BATCHSIZE, TBM_SETTICFREQ, 4, 0);

	// Set up the file type combo box
	int nFileTypes = 0;
	HRESULT hr = m_pConfig->GetFileTypeCount(&nFileTypes);
	if (FAILED(hr))
		return hr;
	SendDlgItemMessage(m_Dlg, IDC_TYPENAME, CB_RESETCONTENT, 0, 0);

	// Populate the combo box
	for (int i = 0; i < nFileTypes; i++) {

		// Get the file type name
		TCHAR szFileTypeName[256];
		hr = m_pConfig->GetFileTypeName(i, szFileTypeName);
		if (FAILED(hr))
			return hr;

		// Insert the file type name string into the combo box
		SendDlgItemMessage(m_Dlg, IDC_TYPENAME, CB_INSERTSTRING, i, (LPARAM)szFileTypeName);
	}
	SendDlgItemMessage(m_Dlg, IDC_TYPENAME, CB_SETCURSEL, 0, 0);
	UpdateFileTypeExtensions(0);

	// Get the options from the underlying config interface
	// and set the controls in the input pin group accordingly

	long nInputBuffers;
	hr = m_pConfig->get_InputBuffersNumber(&nInputBuffers);
	if (FAILED(hr))
		return hr;
	SendDlgItemMessage(m_Dlg, IDC_INPUTBUFFERS, TBM_SETPOS, TRUE, nInputBuffers);
	GetTrackBarPos(IDC_INPUTBUFFERS, IDC_INPUTBUFFERSTEXT);

	BOOL bSyncInput;
	hr = m_pConfig->get_SyncInput(&bSyncInput);
	if (FAILED(hr))
		return hr;
	CheckDlgButton(m_Dlg, IDC_SYNCINPUT, (bSyncInput) ? BST_CHECKED : BST_UNCHECKED);

	// Initialize the output pin's combo box
	hr = m_pConfig->GetOutputPinCount(&m_nOutputPins);
	if (FAILED(hr))
		return hr;
	SendDlgItemMessage(m_Dlg, IDC_PINNAME, CB_RESETCONTENT, 0, 0);

	// Allocate the output pin options array
	ASSERT(m_pOutputPinOptions == NULL);
	if (m_nOutputPins > 0) {
		m_pOutputPinOptions = (tagOutputPinOptions*)CoTaskMemAlloc(m_nOutputPins * sizeof(tagOutputPinOptions));
		if (m_pOutputPinOptions == NULL)
			return E_OUTOFMEMORY;
	} else {
		// If there're no output pins, disable all relevant controls
		EnableWindow(GetDlgItem(m_Dlg, IDC_STATIC1), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_STATIC2), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_STATIC3), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_STATIC4), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_STATIC5), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_STATIC6), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_PINNAME), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_OUTPUTBUFFERSTEXT), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_OUTPUTBUFFERS), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_AUTOQUEUE), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_QUEUE), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_BATCHSIZETEXT), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_BATCHSIZE), FALSE);
		EnableWindow(GetDlgItem(m_Dlg, IDC_BATCHEXACT), FALSE);
		m_pOutputPinOptions = NULL;
	}

	int i = 0;
	for (i = 0; i < m_nOutputPins; i++) {

		// Get the pin name
		WCHAR wszPinName[256];
		hr = m_pConfig->GetOutputPinName(i, wszPinName);
		if (FAILED(hr)) {
			FreeOutputPinOptions();
			return hr;
		}

		// Convert the wide string to TCHAR string
		TCHAR szPinName[256];
		wsprintf(szPinName, "%ls", wszPinName);

		// Insert the pin name string into the combo box
		SendDlgItemMessage(m_Dlg, IDC_PINNAME, CB_INSERTSTRING, i, (LPARAM)szPinName);

		hr = m_pConfig->get_OutputBuffersNumber(i, &m_pOutputPinOptions[i].nOutputBuffers);
		if (FAILED(hr)) {
			FreeOutputPinOptions();
			return hr;
		}

		hr = m_pConfig->get_AutoQueue(i, &m_pOutputPinOptions[i].bAutoQueue);
		if (FAILED(hr)) {
			FreeOutputPinOptions();
			return hr;
		}

		hr = m_pConfig->get_Queue(i, &m_pOutputPinOptions[i].bQueue);
		if (FAILED(hr)) {
			FreeOutputPinOptions();
			return hr;
		}

		hr = m_pConfig->get_BatchSize(i, &m_pOutputPinOptions[i].lBatchSize);
		if (FAILED(hr)) {
			FreeOutputPinOptions();
			return hr;
		}

		hr = m_pConfig->get_BatchExact(i, &m_pOutputPinOptions[i].bBatchExact);
		if (FAILED(hr)) {
			FreeOutputPinOptions();
			return hr;
		}
	}
	SendDlgItemMessage(m_Dlg, IDC_PINNAME, CB_SETCURSEL, 0, 0);

	// Set the output pin group according to the option values
	UpdateOutputPinGroup(0);

	m_bDirty = FALSE;

	return NOERROR;
}

HRESULT CBaseParserPage::OnDeactivate(void)
{
	FreeOutputPinOptions();

	return NOERROR;
}

BOOL CBaseParserPage::GetCheckBox(int nID)
{
	return (IsDlgButtonChecked(m_Dlg, nID) == BST_CHECKED);
}

HRESULT CBaseParserPage::OnApplyChanges()
{
	if (m_bDirty) {

		// At this time we should have the config interface
		if (m_pConfig == NULL)
			return E_UNEXPECTED;

		// Put the input pin options for the filter

		UINT nPos = GetTrackBarPos(IDC_INPUTBUFFERS, IDC_INPUTBUFFERSTEXT);
		HRESULT hr = m_pConfig->put_InputBuffersNumber((long)nPos);
		if (FAILED(hr))
			return hr;

		hr = m_pConfig->put_SyncInput(GetCheckBox(IDC_SYNCINPUT));
		if (FAILED(hr))
			return hr;

		// Put the output pin options for the filter

		for (int i = 0; i < m_nOutputPins; i++) {

			hr = m_pConfig->put_OutputBuffersNumber(i, m_pOutputPinOptions[i].nOutputBuffers);
			if (FAILED(hr))
				return hr;

			hr = m_pConfig->put_AutoQueue(i, m_pOutputPinOptions[i].bAutoQueue);
			if (FAILED(hr))
				return hr;

			if (!m_pOutputPinOptions[i].bAutoQueue) {
				hr = m_pConfig->put_Queue(i, m_pOutputPinOptions[i].bQueue);
				if (FAILED(hr))
					return hr;
			}

			hr = m_pConfig->put_BatchSize(i, m_pOutputPinOptions[i].lBatchSize);
			if (FAILED(hr))
				return hr;

			hr = m_pConfig->put_BatchExact(i, m_pOutputPinOptions[i].bBatchExact);
			if (FAILED(hr))
				return hr;
		}

		m_bDirty = FALSE;
	}

	return NOERROR;
}

//==========================================================================
// CPassThruInputPin methods
//==========================================================================

CPassThruInputPin::CPassThruInputPin(
	CBaseFilter *pFilter,
	CCritSec *pFilterLock,
	CCritSec *pReceiveLock,
	HRESULT *phr,
	LPCWSTR pInputName,
	LPCWSTR pOutputName,
	CPassThruOutputPin **ppOutputPin
) :
	CBaseInputPin(
		NAME("Pass-Through Input Pin"),
		pFilter,
		pFilterLock,
		phr,
		pInputName
	),
	m_pReceiveLock(pReceiveLock),
	m_OutputPin(
		pFilter,
		this,
		pFilterLock,
		phr,
		pOutputName
	)
{
	ASSERT(pFilter);
	ASSERT(pFilterLock);
	ASSERT(pReceiveLock);
	ASSERT(phr);
	ASSERT(ppOutputPin);

	*ppOutputPin = &m_OutputPin;
}

STDMETHODIMP_(ULONG) CPassThruInputPin::NonDelegatingAddRef(void)
{
	// Call the base-class implementation (it does not do correct 
	// refcounting but refcounts parent filter)
	CBaseInputPin::NonDelegatingAddRef();

	// Call the CUnknown implementation (it does correct refcounting).
	// In case of DEBUG build we're doing double refcounting here, 
	// but that's no problem since Release() does double dereferencing
	return CUnknown::NonDelegatingAddRef();
}

STDMETHODIMP_(ULONG) CPassThruInputPin::NonDelegatingRelease(void)
{
	// Call the base-class implementation (it does not do correct 
	// dereferencing but dereferences parent filter)
	CBaseInputPin::NonDelegatingRelease();

	// Call the CUnknown implementation (it does correct dereferencing
	// and deletes the object when it's due time)
	return CUnknown::NonDelegatingRelease();
}

HRESULT CPassThruInputPin::CheckMediaType(const CMediaType* pmt)
{
	// Accept everything
	return S_OK;
}

HRESULT CPassThruInputPin::BreakConnect(void)
{
	// Disconnect the output pin
	HRESULT hr = m_OutputPin.DisconnectBoth();
	if (FAILED(hr))
		return hr;

	// Let the base class break its connection
	return CBaseInputPin::BreakConnect();
}
	
STDMETHODIMP CPassThruInputPin::BeginFlush(void)
{
	CAutoLock lock(m_pLock);

	// Perform the base-class BeginFlush
	HRESULT hr = CBaseInputPin::BeginFlush();
	if (FAILED(hr))
		return hr;

	// Delegate BeginFlush delivery to the output pin
	return m_OutputPin.DeliverBeginFlush();
}

STDMETHODIMP CPassThruInputPin::EndFlush(void)
{
	CAutoLock lock(m_pLock);

	// Delegate EndFlush delivery to the output pin
	HRESULT hr = m_OutputPin.DeliverEndFlush();
	if (FAILED(hr))
		return hr;

	// Perform the base-class EndFlush
	return CBaseInputPin::EndFlush();
}

STDMETHODIMP CPassThruInputPin::Receive(IMediaSample *pSample)
{
	CAutoLock lock(m_pReceiveLock);

	// Delegate the delivery to the output pin
	return m_OutputPin.Deliver(pSample);
}

STDMETHODIMP CPassThruInputPin::NewSegment(
	REFERENCE_TIME tStart, 
	REFERENCE_TIME tStop, 
	double dRate
)
{
	CAutoLock lock(m_pReceiveLock);

	// Delegate NewSegment delivery to the output pin
	HRESULT hr = m_OutputPin.DeliverNewSegment(tStart, tStop, dRate);
	if (FAILED(hr))
		return hr;

	// Perform the base-class NewSegment
	return CBaseInputPin::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CPassThruInputPin::EndOfStream(void)
{
	CAutoLock lock(m_pReceiveLock);

	// Check if we can accept end of stream message
	HRESULT hr = CheckStreaming();
	if (hr != S_OK)
		return hr;
	
	// Delegate EndOfStream delivery to the output pin
	hr = m_OutputPin.DeliverEndOfStream();
	if (FAILED(hr))
		return hr;

	// Perform the base-class EndOfStream
	return CBaseInputPin::EndOfStream();
}

HRESULT CPassThruInputPin::DisconnectBoth(void)
{
	CAutoLock lock(m_pLock);

	// See if the filter is active
	if (!IsStopped())
		return VFW_E_NOT_STOPPED;

	// First disconnect the peer pin (so that it Release()'s us)
	if (m_Connected) {
		HRESULT hr = m_Connected->Disconnect();
		if (FAILED(hr))
			return hr;
	}

	// Now disconnect ourselves
	return Disconnect();
}

//==========================================================================
// CPassThruOutputPin methods
//==========================================================================

CPassThruOutputPin::CPassThruOutputPin(
	CBaseFilter *pFilter,
	CPassThruInputPin *pParentPin,
	CCritSec *pFilterLock,
	HRESULT *phr,
	LPCWSTR pName
) :
	CBaseOutputPin(
		NAME("Pass-Through Output Pin"),
		pFilter,
		pFilterLock,
		phr,
		pName
	),
	m_pParentPin(pParentPin)
{
	ASSERT(pFilter);
	ASSERT(pParentPin);
	ASSERT(pFilterLock);
	ASSERT(phr);

	// Create pass-through seeking and position object
	m_pPosition = NULL;
	*phr = CreatePosPassThru(
		GetOwner(),
		FALSE,
		(IPin*)pParentPin,
		&m_pPosition
	);
	if (m_pPosition == NULL)
		*phr = E_OUTOFMEMORY;
}

CPassThruOutputPin::~CPassThruOutputPin()
{
	// Release the pass-through seeking and position object
	if (m_pPosition) {
		m_pPosition->Release();
		m_pPosition = NULL;
	}
}

STDMETHODIMP CPassThruOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	// Check and validate the pointer
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	// Expose IMediaSeeking (and the base-class interfaces)
	if ((riid == IID_IMediaSeeking) || (riid == IID_IMediaPosition)) {
		if (m_pPosition == NULL)
			return E_NOINTERFACE;
		return m_pPosition->QueryInterface(riid, ppv);
	} else
		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CPassThruOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
	// If we've got a quality sink, pass the message to it
	if (m_pQSink != NULL)
		return m_pQSink->Notify(m_pFilter, q);

	// Otherwise just pass the message upstream
	return m_pParentPin->PassNotify(q);
}

HRESULT CPassThruOutputPin::CheckMediaType(const CMediaType *pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	CAutoLock lock(m_pLock);

	// If the parent input pin is not connected, reject any type
	if (!m_pParentPin->IsConnected())
		return S_FALSE;

	// Otherwise get the parent pin's connection media type
	AM_MEDIA_TYPE mt;
	HRESULT hr = m_pParentPin->ConnectionMediaType(&mt);
	if (FAILED(hr))
		return hr;

	// Compare it to the proposed media type
	hr = (*pmt == mt) ? S_OK : S_FALSE;

	// Free the input pin's media type
	FreeMediaType(mt);

	return hr;
}

HRESULT CPassThruOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Check and validate the pointer
	CheckPointer(pMediaType, E_POINTER);
	ValidateWritePtr(pMediaType, sizeof(CMediaType));

	CAutoLock lock(m_pLock);

	// If the parent input pin is not connected, we've got no types
	if (!m_pParentPin->IsConnected())
		return E_UNEXPECTED;

	// Support only one media type
	if (iPosition < 0)
		return E_INVALIDARG;
	else if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;
	else {

		// Get the parent pin's connection media type
		AM_MEDIA_TYPE mt;
		HRESULT hr = m_pParentPin->ConnectionMediaType(&mt);
		if (FAILED(hr))
			return hr;

		*pMediaType = mt;

		// Free the input pin's media type
		FreeMediaType(mt);
	}

	return S_OK;
}

HRESULT CPassThruOutputPin::CheckConnect(IPin *pPin)
{
	// Check and validate the pointer
	CheckPointer(pPin, E_POINTER);
	ValidateReadPtr(pPin, sizeof(IPin));

	// Perform base-class checking
	HRESULT hr = CBaseOutputPin::CheckConnect(pPin);
	if (FAILED(hr))
		return hr;

	// If the parent input pin is not connected, do not connect
	return (m_pParentPin->IsConnected()) ? NOERROR : E_UNEXPECTED;
}

HRESULT CPassThruOutputPin::DecideAllocator(
	IMemInputPin *pPin,
	IMemAllocator **ppAlloc
)
{
	// Check and validate the pointers
	CheckPointer(pPin, E_POINTER);
	ValidateReadPtr(pPin, sizeof(IMemInputPin));
	CheckPointer(ppAlloc, E_POINTER);
	ValidateWritePtr(ppAlloc, sizeof(IMemAllocator*));

	// If the parent input pin is not connected, we've got no allocator
	if (!m_pParentPin->IsConnected())
		return E_UNEXPECTED;

	// Get the parent input pin's allocator
	HRESULT hr = m_pParentPin->GetAllocator(ppAlloc);
	if (FAILED(hr)) {
		*ppAlloc = NULL;
		return hr;
	}

	// Notify the peer pin of our allocator.
	// If the peer pin rejects our allocator we'll fail:
	// we're not going to create the allocator on our own
	// since it requires undesired copying of data
	hr = pPin->NotifyAllocator(*ppAlloc, m_pParentPin->IsReadOnly());
	if (FAILED(hr)) {
		(*ppAlloc)->Release();
		*ppAlloc = NULL;
		return hr;
	}

	return NOERROR;
}

HRESULT CPassThruOutputPin::DecideBufferSize(IMemAllocator *pMemAllocator, ALLOCATOR_PROPERTIES *pRequest)
{
	// Just do nothing...
	return NOERROR;
}

HRESULT CPassThruOutputPin::DisconnectBoth(void)
{
	CAutoLock lock(m_pLock);

	// See if the filter is active
	if (!IsStopped())
		return VFW_E_NOT_STOPPED;

	// First disconnect the peer pin (so that it Release()'s us)
	if (m_Connected) {
		HRESULT hr = m_Connected->Disconnect();
		if (FAILED(hr))
			return hr;
	}

	// Now disconnect ourselves
	return Disconnect();
}

//==========================================================================
// CParserSeeking methods
//==========================================================================

CParserSeeking::CParserSeeking(
	const TCHAR *pName,
	LPUNKNOWN pUnk,
	CBaseParserFilter *pFilter,
	CParserOutputPin *pPin,
	DWORD dwCapabilities,
	int nTimeFormats,
	GUID *pTimeFormats
) :
	CUnknown(pName, pUnk),
	m_pFilter(pFilter),
	m_pPin(pPin),
	m_dwCapabilities(dwCapabilities),
	m_nTimeFormats(nTimeFormats),
	m_pTimeFormats(pTimeFormats),
	m_iTimeFormat(0),				// Default is the preferred time format (the first one)
	m_llCurrentPosition(0),			// Default is zero
	m_llStopPosition(MAXLONGLONG)	// Guard value
{
	ASSERT(pFilter);
	ASSERT(pPin);
	ASSERT(nTimeFormats >= 0);

	// Reset the seeking positions
	Reset();
}

CParserSeeking::~CParserSeeking()
{
	// Free the array of supported time formats
	if (m_pTimeFormats) {
		CoTaskMemFree(m_pTimeFormats);
		m_pTimeFormats = NULL;
	}
}

STDMETHODIMP CParserSeeking::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	// Check and validate the pointer
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	// Expose IMediaSeeking
	if (riid == IID_IMediaSeeking)
		return GetInterface((IMediaSeeking*)this, ppv);
	else
		return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CParserSeeking::GetCapabilities(DWORD *pCapabilities)
{
	// Check and validate the pointer
	CheckPointer(pCapabilities, E_POINTER);
	ValidateWritePtr(pCapabilities, sizeof(DWORD));

	*pCapabilities = m_dwCapabilities;

	return NOERROR;
}

STDMETHODIMP CParserSeeking::CheckCapabilities(DWORD *pCapabilities)
{
	// Check and validate the pointer
	CheckPointer(pCapabilities, E_POINTER);
	ValidateReadWritePtr(pCapabilities, sizeof(DWORD));

	// Test the requested capabilities against the available ones
	DWORD dwRequestedCaps = *pCapabilities;
	*pCapabilities &= m_dwCapabilities;

	// Work out a return value
	if (*pCapabilities == dwRequestedCaps) {
		// All capabilities are present
		return S_OK;
	} else if (*pCapabilities != 0) {	
		// Not all of the capabilities are present
		return S_FALSE;
	} else {
		// No capabilities are present
		return E_FAIL;
	}
}

STDMETHODIMP CParserSeeking::SetTimeFormat(const GUID *pFormat)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pFormat, E_POINTER);
	ValidateReadPtr(pFormat, sizeof(GUID));

	// If the graph is not stopped we cannot set time format
	if (!m_pFilter->IsStopped())
		return VFW_E_WRONG_STATE;

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_INVALIDARG;

	// Loop through the supported time formats and try to find the
	// requested format among them
	for (int i = 0; i < m_nTimeFormats; i++) {
		if (IsEqualGUID(*pFormat, m_pTimeFormats[i])) {

			// Store the old time format
			int iOldTimeFormat = m_iTimeFormat;

			// Set the new time format
			m_iTimeFormat = i;

			// Convert the start position to the new time format
			LONGLONG llNewCurrent = 0;
			HRESULT hr = ConvertTimeFormat(
				&llNewCurrent,
				&m_pTimeFormats[m_iTimeFormat],
				m_llCurrentPosition,
				&m_pTimeFormats[iOldTimeFormat]
			);
			if (FAILED(hr))
				return hr;

			// Convert the stop position to the new time format
			LONGLONG llNewStop = 0;
			hr = ConvertTimeFormat(
				&llNewStop,
				&m_pTimeFormats[m_iTimeFormat],
				m_llStopPosition,
				&m_pTimeFormats[iOldTimeFormat]
			);
			if (FAILED(hr))
				return hr;

			// Set the start/stop positions in the new time format
			m_llCurrentPosition	= llNewCurrent;
			m_llStopPosition	= llNewStop;

			return NOERROR;
		}
	}

	// We didn't find the requested time format among the supported
	return E_INVALIDARG;
}

STDMETHODIMP CParserSeeking::GetTimeFormat(GUID *pFormat)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pFormat, E_POINTER);
	ValidateWritePtr(pFormat, sizeof(GUID));

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Return the current time format
	*pFormat = m_pTimeFormats[m_iTimeFormat];

	return NOERROR;
}

STDMETHODIMP CParserSeeking::IsUsingTimeFormat(const GUID *pFormat)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pFormat, E_POINTER);
	ValidateReadPtr(pFormat, sizeof(GUID));

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return S_FALSE;

	return IsEqualGUID(*pFormat, m_pTimeFormats[m_iTimeFormat]) ? S_OK : S_FALSE;
}

STDMETHODIMP CParserSeeking::IsFormatSupported(const GUID *pFormat)
{
	// Check and validate the pointer
	CheckPointer(pFormat, E_POINTER);
	ValidateReadPtr(pFormat, sizeof(GUID));

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return S_FALSE;

	// Loop through the supported time formats and try to find the
	// requested format among them
	for (int i = 0; i < m_nTimeFormats; i++) {
		if (IsEqualGUID(*pFormat, m_pTimeFormats[i]))
			return S_OK;
	}

	// We didn't find the requested time format among the supported
	return S_FALSE;
}

STDMETHODIMP CParserSeeking::QueryPreferredFormat(GUID *pFormat)
{
	// Check and validate the pointer
	CheckPointer(pFormat, E_POINTER);
	ValidateWritePtr(pFormat, sizeof(GUID));

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Return the preferred time format (the first one in the array)
	*pFormat = m_pTimeFormats[0];

	return NOERROR;
}

STDMETHODIMP CParserSeeking::ConvertTimeFormat(
	LONGLONG *pTarget,
	const GUID *pTargetFormat,
	LONGLONG llSource,
	const GUID *pSourceFormat
)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pTarget, E_POINTER);
	ValidateWritePtr(pTarget, sizeof(LONGLONG));

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Check if the target or source format is NULL
	GUID guidTargetFormat, guidSourceFormat;
	if (pTargetFormat)
		guidTargetFormat = *pTargetFormat;
	else
		guidTargetFormat = m_pTimeFormats[m_iTimeFormat];
	if (pSourceFormat)
		guidSourceFormat = *pSourceFormat;
	else
		guidSourceFormat = m_pTimeFormats[m_iTimeFormat];

	// Delegate actual work to the parent filter
	return m_pFilter->ConvertTimeFormat(
		m_pPin->Name(),
		pTarget,
		&guidTargetFormat,
		llSource,
		&guidSourceFormat
	);
}

STDMETHODIMP CParserSeeking::SetPositions(
	LONGLONG *pllCurrent,
	DWORD dwCurrentFlags,
	LONGLONG *pllStop,
	DWORD dwStopFlags
)
{
	CAutoLock lock(&m_csLock);

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Check if we need to do anything
	if (
		(dwCurrentFlags	& AM_SEEKING_NoPositioning) &&
		(dwStopFlags	& AM_SEEKING_NoPositioning)
	)
		return S_FALSE;

	// Work out the absolute current position
	LONGLONG llCurrent;
	if (dwCurrentFlags & AM_SEEKING_NoPositioning)
		llCurrent = m_llCurrentPosition;
	else if (dwCurrentFlags & AM_SEEKING_AbsolutePositioning)
		llCurrent = *pllCurrent;
	else if (dwCurrentFlags & AM_SEEKING_RelativePositioning)
		llCurrent = m_llCurrentPosition + *pllCurrent;
	else
		return E_INVALIDARG;

	// Work out the absolute stop position
	LONGLONG llStop;
	if (dwStopFlags & AM_SEEKING_AbsolutePositioning)
		llStop = *pllStop;
	else if (dwStopFlags & AM_SEEKING_RelativePositioning)
		llStop = m_llStopPosition + *pllStop;
	else if (dwStopFlags & AM_SEEKING_IncrementalPositioning)
		llStop = llCurrent + *pllStop;
	else // AM_SEEKING_NoPositioning or no flags at all
		llStop = m_llStopPosition;

	// Delegate actual work to the parent filter
	HRESULT hr = m_pFilter->SetPositions(
		m_pPin->Name(),
		&m_pTimeFormats[m_iTimeFormat],
		&llCurrent,
		dwCurrentFlags,
		&llStop,
		dwStopFlags
	);
	if (FAILED(hr))
		return hr;

	if (!(dwCurrentFlags & AM_SEEKING_NoPositioning)) {

		// Reset the parent pin's stream time to zero
		m_pPin->SetTime(0);

		// Convert the current position to media time and update 
		// the parent pin's media time
		LONGLONG llCurrentMediaTime = 0;
		hr = ConvertTimeFormat(
			&llCurrentMediaTime,
			&TIME_FORMAT_SAMPLE,
			llCurrent,
			&m_pTimeFormats[m_iTimeFormat]
		);
		// If the above call fails we'll compromise media time handling
		if (SUCCEEDED(hr))
			m_pPin->SetMediaTime(llCurrentMediaTime);

		// Set the discontinuity flag
		m_pPin->SetDiscontinuity(TRUE);
	}

	// Convert the current position to stream time
	REFERENCE_TIME rtCurrent = 0;
	hr = ConvertTimeFormat(
		&rtCurrent,
		&TIME_FORMAT_MEDIA_TIME,
		llCurrent,
		&m_pTimeFormats[m_iTimeFormat]
	);
	if (FAILED(hr))
		return hr;

	// Convert the stop position to stream time
	REFERENCE_TIME rtStop = 0;
	hr = ConvertTimeFormat(
		&rtStop,
		&TIME_FORMAT_MEDIA_TIME,
		llStop,
		&m_pTimeFormats[m_iTimeFormat]
	);
	if (FAILED(hr))
		return hr;

	// Force the parent filter to deliver NewSegment() downstream
	hr = m_pFilter->NewSegment(rtCurrent, rtStop, m_pPin->CurrentRate());
	if (FAILED(hr))
		return hr;

	// Store absolute positions in the temporary variables
	LONGLONG	llCurrentTemp	= llCurrent,
				llStopTemp		= llStop;

	// Convert the positions back to the original seek type
	if (dwStopFlags & AM_SEEKING_RelativePositioning)
		llStop -= m_llStopPosition;
	else if (dwStopFlags & AM_SEEKING_IncrementalPositioning)
		llStop -= llCurrent;
	if (dwCurrentFlags & AM_SEEKING_RelativePositioning)
		llCurrent -= m_llCurrentPosition;

	// Update the positions (using their absolute values)
	m_llCurrentPosition	= llCurrentTemp;
	m_llStopPosition	= llStopTemp;

	// Check if we have to convert to reference time and do it if so
	if (dwCurrentFlags & AM_SEEKING_ReturnTime) {
		hr = ConvertTimeFormat(
			&llCurrentTemp,
			&TIME_FORMAT_MEDIA_TIME,
			llCurrent,
			&m_pTimeFormats[m_iTimeFormat]
		);
		if (FAILED(hr))
			return hr;
		llCurrent = llCurrentTemp;
	}

	// Store the resulting current position
	if (pllCurrent)
		*pllCurrent = llCurrent;

	// Check if we have to convert to reference time and do it if so
	if (dwStopFlags & AM_SEEKING_ReturnTime) {
		hr = ConvertTimeFormat(
			&llStopTemp,
			&TIME_FORMAT_MEDIA_TIME,
			llStop,
			&m_pTimeFormats[m_iTimeFormat]
		);
		if (FAILED(hr))
			return hr;
		llStop = llStopTemp;
	}

	// Store the resulting stop position
	if (pllStop)
		*pllStop = llStop;

	return NOERROR;
}

STDMETHODIMP CParserSeeking::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointers
	CheckPointer(pCurrent, E_POINTER);
	ValidateWritePtr(pCurrent, sizeof(LONGLONG));
	CheckPointer(pStop, E_POINTER);
	ValidateWritePtr(pStop, sizeof(LONGLONG));

	// Return the positions
	*pCurrent	= m_llCurrentPosition;
	*pStop		= m_llStopPosition;

	return NOERROR;
}

STDMETHODIMP CParserSeeking::GetCurrentPosition(LONGLONG *pCurrent)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pCurrent, E_POINTER);
	ValidateWritePtr(pCurrent, sizeof(LONGLONG));

	// Return the current position
	*pCurrent = m_llCurrentPosition;

	return NOERROR;
}

STDMETHODIMP CParserSeeking::GetStopPosition(LONGLONG *pStop)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pStop, E_POINTER);
	ValidateWritePtr(pStop, sizeof(LONGLONG));

	// Return the stop position
	*pStop = m_llStopPosition;

	return NOERROR;
}

STDMETHODIMP CParserSeeking::SetRate(double dRate)
{
	CAutoLock lock(&m_csLock);

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Convert the current position to media time
	REFERENCE_TIME rtCurrent = 0;
	HRESULT hr = ConvertTimeFormat(
		&rtCurrent,
		&TIME_FORMAT_MEDIA_TIME,
		m_llCurrentPosition,
		&m_pTimeFormats[m_iTimeFormat]
	);
	if (FAILED(hr))
		return hr;

	// Convert the stop position to media time
	REFERENCE_TIME rtStop = 0;
	hr = ConvertTimeFormat(
		&rtStop,
		&TIME_FORMAT_MEDIA_TIME,
		m_llStopPosition,
		&m_pTimeFormats[m_iTimeFormat]
	);
	if (FAILED(hr))
		return hr;

	// Delegate actual work to the parent filter
	return m_pFilter->SetRate(m_pPin->Name(), rtCurrent, rtStop, dRate);
}

STDMETHODIMP CParserSeeking::GetRate(double *pdRate)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pdRate, E_POINTER);
	ValidateWritePtr(pdRate, sizeof(double));

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Return the current rate from the parent pin
	*pdRate = m_pPin->CurrentRate();

	return NOERROR;
}

STDMETHODIMP CParserSeeking::GetDuration(LONGLONG *pDuration)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pDuration, E_POINTER);
	ValidateWritePtr(pDuration, sizeof(LONGLONG));

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Delegate actual work to the parent filter
	return m_pFilter->GetDuration(
		m_pPin->Name(),
		&m_pTimeFormats[m_iTimeFormat],
		pDuration
	);
}

STDMETHODIMP CParserSeeking::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
{
	CAutoLock lock(&m_csLock);

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Delegate actual work to the parent filter
	return m_pFilter->GetAvailable(
		m_pPin->Name(),
		&m_pTimeFormats[m_iTimeFormat],
		pEarliest,
		pLatest
	);
}

STDMETHODIMP CParserSeeking::GetPreroll(LONGLONG *pllPreroll)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointer
	CheckPointer(pllPreroll, E_POINTER);
	ValidateWritePtr(pllPreroll, sizeof(LONGLONG));

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Delegate actual work to the parent filter
	return m_pFilter->GetPreroll(
		m_pPin->Name(),
		&m_pTimeFormats[m_iTimeFormat],
		pllPreroll
	);
}

HRESULT CParserSeeking::AdvanceCurrentPosition(LONGLONG llDelta)
{
	CAutoLock lock(&m_csLock);

	// Check if we have any time formats supported
	if ((m_nTimeFormats == 0) || (m_pTimeFormats == NULL))
		return E_NOTIMPL;

	// Convert the delta argument to the current time format
	LONGLONG llDeltaTemp = 0;
	HRESULT hr = ConvertTimeFormat(
		&llDeltaTemp,
		&m_pTimeFormats[m_iTimeFormat],
		llDelta,
		&TIME_FORMAT_SAMPLE
	);
	if (FAILED(hr))
		return hr;

	// Increment the current position by the media sample duration
	m_llCurrentPosition += llDeltaTemp;

	return NOERROR;
}

void CParserSeeking::Reset(void)
{
	CAutoLock lock(&m_csLock);

	m_llCurrentPosition	= 0;			// Default is stream start
	m_llStopPosition	= MAXLONGLONG;	// Guard value
	GetDuration(&m_llStopPosition);		// Default is stream duration
}

HRESULT CParserSeeking::GetSegmentParameters(
	REFERENCE_TIME *prtStart,
	REFERENCE_TIME *prtStop,
	double *pdRate
)
{
	CAutoLock lock(&m_csLock);

	// Check and validate the pointers
	CheckPointer(prtStart, E_POINTER);
	ValidateWritePtr(prtStart, sizeof(REFERENCE_TIME));
	CheckPointer(prtStop, E_POINTER);
	ValidateWritePtr(prtStop, sizeof(REFERENCE_TIME));
	CheckPointer(pdRate, E_POINTER);
	ValidateWritePtr(pdRate, sizeof(double));

	// Convert the current position to media time
	HRESULT hr = ConvertTimeFormat(
		prtStart,
		&TIME_FORMAT_MEDIA_TIME,
		m_llCurrentPosition,
		&m_pTimeFormats[m_iTimeFormat]
	);
	if (FAILED(hr))
		return hr;

	// Convert the stop position to media time
	hr = ConvertTimeFormat(
		prtStop,
		&TIME_FORMAT_MEDIA_TIME,
		m_llStopPosition,
		&m_pTimeFormats[m_iTimeFormat]
	);
	if (FAILED(hr))
		return hr;

	// Retrieve the current rate
	return GetRate(pdRate);
}