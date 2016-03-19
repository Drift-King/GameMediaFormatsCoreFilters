//==========================================================================
//
// File: ROQSplitter.cpp
//
// Desc: Game Media Formats - Implementation of ROQ splitter filter
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

#include "ROQSpecs.h"
#include "ROQGUID.h"
#include "ROQSplitter.h"
#include "FilterOptions.h"
#include "resource.h"

#include <string.h>

//==========================================================================
// ROQ splitter setup data
//==========================================================================

// Media content strings
const OLECHAR wszROQAuthorName[]	= L"Id Software";
const OLECHAR wszROQDescription[]	= L"Id Software ROQ video file";

// Global filter name
const WCHAR g_wszROQSplitterName[]	= L"ANX ROQ Splitter";

// Output pin names
const WCHAR wszROQVideoOutputName[]	= L"Video Output";
const WCHAR wszROQAudioOutputName[]	= L"Audio Output";

const AMOVIESETUP_MEDIATYPE sudROQStreamType = {
	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_ROQ
};

const AMOVIESETUP_MEDIATYPE sudROQVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_ROQVideo
};

const AMOVIESETUP_MEDIATYPE sudROQAudioType = {
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_ROQADPCM
};

const AMOVIESETUP_PIN sudROQSplitterPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudROQStreamType	// Media types
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
		&sudROQVideoType	// Media types
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
		&sudROQAudioType	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudROQSplitter = {
	&CLSID_ROQSplitter,		// CLSID of filter
	g_wszROQSplitterName,	// Filter name
	MERIT_NORMAL,			// Filter merit
	3,						// Number of pins
	sudROQSplitterPins		// Pin information
};

//==========================================================================
// ROQ splitter file type setup data
//==========================================================================

const GMF_FILETYPE_PATTERN sudROQPatterns[] = {
	{ 0,	2,	ROQ_IDSTR_ID		},
	{ 2,	4,	ROQ_IDSTR_SIZE		}
};

const GMF_FILETYPE_ENTRY sudROQEntries[] = {
	{ 2, sudROQPatterns } 
};

const TCHAR *sudROQExtensions[] = {
	TEXT(".roq")
};

const GMF_FILETYPE_REGINFO CROQSplitterFilter::g_pRegInfo[] = {
	{
		TEXT("Id Software ROQ Video Format"),	// Type name
		&MEDIATYPE_Stream,						// Media type
		&MEDIASUBTYPE_ROQ,						// Media subtype
		&CLSID_AsyncReader,						// Source filter
		1,										// Number of entries
		sudROQEntries,							// Entries
		1,										// Number of extensions
		sudROQExtensions						// Extensions
	}
};

const int CROQSplitterFilter::g_nRegInfo = 1;

//==========================================================================
// CROQChunkParser methods
//==========================================================================

CROQChunkParser::CROQChunkParser(
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	WORD nFramesPerSecond,
	WORD nSampleSize,
	DWORD nAvgBytesPerSec,
	HRESULT *phr
) :
	CBaseChunkParser(
		NAME("ROQ Chunk Parser"),
		pFilter,
		ppOutputPin,
		sizeof(ROQ_CHUNK_HEADER),
		1,
		phr
	),
	m_pPin(NULL),
	m_pSample(NULL),
	m_rtDelta(0),
	m_llDelta(0),
	m_nFramesPerSecond(nFramesPerSecond),
	m_nSampleSize(nSampleSize),
	m_nAvgBytesPerSec(nAvgBytesPerSec)
{
}

CROQChunkParser::~CROQChunkParser()
{
	// Free any resources allocated by the parser
	ResetParser();
}

// Since we don't expect any seek operation, we reset the parser 
// so that it's ready to parse data from the file beginning
HRESULT CROQChunkParser::ResetParser(void)
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

