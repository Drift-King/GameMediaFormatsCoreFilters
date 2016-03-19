//==========================================================================
//
// File: APCParser.cpp
//
// Desc: Game Media Formats - Implementation of APC parser filter
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

#include "APCSpecs.h"
#include "APCGUID.h"
#include "APCParser.h"
#include "ContinuousIMAADPCM.h"
#include "resource.h"

//==========================================================================
// APC parser setup data
//==========================================================================

// Media content strings
const OLECHAR wszAPCAuthorName[]	= L"Cryo Interactive";
const OLECHAR wszAPCDescription[]	= L"Cryo Interactive APC audio file v1.20";
const OLECHAR wszAPCCopyright[]		= L"Copyright (C) Cryo Interactive";

// Global filter name
const WCHAR g_wszAPCParserName[]	= L"ANX APC Parser";

const AMOVIESETUP_MEDIATYPE sudAPCStreamType = {
	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_APC
};

const AMOVIESETUP_MEDIATYPE sudAPCAudioType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_CIMAADPCM
};

const AMOVIESETUP_PIN sudAPCParserPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudAPCStreamType	// Media types
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
		&sudAPCAudioType	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudAPCParser = {
	&CLSID_APCParser,	// CLSID of filter
	g_wszAPCParserName,	// Filter name
	MERIT_NORMAL,		// Filter merit
	2,					// Number of pins
	sudAPCParserPins	// Pin information
};

//==========================================================================
// APC parser file type setup data
//==========================================================================

const GMF_FILETYPE_PATTERN sudAPCPatterns[] = {
	{ 0,	4,	APC_IDSTR_CRYO	},
	{ 4,	4,	APC_IDSTR_APC	},
	{ 8,	4,	APC_IDSTR_VER	}
};

const GMF_FILETYPE_ENTRY sudAPCEntries[] = {
	{ 3, sudAPCPatterns }
};

const TCHAR *sudAPCExtensions[] = {
	TEXT(".apc")
};

const GMF_FILETYPE_REGINFO CAPCParserFilter::g_pRegInfo[] = {
	{
		TEXT("Cryo Interactive APC Audio Format"),	// Type name
		&MEDIATYPE_Stream,							// Media type
		&MEDIASUBTYPE_APC,							// Media subtype
		&CLSID_AsyncReader,							// Source filter
		1,											// Number of entries
		sudAPCEntries,								// Entries
		1,											// Number of extensions
		sudAPCExtensions							// Extensions
	}
};

const int CAPCParserFilter::g_nRegInfo = 1;

//==========================================================================
// CAPCParserFilter methods
//==========================================================================

IMPLEMENT_FILETYPE(CAPCParserFilter)

CAPCParserFilter::CAPCParserFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePlainParserFilter(
		NAME("APC Parser Filter"),
		pUnk,
		CLSID_APCParser,
		g_wszAPCParserName,
		1,
		&sudAPCStreamType,
		phr
	),
	m_nSampleSize(0),		// No sample size at this time
	m_nAvgBytesPerSec(0),	// No data rate at this time
	m_nSamples(0)			// No duration at this time
{
	// Nothing to do...
}

