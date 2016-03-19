//==========================================================================
//
// File: VQASplitter.cpp
//
// Desc: Game Media Formats - Implementation of VQA splitter filter
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

#include "VQASpecs.h"
#include "VQAGUID.h"
#include "VQASplitter.h"
#include "ContinuousIMAADPCM.h"
#include "WSADPCM.h"
#include "resource.h"

//==========================================================================
// VQA splitter setup data
//==========================================================================

// Media content strings
const OLECHAR wszVQAAuthorName[]	= L"Westwood Studios";
const OLECHAR wszVQADescription[]	= L"Westwood Studios VQA video file";

// Global filter name
const WCHAR g_wszVQASplitterName[]	= L"ANX VQA Splitter";

// Output pin names
const WCHAR wszVQAVideoOutputName[]	= L"Video Output";
const WCHAR wszVQAAudioOutputName[]	= L"Audio Output";

const AMOVIESETUP_MEDIATYPE sudVQAStreamType = {
	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_VQA
};

const AMOVIESETUP_MEDIATYPE sudVQAVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_VQAVideo
};

const AMOVIESETUP_MEDIATYPE sudVQAAudioTypes[] = {
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_PCM
	},
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_CIMAADPCM
	},
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_WSADPCM
	}
};

const AMOVIESETUP_PIN sudVQASplitterPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudVQAStreamType	// Media types
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
		&sudVQAVideoType	// Media types
	},
	{	// Audio output pin
		L"Audio Output",	// Pin name
		FALSE,				// Is it rendered
		TRUE,				// Is it an output
		TRUE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Input",			// Connects to pin
		3,					// Number of types
		sudVQAAudioTypes	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudVQASplitter = {
	&CLSID_VQASplitter,		// CLSID of filter
	g_wszVQASplitterName,	// Filter name
	MERIT_NORMAL,			// Filter merit
	3,						// Number of pins
	sudVQASplitterPins		// Pin information
};

//==========================================================================
// VQA splitter file type setup data
//==========================================================================

const GMF_FILETYPE_PATTERN sudVQAPatterns[] = {
	{ 0,	4,	VQA_IDSTR_FORM	},
	{ 8,	4,	VQA_IDSTR_WVQA	}
};

const GMF_FILETYPE_ENTRY sudVQAEntries[] = {
	{ 2, sudVQAPatterns } 
};

const TCHAR *sudVQAExtensions[] = {
	TEXT(".vqa")
};

const GMF_FILETYPE_REGINFO CVQASplitterFilter::g_pRegInfo[] = {
	{
		TEXT("Westwood Studios VQA Video Format"),	// Type name
		&MEDIATYPE_Stream,							// Media type
		&MEDIASUBTYPE_VQA,							// Media subtype
		&CLSID_AsyncReader,							// Source filter
		1,											// Number of entries
		sudVQAEntries,								// Entries
		1,											// Number of extensions
		sudVQAExtensions							// Extensions
	}
};

const int CVQASplitterFilter::g_nRegInfo = 1;

//==========================================================================
// CVQAChunkParser methods
//==========================================================================

CVQAChunkParser::CVQAChunkParser(
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	BYTE nFramesPerSecond,
	WORD wCompressionRatio,
	WORD nSampleSize,
	DWORD nAvgBytesPerSec,
	HRESULT *phr
) :
	CBaseChunkParser(
		NAME("VQA Chunk Parser"),
		pFilter,
		ppOutputPin,
		sizeof(VQA_CHUNK_HEADER),
		2,
		phr
	),
	m_pPin(NULL),
	m_pSample(NULL),
	m_rtDelta(0),
	m_llDelta(0),
	m_nFramesPerSecond(nFramesPerSecond),
	m_wCompressionRatio(wCompressionRatio),
	m_nSampleSize(nSampleSize),
	m_nAvgBytesPerSec(nAvgBytesPerSec)
{
}

CVQAChunkParser::~CVQAChunkParser()
{
	// Free any resources allocated by the parser
	ResetParser();
}

HRESULT CVQAChunkParser::ResetParser(void)
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

HRESULT CVQAChunkParser::ParseChunkHeader(
	LONGLONG llStartPosition,
	BYTE *pbHeader,
	LONG *plDataSize
)
{
	// Check and validate pointers
	CheckPointer(pbHeader, E_POINTER);
	ValidateReadPtr(pbHeader, sizeof(VQA_CHUNK_HEADER));
	CheckPointer(plDataSize, E_POINTER);
	ValidateWritePtr(plDataSize, sizeof(LONG));

	VQA_CHUNK_HEADER *pHeader = (VQA_CHUNK_HEADER*)pbHeader;

	*plDataSize = (LONG)SWAPDWORD(pHeader->cbSize);

	m_pPin = NULL;
	m_pSample = NULL;
	m_rtDelta = 0;
	m_llDelta = 0;

	LONG lUnpackedDataLength = 0;

	switch (pHeader->dwID) {

		// Audio data (PCM or IMA ADPCM)
		case VQA_ID_SND0:
		case VQA_ID_SND2:

			ASSERT(m_ppOutputPin[1]);	// There should be the audio output pin
			m_pPin = m_ppOutputPin[1];	// We'll be dealing with audio output pin

			// Check if we have correct wave format parameters
			if (
				(m_wCompressionRatio	== 0) ||
				(m_nSampleSize			== 0) ||
				(m_nAvgBytesPerSec		== 0)
			)
				return E_UNEXPECTED;

			// Calculate the stream and media deltas
			lUnpackedDataLength = (*plDataSize) * m_wCompressionRatio;
			m_rtDelta	= ((REFERENCE_TIME)lUnpackedDataLength * UNITS) / m_nAvgBytesPerSec;
			m_llDelta	= (LONGLONG)lUnpackedDataLength / m_nSampleSize;

			break;

		// Audio data (WS ADPCM)
		case VQA_ID_SND1:

			ASSERT(m_ppOutputPin[1]);	// There should be the audio output pin
			m_pPin = m_ppOutputPin[1];	// We'll be dealing with audio output pin

			// We cannot calculate the stream and media deltas at this 
			// point, so we leave this work for the chunk end handler
			m_rtDelta	= (REFERENCE_TIME)-1;
			m_llDelta	= (LONGLONG)-1;

			break;

		// Video frame data
		case VQA_ID_VQFR:

			m_pPin = m_ppOutputPin[0];	// We'll be dealing with video output pin

			// Check if we have correct video format parameters
			if (m_nFramesPerSecond == 0)
				return E_UNEXPECTED;

			m_rtDelta = UNITS / m_nFramesPerSecond;
			m_llDelta = 1;

			break;

		// Full HiColor codebook
		case VQA_ID_VQFL:

			m_pPin = m_ppOutputPin[0];	// We'll be dealing with video output pin

			// We deliver no frame data, only codebook
			m_rtDelta = 0;
			m_llDelta = 0;

			break;

		default:

			// We pay no attention to other chunks
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

	// Reset the sample's actual data length
	hr = m_pSample->SetActualDataLength(0);
	if (FAILED(hr)) {
		m_pSample->Release();
		m_pSample = NULL;
		return hr;
	}

	return NOERROR;
}

HRESULT CVQAChunkParser::ParseChunkData(
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

HRESULT CVQAChunkParser::ParseChunkEnd(void)
{
	// If no sample is supplied there's no need to do anything
	if (!m_pSample)
		return NOERROR;

	// If we've got media sample there should be the output pin
	ASSERT(m_pPin);

	HRESULT hr = NOERROR;

	// If the deltas are unreasonable, we must set them now
	if (
		(m_rtDelta	== (REFERENCE_TIME)-1) &&
		(m_llDelta	== (LONGLONG)-1)
	) {
		// Get the sample's buffer
		BYTE *pbBuffer = NULL;
		hr = m_pSample->GetPointer(&pbBuffer);
		if (FAILED(hr)) {
			m_pSample->Release();
			m_pSample = NULL;
			return hr;
		}

		ValidateReadPtr(pbBuffer, sizeof(WSADPCMINFO));

		// Get WS ADPCM info
		WSADPCMINFO *pInfo = (WSADPCMINFO*)pbBuffer;

		// Check if we have correct wave format parameters
		if (
			(m_nSampleSize		== 0) ||
			(m_nAvgBytesPerSec	== 0)
		)
			return E_UNEXPECTED;

		// Calculate the stream and media deltas
		m_rtDelta	= ((REFERENCE_TIME)pInfo->wOutSize * UNITS) / m_nAvgBytesPerSec;
		m_llDelta	= (LONGLONG)pInfo->wOutSize / m_nSampleSize;
	}

	// Deliver the sample
	hr = m_pPin->Deliver(m_pSample, m_rtDelta, m_llDelta);
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
// CVQASplitterFilter methods
//==========================================================================

IMPLEMENT_FILETYPE(CVQASplitterFilter)

CVQASplitterFilter::CVQASplitterFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseParserFilter(
		NAME("VQA Splitter Filter"),
		pUnk,
		CLSID_VQASplitter,
		g_wszVQASplitterName,
		phr
	),
	m_nFramesPerSecond(0),	// No frame rate at this time
	m_wCompressionRatio(0),	// No compression ratio at this time
	m_nSampleSize(0),		// No audio sample size at this time
	m_nAvgBytesPerSec(0),	// No audio data rate at this time
	m_nVideoFrames(0),		// No video duration at this time
	m_pFrameTable(NULL),	// No frame table at this time
	m_cbFrameTable(0),		// No frame table at this time
	m_pCodebookTable(NULL),	// No codebook info at this time
	m_nCodebooks(0),		// No codebook info at this time
	//m_nCBParts(0),		// No codebook info at this time
	m_pParser(NULL)			// No chunk parser at this time
{
	ASSERT(phr);
}

CVQASplitterFilter::~CVQASplitterFilter()
{
	// Free any resources allocated by Initialize() or InitializeParser()
	Shutdown();
}

CUnknown* WINAPI CVQASplitterFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CVQASplitterFilter(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

HRESULT CVQASplitterFilter::Initialize(IPin *pPin, IAsyncReader *pReader)
{
	// Check and validate the pointer
	CheckPointer(pReader, E_POINTER);
	ValidateReadPtr(pReader, sizeof(IAsyncReader));

	// Read file header
	VQA_FILE_HEADER header;
	HRESULT hr = pReader->SyncRead(0, sizeof(header), (BYTE*)&header);
	if (hr != S_OK)
		return hr;

	// Verify file header
	if (
		(header.dwFileID	!= VQA_ID_FORM)	||
		(header.dwFormatID	!= VQA_ID_WVQA)
	)
		return VFW_E_INVALID_FILE_FORMAT;

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Set file positions
	m_llDefaultStart	= (LONGLONG)sizeof(VQA_FILE_HEADER);	// Right after the header
	m_llDefaultStop		= MAXLONGLONG; // Defaults to file end

	// Decide on the input pin properties
	m_cbInputAlign	= 1;
	m_cbInputBuffer	= 0x40000; // TEMP: Some reasonable value

	// Scan the chunks for audio and video info
	LONGLONG llSeekPos = m_llDefaultStart, llSubSeekPos;
	VQA_INFO info = {0};
	BOOL bFoundInfo = FALSE;
	int iAudioChunk = 0;
	DWORD dwAudioType = 0;
	DWORD cbAudioChunk[2] = { 0, 0 };
	DWORD cbMaxVideoChunk = 0;
	m_pFrameTable = NULL;
	m_cbFrameTable = 0;
	m_pCodebookTable = NULL;
	m_nCodebooks = 0;
	DWORD cbCodebookInfo = 0;
	for (int i = 0;  i < VQA_CHUNKS_TO_SCAN; i++) {

		// Align the seek position
		if (llSeekPos % 2)
			llSeekPos++;

		// Read the chunk header
		VQA_CHUNK_HEADER chunkheader;
		hr = pReader->SyncRead(llSeekPos, sizeof(chunkheader), (BYTE*)&chunkheader);
		if (hr != S_OK)
			break;

		// Update the seek position
		llSeekPos += sizeof(chunkheader);

		DWORD cbChunk = SWAPDWORD(chunkheader.cbSize);

		// Check the chunk type
		switch (chunkheader.dwID) {

			// VQA header
			case VQA_ID_VQHD:

				// Check the info chunk size
				if (cbChunk < sizeof(info)) {
					Shutdown();
					return VFW_E_INVALID_FILE_FORMAT;
				}

				// Read the info data
				hr = pReader->SyncRead(llSeekPos, sizeof(info), (BYTE*)&info);
				if (hr != S_OK) {
					Shutdown();
					return hr;
				}

				bFoundInfo = TRUE;
				break;

			// Full codebook info
			case VQA_ID_CINF:

				// Walk through CINF subchunks
				llSubSeekPos = llSeekPos;
				while (llSubSeekPos < llSeekPos + cbChunk) {

					// Align the seek position
					if (llSubSeekPos % 2)
						llSubSeekPos++;

					// Read the chunk header
					VQA_CHUNK_HEADER subchunkheader;
					hr = pReader->SyncRead(llSubSeekPos, sizeof(subchunkheader), (BYTE*)&subchunkheader);
					if (hr != S_OK)
						break;

					// Update the seek position
					llSubSeekPos += sizeof(subchunkheader);

					DWORD cbSubChunk = SWAPDWORD(subchunkheader.cbSize);

					// Parse subchunk ID
					switch (subchunkheader.dwID) {

						// Full codebook info data
						case VQA_ID_CIND:
							// Read the info data
							m_pCodebookTable = (VQA_CIND_ENTRY*)CoTaskMemAlloc(cbSubChunk);
							if (m_pCodebookTable) {
								hr = pReader->SyncRead(llSubSeekPos, cbSubChunk, (BYTE*)m_pCodebookTable);
								cbCodebookInfo = cbSubChunk;
								if (hr != S_OK) {
									CoTaskMemFree(m_pCodebookTable);
									m_pCodebookTable = NULL;
									cbCodebookInfo = 0;
								}
							}
							break;

						default:
							// No interest
							break;
					}

					// Advance the seek position
					llSubSeekPos += cbSubChunk;
				}
				break;

			// Frame positions array
			case VQA_ID_FINF:

				// Read this in to be able to seek
				m_pFrameTable = (DWORD*)CoTaskMemAlloc(cbChunk);
				if (m_pFrameTable) {
					hr = pReader->SyncRead(llSeekPos, cbChunk, (BYTE*)m_pFrameTable);
					m_cbFrameTable = cbChunk;
					if (hr != S_OK) {
						CoTaskMemFree(m_pFrameTable);
						m_pFrameTable = NULL;
						m_cbFrameTable = 0;
					}
				}
				break;

			// Audio data
			case VQA_ID_SND0:
			case VQA_ID_SND1:
			case VQA_ID_SND2:

				dwAudioType = chunkheader.dwID;

				if (iAudioChunk < 2) {
					// If it's SND1 chunk, get its outsize
					if (chunkheader.dwID == VQA_ID_SND1) {
						WSADPCMINFO wsinfo;
						hr = pReader->SyncRead(llSeekPos, sizeof(wsinfo), (BYTE*)&wsinfo);
						if (hr != S_OK) {
							Shutdown();
							return hr;
						}
						cbAudioChunk[iAudioChunk] = wsinfo.wOutSize;
					} else
						cbAudioChunk[iAudioChunk] = cbChunk;
				}

				iAudioChunk++;

				break;

			// Video frame data
			case VQA_ID_VQFR:

				cbMaxVideoChunk = cbChunk;
				break;

			default:

				// No matter
				break;
		}

		// If we've got all necessary information, quit
		if (
			(dwAudioType		!= 0)		&&
			(cbAudioChunk[0]	!= 0)		&&
			(cbAudioChunk[1]	!= 0)		&&
			(cbMaxVideoChunk	!= 0)		&&
			(m_pFrameTable		!= NULL)	&&
			(m_pCodebookTable	!= NULL)	&&
			(bFoundInfo)
		)
			break;

		// Advance the seek position
		llSeekPos += cbChunk;
	}

	// At this point we should have video chunk size and correct info chunk
	if (
		(cbMaxVideoChunk		== 0)	|| 
		(!bFoundInfo)					||
		(info.bBlockWidth		== 0)	||
		(info.bBlockHeight		== 0)	||
		(info.nFramesPerSecond	== 0)
	) {
		Shutdown();
		return VFW_E_INVALID_FILE_FORMAT;
	}

	// Get the video stream info
	m_nVideoFrames		= info.nVideoFrames;
	m_nFramesPerSecond	= info.nFramesPerSecond;
	m_nCodebooks		= cbCodebookInfo / sizeof(VQA_CIND_ENTRY);
	/* TEMP: Seeking stuff (not working)
	m_nCBParts			= info.nCBParts;
	*/

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Decide on the output pins count
	m_nOutputPins = 1;	// Video always should be present in the file

	// Check if we have soundtrack
	if (dwAudioType != 0)
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

	// Initialize the video media type
	pmtVideo->InitMediaType();
	pmtVideo->SetType(&MEDIATYPE_Video);
	pmtVideo->SetSubtype(&MEDIASUBTYPE_VQAVideo);
	pmtVideo->SetSampleSize(0);
	pmtVideo->SetTemporalCompression(TRUE);
	pmtVideo->SetFormatType(&FORMAT_VQAVideo);
	if (!pmtVideo->SetFormat((BYTE*)&info, sizeof(info))) {
		delete pmtVideo;
		Shutdown();
		return E_FAIL;
	}
	if (m_pCodebookTable != NULL) {

		// Try to reallocate the format buffer
		pmtVideo->ReallocFormatBuffer(sizeof(info) + cbCodebookInfo);

		// Copy the codebook table to the additional format buffer
		if (pmtVideo->FormatLength() >= sizeof(info) + cbCodebookInfo)
			CopyMemory(pmtVideo->Format() + sizeof(info), m_pCodebookTable, cbCodebookInfo);
	}

	// Allocate the video allocator properties
	ALLOCATOR_PROPERTIES *papVideo = (ALLOCATOR_PROPERTIES*)CoTaskMemAlloc(sizeof(ALLOCATOR_PROPERTIES));
	if (papVideo == NULL) {
		delete pmtVideo;
		Shutdown();
		return E_OUTOFMEMORY;
	}

	// Calculate theoretical maximum video chunk size
	long cbMaxCodebook = max(MAX_CODEBOOK_SIZE, info.cbMaxCBFZSize);
	long cbMaxVPTR = 3 * (info.wVideoWidth / info.bBlockWidth) *
						(info.wVideoHeight / info.bBlockHeight);
	long cbThMaxVideoChunk =
		sizeof(VQA_CHUNK_HEADER) +		// CBF? header
		cbMaxCodebook +					// CBF? size
		sizeof(VQA_CHUNK_HEADER) +		// CBP? header
		cbMaxCodebook +					// CBP? size
		sizeof(VQA_CHUNK_HEADER) +		// CPL? header
		MAX_PALETTE_SIZE +				// CPL? size
		sizeof(VQA_CHUNK_HEADER) +		// VPT? header
		cbMaxVPTR;						// VPT? size

	// Set the video allocator properties
	papVideo->cbAlign	= 0;	// No matter
	papVideo->cbPrefix	= 0;	// No matter
	papVideo->cBuffers	= 4;	// TEMP: No need to set larger value?
	papVideo->cbBuffer	= max((long)cbMaxVideoChunk, cbThMaxVideoChunk); // TEMP: we assume the first chunk is the largest one

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

		DWORD dwSeekFlags = (
			(info.nColors		== 0)		&&
			(m_pFrameTable		!= NULL)	&&
			(m_pCodebookTable	!= NULL)	&&
			(dwAudioType		!= VQA_ID_SND2)
		) ? (
			AM_SEEKING_CanSeekAbsolute	|
			AM_SEEKING_CanSeekForwards	|
			AM_SEEKING_CanSeekBackwards
		) : 0;

		dwVideoCapabilities =	AM_SEEKING_CanGetCurrentPos	|
								AM_SEEKING_CanGetStopPos	|
								AM_SEEKING_CanGetDuration	|
								dwSeekFlags;
	}

	// Create video output pin (always the first one!)
	hr = NOERROR;
	m_ppOutputPin[0] = new CParserOutputPin(
		NAME("VQA Splitter Video Output Pin"),
		this,
		&m_csFilter,
		pmtVideo,
		papVideo,
		dwVideoCapabilities,
		nVideoTimeFormats,
		pVideoTimeFormats,
		&hr,
		wszVQAVideoOutputName
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

	if (dwAudioType != 0) {

		// Correct the audio info fields
		if (info.wAudioSampleRate == 0)
			info.wAudioSampleRate = 22050;
		if (info.nAudioChannels == 0)
			info.nAudioChannels = 1;
		if (info.nAudioBits == 0)
			info.nAudioBits = 8;

		// Allocate audio media type
		CMediaType *pmtAudio = new CMediaType();
		if (pmtAudio == NULL) {
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Initialize the audio media type
		pmtAudio->InitMediaType();
		pmtAudio->SetType(&MEDIATYPE_Audio);
		WAVEFORMATEX *pWaveFormat				= NULL;
		WSADPCMWAVEFORMAT *pWSADPCMFormat		= NULL;
		CIMAADPCMWAVEFORMAT *pCIMAADPCMFormat	= NULL;
		switch (dwAudioType) {

			// Raw PCM
			case VQA_ID_SND0:
				pmtAudio->SetSubtype(&MEDIASUBTYPE_PCM);
				pmtAudio->SetSampleSize(info.nAudioChannels * info.nAudioBits / 8);
				pmtAudio->SetTemporalCompression(FALSE);
				pmtAudio->SetFormatType(&FORMAT_WaveFormatEx);
				pWaveFormat = (WAVEFORMATEX*)pmtAudio->AllocFormatBuffer(sizeof(WAVEFORMATEX));
				if (pWaveFormat == NULL) {
					delete pmtAudio;
					Shutdown();
					return E_OUTOFMEMORY;
				}

				// Fill in the audio format block
				pWaveFormat->wFormatTag			= WAVE_FORMAT_PCM;
				pWaveFormat->nChannels			= info.nAudioChannels;
				pWaveFormat->nSamplesPerSec		= info.wAudioSampleRate;
				pWaveFormat->nAvgBytesPerSec	= info.nAudioChannels * info.nAudioBits * info.wAudioSampleRate / 8;
				pWaveFormat->nBlockAlign		= (WORD)(info.nAudioChannels * info.nAudioBits / 8);
				pWaveFormat->wBitsPerSample		= info.nAudioBits;
				pWaveFormat->cbSize				= 0;

				// Set the wave format parameters needed for the calculation
				// of sample stream and media duration
				m_nSampleSize		= pWaveFormat->nBlockAlign;
				m_nAvgBytesPerSec	= pWaveFormat->nAvgBytesPerSec;
				m_wCompressionRatio	= 1;

				break;

			// WS ADPCM
			case VQA_ID_SND1:
				pmtAudio->SetSubtype(&MEDIASUBTYPE_WSADPCM);
				pmtAudio->SetSampleSize(0);
				pmtAudio->SetTemporalCompression(TRUE);
				pmtAudio->SetFormatType(&FORMAT_WSADPCM);
				pWSADPCMFormat = (WSADPCMWAVEFORMAT*)pmtAudio->AllocFormatBuffer(sizeof(WSADPCMWAVEFORMAT));
				if (pWSADPCMFormat == NULL) {
					delete pmtAudio;
					Shutdown();
					return E_OUTOFMEMORY;
				}

				// Fill in the audio format block
				pWSADPCMFormat->nChannels		= info.nAudioChannels;
				pWSADPCMFormat->nSamplesPerSec	= info.wAudioSampleRate;
				pWSADPCMFormat->wBitsPerSample	= info.nAudioBits;

				// Set the wave format parameters needed for the calculation
				// of sample stream and media duration
				m_nSampleSize		= pWSADPCMFormat->wBitsPerSample * pWSADPCMFormat->nChannels / 8;
				m_nAvgBytesPerSec	= m_nSampleSize * pWSADPCMFormat->nSamplesPerSec;
				m_wCompressionRatio	= 0;

				break;

			// IMA ADPCM
			case VQA_ID_SND2:
				pmtAudio->SetSubtype(&MEDIASUBTYPE_CIMAADPCM);
				pmtAudio->SetSampleSize(0);
				pmtAudio->SetTemporalCompression(TRUE);
				pmtAudio->SetFormatType(&FORMAT_CIMAADPCM);
				pCIMAADPCMFormat = (CIMAADPCMWAVEFORMAT*)pmtAudio->AllocFormatBuffer(sizeof(CIMAADPCMWAVEFORMAT) + (info.nAudioChannels - 1) * sizeof(CIMAADPCMINFO));
				if (pCIMAADPCMFormat == NULL) {
					delete pmtAudio;
					Shutdown();
					return E_OUTOFMEMORY;
				}

				// Fill in the audio format block
				pCIMAADPCMFormat->nChannels			= info.nAudioChannels;
				pCIMAADPCMFormat->nSamplesPerSec	= info.wAudioSampleRate;
				pCIMAADPCMFormat->wBitsPerSample	= info.nAudioBits;
				pCIMAADPCMFormat->bIsHiNibbleFirst	= FALSE;
				pCIMAADPCMFormat->dwReserved		= (info.nAudioChannels == 1)
					? CIMAADPCM_INTERLEAVING_NORMAL
					: (
						(info.wVersion < 3)
						? CIMAADPCM_INTERLEAVING_DOUBLE
						: CIMAADPCM_INTERLEAVING_SPLIT
					);

				// Fill in init info block for each channel
				for (i = 0; i < pCIMAADPCMFormat->nChannels; i++) {
					pCIMAADPCMFormat->pInit[i].lSample	= 0;
					pCIMAADPCMFormat->pInit[i].chIndex	= 0;
				}

				// Set the wave format parameters needed for the calculation
				// of sample stream and media duration
				m_nSampleSize		= pCIMAADPCMFormat->wBitsPerSample * pCIMAADPCMFormat->nChannels / 8;
				m_nAvgBytesPerSec	= m_nSampleSize * pCIMAADPCMFormat->nSamplesPerSec;
				m_wCompressionRatio	= pCIMAADPCMFormat->wBitsPerSample / 4;

				break;

			default:
				// Is it possible???
				break;
		}

		/*//TEMP: Should we use video pin time correction???
		// Set the time adjustment for the video output pin
		REFERENCE_TIME rtAdjust = 0;
		if (cbAudioChunk[0] == cbAudioChunk[1]) {

			// No adjustment needed
			rtAdjust = 0;

		} else {

			if (info.wVersion == 1) {

				// First audio chunk duration minus second audio chunk duration
				rtAdjust = (
					(REFERENCE_TIME)(cbAudioChunk[0] - cbAudioChunk[1]) * UNITS *
					((dwAudioType == VQA_ID_SND1) ? 1 : m_wCompressionRatio)
				) / m_nAvgBytesPerSec;

			} else if (info.wVersion == 2) {

				// First audio chunk duration
				rtAdjust = (
					(REFERENCE_TIME)cbAudioChunk[0] * UNITS *
					((dwAudioType == VQA_ID_SND1) ? 1 : m_wCompressionRatio)
				) / m_nAvgBytesPerSec;

			}

		}
		m_ppOutputPin[0]->SetTime(rtAdjust);
		*/

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
		papAudio->cbBuffer	= cbAudioChunk[0];

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
			NAME("VQA Splitter Audio Output Pin"),
			this,
			&m_csFilter,
			pmtAudio,
			papAudio,
			dwAudioCapabilities,
			nAudioTimeFormats,
			pAudioTimeFormats,
			&hr,
			wszVQAAudioOutputName
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
		m_wszAuthorName = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszVQAAuthorName) + 1));
		if (m_wszAuthorName)
			lstrcpyW(m_wszAuthorName, wszVQAAuthorName);
		m_wszDescription = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszVQADescription) + 1));
		if (m_wszDescription)
			lstrcpyW(m_wszDescription, wszVQADescription);
	}

	return NOERROR;
}

