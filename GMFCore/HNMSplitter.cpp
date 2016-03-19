//==========================================================================
//
// File: HNMSplitter.cpp
//
// Desc: Game Media Formats - Implementation of HNM splitter filter
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

#include "HNMSpecs.h"
#include "HNMGUID.h"
#include "HNMSplitter.h"
#include "APCSpecs.h"
#include "APCGUID.h"
#include "APCParser.h"
#include "ContinuousIMAADPCM.h"
#include "FilterOptions.h"
#include "resource.h"

#include <string.h>

//==========================================================================
// HNM splitter setup data
//==========================================================================

// Media content strings
const OLECHAR wszHNMAuthorName[]	= L"Cryo Interactive";
const OLECHAR wszHNMDescription[]	= L"Cryo Interactive HNM video file";
const OLECHAR wszHNMCopyright[]		= OLESTR("-Copyright CRYO-");
const OLECHAR wszHNMMoreInfoText[]	= OLESTR("Pascal URRO  R&D");

// Global filter name
const WCHAR g_wszHNMSplitterName[]	= L"ANX HNM Splitter";

// Output pin names
const WCHAR wszHNMVideoOutputName[]	= L"Video Output";
const WCHAR wszHNMAudioOutputName[]	= L"Audio Output";

const AMOVIESETUP_MEDIATYPE sudHNMStreamType = {
	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_HNM
};

const AMOVIESETUP_MEDIATYPE sudHNMVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_HNMVideo
};

const AMOVIESETUP_MEDIATYPE sudHNMAudioType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_CIMAADPCM
};

