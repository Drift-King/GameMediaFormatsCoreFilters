//==========================================================================
//
// File: CINSplitter.cpp
//
// Desc: Game Media Formats - Implementation of CIN splitter filter
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

#include "CINSpecs.h"
#include "CINGUID.h"
#include "CINSplitter.h"
#include "resource.h"

//==========================================================================
// CIN splitter setup data
//==========================================================================

// Media content strings
const OLECHAR wszCINAuthorName[]	= L"Id Software";
const OLECHAR wszCINDescription[]	= L"Id Software CIN video file";

// Global filter name
const WCHAR g_wszCINSplitterName[]	= L"ANX CIN Splitter";

// Output pin names
const WCHAR wszCINVideoOutputName[]	= L"Video Output";
const WCHAR wszCINAudioOutputName[]	= L"Audio Output";

const AMOVIESETUP_MEDIATYPE sudCINStreamType = {
	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_CIN
};

const AMOVIESETUP_MEDIATYPE sudCINVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_CINVideo
};

const AMOVIESETUP_MEDIATYPE sudCINAudioType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

const AMOVIESETUP_PIN sudCINSplitterPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudCINStreamType	// Media types
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
		&sudCINVideoType	// Media types
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
		&sudCINAudioType	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudCINSplitter = {
	&CLSID_CINSplitter,		// CLSID of filter
	g_wszCINSplitterName,	// Filter name
	MERIT_NORMAL,			// Filter merit
	3,						// Number of pins
	sudCINSplitterPins		// Pin information
};

//==========================================================================
// CIN splitter file type setup data
//==========================================================================

const TCHAR *sudCINExtensions[] = {
	TEXT(".cin")
};

const GMF_FILETYPE_REGINFO CCINSplitterFilter::g_pRegInfo[] = {
	{
		TEXT("Id Software CIN Video Format"),	// Type name
		&MEDIATYPE_Stream,						// Media type
		&MEDIASUBTYPE_CIN,						// Media subtype
		&CLSID_AsyncReader,						// Source filter
		0,										// Number of entries
		NULL,									// Entries
		1,										// Number of extensions
		sudCINExtensions						// Extensions
	}
};

const int CCINSplitterFilter::g_nRegInfo = 1;

//==========================================================================
// CCINChunkParser methods
//==========================================================================

CCINChunkParser::CCINChunkParser(
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	WORD nSampleSize,
	DWORD nAvgBytesPerSec,
	HRESULT *phr
) :
	CBaseChunkParser(
		NAME("CIN Chunk Parser"),
		pFilter,
		ppOutputPin,
		4,
		1,
		phr
	),
	m_pPin(NULL),
	m_pSample(NULL),
	m_rtDelta(0),
	m_llDelta(0),
	m_ChunkType(CCT_COMMAND),
	m_bEndOfFile(FALSE),
	m_nSampleSize(nSampleSize),
	m_nAvgBytesPerSec(nAvgBytesPerSec)
{
	// Set up correction flags
	m_bUseCorrection = nAvgBytesPerSec % CIN_FPS;
	m_bDoCorrection	= FALSE;
}

CCINChunkParser::~CCINChunkParser()
{
	// Free any resources allocated by the parser
	ResetParser();
}

// Since we don't expect any seek operation, we reset the parser 
// so that it's ready to parse data from the file beginning
HRESULT CCINChunkParser::ResetParser(void)
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

	// Reset the type and the flags
	m_ChunkType		= CCT_COMMAND;
	m_bEndOfFile	= FALSE;
	m_bDoCorrection	= FALSE;

	// Call base-class method to reset parser state
	return CBaseChunkParser::ResetParser();
}