HRESULT CVQASplitterFilter::Shutdown(void)
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
		m_wCompressionRatio	= 0;
		m_nSampleSize		= 0;
		m_nAvgBytesPerSec	= 0;
		m_nVideoFrames		= 0;
		//m_nCBParts		= 0;
		if (m_pFrameTable) {
			CoTaskMemFree(m_pFrameTable);
			m_pFrameTable = NULL;
		}
		m_cbFrameTable = 0;
		if (m_pCodebookTable) {
			CoTaskMemFree(m_pCodebookTable);
			m_pCodebookTable = NULL;
		}
		m_nCodebooks = 0;
	}

	// Call the base-class implementation
	return CBaseParserFilter::Shutdown();
}

HRESULT CVQASplitterFilter::InitializeParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	HRESULT hr = NOERROR;

	// Create chunk parser if we have to
	if (m_pParser == NULL) {

		m_pParser = new CVQAChunkParser(
			this,
			m_ppOutputPin,
			m_nFramesPerSecond,
			m_wCompressionRatio,
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

HRESULT CVQASplitterFilter::ShutdownParser(void)
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

HRESULT CVQASplitterFilter::ResetParser(void)
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

HRESULT CVQASplitterFilter::CheckInputType(const CMediaType* pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	if (
		(IsEqualGUID(*pmt->Type(),		MEDIATYPE_Stream)) &&	// We accept byte stream
		(IsEqualGUID(*pmt->Subtype(),	MEDIASUBTYPE_VQA))		// ... from VQA file
	)
		return S_OK;
	else
		return S_FALSE;
}

HRESULT CVQASplitterFilter::GetInputType(int iPosition, CMediaType *pMediaType)
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
		pMediaType->SetSubtype(&MEDIASUBTYPE_VQA);	// From VQA file
		pMediaType->SetVariableSize();				// Variable size samples
		pMediaType->SetTemporalCompression(FALSE);	// No temporal compression
		pMediaType->SetFormatType(&FORMAT_None);	// Is it needed ???
	}

	return S_OK;
}
	