const AMOVIESETUP_PIN sudHNMSplitterPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudHNMStreamType	// Media types
	},
	{	// Video output pin
		L"Video Output",	// Pin name
		FALSE,				// Is it rendered
		TRUE,				// Is it an output
		TRUE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Input",			// Connects to pin
		1,					// Number of types
		&sudHNMVideoType	// Media types
	},
	{	// Audio output pin
		L"Audio Output",	// Pin name
		FALSE,				// Is it rendered
		TRUE,				// Is it an output
		TRUE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Input",			// Connects to pin
		1,					// Number of types
		&sudHNMAudioType	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudHNMSplitter = {
	&CLSID_HNMSplitter,		// CLSID of filter
	g_wszHNMSplitterName,	// Filter name
	MERIT_NORMAL,			// Filter merit
	3,						// Number of pins
	sudHNMSplitterPins		// Pin information
};

//==========================================================================
// HNM splitter file type setup data
//==========================================================================

const GMF_FILETYPE_PATTERN sudHNMPatterns[] = {
	{ 0,	4,	HNM_IDSTR_HNM6		},
	{ 0x20,	16,	HNM_IDSTR_RND		},
	{ 0x30,	16,	HNM_IDSTR_COPYRIGHT	}
};

const GMF_FILETYPE_ENTRY sudHNMEntries[] = {
	{ 3, sudHNMPatterns } 
};

const TCHAR *sudHNMExtensions[] = {
	TEXT(".hnm"),
	TEXT(".hns")
};

const GMF_FILETYPE_REGINFO CHNMSplitterFilter::g_pRegInfo[] = {
	{
		TEXT("Cryo Interactive HNM Video Format"),	// Type name
		&MEDIATYPE_Stream,							// Media type
		&MEDIASUBTYPE_HNM,							// Media subtype
		&CLSID_AsyncReader,							// Source filter
		1,											// Number of entries
		sudHNMEntries,								// Entries
		2,											// Number of extensions
		sudHNMExtensions							// Extensions
	}
};

const int CHNMSplitterFilter::g_nRegInfo = 1;

//==========================================================================
// CHNMInnerChunkParser methods
//==========================================================================

CHNMInnerChunkParser::CHNMInnerChunkParser(
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	WORD nFramesPerSecond,
	WORD nSampleSize,
	DWORD nAvgBytesPerSec,
	HRESULT *phr
) :
	CBaseChunkParser(
		NAME("HNM Inner Chunk Parser"),
		pFilter,
		ppOutputPin,
		sizeof(HNM_BLOCK_HEADER),
		4,
		phr
	),
	m_pPin(NULL),
	m_pSample(NULL),
	m_rtDelta(0),
	m_llDelta(0),
	m_bShouldSkip(TRUE),
	m_lSkipLength(0),
	m_nFramesPerSecond(nFramesPerSecond),
	m_nSampleSize(nSampleSize),
	m_nAvgBytesPerSec(nAvgBytesPerSec)
{
}

CHNMInnerChunkParser::~CHNMInnerChunkParser()
{
	// Free any resources allocated by the parser
	ResetParser();
}

// Since we don't expect any seek operation, we reset the parser 
// so that it's ready to parse data from the file beginning
HRESULT CHNMInnerChunkParser::ResetParser(void)
{
	CAutoLock lock(&m_csLock);

	// Reset the current pin
	m_pPin = NULL;

	// Release the current sample
	if (m_pSample) {
		m_pSample->Release();
		m_pSample = NULL;
	}

	// Reset the sample's durations
	m_rtDelta = 0;
	m_llDelta = 0;

	// Reset the skipping stuff
	m_bShouldSkip = TRUE;	// We reset to the beginning of the file
	m_lSkipLength = 0;		// No data to be ignored

	// Call base-class method to reset parser state
	return CBaseChunkParser::ResetParser();
}

HRESULT CHNMInnerChunkParser::ParseChunkHeader(
	LONGLONG llStartPosition,
	BYTE *pbHeader,
	LONG *plDataSize
)
{
	// Check and validate pointers
	CheckPointer(pbHeader, E_POINTER);
	ValidateReadPtr(pbHeader, sizeof(HNM_BLOCK_HEADER));
	CheckPointer(plDataSize, E_POINTER);
	ValidateWritePtr(plDataSize, sizeof(LONG));

	HNM_BLOCK_HEADER *pBlockHeader = (HNM_BLOCK_HEADER*)pbHeader;
	
	// Set the data size
	*plDataSize = pBlockHeader->cbBlock - sizeof(HNM_BLOCK_HEADER);

	m_pPin = NULL; // Pin to send the sample through
	m_rtDelta = 0;
	m_llDelta = 0;

	LONG lUnpackedDataLength = 0;

	// Recognize block type
	switch (pBlockHeader->wBlockID) {

		// Video data
		case HNM_ID_IX:
		case HNM_ID_IV:

			// Check if we have correct video format parameters
			if (m_nFramesPerSecond == 0)
				return E_UNEXPECTED;

			m_pPin = m_ppOutputPin[0];		// We'll be dealing with video output pin
			m_lSkipLength = 0;				// No need to skip data
			m_rtDelta = UNITS / m_nFramesPerSecond;
			m_llDelta = 1;					// One frame per sample

			break;

		// Audio header & data
		case HNM_ID_AA:

			ASSERT(m_ppOutputPin[1]);	// There should be the audio output pin
			m_pPin = m_ppOutputPin[1];	// We'll be dealing with audio output pin

			// We should skip APC header only for the first AA subchunk
			if (m_bShouldSkip) {
				m_lSkipLength = sizeof(APC_HEADER);	// Skip APC header
				m_bShouldSkip = FALSE; // Further AA subchunks don't contain APC header
			} else
				m_lSkipLength = 0;

			// Check if we have correct wave format parameters
			if (
				(m_nSampleSize		== 0) ||
				(m_nAvgBytesPerSec	== 0)
			)
				return E_UNEXPECTED;

			// Calculate the stream and media deltas
			lUnpackedDataLength = (*plDataSize - m_lSkipLength) * 4;
			m_rtDelta	= ((REFERENCE_TIME)lUnpackedDataLength * UNITS) / m_nAvgBytesPerSec;
			m_llDelta	= (LONGLONG)lUnpackedDataLength / m_nSampleSize;

			break;

		// Audio data
		case HNM_ID_BB:
			
			ASSERT(m_ppOutputPin[1]);	// There should be the audio output pin
			m_pPin = m_ppOutputPin[1];	// We'll be dealing with audio output pin

			m_lSkipLength = 0;	// Nothing to skip

			// Check if we have correct wave format parameters
			if (
				(m_nSampleSize		== 0) ||
				(m_nAvgBytesPerSec	== 0)
			)
				return E_UNEXPECTED;

			// Calculate the stream and media deltas
			lUnpackedDataLength = (*plDataSize) * 4;
			m_rtDelta	= ((REFERENCE_TIME)lUnpackedDataLength * UNITS) / m_nAvgBytesPerSec;
			m_llDelta	= (LONGLONG)lUnpackedDataLength / m_nSampleSize;

			break;

		default:

			// Is it possible?
			break;
	}

	// We cannot find appropriate output pin for this chunk --
	// just ignore it (and return no error)
	if (!m_pPin) {
		m_pSample = NULL;
		return NOERROR;
	}

	// If the output pin is not connected, just quit with no error.
	// There might be other output pins which are connected and
	// delivering samples -- we should not hamper their activities
	if (!m_pPin->IsConnected()) {
		m_pSample = NULL;
		return NOERROR;
	}

	// Get an empty sample from the output pin
	HRESULT hr = m_pPin->GetDeliveryBuffer(&m_pSample, NULL, NULL, 0);
	if (FAILED(hr)) {
		m_pSample = NULL;
		return hr;
	}

	// Initialize actual data length to zero
	hr = m_pSample->SetActualDataLength(0);
	if (FAILED(hr)) {
		m_pSample->Release();
		m_pSample = NULL;
		return hr;
	}

	return NOERROR;
}

HRESULT CHNMInnerChunkParser::ParseChunkData(
	LONGLONG llStartPosition,
	BYTE *pbData,
	LONG lDataSize
)
{
	// Check the pointer
	CheckPointer(pbData, E_POINTER);

	// No sample has been supplied -- just ignore data
	if (!m_pSample)
		return NOERROR;

	// Get the sample's buffer
	BYTE *pbBuffer = NULL;
	HRESULT hr = m_pSample->GetPointer(&pbBuffer);
	if (FAILED(hr)) {
		m_pSample->Release();
		m_pSample = NULL;
		return hr;
	}

	// Find out how many bytes we can copy
	LONG lBufferSize = m_pSample->GetSize();
	LONG lActualDataLength = m_pSample->GetActualDataLength();
	LONG lDataToCopy = min(lDataSize, lBufferSize - lActualDataLength);

	// Find out how many bytes we're to ignore
	LONG lSkip = min(m_lSkipLength, lDataToCopy);

	// Update all the lengths and pointers according to skip length
	m_lSkipLength	-= lSkip;
	pbData			+= lSkip;
	lDataToCopy		-= lSkip;
	lDataSize		-= lSkip;

	// Copy data to buffer
	CopyMemory(pbBuffer + lActualDataLength, pbData, lDataToCopy);
	lActualDataLength += lDataToCopy;

	// Update buffer's actual data length
	hr = m_pSample->SetActualDataLength(lActualDataLength);
	if (FAILED(hr)) {
		m_pSample->Release();
		m_pSample = NULL;
		return hr;
	}

	return (lDataToCopy < lDataSize) ? VFW_E_BUFFER_OVERFLOW : NOERROR;
}

HRESULT CHNMInnerChunkParser::ParseChunkEnd(void)
{
	// If no sample is supplied there's no need to do anything
	if (!m_pSample)
		return NOERROR;

	// If we've got media sample there should be the output pin
	ASSERT(m_pPin);

	// Deliver the sample
	HRESULT hr = m_pPin->Deliver(m_pSample, m_rtDelta, m_llDelta);
	if (FAILED(hr)) {
		m_pSample->Release();
		m_pSample = NULL;
		return hr;
	}

	// Finally release the sample
	m_pSample->Release();
	m_pSample = NULL;

	// We're done with this output pin
	m_pPin = NULL;

	// No sample -- no sample deltas
	m_rtDelta = 0;
	m_llDelta = 0;

	return NOERROR;

}

//==========================================================================
// CHNMOuterChunkParser methods
//==========================================================================

CHNMOuterChunkParser::CHNMOuterChunkParser(
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	WORD nFramesPerSecond,
	WORD nSampleSize,
	DWORD nAvgBytesPerSec,
	HRESULT *phr
) :
	CBaseChunkParser(
		NAME("HNM Outer Chunk Parser"),
		pFilter,
		ppOutputPin,
		sizeof(LONG),
		4,
		phr
	),
	m_InnerParser(
		pFilter,
		ppOutputPin,
		nFramesPerSecond,
		nSampleSize,
		nAvgBytesPerSec,
		phr
	)
{
}

CHNMOuterChunkParser::~CHNMOuterChunkParser()
{
	// Reset and shutdown the parser
	ResetParser();
}

HRESULT CHNMOuterChunkParser::ResetParser(void)
{
	CAutoLock lock(&m_csLock);

	// Call the base-class method to reset parser state
	HRESULT hr = CBaseChunkParser::ResetParser();
	if (FAILED(hr))
		return hr;

	// Delegate the call to the nested one
	return m_InnerParser.ResetParser();
}

HRESULT CHNMOuterChunkParser::ParseChunkHeader(
	LONGLONG llStartPosition,
	BYTE *pbHeader,
	LONG *plDataSize
)
{
	// Check and validate pointers
	CheckPointer(pbHeader, E_POINTER);
	ValidateReadPtr(pbHeader, sizeof(LONG));
	CheckPointer(plDataSize, E_POINTER);
	ValidateWritePtr(plDataSize, sizeof(LONG));

	// Outer chunk header is a mere LONG chunk size
	*plDataSize = *((LONG*)pbHeader) - sizeof(LONG);
	// Correct the chunk size for final (empty) chunk
	if (*plDataSize < 0)
		*plDataSize = 0;

	// Reset the nested parser so that it's clean and ready to 
	// parse forthcoming group of nested chunks
	HRESULT hr = m_InnerParser.ResetParser();
	if (FAILED(hr))
		return hr;

	return NOERROR;
}

HRESULT CHNMOuterChunkParser::ParseChunkData(
	LONGLONG llStartPosition,
	BYTE *pbData,
	LONG lDataSize
)
{
	// Pass the data through to the nested parser
	return m_InnerParser.Receive(llStartPosition, pbData, lDataSize);
}

HRESULT CHNMOuterChunkParser::ParseChunkEnd(void)
{
	// Outer chunk end necessarily implies inner chunk end,
	// but we rely on the correctness of the nested chunks 
	// structure, so we assume that inner parser has already 
	// performed its ParseChunkEnd(). In that case resetting
	// inner parser does no harm (though it does no good, too).
	// If not, we should reset the inner parser to clean up
	// its state and free its resources -- this will discard 
	// incompletely-filled media sample (if any), but that's ok
	// because that sample is invalid anyway
	return m_InnerParser.ResetParser();
}

//==========================================================================
// CHNMSplitterFilter methods
//==========================================================================

IMPLEMENT_FILETYPE(CHNMSplitterFilter)

CHNMSplitterFilter::CHNMSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseParserFilter(
		NAME("HNM Splitter Filter"),
		pUnk,
		CLSID_HNMSplitter,
		g_wszHNMSplitterName,
		phr
	),
	m_nFramesPerSecond(0),		// No frame rate at this time
	m_nSampleSize(0),			// No audio sample size at this time
	m_nAvgBytesPerSec(0),		// No audio data rate at this time
	m_nVideoFrames(0),			// No video stream duration at this time
	m_nAudioSamples(0),			// No audio stream duration at this time
	m_pAPCSource(NULL),			// |
	m_pAPCParser(NULL),			// |
	m_pPassThruLock(NULL),		// |-- No APC stuff at this time
	m_pPassThruInput(NULL),		// |
	m_pPassThruOutput(NULL),	// |
	m_pParser(NULL)				// No chunk parser at this time
{
	ASSERT(phr);

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Retrieve the options from the registry
		m_bUseExternalAPC = TRUE; // Default value
		HRESULT hr = RegGetFilterOptionDWORD(
			m_wszFilterName,
			TEXT("Use External APC"),
			(DWORD*)&m_bUseExternalAPC
		);
		if (FAILED(hr))
			*phr = hr;
		DWORD dwType = 0, cbSize = sizeof(m_szExternalAPCPath);
		hr = RegGetFilterOptionValue(
			m_wszFilterName,
			TEXT("External APC Path"),
			&dwType,
			(LPBYTE)m_szExternalAPCPath,
			&cbSize
		);
		if (FAILED(hr)) {
			lstrcpyW(m_szExternalAPCPath, L"..\\cine"); // Default value
			hr = RegSetFilterOptionValue(
				m_wszFilterName,
				TEXT("External APC Path"),
				REG_BINARY,
				(LPBYTE)m_szExternalAPCPath,
				(lstrlenW(m_szExternalAPCPath) + 1) * sizeof(OLECHAR)
			);
			if (FAILED(hr))
				*phr = hr;
		} else if (dwType != REG_BINARY)
			*phr = E_UNEXPECTED;
	}
}

