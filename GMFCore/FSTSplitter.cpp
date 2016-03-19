//==========================================================================
//
// File: FSTSplitter.cpp
//
// Desc: Game Media Formats - Implementation of FST splitter filter
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

#include "FSTSpecs.h"
#include "FSTGUID.h"
#include "FSTSplitter.h"
#include "resource.h"

//==========================================================================
// FST splitter setup data
//==========================================================================

// Media content strings
const OLECHAR wszFSTAuthorName[]	= L"FutureVision";
const OLECHAR wszFSTDescription[]	= L"FutureVision FST video file";

// Global filter name
const WCHAR g_wszFSTSplitterName[]	= L"ANX FST Splitter";

// Output pin names
const WCHAR wszFSTVideoOutputName[]	= L"Video Output";
const WCHAR wszFSTAudioOutputName[]	= L"Audio Output";

const AMOVIESETUP_MEDIATYPE sudFSTStreamType = {
	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_FST
};

const AMOVIESETUP_MEDIATYPE sudFSTVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_FSTVideo
};

const AMOVIESETUP_MEDIATYPE sudFSTAudioType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

const AMOVIESETUP_PIN sudFSTSplitterPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudFSTStreamType	// Media types
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
		&sudFSTVideoType	// Media types
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
		&sudFSTAudioType	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudFSTSplitter = {
	&CLSID_FSTSplitter,		// CLSID of filter
	g_wszFSTSplitterName,	// Filter name
	MERIT_NORMAL,			// Filter merit
	3,						// Number of pins
	sudFSTSplitterPins		// Pin information
};

//==========================================================================
// FST splitter file type setup data
//==========================================================================

const GMF_FILETYPE_PATTERN sudFSTPatterns[] = {
	{ 0,	4,	FST_IDSTR_2TSF	}
};

const GMF_FILETYPE_ENTRY sudFSTEntries[] = {
	{ 1, sudFSTPatterns } 
};

const TCHAR *sudFSTExtensions[] = {
	TEXT(".fst")
};

const GMF_FILETYPE_REGINFO CFSTSplitterFilter::g_pRegInfo[] = {
	{
		TEXT("FutureVision FST Video Format"),	// Type name
		&MEDIATYPE_Stream,						// Media type
		&MEDIASUBTYPE_FST,						// Media subtype
		&CLSID_AsyncReader,						// Source filter
		1,										// Number of entries
		sudFSTEntries,							// Entries
		1,										// Number of extensions
		sudFSTExtensions						// Extensions
	}
};

const int CFSTSplitterFilter::g_nRegInfo = 1;

//==========================================================================
// CFSTChunkParser methods
//==========================================================================

CFSTChunkParser::CFSTChunkParser(
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	DWORD nVideoFrames,
	FST_FRAME_ENTRY *pFrameTable,
	DWORD nFramesPerSecond,
	WORD nSampleSize,
	DWORD nAvgBytesPerSec,
	HRESULT *phr
) :
	CBaseChunkParser(
		NAME("FST Chunk Parser"),
		pFilter,
		ppOutputPin,
		0,
		1,
		phr
	),
	m_pPin(NULL),
	m_pSample(NULL),
	m_rtDelta(0),
	m_llDelta(0),
	m_nVideoFrames(nVideoFrames),
	m_pFrameTable(pFrameTable),
	m_nFramesPerSecond(nFramesPerSecond),
	m_nSampleSize(nSampleSize),
	m_nAvgBytesPerSec(nAvgBytesPerSec)
{
	ASSERT(pFrameTable != NULL);
}

CFSTChunkParser::~CFSTChunkParser()
{
	// Free any resources allocated by the parser
	ResetParser();
}

// Since we don't expect any seek operation, we reset the parser 
// so that it's ready to parse data from the file beginning
HRESULT CFSTChunkParser::ResetParser(void)
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

	// Call base-class method to reset parser state
	return CBaseChunkParser::ResetParser();
}