CUnknown* WINAPI CAPCParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CAPCParserFilter(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

HRESULT CAPCParserFilter::Initialize(
	IPin *pPin,
	IAsyncReader *pReader,
	CMediaType *pmt,
	ALLOCATOR_PROPERTIES *pap,
	DWORD *pdwCapabilities,
	int *pnTimeFormats,
	GUID **ppTimeFormats
)
{
	// Check and validate the pointers
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));
	CheckPointer(pap, E_POINTER);
	ValidateWritePtr(pap, sizeof(ALLOCATOR_PROPERTIES));
	CheckPointer(pdwCapabilities, E_POINTER);
	ValidateWritePtr(pdwCapabilities, sizeof(DWORD));
	CheckPointer(pnTimeFormats, E_POINTER);
	ValidateWritePtr(pnTimeFormats, sizeof(int));
	CheckPointer(ppTimeFormats, E_POINTER);
	ValidateWritePtr(ppTimeFormats, sizeof(GUID*));

	// Read file header
	APC_HEADER header;
	HRESULT hr = pReader->SyncRead(0, sizeof(header), (BYTE*)&header);
	if (hr != S_OK)
		return hr;

	// Verify file header
	if (
		(header.dwID1		!= APC_ID_CRYO)	||
		(header.dwID2		!= APC_ID_APC)	||
		(header.dwVersion	!= APC_ID_VER)
	)
		return VFW_E_INVALID_FILE_FORMAT;

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Set the default start/stop positions
	m_llDefaultStart	= (LONGLONG)sizeof(APC_HEADER); // Right after the header
	m_llDefaultStop		= MAXLONGLONG; // Defaults to file end

	// Decide on the input pin properties
	m_cbInputAlign	= 1;		// No use in any special alignment
	m_cbInputBuffer	= 0x10000;	// Any reasonable value works

	// Decide on the packet size
	m_lPacketSize = 1; // No packeting needed

	// Initialize the media type
	pmt->InitMediaType();
	pmt->SetType(sudAPCAudioType.clsMajorType);
	pmt->SetSubtype(sudAPCAudioType.clsMinorType);
	pmt->SetSampleSize(0);				// Variable size samples
	pmt->SetTemporalCompression(TRUE);	// Using temporal compression
	pmt->SetFormatType(&FORMAT_CIMAADPCM);
	CIMAADPCMWAVEFORMAT *pFormat = (CIMAADPCMWAVEFORMAT*)pmt->AllocFormatBuffer(sizeof(CIMAADPCMWAVEFORMAT) + (((header.bIsStereo) ? 2 : 1) - 1) * sizeof(CIMAADPCMINFO));
	if (pFormat == NULL)
		return E_OUTOFMEMORY;

	// Fill in the format block
	pFormat->nChannels			= ((header.bIsStereo) ? 2 : 1);
	pFormat->nSamplesPerSec		= header.dwSampleRate;
	pFormat->wBitsPerSample		= 16;
	pFormat->bIsHiNibbleFirst	= TRUE;
	pFormat->dwReserved			= CIMAADPCM_INTERLEAVING_NORMAL;

	// Fill in init info block for each channel
	pFormat->pInit[0].lSample	= header.lLeftSample;
	pFormat->pInit[0].chIndex	= 0;
	if (header.bIsStereo) {
		pFormat->pInit[1].lSample	= header.lRightSample;
		pFormat->pInit[1].chIndex	= 0;
	}

	// Set the allocator properties
	pap->cbAlign	= 0;				// No matter
	pap->cbPrefix	= 0;				// No matter
	pap->cBuffers	= 1;				// One buffer is enough
	pap->cbBuffer	= m_cbInputBuffer;	// Output buffer size is the same as for input one

	// Set the wave format parameters needed for GetSampleDelta()
	m_nSampleSize		= 2 * pFormat->nChannels;
	m_nAvgBytesPerSec	= m_nSampleSize * pFormat->nSamplesPerSec;

	// Set the number of samples in the stream
	m_nSamples = header.nSamples;

	// Scope for the locking
	{
		// Protect media content information
		CAutoLock infolock(&m_csInfo);

		// Set the media content strings
		m_wszAuthorName = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszAPCAuthorName) + 1));
		if (m_wszAuthorName)
			lstrcpyW(m_wszAuthorName, wszAPCAuthorName);
		m_wszDescription = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszAPCDescription) + 1));
		if (m_wszDescription)
			lstrcpyW(m_wszDescription, wszAPCDescription);
		m_wszCopyright = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszAPCCopyright) + 1));
		if (m_wszCopyright)
			lstrcpyW(m_wszCopyright, wszAPCCopyright);
	}

	// Allocate time formats array. If we fail here, it's not an error, 
	// we'll just return with zero seeker parameters and may proceed
	*ppTimeFormats = (GUID*)CoTaskMemAlloc(3 * sizeof(GUID));
	if (*ppTimeFormats) {

		*pnTimeFormats = 3;
		
		// Fill in the time formats array
		(*ppTimeFormats)[0] = TIME_FORMAT_MEDIA_TIME;
		(*ppTimeFormats)[1] = TIME_FORMAT_SAMPLE;
		(*ppTimeFormats)[2] = TIME_FORMAT_BYTE;

		// Set capabilities
		*pdwCapabilities =	AM_SEEKING_CanGetCurrentPos	|
							AM_SEEKING_CanGetStopPos	|
							AM_SEEKING_CanGetDuration;
	} else {
		// Set all parameters to zeroes (dummy parser seeker)
		*pdwCapabilities	= 0;
		*pnTimeFormats		= 0;
		*ppTimeFormats		= NULL;
	}

	return NOERROR;
}