CHNMSplitterFilter::~CHNMSplitterFilter()
{
	// Free any resources allocated by Initialize() or InitializeParser()
	Shutdown();

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Store the options in the registry
		RegSetFilterOptionValue(
			m_wszFilterName,
			TEXT("Use External APC"),
			REG_DWORD,
			(LPBYTE)&m_bUseExternalAPC,
			sizeof(BOOL)
		);
		RegSetFilterOptionValue(
			m_wszFilterName,
			TEXT("External APC Path"),
			REG_BINARY,
			(LPBYTE)m_szExternalAPCPath,
			(lstrlenW(m_szExternalAPCPath) + 1) * sizeof(OLECHAR)
		);
	}
}

CUnknown* WINAPI CHNMSplitterFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CHNMSplitterFilter(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CHNMSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	// Check and validate the pointer
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	// Expose IConfigHNMSplitter (and the base-class interfaces)
	if (riid == IID_IConfigHNMSplitter)
		return GetInterface((IConfigHNMSplitter*)this, ppv);
	else
		return CBaseParserFilter::NonDelegatingQueryInterface(riid, ppv);
}

int CHNMSplitterFilter::GetPinCount(void)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	return 1								// Input pin
		+ m_nOutputPins						// Output pins
		+ ((m_pPassThruInput) ? 1 : 0)		// Pass-through input pin
		+ ((m_pPassThruOutput) ? 1 : 0);	// Pass-through output pin
}

CBasePin* CHNMSplitterFilter::GetPin(int n)
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
	} else if (n == m_nOutputPins + 1) {
		// Pass-through input pin
		return m_pPassThruInput;
	} else if (n == m_nOutputPins + 2) {
		// Pass-through output pin
		return m_pPassThruOutput;
	} else {
		// No more pins
		return NULL;
	}
}