HRESULT CVQASplitterFilter::Receive(
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

HRESULT CVQASplitterFilter::ConvertTimeFormat(
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

	if (!lstrcmpW(pPinName, wszVQAVideoOutputName)) {

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

	} else if (!lstrcmpW(pPinName, wszVQAAudioOutputName)) {

		// Process the audio output pin's request

		// Protect the filter data
		CAutoLock datalock(&m_csData);

		// Check if we have correct wave format parameters
		if (
			(m_wCompressionRatio	== 0) ||
			(m_nSampleSize			== 0) ||
			(m_nAvgBytesPerSec		== 0)
		)
			return E_UNEXPECTED;

		// Convert the source value to samples
		LONGLONG llSourceInSamples;
		if (IsEqualGUID(*pSourceFormat, TIME_FORMAT_SAMPLE))
			llSourceInSamples = llSource;
		else if (IsEqualGUID(*pSourceFormat, TIME_FORMAT_BYTE))
			llSourceInSamples = (llSource * m_wCompressionRatio) / m_nSampleSize;
		else if (IsEqualGUID(*pSourceFormat, TIME_FORMAT_MEDIA_TIME))
			llSourceInSamples = (llSource * m_nAvgBytesPerSec) / (m_nSampleSize  * UNITS);
		else
			return E_INVALIDARG;

		// Convert the source in samples to target format
		if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_SAMPLE))
			*pTarget = llSourceInSamples;
		else if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_BYTE))
			*pTarget = (llSourceInSamples * m_nSampleSize) / m_wCompressionRatio;
		else if (IsEqualGUID(*pTargetFormat, TIME_FORMAT_MEDIA_TIME))
			*pTarget = (llSourceInSamples * m_nSampleSize * UNITS) / m_nAvgBytesPerSec;
		else
			return E_INVALIDARG;

		// At this point the conversion is successfully done
		return NOERROR;
	}

	return E_NOTIMPL;
}