HRESULT CAPCParserFilter::Shutdown(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Reset the stream-specific variables
	m_nSampleSize		= 0;
	m_nAvgBytesPerSec	= 0;
	m_nSamples			= 0;

	// Call the base-class implementation
	return CBasePlainParserFilter::Shutdown();
}

HRESULT CAPCParserFilter::GetSampleDelta(
	LONG lDataLength,
	REFERENCE_TIME *prtStreamDelta,
	LONGLONG *pllMediaDelta
)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Check if we have correct wave format parameters
	if (
		(m_nSampleSize		== 0) ||
		(m_nAvgBytesPerSec	== 0)
	)
		return E_UNEXPECTED;

	// Check and validate pointers
	CheckPointer(prtStreamDelta, E_POINTER);
	ValidateWritePtr(prtStreamDelta, sizeof(REFERENCE_TIME));
	CheckPointer(pllMediaDelta, E_POINTER);
	ValidateWritePtr(pllMediaDelta, sizeof(LONGLONG));

	LONG lUnpackedDataLength = lDataLength * 4;
	*prtStreamDelta	= ((REFERENCE_TIME)lUnpackedDataLength * UNITS) / m_nAvgBytesPerSec;
	*pllMediaDelta	= (LONGLONG)lUnpackedDataLength / m_nSampleSize;

	return NOERROR;
}

HRESULT CAPCParserFilter::ConvertTimeFormat(
	LPCWSTR pPinName,
	LONGLONG *pTarget,
	const GUID *pTargetFormat,
	LONGLONG llSource,
	const GUID *pSourceFormat
)
{
	// If the formats coincide, no work to be done
	if (IsEqualGUID(*pTargetFormat, *pSourceFormat)) {
		*pTarget = llSource;
		return NOERROR;
	}

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Check if we have correct wave format parameters
	if (
		(m_nSampleSize		== 0) ||
		(m_nAvgBytesPerSec	== 0)
	)
		return E_UNEXPECTED;

	// Convert the source value to samples
	LONGLONG llSourceInSamples;
	if (IsEqualGUID(*pSourceFormat, TIME_FORMAT_SAMPLE))
		llSourceInSamples = llSource;
	else if (IsEqualGUID(*pSourceFormat, TIME_FORMAT_BYTE))
		llSourceInSamples = (llSource * 4) / m_nSampleSize;
	else if (IsEqualGUID(*pSourceFormat, TIME_FORMAT_MEDIA_TIME))
		llSourceInSamples = (llSource * m_nAvgBytesPerSec) / (m_nSampleSize  * UNITS);
	else
		return E_INVALIDARG;

	// Convert the source in samples to target format
	if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_SAMPLE))
		*pTarget = llSourceInSamples;
	else if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_BYTE))
		*pTarget = (llSourceInSamples * m_nSampleSize) / 4;
	else if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_MEDIA_TIME))
		*pTarget = (llSourceInSamples * m_nSampleSize * UNITS) / m_nAvgBytesPerSec;
	else
		return E_INVALIDARG;

	return NOERROR;

}

HRESULT CAPCParserFilter::GetDuration(
	LPCWSTR pPinName,
	const GUID *pCurrentFormat,
	LONGLONG *pDuration
)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Convert the duration in samples to the current time format
	return ConvertTimeFormat(
		pPinName,
		pDuration,
		pCurrentFormat,
		(LONGLONG)m_nSamples,
		&TIME_FORMAT_SAMPLE
	);
}

STDMETHODIMP CAPCParserFilter::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_APCParserPage;
	pPages->pElems[1] = CLSID_BaseParserPage;

	return NOERROR;
}

//==========================================================================
// CAPCParserPage methods
//==========================================================================

const WCHAR g_wszAPCParserPageName[] = L"ANX APC Parser Property Page";

CAPCParserPage::CAPCParserPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("APC Parser Property Page"),
		pUnk,
		IDD_APCPARSERPAGE,
		IDS_TITLE_APCPARSERPAGE
	)
{
}

CUnknown* WINAPI CAPCParserPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CAPCParserPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