IPin* CHNMSplitterFilter::GetPinWithDirection(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
{
	BOOL bFound = FALSE;

	// Get pin enumerator
	IEnumPins *pEnum = NULL;
	if (FAILED(pFilter->EnumPins(&pEnum)))
		return NULL;

	// Walk through all pins
	IPin *pPin = NULL;
	while (pEnum->Next(1, &pPin, NULL) == S_OK) {
		// Check the pin direction
		PIN_DIRECTION PinDirThis;
		if (SUCCEEDED(pPin->QueryDirection(&PinDirThis))) {
			if (bFound = (PinDir == PinDirThis))
				break;
		}
		pPin->Release();
	}
	pEnum->Release();

	return (bFound ? pPin : NULL);  
}

HRESULT CHNMSplitterFilter::Initialize(IPin *pPin, IAsyncReader *pReader)
{
	// Check and validate the pointer
	CheckPointer(pReader, E_POINTER);
	ValidateReadPtr(pReader, sizeof(IAsyncReader));

	// Read file header
	HNM_HEADER videohdr;
	HRESULT hr = pReader->SyncRead(0, sizeof(videohdr), (BYTE*)&videohdr);
	if (hr != S_OK)
		return hr;

	// Get the file size
	LONGLONG llTotal = 0, llAvail = 0;
	hr = pReader->Length(&llTotal, &llAvail);
	if (FAILED(hr))
		return hr;

	// Verify file header
	if (
		(videohdr.dwID   != HNM_ID_HNM6) ||
		(videohdr.cbFile != (DWORD)llTotal) ||
		(memcmp(videohdr.szRND, HNM_IDSTR_RND, strlen(HNM_IDSTR_RND))) ||
		(memcmp(videohdr.szCopyright, HNM_IDSTR_COPYRIGHT, strlen(HNM_IDSTR_COPYRIGHT)))
	)
		return VFW_E_INVALID_FILE_FORMAT;

	// Scan two frames
	LONGLONG llSeekPos = sizeof(HNM_HEADER);
	APC_HEADER audiohdr = {0};
	DWORD cbAudioBlock[2] = { 0, 0 };
	for (int i = 0; i < 2; i++) {

		// Align seek position
		if (llSeekPos % 4)
			llSeekPos += 4 - (llSeekPos % 4);

		// Read chunk header
		DWORD cbChunk = 0;
		hr = pReader->SyncRead(llSeekPos, sizeof(DWORD), (BYTE*)&cbChunk);
		if (hr != S_OK)
			break;

		llSeekPos += sizeof(DWORD);
		cbChunk -= sizeof(DWORD);

		// Walk blocks of this chunk
		LONGLONG llBlockSeekPos = llSeekPos;
		while (llBlockSeekPos < llSeekPos + cbChunk) {

			// Align seek position
			if (llBlockSeekPos % 4)
				llBlockSeekPos += 4 - (llBlockSeekPos % 4);

			// Read block header
			HNM_BLOCK_HEADER blockheader = {0};
			hr = pReader->SyncRead(llBlockSeekPos, sizeof(blockheader), (BYTE*)&blockheader);
			if (hr != S_OK)
				break;

			llBlockSeekPos += sizeof(blockheader);
			DWORD cbBlock = blockheader.cbBlock - sizeof(blockheader);

			// Check if it's an audio block
			if (
				(blockheader.wBlockID == HNM_ID_AA) ||
				(blockheader.wBlockID == HNM_ID_BB)
			) {
				cbAudioBlock[i] = cbBlock;
				if (i == 0)
					pReader->SyncRead(llBlockSeekPos, sizeof(audiohdr), (BYTE*)&audiohdr);
				break;
			}

			// Advance seek position
			llBlockSeekPos += cbBlock;
		}

		// Advance seek position
		llSeekPos += cbChunk;
	}

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Work out the frame rate for the video stream and audio buffers number
	WORD nFramesPerSecond = HNM_FPS_DEFAULT;
	long cAudioBuffers = 33;
	if (
		(cbAudioBlock[0]		!= 0) &&
		(cbAudioBlock[1]		!= 0) &&
		(audiohdr.dwSampleRate	!= 0)
	) {
		// First audio block contains APC header
		cbAudioBlock[0] -= sizeof(APC_HEADER);

		// Number of audio buffers should be enough to accept all 
		// frames covered by the first audio block data
		cAudioBuffers = cbAudioBlock[0] / cbAudioBlock[1] + 1;

		// Frame rate can be derived from the second audio block duration
		float fFramesPerSecond = (
			(float)2 * ((audiohdr.bIsStereo) ? 2 : 1) * audiohdr.dwSampleRate
		) / (cbAudioBlock[1] * 4);
		nFramesPerSecond = (WORD)fFramesPerSecond;
		if (fFramesPerSecond - nFramesPerSecond >= 0.5)
			nFramesPerSecond++;
	}

	// Replace the unknown1 value in the HNM header with the frame rate
	videohdr.wUnknown1 = m_nFramesPerSecond = nFramesPerSecond;

	// Set file positions
	m_llDefaultStart	= (LONGLONG)sizeof(HNM_HEADER);	// Right after the header
	m_llDefaultStop		= MAXLONGLONG; // Defaults to file end
	// Align starting position (if necessary)
	if (m_llDefaultStart % 4)
		m_llDefaultStart += 4 - m_llDefaultStart % 4;

	// Decide on the input pin properties
	m_cbInputAlign	= 1; // TEMP: maybe we should use actual alignment?
	m_cbInputBuffer	= videohdr.cbMaxChunk; // TEMP: maybe we should use 0x40000 instead?

	// Set the video stream duration
	m_nVideoFrames = videohdr.nFrames;

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Decide on the output pins count
	m_nOutputPins = 1;		// Video always should be present in the file
	if (videohdr.bSoundFlags)
		m_nOutputPins++;	// There's also audio stream in the file

	// Create output pins array
	ASSERT(m_ppOutputPin == NULL);
	m_ppOutputPin = new CParserOutputPin*[m_nOutputPins];
	if (m_ppOutputPin == NULL) {
		m_nOutputPins = 0;
		Shutdown();
		return E_OUTOFMEMORY;
	}

	// Reset the output pin array elements to NULLs
	int i = 0;
	for (i = 0; i < m_nOutputPins; i++)
		m_ppOutputPin[i] = NULL;

	// Allocate video media type
	CMediaType *pmtVideo = new CMediaType();
	if (pmtVideo == NULL) {
		Shutdown();
		return E_OUTOFMEMORY;
	}

	// Initialize the video media type
	pmtVideo->InitMediaType();
	pmtVideo->SetType(&MEDIATYPE_Video);
	pmtVideo->SetSubtype(&MEDIASUBTYPE_HNMVideo);
	pmtVideo->SetSampleSize(0);				// Variable size samples
	pmtVideo->SetTemporalCompression(TRUE);	// Using temporal compression
	pmtVideo->SetFormatType(&FORMAT_HNMVideo);
	if (!pmtVideo->SetFormat((BYTE*)&videohdr, sizeof(videohdr))) {
		delete pmtVideo;
		Shutdown();
		return E_FAIL;
	}

	// Allocate the video allocator properties
	ALLOCATOR_PROPERTIES *papVideo = (ALLOCATOR_PROPERTIES*)CoTaskMemAlloc(sizeof(ALLOCATOR_PROPERTIES));
	if (papVideo == NULL) {
		delete pmtVideo;
		Shutdown();
		return E_OUTOFMEMORY;
	}

	// Set the video allocator properties
	papVideo->cbAlign	= 0;		// No matter
	papVideo->cbPrefix	= 0;		// No matter
	papVideo->cBuffers	= 4;		// TEMP: small value
	papVideo->cbBuffer	= 0x40000;	// TEMP: mere guess

	// Allocate time formats array. If we fail here, it's not an error, 
	// we'll just set zero seeker parameters and may proceed
	DWORD dwVideoCapabilities = 0;
	int nVideoTimeFormats = 0;
	GUID *pVideoTimeFormats = (GUID*)CoTaskMemAlloc(3 * sizeof(GUID));
	if (pVideoTimeFormats) {

		nVideoTimeFormats = 3;

		// Fill in the time formats array
		pVideoTimeFormats[0] = TIME_FORMAT_MEDIA_TIME;
		pVideoTimeFormats[1] = TIME_FORMAT_FRAME;
		pVideoTimeFormats[2] = TIME_FORMAT_SAMPLE;

		dwVideoCapabilities =	AM_SEEKING_CanGetCurrentPos	|
								AM_SEEKING_CanGetStopPos	|
								AM_SEEKING_CanGetDuration;
	}

	// Create video output pin (always the first one!)
	hr = NOERROR;
	m_ppOutputPin[0] = new CParserOutputPin(
		NAME("HNM Splitter Video Output Pin"),
		this,
		&m_csFilter,
		pmtVideo,
		papVideo,
		dwVideoCapabilities,
		nVideoTimeFormats,
		pVideoTimeFormats,
		&hr,
		wszHNMVideoOutputName
	);
	if (
		(FAILED(hr)) ||
		(m_ppOutputPin[0] == NULL)
	) {
		if (m_ppOutputPin[0]) {
			delete m_ppOutputPin[0];
			m_ppOutputPin[0] = NULL;
		} else {
			delete pmtVideo;
			CoTaskMemFree(papVideo);
			if (pVideoTimeFormats)
				CoTaskMemFree(pVideoTimeFormats);
		}
		Shutdown();

		if (FAILED(hr))
			return hr;
		else
			return E_OUTOFMEMORY;
	}
	// Hold a reference on the video output pin
	m_ppOutputPin[0]->AddRef();

	// We've created a new pin -- so increment pin version
	IncrementPinVersion();

	// Scope for the locking
	{
		// Protect media content information
		CAutoLock infolock(&m_csInfo);

		// Set the media content strings
		m_wszAuthorName = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszHNMAuthorName) + 1));
		if (m_wszAuthorName)
			lstrcpyW(m_wszAuthorName, wszHNMAuthorName);
		m_wszDescription = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszHNMDescription) + 1));
		if (m_wszDescription)
			lstrcpyW(m_wszDescription, wszHNMDescription);
		m_wszCopyright = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszHNMCopyright) + 1));
		if (m_wszCopyright)
			lstrcpyW(m_wszCopyright, wszHNMCopyright);
		m_wszMoreInfoText = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszHNMMoreInfoText) + 1));
		if (m_wszMoreInfoText)
			lstrcpyW(m_wszMoreInfoText, wszHNMMoreInfoText);
	}

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	// Create audio output pin if we have to
	if (videohdr.bSoundFlags) {

		// Verify audio header
		if (
			(audiohdr.dwID1		!= APC_ID_CRYO)	||
			(audiohdr.dwID2		!= APC_ID_APC)	||
			(audiohdr.dwVersion	!= APC_ID_VER)
		) {
			Shutdown();
			return VFW_E_INVALID_FILE_FORMAT;
		}

		// Allocate audio media type
		CMediaType *pmtAudio = new CMediaType();
		if (pmtAudio == NULL) {
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Initialize the audio media type
		pmtAudio->InitMediaType();
		pmtAudio->SetType(&MEDIATYPE_Audio);
		pmtAudio->SetSubtype(&MEDIASUBTYPE_CIMAADPCM);
		pmtAudio->SetSampleSize(0);				// Variable size samples
		pmtAudio->SetTemporalCompression(TRUE);	// Using temporal compression
		pmtAudio->SetFormatType(&FORMAT_CIMAADPCM);
		CIMAADPCMWAVEFORMAT *pFormat = (CIMAADPCMWAVEFORMAT*)pmtAudio->AllocFormatBuffer(sizeof(CIMAADPCMWAVEFORMAT) + (((audiohdr.bIsStereo) ? 2 : 1) - 1) * sizeof(CIMAADPCMINFO));
		if (pFormat == NULL) {
			delete pmtAudio;
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Fill in the audio format block
		pFormat->nChannels			= ((audiohdr.bIsStereo) ? 2 : 1);
		pFormat->nSamplesPerSec		= audiohdr.dwSampleRate;
		pFormat->wBitsPerSample		= 16;
		pFormat->bIsHiNibbleFirst	= TRUE;
		pFormat->dwReserved			= CIMAADPCM_INTERLEAVING_NORMAL;

		// Fill in init info block for each channel
		pFormat->pInit[0].lSample	= audiohdr.lLeftSample;
		pFormat->pInit[0].chIndex	= 0;
		if (audiohdr.bIsStereo) {
			pFormat->pInit[1].lSample	= audiohdr.lRightSample;
			pFormat->pInit[1].chIndex	= 0;
		}

		// Allocate the audio allocator properties
		ALLOCATOR_PROPERTIES *papAudio = (ALLOCATOR_PROPERTIES*)CoTaskMemAlloc(sizeof(ALLOCATOR_PROPERTIES));
		if (papAudio == NULL) {
			delete pmtAudio;
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Set the audio allocator properties
		papAudio->cbAlign	= 0;	// No matter
		papAudio->cbPrefix	= 0;	// No matter
		papAudio->cBuffers	= cAudioBuffers;
		papAudio->cbBuffer	= videohdr.cbMaxChunk;	// DANGER: this might be not enough

		// Set the wave format parameters needed for the calculation
		// of sample stream and media duration
		m_nSampleSize		= 2 * pFormat->nChannels;
		m_nAvgBytesPerSec	= m_nSampleSize * pFormat->nSamplesPerSec;

		// Set the audio stream duration
		m_nAudioSamples = audiohdr.nSamples;

		// Allocate time formats array. If we fail here, it's not an error, 
		// we'll just set zero seeker parameters and may proceed
		DWORD dwAudioCapabilities = 0;
		int nAudioTimeFormats = 0;
		GUID *pAudioTimeFormats = (GUID*)CoTaskMemAlloc(3 * sizeof(GUID));
		if (pAudioTimeFormats) {

			nAudioTimeFormats = 3;

			// Fill in the time formats array
			pAudioTimeFormats[0] = TIME_FORMAT_MEDIA_TIME;
			pAudioTimeFormats[1] = TIME_FORMAT_SAMPLE;
			pAudioTimeFormats[2] = TIME_FORMAT_BYTE;

			dwAudioCapabilities =	AM_SEEKING_CanGetCurrentPos	|
									AM_SEEKING_CanGetStopPos	|
									AM_SEEKING_CanGetDuration;
		}

		// Create audio output pin
		hr = NOERROR;
		m_ppOutputPin[1] = new CParserOutputPin(
			NAME("HNM Splitter Audio Output Pin"),
			this,
			&m_csFilter,
			pmtAudio,
			papAudio,
			dwAudioCapabilities,
			nAudioTimeFormats,
			pAudioTimeFormats,
			&hr,
			wszHNMAudioOutputName
		);
		if (
			(FAILED(hr)) ||
			(m_ppOutputPin[1] == NULL)
		) {
			if (m_ppOutputPin[1]) {
				delete m_ppOutputPin[1];
				m_ppOutputPin[1] = NULL;
			} else {
				delete pmtAudio;
				CoTaskMemFree(papAudio);
				if (pAudioTimeFormats)
					CoTaskMemFree(pAudioTimeFormats);
			}
			Shutdown();

			if (FAILED(hr))
				return hr;
			else
				return E_OUTOFMEMORY;
		}
		// Hold a reference on the audio output pin
		m_ppOutputPin[1]->AddRef();

		// We've created a new pin -- so increment pin version
		IncrementPinVersion();

	} else if (m_bUseExternalAPC) { // !videohdr.bSoundFlags

		// Work out the filename of the APC file from the filename
		// of the HNM file and throw in the source filter for APC 
		// file into the filter graph. If we fail, return success 
		// code (the playback may proceed without audio output)

		// Check if we're in the filter graph
		if (m_pGraph == NULL)
			return VFW_S_PARTIAL_RENDER;

		// Query the connecting pin info
		PIN_INFO pininfo;
		hr = pPin->QueryPinInfo(&pininfo);
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;

		if (pininfo.pFilter == NULL)
			return VFW_S_PARTIAL_RENDER;

		// Get the CLSID of the HNM source filter
		CLSID clsidSource;
		hr = pininfo.pFilter->GetClassID(&clsidSource);
		if (FAILED(hr)) {
			pininfo.pFilter->Release();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get the file source interface from the filter
		IFileSourceFilter *pHNMSource = NULL;
		hr = pininfo.pFilter->QueryInterface(IID_IFileSourceFilter, (void**)&pHNMSource);
		pininfo.pFilter->Release();
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;

		// Get the HNM filename from the filter
		OLECHAR *pszTemp;
		AM_MEDIA_TYPE mt;
		hr = pHNMSource->GetCurFile(&pszTemp, &mt);
		pHNMSource->Release();
		FreeMediaType(mt); // Is that really needed???
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;
		OLECHAR szFileName[MAX_PATH];
		lstrcpyW(szFileName, pszTemp);

		// Get the file title from the HNM filename
		OLECHAR *pTitleStart = wcsrchr(szFileName, L'\\');
		if (pTitleStart == NULL)
			pTitleStart = szFileName;
		else
			pTitleStart++;

		// Copy file title to the temp storage
		OLECHAR szFileTitle[MAX_PATH];
		lstrcpyW(szFileTitle, pTitleStart);

		// Append APC (relative) directory to the directory path
		lstrcpyW(pTitleStart, m_szExternalAPCPath);
		if (szFileName[lstrlenW(szFileName) - 1] != L'\\')
			lstrcpyW(szFileName + lstrlenW(szFileName), L"\\");

		// Replace the extension in the file title
		OLECHAR *pExt = wcsrchr(szFileTitle, L'.');
		if (pExt == NULL)
			pExt = szFileTitle + lstrlenW(szFileTitle);
		lstrcpyW(pExt, L".apc");

		// Compose the APC filename
		lstrcpyW(szFileName + lstrlenW(szFileName), szFileTitle);

		// Create async reader object
		IFileSourceFilter *pAPCSource = NULL;
		hr = CoCreateInstance(
			clsidSource,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IFileSourceFilter,
			(void**)&pAPCSource
		);
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;

		// Load APC file
		hr = pAPCSource->Load(szFileName, NULL);
		if (FAILED(hr)) {
			pAPCSource->Release();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get base filter interface on the source filter
		hr = pAPCSource->QueryInterface(IID_IBaseFilter, (void**)&m_pAPCSource);
		pAPCSource->Release();
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;

		// Add the source filter to our filter graph
		hr = m_pGraph->AddFilter(m_pAPCSource, NULL);
		if (FAILED(hr)) {
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get an output pin from the source filter
		IPin *pAPCSourceOutput = GetPinWithDirection(m_pAPCSource, PINDIR_OUTPUT);
		if (pAPCSourceOutput == NULL) {
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Create APC parser object
		hr = CoCreateInstance(
			CLSID_APCParser,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IBaseFilter,
			(void**)&m_pAPCParser
		);
		if (FAILED(hr)) {
			pAPCSourceOutput->Release();
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Add the parser filter to our filter graph
		hr = m_pGraph->AddFilter(m_pAPCParser, g_wszAPCParserName);
		if (FAILED(hr)) {
			pAPCSourceOutput->Release();
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get the parser input pin
		IPin *pAPCParserInput = GetPinWithDirection(m_pAPCParser, PINDIR_INPUT);
		if (pAPCParserInput == NULL) {
			pAPCSourceOutput->Release();
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Connect the source output pin to the parser input pin
		hr = m_pGraph->ConnectDirect(pAPCSourceOutput, pAPCParserInput, NULL);
		pAPCSourceOutput->Release();
		pAPCParserInput->Release();
		if (FAILED(hr)) {
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get the parser output pin
		IPin *pAPCParserOutput = GetPinWithDirection(m_pAPCParser, PINDIR_OUTPUT);
		if (pAPCParserOutput == NULL) {
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		m_pPassThruLock = new CCritSec();
		if (m_pPassThruLock == NULL) {
			pAPCParserOutput->Release();
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Create pass-through input and output pins
		m_pPassThruInput = new CPassThruInputPin(
			this,
			&m_csFilter,
			m_pPassThruLock,
			&hr,
			L"PassThru Input",
			L"PassThru Output",
			&m_pPassThruOutput
		);
		if (
			(FAILED(hr))					||
			(m_pPassThruInput	== NULL)	||
			(m_pPassThruOutput	== NULL)
		) {
			if (m_pPassThruInput) {
				delete m_pPassThruInput;
				m_pPassThruInput	= NULL;
				m_pPassThruOutput	= NULL;
			}
			pAPCParserOutput->Release();
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Hold a reference on the pass-through input pin
		m_pPassThruInput->AddRef();
		// The pass-through output pin does not require 
		// refcounting as its lifetime is controlled 
		// by the containing pass-through input pin

		// We've created new pins -- so increment pin version
		IncrementPinVersion();

		// Connect the parser output to the pass-through input
		hr = m_pGraph->ConnectDirect(pAPCParserOutput, m_pPassThruInput, NULL);
		pAPCParserOutput->Release();
		if (FAILED(hr)) {
			m_pPassThruInput->Release();
			delete m_pPassThruInput;
			m_pPassThruInput	= NULL;
			m_pPassThruOutput	= NULL;
			IncrementPinVersion();
			ShutdownExternalAPC();
			return VFW_S_PARTIAL_RENDER;
		}

		// Notify the filter graph manager of the changes made
		NotifyEvent(EC_GRAPH_CHANGED, 0, 0);

	}

	return NOERROR;
}

HRESULT CHNMSplitterFilter::ShutdownExternalAPC(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	HRESULT hr = NOERROR;

	// Remove from the graph & release APC parser
	if (m_pAPCParser) {
		FILTER_INFO info;
		if (m_pAPCParser->QueryFilterInfo(&info) == S_OK) {
			if (info.pGraph) {
				hr = info.pGraph->RemoveFilter(m_pAPCParser);
				info.pGraph->Release();
				if (FAILED(hr))
					return hr;
			}
		}
		m_pAPCParser->Release();
		m_pAPCParser = NULL;
	}

	// Remove from the graph & release APC source
	if (m_pAPCSource) {
		FILTER_INFO info;
		if (m_pAPCSource->QueryFilterInfo(&info) == S_OK) {
			if (info.pGraph) {
				hr = info.pGraph->RemoveFilter(m_pAPCSource);
				info.pGraph->Release();
				if (FAILED(hr))
					return hr;
			}
		}
		m_pAPCSource->Release();
		m_pAPCSource = NULL;
	}

	// Delete the pass-through lock
	if (m_pPassThruLock) {
		delete m_pPassThruLock;
		m_pPassThruLock = NULL;
	}

	// Disconnect and release the pass-through pins
	if (m_pPassThruInput) {
		m_pPassThruInput->DisconnectBoth();
		m_pPassThruInput->Release();
		m_pPassThruInput	= NULL;
		m_pPassThruOutput	= NULL;
		IncrementPinVersion();
	}
	// The pass-through output pin is destroyed automatically 
	// by the pass-through input pin's destructor

	// Notify the filter graph manager of the changes made
	NotifyEvent(EC_GRAPH_CHANGED, 0, 0);

	return NOERROR;
}

HRESULT CHNMSplitterFilter::Shutdown(void)
{
	// Shutdown the parser
	HRESULT hr = ShutdownParser();
	if (FAILED(hr))
		return hr;

	// Shutdown external APC parsing system
	hr = ShutdownExternalAPC();
	if (FAILED(hr))
		return hr;

	// Scope for the locking
	{
		// Protect the filter data
		CAutoLock datalock(&m_csData);

		// Reset the stream-specific variables
		m_nFramesPerSecond	= 0;
		m_nSampleSize		= 0;
		m_nAvgBytesPerSec	= 0;
		m_nVideoFrames		= 0;
		m_nAudioSamples		= 0;
	}

	// Call the base-class implementation
	return CBaseParserFilter::Shutdown();
}

HRESULT CHNMSplitterFilter::InitializeParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	HRESULT hr = NOERROR;

	// Create chunk parser if we have to
	if (m_pParser == NULL) {

		m_pParser = new CHNMOuterChunkParser(
			this,
			m_ppOutputPin,
			m_nFramesPerSecond,
			m_nSampleSize,
			m_nAvgBytesPerSec,
			&hr
		);
		if (
			(FAILED(hr)) ||
			(m_pParser == NULL)
		) {
			
			if (m_pParser)
				delete m_pParser;
			m_pParser = NULL;
	
			if (FAILED(hr))
				return hr;
			else
				return E_OUTOFMEMORY;
		}
	}

	ASSERT(m_pParser);

	// Reset the parser
	hr = m_pParser->ResetParser();
	if (FAILED(hr))
		return hr;

	return NOERROR;
}

HRESULT CHNMSplitterFilter::ShutdownParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Delete chunk parser (if any)
	if (m_pParser) {

		// Reset the parser
		HRESULT hr = m_pParser->ResetParser();
		if (FAILED(hr))
			return hr;

		delete m_pParser;
		m_pParser = NULL;
	}

	return NOERROR;
}

HRESULT CHNMSplitterFilter::ResetParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Reset the chunk parser
	if (m_pParser) {
		HRESULT hr = m_pParser->ResetParser();
		if (FAILED(hr))
			return hr;
	}

	return NOERROR;
}

HRESULT CHNMSplitterFilter::CheckInputType(const CMediaType* pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	if (
		(IsEqualGUID(*pmt->Type(),		MEDIATYPE_Stream)) &&	// We accept byte stream
		(IsEqualGUID(*pmt->Subtype(),	MEDIASUBTYPE_HNM))		// ... from HNM file
	)
		return S_OK;
	else
		return S_FALSE;
}

HRESULT CHNMSplitterFilter::GetInputType(int iPosition, CMediaType *pMediaType)
{
	// Check and validate the pointer
	CheckPointer(pMediaType, E_POINTER);
	ValidateWritePtr(pMediaType, sizeof(CMediaType));

	if (iPosition < 0)
		return E_INVALIDARG;
	else if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;
	else {
		// We support the only media type
		pMediaType->InitMediaType();				// Is it needed ???
		pMediaType->SetType(&MEDIATYPE_Stream);		// Byte stream
		pMediaType->SetSubtype(&MEDIASUBTYPE_HNM);	// From HNM file
		pMediaType->SetVariableSize();				// Variable size samples
		pMediaType->SetTemporalCompression(FALSE);	// No temporal compression
		pMediaType->SetFormatType(&FORMAT_None);	// Is it needed ???
	}

	return S_OK;
}
	
HRESULT CHNMSplitterFilter::Receive(
	LONGLONG llStartPosition,
	BYTE *pbData,
	LONG lDataSize
)
{
	// At this point we should have the chunk parser
	if (m_pParser == NULL)
		return E_UNEXPECTED;

	// Pass data to the chunk parser
	return m_pParser->Receive(llStartPosition, pbData, lDataSize);
}

STDMETHODIMP CHNMSplitterFilter::Stop(void)
{
	CAutoLock lock1(&m_csFilter);

	if (m_State == State_Stopped)
		return NOERROR;

	// Decommit the input pin before locking or we can deadlock
	m_InputPin.Inactive();

	// Also decommit the pass-through input pin
	if (m_pPassThruInput)
		m_pPassThruInput->Inactive();

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
	
		// Also decommit the pass-through output pin
		if (m_pPassThruOutput)
			m_pPassThruOutput->Inactive();
	}

	m_State = State_Stopped;

	return NOERROR;
}

HRESULT CHNMSplitterFilter::ConvertTimeFormat(
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

	if (!lstrcmpW(pPinName, wszHNMVideoOutputName)) {

		// Process the video output pin's request

		// Frames and samples are identical
		if (
			(
				(IsEqualGUID(*pSourceFormat, TIME_FORMAT_SAMPLE)) &&
				(IsEqualGUID(*pTargetFormat, TIME_FORMAT_FRAME))
			) ||
			(
				(IsEqualGUID(*pSourceFormat, TIME_FORMAT_FRAME)) &&
				(IsEqualGUID(*pTargetFormat, TIME_FORMAT_SAMPLE))
			)
		) {
			*pTarget = llSource;
			return NOERROR;
		}

		// Convert samples/frames to media time
		if (
			(
				(IsEqualGUID(*pSourceFormat, TIME_FORMAT_SAMPLE)) ||
				(IsEqualGUID(*pSourceFormat, TIME_FORMAT_FRAME))
			) &&
			(IsEqualGUID(*pTargetFormat, TIME_FORMAT_MEDIA_TIME))
		) {
			// Check if we have correct video format parameters
			if (m_nFramesPerSecond == 0)
				return E_UNEXPECTED;

			*pTarget = (llSource * UNITS) / m_nFramesPerSecond;
			return NOERROR;
		}

		// Convert media time to samples/frames
		if (
			(IsEqualGUID(*pSourceFormat, TIME_FORMAT_MEDIA_TIME)) &&
			(
				(IsEqualGUID(*pTargetFormat, TIME_FORMAT_SAMPLE)) ||
				(IsEqualGUID(*pTargetFormat, TIME_FORMAT_FRAME))
			)
		) {
			*pTarget = (llSource * m_nFramesPerSecond) / UNITS;
			return NOERROR;
		}

		// If the source/target formats pair does not match one of the 
		// above, we cannot perform convertion
		return E_INVALIDARG;

	} else if (!lstrcmpW(pPinName, wszHNMAudioOutputName)) {

		// Process the audio output pin's request

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

		// At this point the conversion is successfully done
		return NOERROR;
	}

	return E_NOTIMPL;
}

HRESULT CHNMSplitterFilter::GetDuration(
	LPCWSTR pPinName,
	const GUID *pCurrentFormat,
	LONGLONG *pDuration
)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	if (!lstrcmpW(pPinName, wszHNMVideoOutputName)) {

		// Process the video output pin's request

		// Convert the duration in frames to the current time format
		return ConvertTimeFormat(
			pPinName,
			pDuration,
			pCurrentFormat,
			(LONGLONG)m_nVideoFrames,
			&TIME_FORMAT_FRAME
		);

	} else if (!lstrcmpW(pPinName, wszHNMAudioOutputName)) {

		// Process the audio output pin's request

		// Convert the duration in audio samples to the current time format
		return ConvertTimeFormat(
			pPinName,
			pDuration,
			pCurrentFormat,
			(LONGLONG)m_nAudioSamples,
			&TIME_FORMAT_SAMPLE
		);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CHNMSplitterFilter::get_UseExternalAPC(BOOL *pbUseExternalAPC)
{
	// Check and validate the pointer
	CheckPointer(pbUseExternalAPC, E_POINTER);
	ValidateWritePtr(pbUseExternalAPC, sizeof(BOOL));

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	*pbUseExternalAPC = m_bUseExternalAPC;

	return NOERROR;
}

STDMETHODIMP CHNMSplitterFilter::get_ExternalAPCPath(OLECHAR *szExternalAPCPath)
{
	// Check the pointer
	CheckPointer(szExternalAPCPath, E_POINTER);

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	lstrcpyW(szExternalAPCPath, m_szExternalAPCPath);

	return NOERROR;
}

STDMETHODIMP CHNMSplitterFilter::put_UseExternalAPC(BOOL bUseExternalAPC)
{
	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	m_bUseExternalAPC = bUseExternalAPC;

	return NOERROR;
}

STDMETHODIMP CHNMSplitterFilter::put_ExternalAPCPath(OLECHAR *szExternalAPCPath)
{
	// Check the pointer
	CheckPointer(szExternalAPCPath, E_POINTER);

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	lstrcpyW(m_szExternalAPCPath, szExternalAPCPath);

	return NOERROR;
}

STDMETHODIMP CHNMSplitterFilter::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_HNMSplitterPage;
	pPages->pElems[1] = CLSID_BaseParserPage;

	return NOERROR;
}

//==========================================================================
// CHNMSplitterPage methods
//==========================================================================

const WCHAR g_wszHNMSplitterPageName[] = L"ANX HNM Splitter Property Page";

CHNMSplitterPage::CHNMSplitterPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("HNM Splitter Property Page"),
		pUnk,
		IDD_HNMSPLITTERPAGE,
		IDS_TITLE_HNMSPLITTERPAGE
	),
	m_pConfig(NULL) // No config interface at this time
{
}

CUnknown* WINAPI CHNMSplitterPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CHNMSplitterPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}

void CHNMSplitterPage::SetDirty()
{
	// Set the dirty flag
	m_bDirty = TRUE;

	// Notify the page site of the dirty state
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

BOOL CHNMSplitterPage::GetUseExternalAPC()
{
	// Get the checkbox state 
	return (IsDlgButtonChecked(m_Dlg, IDC_USEEXTERNALAPC) == BST_CHECKED);
}

void CHNMSplitterPage::EnableExternalAPCPath(BOOL bEnable)
{
	EnableWindow(GetDlgItem(m_Dlg, IDC_EXTERNALAPCPATH), bEnable);
	EnableWindow(GetDlgItem(m_Dlg, IDC_EXTERNALAPCPATHFRAME), bEnable);
}

BOOL CHNMSplitterPage::OnReceiveMessage(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMsg) {

		case WM_COMMAND:

			if (LOWORD(wParam) == IDC_USEEXTERNALAPC) {

				SetDirty();

				// Set the enabled state of the edit control according 
				// to the checkbox state
				EnableExternalAPCPath(GetUseExternalAPC());

			} else if (LOWORD(wParam) == IDC_EXTERNALAPCPATH) {

				if (HIWORD(wParam) == EN_CHANGE) {
					if (m_bIsFirstSetText)
						m_bIsFirstSetText = FALSE;
					else
						SetDirty();
				}

			} else if (LOWORD(wParam) == IDC_DEFAULTS) {

				// Set the default values
				CheckDlgButton(m_Dlg, IDC_USEEXTERNALAPC, BST_CHECKED);
				EnableExternalAPCPath(TRUE);
				SetDlgItemText(m_Dlg, IDC_EXTERNALAPCPATH, TEXT("..\\cine"));

				SetDirty();

			}

			return (LRESULT)1;
	}

	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CHNMSplitterPage::OnConnect(IUnknown *pUnknown)
{
	// At this time there should be no config interface
	ASSERT(m_pConfig == NULL);

	// Retrieve the config interface from the filter
	HRESULT hr = pUnknown->QueryInterface(
		IID_IConfigHNMSplitter,
		(void**)&m_pConfig
	);
	if (FAILED(hr))
		return hr;

	ASSERT(m_pConfig);

	return NOERROR;
}

HRESULT CHNMSplitterPage::OnDisconnect()
{
	// At this time we should have the config interface
	if (m_pConfig == NULL)
		return E_UNEXPECTED;

	// Release the config interface
	m_pConfig->Release();
	m_pConfig = NULL;

	return NOERROR;
}

HRESULT CHNMSplitterPage::OnActivate()
{
	// At this time we should have the config interface
	if (m_pConfig == NULL)
		return E_UNEXPECTED;

	// Get the option value from the filter
	BOOL bUseExternalAPC;
	HRESULT hr = m_pConfig->get_UseExternalAPC(&bUseExternalAPC);
	if (FAILED(hr))
		return hr;

	// Set the checkbox appropriately
	CheckDlgButton(m_Dlg, IDC_USEEXTERNALAPC, (bUseExternalAPC) ? BST_CHECKED : BST_UNCHECKED);

	// Get the option value from the filter
	OLECHAR wszExternalAPCPath[MAX_PATH];
	hr = m_pConfig->get_ExternalAPCPath(wszExternalAPCPath);
	if (FAILED(hr))
		return hr;

	// Convert the wide string to TCHAR string
	TCHAR szExternalAPCPath[MAX_PATH];
	wsprintf(szExternalAPCPath, TEXT("%ls"), wszExternalAPCPath);

	// We set text in the edit control for the first time
	m_bIsFirstSetText = TRUE;

	// Set the edit control text appropriately
	SetDlgItemText(m_Dlg, IDC_EXTERNALAPCPATH, szExternalAPCPath);

	EnableExternalAPCPath(bUseExternalAPC);

	m_bDirty = FALSE;

	return NOERROR;
}

HRESULT CHNMSplitterPage::OnApplyChanges()
{
	if (m_bDirty) {

		// At this time we should have the config interface
		if (m_pConfig == NULL)
			return E_UNEXPECTED;

		// Put the option on the filter
		HRESULT hr = m_pConfig->put_UseExternalAPC(GetUseExternalAPC());
		if (FAILED(hr))
			return hr;

		if (GetUseExternalAPC()) {

			// Get the edit control text
			TCHAR szExternalAPCPath[MAX_PATH];
			SendDlgItemMessage(
				m_Dlg,
				IDC_EXTERNALAPCPATH,
				WM_GETTEXT,
				(WPARAM)MAX_PATH,
				(LPARAM)szExternalAPCPath
			);

			// Convert the TCHAR string to wide string
			OLECHAR wszExternalAPCPath[MAX_PATH];
			MultiByteToWideChar(
				CP_ACP,
				0,
				szExternalAPCPath,
				-1,
				wszExternalAPCPath,
				MAX_PATH
			);
			
			// Put the option on the filter
			hr = m_pConfig->put_ExternalAPCPath(wszExternalAPCPath);
			if (FAILED(hr))
				return hr;

		}

		m_bDirty = FALSE;
	}

	return NOERROR;
}

