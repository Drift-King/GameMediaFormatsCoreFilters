//==========================================================================
//
// File: H263Parser.cpp
//
// Desc: Game Media Formats - Implementation of H263 parser filter
//
// Copyright (C) 2005 ANX Software.
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

#include <windows.h>
#include <mmsystem.h>

#include "H263GUID.h"
#include "H263Parser.h"
#include "resource.h"

//==========================================================================
// H263 parser setup data
//==========================================================================

// Global filter name
const WCHAR g_wszH263ParserName[]	= L"ANX H263 Parser";

const AMOVIESETUP_MEDIATYPE sudH263StreamType = {
	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_H263
};

const AMOVIESETUP_MEDIATYPE sudH263VideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_H263
};

const AMOVIESETUP_PIN sudH263ParserPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudH263StreamType	// Media types
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
		&sudH263VideoType	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudH263Parser = {
	&CLSID_H263Parser,	// CLSID of filter
	g_wszH263ParserName,	// Filter name
	MERIT_NORMAL,		// Filter merit
	2,					// Number of pins
	sudH263ParserPins	// Pin information
};

//==========================================================================
// H263 parser file type setup data
//==========================================================================

const TCHAR *sudH263Extensions[] = {
	TEXT(".263")
};

const GMF_FILETYPE_REGINFO CH263ParserFilter::g_pRegInfo[] = {
	{
		TEXT("H263 Video Format"),					// Type name
		&MEDIATYPE_Stream,							// Media type
		&MEDIASUBTYPE_H263,							// Media subtype
		&CLSID_AsyncReader,							// Source filter
		0,											// Number of entries
		NULL,										// Entries
		1,											// Number of extensions
		sudH263Extensions							// Extensions
	}
};

const int CH263ParserFilter::g_nRegInfo = 1;

//==========================================================================
// CH263ParserFilter methods
//==========================================================================

IMPLEMENT_FILETYPE(CH263ParserFilter)

CH263ParserFilter::CH263ParserFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePlainParserFilter(
		NAME("H263 Parser Filter"),
		pUnk,
		CLSID_H263Parser,
		g_wszH263ParserName,
		1,
		&sudH263StreamType,
		phr
	)
{
	// Nothing to do...
}

CUnknown* WINAPI CH263ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CH263ParserFilter(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

HRESULT CH263ParserFilter::Initialize(
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

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Set the default start/stop positions
	m_llDefaultStart	= (LONGLONG)0;
	m_llDefaultStop		= MAXLONGLONG; // Defaults to file end

	// Decide on the input pin properties
	m_cbInputAlign	= 1;		// No use in any special alignment
	m_cbInputBuffer	= 0x10000;	// Any reasonable value works

	// Decide on the packet size
	m_lPacketSize = 1; // No packeting needed

	// Initialize the media type
	pmt->InitMediaType();
	pmt->SetType(sudH263VideoType.clsMajorType);
	pmt->SetSubtype(sudH263VideoType.clsMinorType);
	pmt->SetSampleSize(0);				// Variable size samples
	pmt->SetTemporalCompression(TRUE);	// Using temporal compression
	pmt->SetFormatType(&FORMAT_None);
	pmt->cbFormat = 0;
	pmt->pbFormat = NULL;

	// Set the allocator properties
	pap->cbAlign	= 0;				// No matter
	pap->cbPrefix	= 0;				// No matter
	pap->cBuffers	= 1;				// One buffer is enough
	pap->cbBuffer	= m_cbInputBuffer;	// Output buffer size is the same as for input one

	// Set all parameters to zeroes (dummy parser seeker)
	*pdwCapabilities	= 0;
	*pnTimeFormats		= 0;
	*ppTimeFormats		= NULL;

	return NOERROR;
}

HRESULT CH263ParserFilter::GetSampleDelta(
	LONG lDataLength,
	REFERENCE_TIME *prtStreamDelta,
	LONGLONG *pllMediaDelta
)
{
	// Check and validate pointers
	CheckPointer(prtStreamDelta, E_POINTER);
	ValidateWritePtr(prtStreamDelta, sizeof(REFERENCE_TIME));
	CheckPointer(pllMediaDelta, E_POINTER);
	ValidateWritePtr(pllMediaDelta, sizeof(LONGLONG));

	*prtStreamDelta	= 0;
	*pllMediaDelta	= 0;

	return NOERROR;
}