HRESULT CVQASplitterFilter::GetDuration(
	LPCWSTR pPinName,
	const GUID *pCurrentFormat,
	LONGLONG *pDuration
)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Convert the duration in frames to the current time format
	return ConvertTimeFormat(
		wszVQAVideoOutputName,
		pDuration,
		pCurrentFormat,
		(LONGLONG)m_nVideoFrames,
		&TIME_FORMAT_FRAME
	);
}

/* TEMP: Seeking stuff (not working)
HRESULT CVQASplitterFilter::DeliverChunk(
	CParserOutputPin *pPin,
	LONGLONG llStart,
	LONGLONG llStop,
	DWORD dwID,
	DWORD dwMask
)
{
	HRESULT hr = NOERROR;

	// Get async reader from the input pin
	IAsyncReader *pReader = m_InputPin.GetReader();
	if (pReader == NULL)
		return E_UNEXPECTED;

	// Scan the file until the chunk is found or stop position is reached
	DWORD cbData = 0;
	while (llStart < llStop) {

		// Align the seek position
		if (llStart % 2)
			llStart++;

		// Read the chunk header
		VQA_CHUNK_HEADER header;
		hr = pReader->SyncRead(llStart, sizeof(header), (BYTE*)&header);
		if (hr != S_OK)
			break;

		llStart += sizeof(header);

		DWORD cbChunk = SWAPDWORD(header.cbSize);

		// Parse chunk ID
		if ((header.dwID & dwMask) == (dwID & dwMask)) {
			llStart -= sizeof(header);
			cbData = sizeof(header) + cbChunk;
			break;
		} else if (
			(header.dwID != VQA_ID_VQFR) && 
			(header.dwID != VQA_ID_VQFL)
		)
			llStart += cbChunk;
	}

	// Check if we found the requested chunk
	if (cbData == 0) {
		pReader->Release();
		return VFW_E_NOT_FOUND;
	}

	// Get the empty media sample from the video output pin
	IMediaSample *pSample = NULL;
	hr = pPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0);
	if (FAILED(hr)) {
		pReader->Release();
		return hr;
	}

	// Find out how many bytes we can copy
	LONG cbBuffer = pSample->GetSize();

	// Check that the buffer size is large enough
	if (cbData > cbBuffer) {
		pReader->Release();
		pSample->Release();
		return VFW_E_BUFFER_OVERFLOW;
	}

	// Get the sample's buffer
	BYTE *pbBuffer = NULL;
	hr = pSample->GetPointer(&pbBuffer);
	if (FAILED(hr)) {
		pReader->Release();
		pSample->Release();
		return hr;
	}

	// Read in the chunk
	hr = pReader->SyncRead(llStart, cbData, pbBuffer);
	pReader->Release();
	if (hr != S_OK) {
		pSample->Release();
		return hr;
	}

	// Set the sample's actual data length
	hr = pSample->SetActualDataLength(cbData);
	if (FAILED(hr)) {
		pSample->Release();
		return hr;
	}

	// Deliver the sample
	hr = pPin->Deliver(pSample, 0, 0);
	pSample->Release();

	return hr;
}*/