HRESULT CCINChunkParser::ParseChunkHeader(
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

	// If we've encountered end-of-file chunk previously, no need to proceed
	if (m_bEndOfFile) {
		*plDataSize = 0;
		return NOERROR;
	}

	LONG *plHeader = (LONG*)pbHeader;

	m_rtDelta = 0;
	m_llDelta = 0;
	
	switch (m_ChunkType) {

		// Optional palette
		case CCT_COMMAND:

			*plDataSize = (*plHeader == CIN_COMMAND_PALETTE) ? 0x300 : 0;
			m_bEndOfFile = *plHeader == CIN_COMMAND_EOF;
			m_pPin = m_ppOutputPin[0];	// We'll be dealing with video output pin

			break;

		// Video data
		case CCT_VIDEO:

			*plDataSize = *plHeader;
			ASSERT(m_pPin == m_ppOutputPin[0]);	// It should already be video output pin
			m_rtDelta = UNITS / CIN_FPS;		// 1/14 of second per frame
			m_llDelta = 1;						// One frame per sample

			break;

		// Audio data
		case CCT_AUDIO:

			*plDataSize = (m_nAvgBytesPerSec / CIN_FPS) - sizeof(LONG);
			// Correct the chunk size if needed
			if (m_bUseCorrection) {
				if (m_bDoCorrection)
					(*plDataSize)++;
				m_bDoCorrection = !m_bDoCorrection;
			}

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

			break;

		default:

			// Is it possible?
			break;
	}

	// If it's the end-of-file chunk, no need to proceed
	if (m_bEndOfFile) {
		m_pPin = NULL;
		m_pSample = NULL;
		return NOERROR;
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

	if (m_ChunkType != CCT_VIDEO) {

		// Get an empty sample from the output pin
		HRESULT hr = m_pPin->GetDeliveryBuffer(&m_pSample, NULL, NULL, 0);
		if (FAILED(hr)) {
			m_pSample = NULL;
			return hr;
		}

		// If it's a command or audio chunk copy the header data to the sample
		BYTE *pbBuffer = NULL;
		hr = m_pSample->GetPointer(&pbBuffer);
		if (FAILED(hr)) {
			m_pSample->Release();
			m_pSample = NULL;
			return hr;
		}
		CopyMemory(pbBuffer, pbHeader, sizeof(LONG));
		hr = m_pSample->SetActualDataLength(sizeof(LONG));
		if (FAILED(hr)) {
			m_pSample->Release();
			m_pSample = NULL;
			return hr;
		}
	}

	return NOERROR;
}

HRESULT CCINChunkParser::ParseChunkData(
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

	// If we've encountered end-of-file chunk previously, no need to proceed
	if (m_bEndOfFile)
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

HRESULT CCINChunkParser::ParseChunkEnd(void)
{
	// If we've encountered end-of-file chunk previously, no need to proceed
	if (m_bEndOfFile)
		return NOERROR;

	ChunkType oldChunkType = m_ChunkType;

	// Update the chunk type
	switch (m_ChunkType) {

		// Video chunk follows command chunk
		case CCT_COMMAND:
			m_ChunkType = CCT_VIDEO;
			break;

		// Audio chunk follows video chunk
		case CCT_VIDEO:
			m_ChunkType = CCT_AUDIO;
			break;

		// Command chunk follows audio chunk
		case CCT_AUDIO:
			m_ChunkType = CCT_COMMAND;
			break;

		default:

			// Is it possible?
			break;
	}

	if (
		(oldChunkType	!= CCT_COMMAND) &&
		(m_pSample		!= NULL)
	) {

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
	}

	return NOERROR;
}

//==========================================================================
// CCINSplitterFilter methods
//==========================================================================

IMPLEMENT_FILETYPE(CCINSplitterFilter)

CCINSplitterFilter::CCINSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseParserFilter(
		NAME("CIN Splitter Filter"),
		pUnk,
		CLSID_CINSplitter,
		g_wszCINSplitterName,
		phr
	),
	m_nSampleSize(0),		// No audio sample size at this time
	m_nAvgBytesPerSec(0),	// No audio data rate at this time
	m_pParser(NULL)			// No chunk parser at this time
{
	ASSERT(phr);
}

CCINSplitterFilter::~CCINSplitterFilter()
{
	// Free any resources allocated by Initialize() or InitializeParser()
	Shutdown();
}

CUnknown* WINAPI CCINSplitterFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CCINSplitterFilter(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

HRESULT CCINSplitterFilter::Initialize(IPin *pPin, IAsyncReader *pReader)
{
	// Check and validate the pointer
	CheckPointer(pReader, E_POINTER);
	ValidateReadPtr(pReader, sizeof(IAsyncReader));

	// Read file header
	CIN_HEADER header;
	HRESULT hr = pReader->SyncRead(0, sizeof(header), (BYTE*)&header);
	if (hr != S_OK)
		return hr;

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Set file positions
	m_llDefaultStart	= (LONGLONG)sizeof(CIN_HEADER);	// Right after the header
	m_llDefaultStop		= MAXLONGLONG; // Defaults to file end

	// ---- Calculate the max sizes for various buffers ----

	// Video data: worst case compression, 
	// command code, palette and decode count
	LONG cbMaxVideo = header.dwVideoWidth * header.dwVideoHeight * 2 + 4 + 1024;

	// Audio data: just raw PCM size for 1/14 of second audio.
	// Additionally, provide some extra space for size correction
	LONG cbMaxAudio = (header.dwAudioSampleWidth * header.nAudioChannels * header.dwAudioSampleRate) / CIN_FPS 
						+ (header.dwAudioSampleWidth * header.nAudioChannels * header.dwAudioSampleRate) % CIN_FPS;

	// Decide on the input pin properties
	m_cbInputAlign	= 1;
	m_cbInputBuffer	= cbMaxVideo + cbMaxAudio + 4; // Additionally includes Huffman count

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Decide on the output pins count
	m_nOutputPins = 2;	// Both video and audio are always here

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
	pmtVideo->SetSubtype(&MEDIASUBTYPE_CINVideo);
	pmtVideo->SetSampleSize(0);
	pmtVideo->SetTemporalCompression(FALSE);
	pmtVideo->SetFormatType(&FORMAT_CINVideo);
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
	papVideo->cbBuffer	= cbMaxVideo;

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
								AM_SEEKING_CanGetStopPos;
	}

	// Create video output pin (always the first one!)
	hr = NOERROR;
	m_ppOutputPin[0] = new CParserOutputPin(
		NAME("CIN Splitter Video Output Pin"),
		this,
		&m_csFilter,
		pmtVideo,
		papVideo,
		dwVideoCapabilities,
		nVideoTimeFormats,
		pVideoTimeFormats,
		&hr,
		wszCINVideoOutputName
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
	pmtAudio->SetSampleSize(header.dwAudioSampleWidth * header.nAudioChannels);
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
	pFormat->nChannels			= (WORD)header.nAudioChannels;
	pFormat->nSamplesPerSec		= header.dwAudioSampleRate;
	pFormat->nAvgBytesPerSec	= header.nAudioChannels * header.dwAudioSampleWidth * header.dwAudioSampleRate;
	pFormat->nBlockAlign		= (WORD)(header.nAudioChannels * header.dwAudioSampleWidth);
	pFormat->wBitsPerSample		= (WORD)(header.dwAudioSampleWidth * 8);
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
	papAudio->cbBuffer	= cbMaxAudio;

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
								AM_SEEKING_CanGetStopPos;
	}

	// Create audio output pin
	hr = NOERROR;
	m_ppOutputPin[1] = new CParserOutputPin(
		NAME("CIN Splitter Audio Output Pin"),
		this,
		&m_csFilter,
		pmtAudio,
		papAudio,
		dwAudioCapabilities,
		nAudioTimeFormats,
		pAudioTimeFormats,
		&hr,
		wszCINAudioOutputName
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

	// Scope for the locking
	{
		// Protect media content information
		CAutoLock infolock(&m_csInfo);

		// Set the media content strings
		m_wszAuthorName = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszCINAuthorName) + 1));
		if (m_wszAuthorName)
			lstrcpyW(m_wszAuthorName, wszCINAuthorName);
		m_wszDescription = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszCINDescription) + 1));
		if (m_wszDescription)
			lstrcpyW(m_wszDescription, wszCINDescription);
	}

	return NOERROR;
}