HRESULT CFSTChunkParser::ParseChunkHeader(
	LONGLONG llStartPosition,
	BYTE *pbHeader,
	LONG *plDataSize
)
{
	// Check and validate the pointer
	CheckPointer(plDataSize, E_POINTER);
	ValidateWritePtr(plDataSize, sizeof(LONG));

	m_pPin = NULL;
	m_pSample = NULL;
	m_rtDelta = 0;
	m_llDelta = 0;
	
	// Adjust data start position to video/audio stream start
	llStartPosition -= sizeof(FST_HEADER) + m_nVideoFrames * sizeof(FST_FRAME_ENTRY);

	// Walk frame entries table until data start position falls
	// into the frame data range
	DWORD iFrame = 0;
	for (iFrame = 0; iFrame < m_nVideoFrames; iFrame++) {
		DWORD cbFrameData = m_pFrameTable[iFrame].cbImage + m_pFrameTable[iFrame].cbSound; 
		if (llStartPosition < cbFrameData)
			break;
		llStartPosition -= cbFrameData;
	};

	// At this point we should be either at the start of image data 
	// or at the start of the sound data and nowhere else
	if (
		(llStartPosition != 0) &&
		(llStartPosition != m_pFrameTable[iFrame].cbImage)
	)
		return E_UNEXPECTED;

	if (llStartPosition == 0) {

		// ---- Video data ----

		*plDataSize = m_pFrameTable[iFrame].cbImage;

		m_pPin = m_ppOutputPin[0];	// We'll be dealing with video output pin

		// Check if we have correct video format parameters
		if (m_nFramesPerSecond == 0)
			return E_UNEXPECTED;

		m_rtDelta = UNITS / m_nFramesPerSecond;
		m_llDelta = 1;

	} else if (llStartPosition == m_pFrameTable[iFrame].cbImage) {

		// ---- Audio data ----

		*plDataSize = m_pFrameTable[iFrame].cbSound;

		ASSERT(m_ppOutputPin[1]);	// There should be the audio output pin
		m_pPin = m_ppOutputPin[1];	// We'll be dealing with audio output pin

		// Check if we have correct wave format parameters
		if (
			(m_nSampleSize		== 0) ||
			(m_nAvgBytesPerSec	== 0)
		)
			return E_UNEXPECTED;

		// Calculate the stream and media deltas
		m_rtDelta	= ((REFERENCE_TIME)(*plDataSize) * UNITS) / m_nAvgBytesPerSec;
		m_llDelta	= (LONGLONG)(*plDataSize) / m_nSampleSize;

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

	hr = m_pSample->SetActualDataLength(0);
	if (FAILED(hr)) {
		m_pSample->Release();
		m_pSample = NULL;
		return hr;
	}

	return NOERROR;
}

HRESULT CFSTChunkParser::ParseChunkData(
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

HRESULT CFSTChunkParser::ParseChunkEnd(void)
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
// CFSTSplitterFilter methods
//==========================================================================

IMPLEMENT_FILETYPE(CFSTSplitterFilter)

CFSTSplitterFilter::CFSTSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseParserFilter(
		NAME("FST Splitter Filter"),
		pUnk,
		CLSID_FSTSplitter,
		g_wszFSTSplitterName,
		phr
	),
	m_nFramesPerSecond(0),	// No frame rate at this time
	m_nSampleSize(0),		// No audio sample size at this time
	m_nAvgBytesPerSec(0),	// No audio data rate at this time
	m_nVideoFrames(0),		// No frame number at this time
	m_pFrameTable(NULL),	// No frame table at this time
	m_pParser(NULL)			// No chunk parser at this time
{
	ASSERT(phr);
}

CFSTSplitterFilter::~CFSTSplitterFilter()
{
	// Free any resources allocated by Initialize() or InitializeParser()
	Shutdown();
}

CUnknown* WINAPI CFSTSplitterFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CFSTSplitterFilter(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

HRESULT CFSTSplitterFilter::Initialize(IPin *pPin, IAsyncReader *pReader)
{
	// Check and validate the pointer
	CheckPointer(pReader, E_POINTER);
	ValidateReadPtr(pReader, sizeof(IAsyncReader));

	// Read file header
	FST_HEADER header;
	HRESULT hr = pReader->SyncRead(0, sizeof(header), (BYTE*)&header);
	if (hr != S_OK)
		return hr;

	// Verify file header
	if (header.dwID != FST_ID_2TSF)
		return VFW_E_INVALID_FILE_FORMAT;

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Set video stream info
	m_nVideoFrames		= header.nVideoFrames;
	m_nFramesPerSecond	= header.nFramesPerSecond;

	// Allocate frame table
	m_pFrameTable = (FST_FRAME_ENTRY*)CoTaskMemAlloc(m_nVideoFrames * sizeof(FST_FRAME_ENTRY));
	if (m_pFrameTable == NULL) {
		Shutdown();
		return E_OUTOFMEMORY;
	}

	// Read in the frame table
	hr = pReader->SyncRead(sizeof(header), m_nVideoFrames * sizeof(FST_FRAME_ENTRY), (BYTE*)m_pFrameTable);
	if (hr != S_OK) {
		Shutdown();
		return hr;
	}

	// Walk the frame table and determine the maximum frame data sizes
	DWORD cbMaxImage = 0, cbMaxSound = 0;
	for (DWORD iFrame = 0; iFrame < m_nVideoFrames; iFrame++) {
		if (m_pFrameTable[iFrame].cbImage > cbMaxImage)
			cbMaxImage = m_pFrameTable[iFrame].cbImage;
		if (m_pFrameTable[iFrame].cbSound > cbMaxSound)
			cbMaxSound = m_pFrameTable[iFrame].cbSound;
	}

	// Set file positions
	m_llDefaultStart	= (LONGLONG)sizeof(FST_HEADER) + m_nVideoFrames * sizeof(FST_FRAME_ENTRY);	// Right after the header and frame table
	m_llDefaultStop		= MAXLONGLONG; // Defaults to file end

	// Decide on the input pin properties
	m_cbInputAlign	= 1;
	m_cbInputBuffer	= cbMaxImage + cbMaxSound;

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Decide on the output pins count
	m_nOutputPins = 1;	// Video is always present

	// Check if we have soundtrack
	if (
		(header.dwAudioSampleRate	!= 0) &&
		(header.nAudioBits			!= 0) &&
		(m_pFrameTable[0].cbSound	!= 0)
	)
		m_nOutputPins++;

	// Create output pins array
	ASSERT(m_ppOutputPin == NULL);
	m_ppOutputPin = new CParserOutputPin*[m_nOutputPins];
	if (m_ppOutputPin == NULL) {
		m_nOutputPins = 0;
		Shutdown();
		return E_OUTOFMEMORY;
	}

	// Reset the output pin array elements to NULLs
	for (int i = 0; i < m_nOutputPins; i++)
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
	pmtVideo->SetSubtype(&MEDIASUBTYPE_FSTVideo);
	pmtVideo->SetSampleSize(0);
	pmtVideo->SetTemporalCompression(TRUE);
	pmtVideo->SetFormatType(&FORMAT_FSTVideo);
	if (!pmtVideo->SetFormat((BYTE*)&header, sizeof(header))) {
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
	papVideo->cbAlign	= 0;	// No matter
	papVideo->cbPrefix	= 0;	// No matter
	papVideo->cBuffers	= 4;	// TEMP: No need to set larger value?
	papVideo->cbBuffer	= cbMaxImage;

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
		NAME("FST Splitter Video Output Pin"),
		this,
		&m_csFilter,
		pmtVideo,
		papVideo,
		dwVideoCapabilities,
		nVideoTimeFormats,
		pVideoTimeFormats,
		&hr,
		wszFSTVideoOutputName
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

	if (m_nOutputPins > 1) {

		// Allocate audio media type
		CMediaType *pmtAudio = new CMediaType();
		if (pmtAudio == NULL) {
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Initialize the audio media type
		pmtAudio->InitMediaType();
		pmtAudio->SetType(&MEDIATYPE_Audio);
		pmtAudio->SetSubtype(&MEDIASUBTYPE_PCM);
		pmtAudio->SetSampleSize(header.nAudioBits * (header.wAudioChannels + 1) / 8);
		pmtAudio->SetTemporalCompression(FALSE);
		pmtAudio->SetFormatType(&FORMAT_WaveFormatEx);
		WAVEFORMATEX *pFormat = (WAVEFORMATEX*)pmtAudio->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		if (pFormat == NULL) {
			delete pmtAudio;
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Fill in the audio format block
		pFormat->wFormatTag			= WAVE_FORMAT_PCM;
		pFormat->nChannels			= header.wAudioChannels + 1; // TEMP: Is it really so?
		pFormat->nSamplesPerSec		= header.dwAudioSampleRate;
		pFormat->nAvgBytesPerSec	= pFormat->nChannels * header.nAudioBits * header.dwAudioSampleRate / 8;
		pFormat->nBlockAlign		= pFormat->nChannels * header.nAudioBits / 8;
		pFormat->wBitsPerSample		= header.nAudioBits;
		pFormat->cbSize				= 0;

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
		papAudio->cBuffers	= 4;	// No use to set different from video value
		papAudio->cbBuffer	= cbMaxSound;

		// Set the wave format parameters needed for the calculation
		// of sample stream and media duration
		m_nSampleSize		= pFormat->nBlockAlign;
		m_nAvgBytesPerSec	= pFormat->nAvgBytesPerSec;

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

			dwAudioCapabilities	=	AM_SEEKING_CanGetCurrentPos	|
									AM_SEEKING_CanGetStopPos	|
									AM_SEEKING_CanGetDuration;
		}

		// Create audio output pin
		hr = NOERROR;
		m_ppOutputPin[1] = new CParserOutputPin(
			NAME("FST Splitter Audio Output Pin"),
			this,
			&m_csFilter,
			pmtAudio,
			papAudio,
			dwAudioCapabilities,
			nAudioTimeFormats,
			pAudioTimeFormats,
			&hr,
			wszFSTAudioOutputName
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
	}

	// Scope for the locking
	{
		// Protect media content information
		CAutoLock infolock(&m_csInfo);

		// Set the media content strings
		m_wszAuthorName = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszFSTAuthorName) + 1));
		if (m_wszAuthorName)
			lstrcpyW(m_wszAuthorName, wszFSTAuthorName);
		m_wszDescription = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszFSTDescription) + 1));
		if (m_wszDescription)
			lstrcpyW(m_wszDescription, wszFSTDescription);
	}

	return NOERROR;
}