#define CORRPOS(x)							\
(											\
	((x) >= VQA_PALETTE_MARKER)				\
	? (2 * ((x) - VQA_PALETTE_MARKER))		\
	: (2 * (x))								\
)

#define FRAMEPOS(i)							\
(											\
	((i) * sizeof(DWORD) < m_cbFrameTable)	\
	? CORRPOS(m_pFrameTable[(i)])			\
	: m_llDefaultStop						\
)

HRESULT CVQASplitterFilter::ConvertPosition(
	LONGLONG *pllTarget,
	LPCWSTR pPinName,
	const GUID *pSourceFormat,
	LONGLONG *pllSource,
	DWORD dwSourceFlags
)
{
	// Sanity check of the frame and codebook info tables
	if (
		(m_pFrameTable		== NULL)	||
		(m_cbFrameTable		== 0)		||
		(m_pCodebookTable	== NULL)	||
		(m_nCodebooks		== 0)
	)
		return E_UNEXPECTED;

	// Convert seek position to frames
	LONGLONG llFrame;
	HRESULT hr = ConvertTimeFormat(
		wszVQAVideoOutputName,
		&llFrame,
		&TIME_FORMAT_FRAME,
		*pllSource,
		pSourceFormat
	);
	if (FAILED(hr))
		return hr;

	// Find the last frame with full codebook before our frame
	int i = (int)m_nCodebooks - 1;
	for (i = (int)m_nCodebooks - 1; i >= 0; i--) {
		if (
			(m_pCodebookTable[i].iFrame <= llFrame) &&
			(m_pCodebookTable[i].cbSize != 0)
		)
			break;
	}

	// Check if we found proper codebook
	if (
		(i < 0) ||
		(m_pCodebookTable[i].cbSize == 0)
	)
		return E_FAIL;

	// We should seek to the the frame with full codebook
	llFrame = m_pCodebookTable[i].iFrame;

	// Sanity check of the frame index
	if (
		(llFrame >= m_nVideoFrames) || 
		(llFrame * sizeof(DWORD) >= m_cbFrameTable)
	)
		return E_INVALIDARG;

	// Return the file position correspondent to the found frame
	*pllTarget = FRAMEPOS(llFrame);

	// Convert the frame index to source time format
	// so that the caller knows actual seek point
	hr = ConvertTimeFormat(
		wszVQAVideoOutputName,
		pllSource,
		pSourceFormat,
		llFrame,
		&TIME_FORMAT_FRAME
	);
	if (FAILED(hr))
		return hr;

	// Reset the stream and media times on the output pins
	if (m_nOutputPins > 0) {
		m_ppOutputPin[0]->SetTime(0);
		m_ppOutputPin[0]->SetMediaTime(0);
	}
	if (m_nOutputPins > 1) {
		m_ppOutputPin[1]->SetTime(0);
		m_ppOutputPin[1]->SetMediaTime(0);
	}

	return NOERROR;
}