HRESULT CCINSplitterFilter::Shutdown(void)
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
		m_nSampleSize		= 0;
		m_nAvgBytesPerSec	= 0;
	}

	// Call the base-class implementation
	return CBaseParserFilter::Shutdown();
}

HRESULT CCINSplitterFilter::InitializeParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	HRESULT hr = NOERROR;

	// Create chunk parser if we have to
	if (m_pParser == NULL) {

		m_pParser = new CCINChunkParser(
			this,
			m_ppOutputPin,
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

HRESULT CCINSplitterFilter::ShutdownParser(void)
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

HRESULT CCINSplitterFilter::ResetParser(void)
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

HRESULT CCINSplitterFilter::CheckInputType(const CMediaType* pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	if (
		(IsEqualGUID(*pmt->Type(),		MEDIATYPE_Stream)) &&	// We accept byte stream
		(IsEqualGUID(*pmt->Subtype(),	MEDIASUBTYPE_CIN))		// ... from CIN file
	)
		return S_OK;
	else
		return S_FALSE;
}

HRESULT CCINSplitterFilter::GetInputType(int iPosition, CMediaType *pMediaType)
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
		pMediaType->SetSubtype(&MEDIASUBTYPE_CIN);	// From CIN file
		pMediaType->SetVariableSize();				// Variable size samples
		pMediaType->SetTemporalCompression(FALSE);	// No temporal compression
		pMediaType->SetFormatType(&FORMAT_None);	// Is it needed ???
	}

	return S_OK;
}
	
HRESULT CCINSplitterFilter::Receive(
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

HRESULT CCINSplitterFilter::ConvertTimeFormat(
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

	if (!lstrcmpW(pPinName, wszCINVideoOutputName)) {

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
			*pTarget = (llSource * UNITS) / CIN_FPS;
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
			*pTarget = (llSource * CIN_FPS) / UNITS;
			return NOERROR;
		}

		// If the source/target formats pair does not match one of the 
		// above, we cannot perform convertion
		return E_INVALIDARG;

	} else if (!lstrcmpW(pPinName, wszCINAudioOutputName)) {

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

STDMETHODIMP CCINSplitterFilter::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_CINSplitterPage;
	pPages->pElems[1] = CLSID_BaseParserPage;

	return NOERROR;
}

//==========================================================================
// CCINSplitterPage methods
//==========================================================================

const WCHAR g_wszCINSplitterPageName[] = L"ANX CIN Splitter Property Page";

CCINSplitterPage::CCINSplitterPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("CIN Splitter Property Page"),
		pUnk,
		IDD_CINSPLITTERPAGE,
		IDS_TITLE_CINSPLITTERPAGE
	)
{
}

CUnknown* WINAPI CCINSplitterPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CCINSplitterPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