HRESULT CROQChunkParser::ParseChunkHeader(
	LONGLONG llStartPosition,
	BYTE *pbHeader,
	LONG *plDataSize
)
{
	// Check and validate pointers
	CheckPointer(pbHeader, E_POINTER);
	ValidateReadPtr(pbHeader, sizeof(ROQ_CHUNK_HEADER));
	CheckPointer(plDataSize, E_POINTER);
	ValidateWritePtr(plDataSize, sizeof(LONG));

	ROQ_CHUNK_HEADER *pChunkHeader = (ROQ_CHUNK_HEADER*)pbHeader;
	
	// Set the data size
	*plDataSize = pChunkHeader->cbSize;

	LONG lUnpackedDataLength = 0;

	// Recognize chunk type
	switch (pChunkHeader->wID) {

		// Video info
		case ROQ_CHUNK_VIDEO_INFO:

			m_pPin = NULL; // Just skip this chunk

			break;

		// Video codebook
		case ROQ_CHUNK_VIDEO_CODEBOOK:

			m_pPin = m_ppOutputPin[0];	// We'll be dealing with video output pin
			m_rtDelta = 0;				// Codebook has no duration
			m_llDelta = 0;				// Codebook has no duration

			break;

		// Video frame data
		case ROQ_CHUNK_VIDEO_FRAME:

			m_pPin = m_ppOutputPin[0];	// We'll be dealing with video output pin

			// Check if we have correct video format parameters
			if (m_nFramesPerSecond == 0)
				return E_UNEXPECTED;

			m_rtDelta = UNITS / m_nFramesPerSecond;
			m_llDelta = 1;

			break;

		// Audio data
		case ROQ_CHUNK_SOUND_MONO:
		case ROQ_CHUNK_SOUND_STEREO:
			
			ASSERT(m_ppOutputPin[1]);	// There should be the audio output pin
			m_pPin = m_ppOutputPin[1];	// We'll be dealing with audio output pin

			// Check if we have correct wave format parameters
			if (
				(m_nSampleSize		== 0) ||
				(m_nAvgBytesPerSec	== 0)
			)
				return E_UNEXPECTED;

			// Calculate the stream and media deltas
			lUnpackedDataLength = (*plDataSize) * 2;
			m_rtDelta	= ((REFERENCE_TIME)lUnpackedDataLength * UNITS) / m_nAvgBytesPerSec;
			m_llDelta	= (LONGLONG)lUnpackedDataLength / m_nSampleSize;

			break;

		// Is it possible?
		default:

			m_pPin = NULL; // No output pin
			
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

	// Get the sample's buffer
	BYTE *pbBuffer = NULL;
	hr = m_pSample->GetPointer(&pbBuffer);
	if (FAILED(hr)) {
		m_pSample->Release();
		m_pSample = NULL;
		return hr;
	}

	// Prepare the sample
	switch (pChunkHeader->wID) {

		// Video codebook and frame data
		case ROQ_CHUNK_VIDEO_CODEBOOK:
		case ROQ_CHUNK_VIDEO_FRAME:

			// Store the chunk header in the buffer
			CopyMemory(
				pbBuffer,
				pbHeader,
				sizeof(ROQ_CHUNK_HEADER)
			);

			// Set the actual data length
			hr = m_pSample->SetActualDataLength(sizeof(ROQ_CHUNK_HEADER));
			if (FAILED(hr)) {
				m_pSample->Release();
				m_pSample = NULL;
				return hr;
			}

			break;

		// Audio data
		case ROQ_CHUNK_SOUND_MONO:
		case ROQ_CHUNK_SOUND_STEREO:

			// Store the chunk argument in the buffer
			*((WORD*)pbBuffer) = pChunkHeader->wArgument;

			// Set the actual data length
			hr = m_pSample->SetActualDataLength(sizeof(WORD));
			if (FAILED(hr)) {
				m_pSample->Release();
				m_pSample = NULL;
				return hr;
			}
			
			break;

		default:

			// Is it possible?
			break;
	}

	return NOERROR;
}

HRESULT CROQChunkParser::ParseChunkData(
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

HRESULT CROQChunkParser::ParseChunkEnd(void)
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
// CROQSplitterFilter methods
//==========================================================================

IMPLEMENT_FILETYPE(CROQSplitterFilter)

CROQSplitterFilter::CROQSplitterFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseParserFilter(
		NAME("ROQ Splitter Filter"),
		pUnk,
		CLSID_ROQSplitter,
		g_wszROQSplitterName,
		phr
	),
	m_nSampleSize(0),			// No audio sample size at this time
	m_nAvgBytesPerSec(0),		// No audio data rate at this time
	m_pMP3Source(NULL),			// |
	m_pMPEGSplitter(NULL),		// |
	m_pMP3Decoder(NULL),		// |-- No MP3 stuff at this time
	m_pPassThruLock(NULL),		// |
	m_pPassThruInput(NULL),		// |
	m_pPassThruOutput(NULL),	// |
	m_pParser(NULL)				// No chunk parser at this time
{
	ASSERT(phr);

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Retrieve the options from the registry
		m_bUseExternalMP3 = TRUE; // Default value
		HRESULT hr = RegGetFilterOptionDWORD(
			m_wszFilterName,
			TEXT("Use External MP3"),
			(DWORD*)&m_bUseExternalMP3
		);
		if (FAILED(hr))
			*phr = hr;
		DWORD dwType = 0, cbSize = sizeof(m_szExternalMP3Path);
		hr = RegGetFilterOptionValue(
			m_wszFilterName,
			TEXT("External MP3 Path"),
			&dwType,
			(LPBYTE)m_szExternalMP3Path,
			&cbSize
		);
		if (FAILED(hr)) {
			lstrcpyW(m_szExternalMP3Path, L"."); // Default value
			hr = RegSetFilterOptionValue(
				m_wszFilterName,
				TEXT("External MP3 Path"),
				REG_BINARY,
				(LPBYTE)m_szExternalMP3Path,
				(lstrlenW(m_szExternalMP3Path) + 1) * sizeof(OLECHAR)
			);
			if (FAILED(hr))
				*phr = hr;
		} else if (dwType != REG_BINARY)
			*phr = E_UNEXPECTED;
	}
}

CROQSplitterFilter::~CROQSplitterFilter()
{
	// Free any resources allocated by Initialize() or InitializeParser()
	Shutdown();

	{
		// Protect the filter options
		CAutoLock optionlock(&m_csOptions);

		// Store the options in the registry
		RegSetFilterOptionValue(
			m_wszFilterName,
			TEXT("Use External MP3"),
			REG_DWORD,
			(LPBYTE)&m_bUseExternalMP3,
			sizeof(BOOL)
		);
		RegSetFilterOptionValue(
			m_wszFilterName,
			TEXT("External MP3 Path"),
			REG_BINARY,
			(LPBYTE)m_szExternalMP3Path,
			(lstrlenW(m_szExternalMP3Path) + 1) * sizeof(OLECHAR)
		);
	}
}

CUnknown* WINAPI CROQSplitterFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CROQSplitterFilter(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

STDMETHODIMP CROQSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	// Check and validate the pointer
	CheckPointer(ppv, E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));

	// Expose IConfigROQSplitter (and the base-class interfaces)
	if (riid == IID_IConfigROQSplitter)
		return GetInterface((IConfigROQSplitter*)this, ppv);
	else
		return CBaseParserFilter::NonDelegatingQueryInterface(riid, ppv);
}