/* TEMP: Seeking stuff (not working)
HRESULT CVQASplitterFilter::ConvertPosition(
	LONGLONG *pllTarget,
	LPCWSTR pPinName,
	const GUID *pSourceFormat,
	LONGLONG *pllSource,
	DWORD dwSourceFlags
)
{
	// Sanity check for frame info table
	if (m_pFrameTable == NULL)
		return E_UNEXPECTED;

	// Convert seek position to frames
	LONGLONG llFrame;
	HRESULT hr = ConvertTimeFormat(
		wszVQAVideoOutputName,
		&llFrame,
		&TIME_FORMAT_FRAME,
		*pllSource,
		pSourceFormat
	);
	if (FAILED(hr))
		return hr;

	// Sanity check of the frame index
	if (
		(llFrame >= m_nVideoFrames) || 
		(llFrame * sizeof(DWORD) >= m_cbFrameTable)
	)
		return E_INVALIDARG;

	// Find the last frame with palette before our frame
	for (int i = (int)llFrame; i >= 0; i--) {
		if (m_pFrameTable[i] >= VQA_PALETTE_MARKER)
			break;
	}

	// Send the palette downstream
	hr = DeliverChunk(
		m_ppOutputPin[0],
		FRAMEPOS(i),
		FRAMEPOS(i + 1),
		VQA_ID_CPL0,
		0x00FFFFFF
	);
	if (hr != S_OK)
		return hr;

	// Find out from which frame we should collect the current codebook
	int iKeyFrame = ((int)llFrame / m_nCBParts) * m_nCBParts;
	if (iKeyFrame == 0) {

		// We should take the first (full) codebook
		hr = DeliverChunk(
			m_ppOutputPin[0],
			FRAMEPOS(0),
			FRAMEPOS(1),
			VQA_ID_CBF0,
			0x00FFFFFF
		);
		if (hr != S_OK)
			return hr;

	} else {

		// We should walk nCBParts chunks to collect the entire codebook
		for (i = iKeyFrame - m_nCBParts; i < iKeyFrame; i++) {
			hr = DeliverChunk(
				m_ppOutputPin[0],
				FRAMEPOS(i),
				FRAMEPOS(i + 1),
				VQA_ID_CBP0,
				0x00FFFFFF
			);
			if (hr != S_OK)
				return hr;
		}

	}

	// Now collect the next codebook parts preceding our frame
	for (i = iKeyFrame; i < llFrame; i++) {
		hr = DeliverChunk(
			m_ppOutputPin[0],
			FRAMEPOS(i),
			FRAMEPOS(i + 1),
			VQA_ID_CBP0,
			0x00FFFFFF
		);
		if (hr != S_OK)
			return hr;
	}

	// Return the file position correspondent to our frame
	*pllTarget = FRAMEPOS(llFrame);

	// Convert our frame index to source time format
	// so that the caller knows actual seek point
	hr = ConvertTimeFormat(
		wszVQAVideoOutputName,
		pllSource,
		pSourceFormat,
		llFrame,
		&TIME_FORMAT_FRAME
	);
	if (FAILED(hr))
		return hr;

	return NOERROR;
}
*/

STDMETHODIMP CVQASplitterFilter::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_VQASplitterPage;
	pPages->pElems[1] = CLSID_BaseParserPage;

	return NOERROR;
}

//==========================================================================
// CVQASplitterPage methods
//==========================================================================

const WCHAR g_wszVQASplitterPageName[] = L"ANX VQA Splitter Property Page";

CVQASplitterPage::CVQASplitterPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("VQA Splitter Property Page"),
		pUnk,
		IDD_VQASPLITTERPAGE,
		IDS_TITLE_VQASPLITTERPAGE
	)
{
}

CUnknown* WINAPI CVQASplitterPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CVQASplitterPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