HRESULT CFSTSplitterFilter::Shutdown(void)
{
	// Shutdown the parser
	HRESULT hr = ShutdownParser();
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
		if (m_pFrameTable) {
			CoTaskMemFree(m_pFrameTable);
			m_pFrameTable = NULL;
		}
	}

	// Call the base-class implementation
	return CBaseParserFilter::Shutdown();
}

HRESULT CFSTSplitterFilter::InitializeParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	HRESULT hr = NOERROR;

	// Create chunk parser if we have to
	if (m_pParser == NULL) {

		m_pParser = new CFSTChunkParser(
			this,
			m_ppOutputPin,
			m_nVideoFrames,
			m_pFrameTable,
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

HRESULT CFSTSplitterFilter::ShutdownParser(void)
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

HRESULT CFSTSplitterFilter::ResetParser(void)
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

HRESULT CFSTSplitterFilter::CheckInputType(const CMediaType* pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	if (
		(IsEqualGUID(*pmt->Type(),		MEDIATYPE_Stream)) &&	// We accept byte stream
		(IsEqualGUID(*pmt->Subtype(),	MEDIASUBTYPE_FST))		// ... from FST file
	)
		return S_OK;
	else
		return S_FALSE;
}

HRESULT CFSTSplitterFilter::GetInputType(int iPosition, CMediaType *pMediaType)
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
		pMediaType->SetSubtype(&MEDIASUBTYPE_FST);	// From FST file
		pMediaType->SetVariableSize();				// Variable size samples
		pMediaType->SetTemporalCompression(FALSE);	// No temporal compression
		pMediaType->SetFormatType(&FORMAT_None);	// Is it needed ???
	}

	return S_OK;
}
	