int CROQSplitterFilter::GetPinCount(void)
{
	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	return 1								// Input pin
		+ m_nOutputPins						// Output pins
		+ ((m_pPassThruInput) ? 1 : 0)		// Pass-through input pin
		+ ((m_pPassThruOutput) ? 1 : 0);	// Pass-through output pin
}

CBasePin* CROQSplitterFilter::GetPin(int n)
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

IPin* CROQSplitterFilter::GetPinWithDirection(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
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

HRESULT CROQSplitterFilter::Initialize(IPin *pPin, IAsyncReader *pReader)
{
	// Check and validate the pointer
	CheckPointer(pReader, E_POINTER);
	ValidateReadPtr(pReader, sizeof(IAsyncReader));

	// Read file header
	ROQ_CHUNK_HEADER header;
	HRESULT hr = pReader->SyncRead(0, sizeof(header), (BYTE*)&header);
	if (hr != S_OK)
		return hr;

	// Verify file header
	if (
		(header.wID			!= ROQ_ID_ID)	||
		(header.cbSize		!= ROQ_ID_SIZE)	||
		(header.wArgument	== 0)
	)
		return VFW_E_INVALID_FILE_FORMAT;

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Set file positions
	m_llDefaultStart	= (LONGLONG)sizeof(ROQ_CHUNK_HEADER);	// Right after the header
	m_llDefaultStop		= MAXLONGLONG; // Defaults to file end

	// Decide on the input pin properties
	m_cbInputAlign	= 1;
	m_cbInputBuffer	= 0x40000; // TEMP: Some reasonable value

	// Set the frame rate
	m_nFramesPerSecond = header.wArgument;

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Decide on the output pins count
	m_nOutputPins = 1;		// Video always should be present in the file

	// Scan the chunks for audio and video info
	LONGLONG llSeekPos = m_llDefaultStart;
	ROQ_VIDEO_INFO videoinfo = {0};
	WORD nChannels = 0;
	DWORD cbMaxAudioChunk = 0;
	for (int i = 0;  i < ROQ_CHUNKS_TO_SCAN; i++) {

		// Read the chunk header
		hr = pReader->SyncRead(llSeekPos, sizeof(header), (BYTE*)&header);
		if (hr != S_OK)
			break;

		// Update the seek position
		llSeekPos += sizeof(header);

		// Check the chunk type
		if (header.wID == ROQ_CHUNK_VIDEO_INFO) {

			// Check the info chunk size
			if (header.cbSize < sizeof(videoinfo)) {
				Shutdown();
				return VFW_E_INVALID_FILE_FORMAT;
			}

			// Read the video info data
			hr = pReader->SyncRead(llSeekPos, sizeof(videoinfo), (BYTE*)&videoinfo);
			if (hr != S_OK) {
				Shutdown();
				return hr;
			}

		} else if (header.wID == ROQ_CHUNK_SOUND_MONO) {

			nChannels = 1;
			cbMaxAudioChunk = header.cbSize;

		} else if (header.wID == ROQ_CHUNK_SOUND_STEREO) {

			nChannels = 2;
			cbMaxAudioChunk = header.cbSize;

		}

		// If we've got all necessary information, quit
		if (
			(nChannels						!= 0) &&
			(cbMaxAudioChunk				!= 0) &&
			(videoinfo.wWidth				!= 0) &&
			(videoinfo.wHeight				!= 0) &&
			(videoinfo.wBlockDimension		!= 0) &&
			(videoinfo.wSubBlockDimension	!= 0)
		)
			break;

		// Advance the seek position
		llSeekPos += header.cbSize;
	}

	// Check if we have correct video info
	if (
		(videoinfo.wWidth				== 0) ||
		(videoinfo.wHeight				== 0) ||
		(videoinfo.wBlockDimension		== 0) ||
		(videoinfo.wSubBlockDimension	== 0)
	) {
		Shutdown();
		return VFW_E_INVALID_FILE_FORMAT;
	}

	// Check if we have soundtrack
	if (nChannels != 0)
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
	int i = 0;
	for (i = 0; i < m_nOutputPins; i++)
		m_ppOutputPin[i] = NULL;

	// Allocate video media type
	CMediaType *pmtVideo = new CMediaType();
	if (pmtVideo == NULL) {
		Shutdown();
		return E_OUTOFMEMORY;
	}

	// Initialize video format structure
	ROQ_VIDEO_FORMAT videoformat = {0};
	videoformat.nFramesPerSecond	= m_nFramesPerSecond;
	videoformat.wWidth				= videoinfo.wWidth;
	videoformat.wHeight				= videoinfo.wHeight;
	videoformat.wBlockDimension		= videoinfo.wBlockDimension;
	videoformat.wSubBlockDimension	= videoinfo.wSubBlockDimension;

	// Initialize the video media type
	pmtVideo->InitMediaType();
	pmtVideo->SetType(&MEDIATYPE_Video);
	pmtVideo->SetSubtype(&MEDIASUBTYPE_ROQVideo);
	pmtVideo->SetSampleSize(0);				// Variable size samples
	pmtVideo->SetTemporalCompression(TRUE);	// Using temporal compression
	pmtVideo->SetFormatType(&FORMAT_ROQVideo);
	if (!pmtVideo->SetFormat((BYTE*)&videoformat, sizeof(videoformat))) {
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

	// Prepare pixel block dimensions
	WORD wHalfSubBlockSize	= (videoinfo.wSubBlockDimension / 2) * (videoinfo.wSubBlockDimension / 2);
	WORD wHalfSubBlockCell	= (wHalfSubBlockSize * 3) / 2;
	WORD wSubBlockCell		= 4;
	DWORD nBlocks			= (videoinfo.wWidth * videoinfo.wHeight) / (videoinfo.wBlockDimension * videoinfo.wBlockDimension);
	DWORD dwMaxFrameSize	= (nBlocks / 8) * (2 + 8 * (2 + 4 * 4));

	// Set the video allocator properties
	papVideo->cbAlign	= 0;		// No matter
	papVideo->cbPrefix	= 0;		// No matter
	papVideo->cBuffers	= 4;		// TEMP: No need to set larger value?
	papVideo->cbBuffer	= max(		// Set this to the max of the codebook and frame sizes
		sizeof(ROQ_CHUNK_HEADER) +	//          | Codebook header
		256 * wHalfSubBlockCell +	// Codebook | Codebook for half-sub-block
		256 * wSubBlockCell,		//          | Codebook for sub-block
		sizeof(ROQ_CHUNK_HEADER) +	//          + Frame header
		dwMaxFrameSize				// Frame    + Maximal frame size
	);

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
		NAME("ROQ Splitter Video Output Pin"),
		this,
		&m_csFilter,
		pmtVideo,
		papVideo,
		dwVideoCapabilities,
		nVideoTimeFormats,
		pVideoTimeFormats,
		&hr,
		wszROQVideoOutputName
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
		m_wszAuthorName = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszROQAuthorName) + 1));
		if (m_wszAuthorName)
			lstrcpyW(m_wszAuthorName, wszROQAuthorName);
		m_wszDescription = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszROQDescription) + 1));
		if (m_wszDescription)
			lstrcpyW(m_wszDescription, wszROQDescription);
	}

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	// Create an audio output pin if we have to
	if (nChannels != 0) {

		// Allocate audio media type
		CMediaType *pmtAudio = new CMediaType();
		if (pmtAudio == NULL) {
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Initialize the audio media type
		pmtAudio->InitMediaType();
		pmtAudio->SetType(&MEDIATYPE_Audio);
		pmtAudio->SetSubtype(&MEDIASUBTYPE_ROQADPCM);
		pmtAudio->SetSampleSize(0);				// Variable size samples
		pmtAudio->SetTemporalCompression(TRUE);	// Using temporal compression
		pmtAudio->SetFormatType(&FORMAT_ROQADPCM);
		ROQADPCMWAVEFORMAT *pFormat = (ROQADPCMWAVEFORMAT*)pmtAudio->AllocFormatBuffer(sizeof(ROQADPCMWAVEFORMAT));
		if (pFormat == NULL) {
			delete pmtAudio;
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Fill in the audio format block
		pFormat->nChannels		= nChannels;
		pFormat->nSamplesPerSec	= ROQ_SAMPLE_RATE;
		pFormat->wBitsPerSample	= ROQ_SAMPLE_BITS;

		// Allocate the audio allocator properties
		ALLOCATOR_PROPERTIES *papAudio = (ALLOCATOR_PROPERTIES*)CoTaskMemAlloc(sizeof(ALLOCATOR_PROPERTIES));
		if (papAudio == NULL) {
			delete pmtAudio;
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Set the wave format parameters needed for the calculation
		// of sample stream and media duration
		m_nSampleSize		= (pFormat->wBitsPerSample / 8) * pFormat->nChannels;
		m_nAvgBytesPerSec	= m_nSampleSize * pFormat->nSamplesPerSec;

		// Set the audio allocator properties
		papAudio->cbAlign	= 0;	// No matter
		papAudio->cbPrefix	= 0;	// No matter
		papAudio->cBuffers	= 8;	// The first audio chunk contains sound for 8 frames
		papAudio->cbBuffer	= 2 + cbMaxAudioChunk; // TEMP: we assume the first chunk is the largest one

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
									AM_SEEKING_CanGetStopPos;
		}

		// Create audio output pin
		hr = NOERROR;
		m_ppOutputPin[1] = new CParserOutputPin(
			NAME("ROQ Splitter Audio Output Pin"),
			this,
			&m_csFilter,
			pmtAudio,
			papAudio,
			dwAudioCapabilities,
			nAudioTimeFormats,
			pAudioTimeFormats,
			&hr,
			wszROQAudioOutputName
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

	} else if (m_bUseExternalMP3) { // !nChannels

		// Work out the filename of the MP3 file from the filename
		// of the ROQ file and throw in the source filter for MP3 
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

		// Get the CLSID of the ROQ source filter
		CLSID clsidSource;
		hr = pininfo.pFilter->GetClassID(&clsidSource);
		if (FAILED(hr)) {
			pininfo.pFilter->Release();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get the file source interface from the filter
		IFileSourceFilter *pROQSource = NULL;
		hr = pininfo.pFilter->QueryInterface(IID_IFileSourceFilter, (void**)&pROQSource);
		pininfo.pFilter->Release();
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;

		// Get the ROQ filename from the filter
		OLECHAR *pszTemp;
		AM_MEDIA_TYPE mt;
		hr = pROQSource->GetCurFile(&pszTemp, &mt);
		pROQSource->Release();
		FreeMediaType(mt); // Is that really needed???
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;
		OLECHAR szFileName[MAX_PATH];
		lstrcpyW(szFileName, pszTemp);

		// Get the file title from the ROQ filename
		OLECHAR *pTitleStart = wcsrchr(szFileName, L'\\');
		if (pTitleStart == NULL)
			pTitleStart = szFileName;
		else
			pTitleStart++;

		// Copy file title to the temp storage
		OLECHAR szFileTitle[MAX_PATH];
		lstrcpyW(szFileTitle, pTitleStart);

		// Append MP3 (relative) directory to the directory path
		lstrcpyW(pTitleStart, m_szExternalMP3Path);
		if (szFileName[lstrlenW(szFileName) - 1] != L'\\')
			lstrcpyW(szFileName + lstrlenW(szFileName), L"\\");

		// Replace the extension in the file title
		OLECHAR *pExt = wcsrchr(szFileTitle, L'.');
		if (pExt == NULL)
			pExt = szFileTitle + lstrlenW(szFileTitle);
		lstrcpyW(pExt, L".mp3");

		// Compose the MP3 filename
		lstrcpyW(szFileName + lstrlenW(szFileName), szFileTitle);

		// Create async reader object
		IFileSourceFilter *pMP3Source = NULL;
		hr = CoCreateInstance(
			clsidSource,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IFileSourceFilter,
			(void**)&pMP3Source
		);
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;

		// Load MP3 file
		hr = pMP3Source->Load(szFileName, NULL);
		if (FAILED(hr)) {
			pMP3Source->Release();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get base filter interface on the source filter
		hr = pMP3Source->QueryInterface(IID_IBaseFilter, (void**)&m_pMP3Source);
		pMP3Source->Release();
		if (FAILED(hr))
			return VFW_S_PARTIAL_RENDER;

		// Add the source filter to our filter graph
		hr = m_pGraph->AddFilter(m_pMP3Source, NULL);
		if (FAILED(hr)) {
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get an output pin from the source filter
		IPin *pMP3SourceOutput = GetPinWithDirection(m_pMP3Source, PINDIR_OUTPUT);
		if (pMP3SourceOutput == NULL) {
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Create MPEG splitter object
		hr = CoCreateInstance(
			CLSID_MPEG1Splitter,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IBaseFilter,
			(void**)&m_pMPEGSplitter
		);
		if (FAILED(hr)) {
			pMP3SourceOutput->Release();
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Add the splitter filter to our filter graph
		hr = m_pGraph->AddFilter(m_pMPEGSplitter, L"MPEG-1 Stream Splitter");
		if (FAILED(hr)) {
			pMP3SourceOutput->Release();
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get the splitter input pin
		IPin *pMPEGSplitterInput = GetPinWithDirection(m_pMPEGSplitter, PINDIR_INPUT);
		if (pMPEGSplitterInput == NULL) {
			pMP3SourceOutput->Release();
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Connect the source output pin to the splitter input pin
		hr = m_pGraph->ConnectDirect(pMP3SourceOutput, pMPEGSplitterInput, NULL);
		pMP3SourceOutput->Release();
		pMPEGSplitterInput->Release();
		if (FAILED(hr)) {
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Get the splitter output pin
		IPin *pMPEGSplitterOutput = GetPinWithDirection(m_pMPEGSplitter, PINDIR_OUTPUT);
		if (pMPEGSplitterOutput == NULL) {
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Pass-through MP3 delivery appeared to be a tricky issue:
		// Fraunhofer decoder rejects connection to pass-through output 
		// pin and Nero Audio Decoder handles MP3 incorrectly (it's also 
		// not necessarily present in the user's system).
		// Thus, try to add Fraunhofer decoder right after MPEG-1 splitter
		// and if that fails, just pass through MP3 stream hoping that
		// some other filter handles it

		// An output pin to which we'll connect our pass-through input pin
		IPin *pMP3Output = NULL;

		// Create MP3 decoder object
		hr = CoCreateInstance(
			CLSID_MP3Decoder,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IBaseFilter,
			(void**)&m_pMP3Decoder
		);
		if (SUCCEEDED(hr)) {

			// Add the decoder filter to our filter graph
			hr = m_pGraph->AddFilter(m_pMP3Decoder, L"MPEG Layer-3 Decoder");
			if (FAILED(hr)) {
				pMPEGSplitterOutput->Release();
				ShutdownExternalMP3();
				return VFW_S_PARTIAL_RENDER;
			}

			// Get the decoder input pin
			IPin *pMP3DecoderInput = GetPinWithDirection(m_pMP3Decoder, PINDIR_INPUT);
			if (pMP3DecoderInput == NULL) {
				pMPEGSplitterOutput->Release();
				ShutdownExternalMP3();
				return VFW_S_PARTIAL_RENDER;
			}

			// Connect the splitter output pin to the decoder input pin
			hr = m_pGraph->ConnectDirect(pMPEGSplitterOutput, pMP3DecoderInput, NULL);
			pMPEGSplitterOutput->Release();
			pMP3DecoderInput->Release();
			if (FAILED(hr)) {
				ShutdownExternalMP3();
				return VFW_S_PARTIAL_RENDER;
			}

			// Get the decoder output pin
			IPin *pMP3DecoderOutput = GetPinWithDirection(m_pMP3Decoder, PINDIR_OUTPUT);
			if (pMP3DecoderOutput == NULL) {
				ShutdownExternalMP3();
				return VFW_S_PARTIAL_RENDER;
			}

			pMP3Output = pMP3DecoderOutput;

		} else
			pMP3Output = pMPEGSplitterOutput;

		m_pPassThruLock = new CCritSec();
		if (m_pPassThruLock == NULL) {
			pMP3Output->Release();
			ShutdownExternalMP3();
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
			pMP3Output->Release();
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Hold a reference on the pass-through input pin
		m_pPassThruInput->AddRef();
		// The pass-through output pin does not require 
		// refcounting as its lifetime is controlled 
		// by the containing pass-through input pin

		// We've created new pins -- so increment pin version
		IncrementPinVersion();

		// Connect the splitter or decoder output to the pass-through input
		hr = m_pGraph->ConnectDirect(pMP3Output, m_pPassThruInput, NULL);
		pMP3Output->Release();
		if (FAILED(hr)) {
			m_pPassThruInput->Release();
			delete m_pPassThruInput;
			m_pPassThruInput	= NULL;
			m_pPassThruOutput	= NULL;
			IncrementPinVersion();
			ShutdownExternalMP3();
			return VFW_S_PARTIAL_RENDER;
		}

		// Notify the filter graph manager of the changes made
		NotifyEvent(EC_GRAPH_CHANGED, 0, 0);

	}

	return NOERROR;
}

HRESULT CROQSplitterFilter::ShutdownExternalMP3(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	HRESULT hr = NOERROR;

	// Remove from the graph & release MP3 decoder
	if (m_pMP3Decoder) {
		FILTER_INFO info;
		if (m_pMP3Decoder->QueryFilterInfo(&info) == S_OK) {
			if (info.pGraph) {
				hr = info.pGraph->RemoveFilter(m_pMP3Decoder);
				info.pGraph->Release();
				if (FAILED(hr))
					return hr;
			}
		}
		m_pMP3Decoder->Release();
		m_pMP3Decoder = NULL;
	}

	// Remove from the graph & release MPEG splitter
	if (m_pMPEGSplitter) {
		FILTER_INFO info;
		if (m_pMPEGSplitter->QueryFilterInfo(&info) == S_OK) {
			if (info.pGraph) {
				hr = info.pGraph->RemoveFilter(m_pMPEGSplitter);
				info.pGraph->Release();
				if (FAILED(hr))
					return hr;
			}
		}
		m_pMPEGSplitter->Release();
		m_pMPEGSplitter = NULL;
	}

	// Remove from the graph & release MP3 source
	if (m_pMP3Source) {
		FILTER_INFO info;
		if (m_pMP3Source->QueryFilterInfo(&info) == S_OK) {
			if (info.pGraph) {
				hr = info.pGraph->RemoveFilter(m_pMP3Source);
				info.pGraph->Release();
				if (FAILED(hr))
					return hr;
			}
		}
		m_pMP3Source->Release();
		m_pMP3Source = NULL;
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

HRESULT CROQSplitterFilter::Shutdown(void)
{
	// Shutdown the parser
	HRESULT hr = ShutdownParser();
	if (FAILED(hr))
		return hr;

	// Shutdown external MP3 parsing system
	hr = ShutdownExternalMP3();
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
	}

	// Call the base-class implementation
	return CBaseParserFilter::Shutdown();
}

HRESULT CROQSplitterFilter::InitializeParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	HRESULT hr = NOERROR;

	// Create chunk parser if we have to
	if (m_pParser == NULL) {

		m_pParser = new CROQChunkParser(
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

HRESULT CROQSplitterFilter::ShutdownParser(void)
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

HRESULT CROQSplitterFilter::ResetParser(void)
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

HRESULT CROQSplitterFilter::CheckInputType(const CMediaType* pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	if (
		(IsEqualGUID(*pmt->Type(),		MEDIATYPE_Stream)) &&	// We accept byte stream
		(IsEqualGUID(*pmt->Subtype(),	MEDIASUBTYPE_ROQ))		// ... from ROQ file
	)
		return S_OK;
	else
		return S_FALSE;
}

HRESULT CROQSplitterFilter::GetInputType(int iPosition, CMediaType *pMediaType)
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
		pMediaType->SetSubtype(&MEDIASUBTYPE_ROQ);	// From ROQ file
		pMediaType->SetVariableSize();				// Variable size samples
		pMediaType->SetTemporalCompression(FALSE);	// No temporal compression
		pMediaType->SetFormatType(&FORMAT_None);	// Is it needed ???
	}

	return S_OK;
}
	
HRESULT CROQSplitterFilter::Receive(
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

STDMETHODIMP CROQSplitterFilter::Stop(void)
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

HRESULT CROQSplitterFilter::ConvertTimeFormat(
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

	if (!lstrcmpW(pPinName, wszROQVideoOutputName)) {

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

	} else if (!lstrcmpW(pPinName, wszROQAudioOutputName)) {

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
			llSourceInSamples = (llSource * 2) / m_nSampleSize;
		else if (IsEqualGUID(*pSourceFormat, TIME_FORMAT_MEDIA_TIME))
			llSourceInSamples = (llSource * m_nAvgBytesPerSec) / (m_nSampleSize  * UNITS);
		else
			return E_INVALIDARG;

		// Convert the source in samples to target format
		if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_SAMPLE))
			*pTarget = llSourceInSamples;
		else if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_BYTE))
			*pTarget = (llSourceInSamples * m_nSampleSize) / 2;
		else if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_MEDIA_TIME))
			*pTarget = (llSourceInSamples * m_nSampleSize * UNITS) / m_nAvgBytesPerSec;
		else
			return E_INVALIDARG;

		// At this point the conversion is successfully done
		return NOERROR;
	}

	return E_NOTIMPL;
}

STDMETHODIMP CROQSplitterFilter::get_UseExternalMP3(BOOL *pbUseExternalMP3)
{
	// Check and validate the pointer
	CheckPointer(pbUseExternalMP3, E_POINTER);
	ValidateWritePtr(pbUseExternalMP3, sizeof(BOOL));

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	*pbUseExternalMP3 = m_bUseExternalMP3;

	return NOERROR;
}

STDMETHODIMP CROQSplitterFilter::get_ExternalMP3Path(OLECHAR *szExternalMP3Path)
{
	// Check the pointer
	CheckPointer(szExternalMP3Path, E_POINTER);

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	lstrcpyW(szExternalMP3Path, m_szExternalMP3Path);

	return NOERROR;
}

STDMETHODIMP CROQSplitterFilter::put_UseExternalMP3(BOOL bUseExternalMP3)
{
	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	m_bUseExternalMP3 = bUseExternalMP3;

	return NOERROR;
}

STDMETHODIMP CROQSplitterFilter::put_ExternalMP3Path(OLECHAR *szExternalMP3Path)
{
	// Check the pointer
	CheckPointer(szExternalMP3Path, E_POINTER);

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	lstrcpyW(m_szExternalMP3Path, szExternalMP3Path);

	return NOERROR;
}

STDMETHODIMP CROQSplitterFilter::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_ROQSplitterPage;
	pPages->pElems[1] = CLSID_BaseParserPage;

	return NOERROR;
}

//==========================================================================
// CROQSplitterPage methods
//==========================================================================

const WCHAR g_wszROQSplitterPageName[] = L"ANX ROQ Splitter Property Page";

CROQSplitterPage::CROQSplitterPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("ROQ Splitter Property Page"),
		pUnk,
		IDD_ROQSPLITTERPAGE,
		IDS_TITLE_ROQSPLITTERPAGE
	),
	m_pConfig(NULL) // No config interface at this time
{
}

CUnknown* WINAPI CROQSplitterPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CROQSplitterPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}

void CROQSplitterPage::SetDirty()
{
	// Set the dirty flag
	m_bDirty = TRUE;

	// Notify the page site of the dirty state
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

BOOL CROQSplitterPage::GetUseExternalMP3()
{
	// Get the checkbox state 
	return (IsDlgButtonChecked(m_Dlg, IDC_USEEXTERNALMP3) == BST_CHECKED);
}

void CROQSplitterPage::EnableExternalMP3Path(BOOL bEnable)
{
	EnableWindow(GetDlgItem(m_Dlg, IDC_EXTERNALMP3PATH), bEnable);
	EnableWindow(GetDlgItem(m_Dlg, IDC_EXTERNALMP3PATHFRAME), bEnable);
}

BOOL CROQSplitterPage::OnReceiveMessage(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMsg) {

		case WM_COMMAND:

			if (LOWORD(wParam) == IDC_USEEXTERNALMP3) {

				SetDirty();

				// Set the enabled state of the edit control according 
				// to the checkbox state
				EnableExternalMP3Path(GetUseExternalMP3());

			} else if (LOWORD(wParam) == IDC_EXTERNALMP3PATH) {

				if (HIWORD(wParam) == EN_CHANGE) {
					if (m_bIsFirstSetText)
						m_bIsFirstSetText = FALSE;
					else
						SetDirty();
				}

			} else if (LOWORD(wParam) == IDC_DEFAULTS) {

				// Set the default values
				CheckDlgButton(m_Dlg, IDC_USEEXTERNALMP3, BST_CHECKED);
				EnableExternalMP3Path(TRUE);
				SetDlgItemText(m_Dlg, IDC_EXTERNALMP3PATH, TEXT("."));

				SetDirty();

			}

			return (LRESULT)1;
	}

	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CROQSplitterPage::OnConnect(IUnknown *pUnknown)
{
	// At this time there should be no config interface
	ASSERT(m_pConfig == NULL);

	// Retrieve the config interface from the filter
	HRESULT hr = pUnknown->QueryInterface(
		IID_IConfigROQSplitter,
		(void**)&m_pConfig
	);
	if (FAILED(hr))
		return hr;

	ASSERT(m_pConfig);

	return NOERROR;
}

HRESULT CROQSplitterPage::OnDisconnect()
{
	// At this time we should have the config interface
	if (m_pConfig == NULL)
		return E_UNEXPECTED;

	// Release the config interface
	m_pConfig->Release();
	m_pConfig = NULL;

	return NOERROR;
}

HRESULT CROQSplitterPage::OnActivate()
{
	// At this time we should have the config interface
	if (m_pConfig == NULL)
		return E_UNEXPECTED;

	// Get the option value from the filter
	BOOL bUseExternalMP3;
	HRESULT hr = m_pConfig->get_UseExternalMP3(&bUseExternalMP3);
	if (FAILED(hr))
		return hr;

	// Set the checkbox appropriately
	CheckDlgButton(m_Dlg, IDC_USEEXTERNALMP3, (bUseExternalMP3) ? BST_CHECKED : BST_UNCHECKED);

	// Get the option value from the filter
	OLECHAR wszExternalMP3Path[MAX_PATH];
	hr = m_pConfig->get_ExternalMP3Path(wszExternalMP3Path);
	if (FAILED(hr))
		return hr;

	// Convert the wide string to TCHAR string
	TCHAR szExternalMP3Path[MAX_PATH];
	wsprintf(szExternalMP3Path, TEXT("%ls"), wszExternalMP3Path);

	// We set text in the edit control for the first time
	m_bIsFirstSetText = TRUE;

	// Set the edit control text appropriately
	SetDlgItemText(m_Dlg, IDC_EXTERNALMP3PATH, szExternalMP3Path);

	EnableExternalMP3Path(bUseExternalMP3);

	m_bDirty = FALSE;

	return NOERROR;
}

HRESULT CROQSplitterPage::OnApplyChanges()
{
	if (m_bDirty) {

		// At this time we should have the config interface
		if (m_pConfig == NULL)
			return E_UNEXPECTED;

		// Put the option on the filter
		HRESULT hr = m_pConfig->put_UseExternalMP3(GetUseExternalMP3());
		if (FAILED(hr))
			return hr;

		if (GetUseExternalMP3()) {

			// Get the edit control text
			TCHAR szExternalMP3Path[MAX_PATH];
			SendDlgItemMessage(
				m_Dlg,
				IDC_EXTERNALMP3PATH,
				WM_GETTEXT,
				(WPARAM)MAX_PATH,
				(LPARAM)szExternalMP3Path
			);

			// Convert the TCHAR string to wide string
			OLECHAR wszExternalMP3Path[MAX_PATH];
			MultiByteToWideChar(
				CP_ACP,
				0,
				szExternalMP3Path,
				-1,
				wszExternalMP3Path,
				MAX_PATH
			);
			
			// Put the option on the filter
			hr = m_pConfig->put_ExternalMP3Path(wszExternalMP3Path);
			if (FAILED(hr))
				return hr;

		}

		m_bDirty = FALSE;
	}

	return NOERROR;
}