HRESULT CFSTSplitterFilter::Receive(
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

HRESULT CFSTSplitterFilter::ConvertTimeFormat(
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

	if (!lstrcmpW(pPinName, wszFSTVideoOutputName)) {

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

	} else if (!lstrcmpW(pPinName, wszFSTAudioOutputName)) {

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
			llSourceInSamples = llSource / m_nSampleSize;
		else if (IsEqualGUID(*pSourceFormat, TIME_FORMAT_MEDIA_TIME))
			llSourceInSamples = (llSource * m_nAvgBytesPerSec) / (m_nSampleSize  * UNITS);
		else
			return E_INVALIDARG;

		// Convert the source in samples to target format
		if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_SAMPLE))
			*pTarget = llSourceInSamples;
		else if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_BYTE))
			*pTarget = llSourceInSamples * m_nSampleSize;
		else if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_MEDIA_TIME))
			*pTarget = (llSourceInSamples * m_nSampleSize * UNITS) / m_nAvgBytesPerSec;
		else
			return E_INVALIDARG;

		// At this point the conversion is successfully done
		return NOERROR;
	}

	return E_NOTIMPL;
}

HRESULT CFSTSplitterFilter::GetDuration(
	LPCWSTR pPinName,
	const GUID *pCurrentFormat,
	LONGLONG *pDuration
)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Convert the duration in frames to the current time format
	return ConvertTimeFormat(
		wszFSTVideoOutputName,
		pDuration,
		pCurrentFormat,
		(LONGLONG)m_nVideoFrames,
		&TIME_FORMAT_FRAME
	);
}

STDMETHODIMP CFSTSplitterFilter::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_FSTSplitterPage;
	pPages->pElems[1] = CLSID_BaseParserPage;

	return NOERROR;
}

//==========================================================================
// CFSTSplitterPage methods
//==========================================================================

const WCHAR g_wszFSTSplitterPageName[] = L"ANX FST Splitter Property Page";

CFSTSplitterPage::CFSTSplitterPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("FST Splitter Property Page"),
		pUnk,
		IDD_FSTSPLITTERPAGE,
		IDS_TITLE_FSTSPLITTERPAGE
	)
{
}

CUnknown* WINAPI CFSTSplitterPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CFSTSplitterPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
