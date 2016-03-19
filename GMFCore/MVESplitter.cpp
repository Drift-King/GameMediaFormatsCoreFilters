//==========================================================================
//
// File: MVESplitter.cpp
//
// Desc: Game Media Formats - Implementation of MVE splitter filter
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

#include "MVESpecs.h"
#include "MVEGUID.h"
#include "MVESplitter.h"
#include "FilterOptions.h"
#include "resource.h"

#include <string.h>

//==========================================================================
// MVE splitter setup data
//==========================================================================

// Media content strings
const OLECHAR wszMVEAuthorName[]	= L"Interplay Productions";
const OLECHAR wszMVEDescription[]	= L"Interplay Productions MVE video file";

// Global filter name
const WCHAR g_wszMVESplitterName[]	= L"ANX MVE Splitter";

// Output pin names
const WCHAR wszMVEVideoOutputName[]	= L"Video Output";
const WCHAR wszMVEAudioOutputName[]	= L"Audio Output";

const AMOVIESETUP_MEDIATYPE sudMVEStreamType = {
	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_MVE
};

const AMOVIESETUP_MEDIATYPE sudMVEVideoType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_MVEVideo
};

const AMOVIESETUP_MEDIATYPE sudMVEAudioTypes[] = {
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_PCM
	},
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_MVEADPCM
	}
};

const AMOVIESETUP_PIN sudMVESplitterPins[] = {
	{	// Input pin
		L"Input",			// Pin name
		FALSE,				// Is it rendered
		FALSE,				// Is it an output
		FALSE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Output",			// Connects to pin
		1,					// Number of types
		&sudMVEStreamType	// Media types
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
		&sudMVEVideoType	// Media types
	},
	{	// Audio output pin
		L"Audio Output",	// Pin name
		FALSE,				// Is it rendered
		TRUE,				// Is it an output
		TRUE,				// Allowed none
		FALSE,				// Allowed many
		&CLSID_NULL,		// Connects to filter
		L"Input",			// Connects to pin
		2,					// Number of types
		sudMVEAudioTypes	// Media types
	}
};

const AMOVIESETUP_FILTER g_sudMVESplitter = {
	&CLSID_MVESplitter,		// CLSID of filter
	g_wszMVESplitterName,	// Filter name
	MERIT_NORMAL,			// Filter merit
	3,						// Number of pins
	sudMVESplitterPins		// Pin information
};

//==========================================================================
// MVE splitter file type setup data
//==========================================================================

const GMF_FILETYPE_PATTERN sudMVEPatterns[] = {
	{ 0,	20,	MVE_IDSTR_MVE		},
	{ 20,	2,	MVE_IDSTR_MAGIC1	},
	{ 22,	2,	MVE_IDSTR_MAGIC2	},
	{ 24,	2,	MVE_IDSTR_MAGIC3	}
};

const GMF_FILETYPE_ENTRY sudMVEEntries[] = {
	{ 4, sudMVEPatterns } 
};

const TCHAR *sudMVEExtensions[] = {
	TEXT(".mve"),
	TEXT(".mv8")
};

const GMF_FILETYPE_REGINFO CMVESplitterFilter::g_pRegInfo[] = {
	{
		TEXT("Interplay Productions MVE Video Format"),	// Type name
		&MEDIATYPE_Stream,								// Media type
		&MEDIASUBTYPE_MVE,								// Media subtype
		&CLSID_AsyncReader,								// Source filter
		1,												// Number of entries
		sudMVEEntries,									// Entries
		2,												// Number of extensions
		sudMVEExtensions								// Extensions
	}
};

const int CMVESplitterFilter::g_nRegInfo = 1;

//==========================================================================
// CMVEInnerChunkParser methods
//==========================================================================

CMVEInnerChunkParser::CMVEInnerChunkParser(
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	REFERENCE_TIME rtFrameDelta,
	WORD nSampleSize,
	DWORD nAvgBytesPerSec,
	WORD wCompressionRatio,
	BOOL bIs16Bit,
	HRESULT *phr
) :
	CBaseChunkParser(
		NAME("MVE Inner Chunk Parser"),
		pFilter,
		ppOutputPin,
		sizeof(MVE_CHUNK_HEADER),
		1,
		phr
	),
	m_pPin(NULL),
	m_pSample(NULL),
	m_rtDelta(0),
	m_llDelta(0),
	m_bCurrentSubchunkType(0xFF),
	m_bCurrentSubchunkSubtype(0xFF),
	m_rtFrameDelta(rtFrameDelta),
	m_nSampleSize(nSampleSize),
	m_nAvgBytesPerSec(nAvgBytesPerSec),
	m_wCompressionRatio(wCompressionRatio),
	m_bIs16Bit(bIs16Bit)
{
}

CMVEInnerChunkParser::~CMVEInnerChunkParser()
{
	// Free any resources allocated by the parser
	ResetParser();
}

// Since we don't expect any seek operation, we reset the parser 
// so that it's ready to parse data from the file beginning
HRESULT CMVEInnerChunkParser::ResetParser(void)
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

	// Reset subchunk type stuff
	m_bCurrentSubchunkType		= 0xFF;
	m_bCurrentSubchunkSubtype	= 0xFF;

	// Call base-class method to reset parser state
	return CBaseChunkParser::ResetParser();
}

HRESULT CMVEInnerChunkParser::ParseChunkHeader(
	LONGLONG llStartPosition,
	BYTE *pbHeader,
	LONG *plDataSize
)
{
	// Check and validate pointers
	CheckPointer(pbHeader, E_POINTER);
	ValidateReadPtr(pbHeader, sizeof(MVE_CHUNK_HEADER));
	CheckPointer(plDataSize, E_POINTER);
	ValidateWritePtr(plDataSize, sizeof(LONG));

	MVE_CHUNK_HEADER *pHeader = (MVE_CHUNK_HEADER*)pbHeader;
	
	// Set the data size
	*plDataSize = pHeader->cbData;

	m_pPin = NULL; // Pin to send the sample through
	m_rtDelta = 0;
	m_llDelta = 0;

	// Save the subchunk type and subtype for later use
	m_bCurrentSubchunkType		= pHeader->bType;
	m_bCurrentSubchunkSubtype	= pHeader->bSubtype;

	// Recognize subchunk type
	switch (pHeader->bType) {

		// Timer command
		case MVE_SUBCHUNK_TIMER:
		// Palette setting commands
		case MVE_SUBCHUNK_PALETTE_GRAD:
		case MVE_SUBCHUNK_PALETTE:
		case MVE_SUBCHUNK_PALETTE_RLE:
		// Video info command
		case MVE_SUBCHUNK_VIDEOINFO:
		// Video decoding map and data
		case MVE_SUBCHUNK_VIDEOMAP:
		case MVE_SUBCHUNK_VIDEODATA:

			m_pPin = m_ppOutputPin[0];	// We'll be dealing with video output pin
			m_rtDelta = 0;				// No sample deltas for this subchunk
			m_llDelta = 0;				// ----||----
			break;

		// "Send buffer" video command
		case MVE_SUBCHUNK_VIDEOCMD:

			m_pPin = m_ppOutputPin[0];	// We'll be dealing with video output pin
			m_rtDelta = m_rtFrameDelta;
			m_llDelta = 1;				// One frame per sample
			break;

		// Audio data
		case MVE_SUBCHUNK_AUDIODATA:
		case MVE_SUBCHUNK_AUDIOSILENCE:

			ASSERT(m_ppOutputPin[1]);	// There should be the audio output pin
			m_pPin = m_ppOutputPin[1];	// We'll be dealing with audio output pin
			break;

		default:

			// No interest in other subchunk types
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

	// Prepare the sample buffer
	LONG cbDataLength = 0;
	BYTE *pbBuffer = NULL;
	switch (pHeader->bType) {

		// Whatever pertains to video
		case MVE_SUBCHUNK_PALETTE_GRAD:
		case MVE_SUBCHUNK_PALETTE:
		case MVE_SUBCHUNK_PALETTE_RLE:
		case MVE_SUBCHUNK_VIDEOINFO:
		case MVE_SUBCHUNK_VIDEOMAP:
		case MVE_SUBCHUNK_VIDEODATA:
		case MVE_SUBCHUNK_VIDEOCMD:

			// Get the sample's buffer
			hr = m_pSample->GetPointer(&pbBuffer);
			if (FAILED(hr)) {
				m_pSample->Release();
				m_pSample = NULL;
				return hr;
			}

			// Put subchunk type/subtype
			*pbBuffer++ = pHeader->bType;
			*pbBuffer = pHeader->bSubtype;
			cbDataLength = 2;
			break;

		default:

			// No preparation for other subchunk types
			break;
	}

	// Initialize actual data length
	hr = m_pSample->SetActualDataLength(cbDataLength);
	if (FAILED(hr)) {
		m_pSample->Release();
		m_pSample = NULL;
		return hr;
	}

	return NOERROR;
}

HRESULT CMVEInnerChunkParser::ParseChunkData(
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

HRESULT CMVEInnerChunkParser::ParseChunkEnd(void)
{
	// If no sample is supplied there's no need to do anything
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

	// Post-process the sample buffer data (if needed)
	MVE_TIMER_DATA *pTimerData = (MVE_TIMER_DATA*)pbBuffer;
	MVE_AUDIO_HEADER *pAudioHeader = (MVE_AUDIO_HEADER*)pbBuffer;
	switch (m_bCurrentSubchunkType) {

		// Timer command
		case MVE_SUBCHUNK_TIMER:

			// Check if we can read the timer data
			if (m_pSample->GetActualDataLength() < sizeof(MVE_TIMER_DATA)) {
				m_pSample->Release();
				m_pSample = NULL;
				m_pPin = NULL;
				return E_UNEXPECTED;
			}

			// Set up new frame delta
			m_rtFrameDelta = pTimerData->dwRate * pTimerData->wSubdivision * 10;

			// No need to deliver this sample
			m_pSample->Release();
			m_pSample = NULL;
			m_pPin = NULL;
			return NOERROR;

		case MVE_SUBCHUNK_AUDIODATA:
		case MVE_SUBCHUNK_AUDIOSILENCE:

			// Check if we can read the audio header
			if (m_pSample->GetActualDataLength() < sizeof(MVE_AUDIO_HEADER)) {
				m_pSample->Release();
				m_pSample = NULL;
				m_pPin = NULL;
				return E_UNEXPECTED;
			}

			// Check if we have correct wave format parameters
			if (
				(m_nSampleSize			== 0) ||
				(m_nAvgBytesPerSec		== 0) ||
				(m_wCompressionRatio	== 0)
			) {
				m_pSample->Release();
				m_pSample = NULL;
				m_pPin = NULL;
				return E_UNEXPECTED;
			}

			// If it's not the first stream, deliver nothing
			if (!(pAudioHeader->wStreamMask & 1)) {
				m_pSample->Release();
				m_pSample = NULL;
				m_pPin = NULL;
				return NOERROR;
			}

			// Calculate the stream and media deltas
			m_rtDelta	= ((REFERENCE_TIME)pAudioHeader->cbPCMData * UNITS) / m_nAvgBytesPerSec;
			m_llDelta	= (LONGLONG)pAudioHeader->cbPCMData / m_nSampleSize;
			break;

		default:

			// No post-processing for other subchunk type
			break;
	}

	// If it's an audio silence subchunk, fill the buffer with silence
	if (m_bCurrentSubchunkType == MVE_SUBCHUNK_AUDIOSILENCE) {

		// Decide on fill size
		LONG cbFillSize = (m_bIs16Bit)
			? (m_nSampleSize + (pAudioHeader->cbPCMData - m_nSampleSize) / m_wCompressionRatio)
			: pAudioHeader->cbPCMData;

		// Check the buffer size
		if (m_pSample->GetSize() < cbFillSize) {
			m_pSample->Release();
			m_pSample = NULL;
			m_pPin = NULL;
			return VFW_E_BUFFER_OVERFLOW;
		}

		// Fill the buffer
		BYTE bFiller = (m_bIs16Bit) ? 0x00 : 0x80;
		FillMemory(pbBuffer, cbFillSize, bFiller);
		m_pSample->SetActualDataLength(cbFillSize);

	} else if (m_bCurrentSubchunkType == MVE_SUBCHUNK_AUDIODATA) {

		// Move audio data to the buffer beginning
		LONG cbData = m_pSample->GetActualDataLength() - sizeof(MVE_AUDIO_HEADER);
		MoveMemory(pbBuffer, pbBuffer + sizeof(MVE_AUDIO_HEADER), cbData);
		m_pSample->SetActualDataLength(cbData);

	}

	// If we've got media sample there should be the output pin
	ASSERT(m_pPin);

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
// CMVEOuterChunkParser methods
//==========================================================================

CMVEOuterChunkParser::CMVEOuterChunkParser(
	CBaseParserFilter *pFilter,
	CParserOutputPin **ppOutputPin,
	REFERENCE_TIME rtFrameDelta,
	WORD nSampleSize,
	DWORD nAvgBytesPerSec,
	WORD wCompressionRatio,
	BOOL bIs16Bit,
	HRESULT *phr
) :
	CBaseChunkParser(
		NAME("MVE Outer Chunk Parser"),
		pFilter,
		ppOutputPin,
		sizeof(MVE_CHUNK_HEADER),
		1,
		phr
	),
	m_InnerParser(
		pFilter,
		ppOutputPin,
		rtFrameDelta,
		nSampleSize,
		nAvgBytesPerSec,
		wCompressionRatio,
		bIs16Bit,
		phr
	)
{
}

CMVEOuterChunkParser::~CMVEOuterChunkParser()
{
	// Reset and shutdown the parser
	ResetParser();
}

HRESULT CMVEOuterChunkParser::ResetParser(void)
{
	CAutoLock lock(&m_csLock);

	// Call the base-class method to reset parser state
	HRESULT hr = CBaseChunkParser::ResetParser();
	if (FAILED(hr))
		return hr;

	// Delegate the call to the nested one
	return m_InnerParser.ResetParser();
}

HRESULT CMVEOuterChunkParser::ParseChunkHeader(
	LONGLONG llStartPosition,
	BYTE *pbHeader,
	LONG *plDataSize
)
{
	// Check and validate pointers
	CheckPointer(pbHeader, E_POINTER);
	ValidateReadPtr(pbHeader, sizeof(MVE_CHUNK_HEADER));
	CheckPointer(plDataSize, E_POINTER);
	ValidateWritePtr(plDataSize, sizeof(LONG));

	MVE_CHUNK_HEADER *pHeader = (MVE_CHUNK_HEADER*)pbHeader;
	
	// Set the data size
	*plDataSize = pHeader->cbData;

	// Reset the nested parser so that it's clean and ready to 
	// parse forthcoming group of nested chunks
	HRESULT hr = m_InnerParser.ResetParser();
	if (FAILED(hr))
		return hr;

	return NOERROR;
}

HRESULT CMVEOuterChunkParser::ParseChunkData(
	LONGLONG llStartPosition,
	BYTE *pbData,
	LONG lDataSize
)
{
	// Pass the data through to the nested parser
	return m_InnerParser.Receive(llStartPosition, pbData, lDataSize);
}

HRESULT CMVEOuterChunkParser::ParseChunkEnd(void)
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
// CMVESplitterFilter methods
//==========================================================================

IMPLEMENT_FILETYPE(CMVESplitterFilter)

CMVESplitterFilter::CMVESplitterFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseParserFilter(
		NAME("MVE Splitter Filter"),
		pUnk,
		CLSID_MVESplitter,
		g_wszMVESplitterName,
		phr
	),
	m_rtFrameDelta(0),			// No frame delta at this time
	m_nSampleSize(0),			// No audio sample size at this time
	m_nAvgBytesPerSec(0),		// No audio data rate at this time
	m_wCompressionRatio(0),		// No audio compression ratio at this time
	m_bIs16Bit(TRUE),			// Default is 16-bit
	m_pParser(NULL)				// No chunk parser at this time
{
	ASSERT(phr);
}

CMVESplitterFilter::~CMVESplitterFilter()
{
	// Free any resources allocated by Initialize() or InitializeParser()
	Shutdown();
}

CUnknown* WINAPI CMVESplitterFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CMVESplitterFilter(pUnk, phr);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pObject;
}

HRESULT CMVESplitterFilter::Initialize(IPin *pPin, IAsyncReader *pReader)
{
	// Check and validate the pointer
	CheckPointer(pReader, E_POINTER);
	ValidateReadPtr(pReader, sizeof(IAsyncReader));

	// Read file header
	MVE_HEADER videohdr;
	HRESULT hr = pReader->SyncRead(0, sizeof(videohdr), (BYTE*)&videohdr);
	if (hr != S_OK)
		return hr;

	// Verify file header
	if (
		(memcmp(videohdr.szID, MVE_IDSTR_MVE, strlen(MVE_IDSTR_MVE))) ||
		(videohdr.wMagic1 != MVE_ID_MAGIC1) ||
		(videohdr.wMagic2 != MVE_ID_MAGIC2) ||
		(videohdr.wMagic3 != MVE_ID_MAGIC3)
	)
		return VFW_E_INVALID_FILE_FORMAT;

	// Scan the chunks for video and audio info
	LONGLONG llSeekPos = sizeof(MVE_HEADER);
	BOOL bFoundVideoModeInfo = FALSE;
	MVE_VIDEO_MODE_INFO videomodeinfo = {0};
	BOOL bFoundVideoInfo = FALSE;
	MVE_VIDEO_INFO videoinfo = {0};
	BOOL bFoundAudioInfo = FALSE;
	MVE_AUDIO_INFO audioinfo = {0};
	BOOL bIsAudioCompressed = TRUE;
	BOOL bFoundTimerData = FALSE;
	MVE_TIMER_DATA timerdata = {0};
	int iFirstVideoFrame = -1;
	int iAudioData = 0;
	WORD cbAudioData[2] = { 0, 0 };
	for (int i = 0; i < MVE_CHUNKS_TO_SCAN; i++) {

		// Read chunk header
		MVE_CHUNK_HEADER chunkheader = {0};
		hr = pReader->SyncRead(llSeekPos, sizeof(chunkheader), (BYTE*)&chunkheader);
		if (hr != S_OK)
			break;

		llSeekPos += sizeof(chunkheader);

		// Walk subchunks of this chunk
		LONGLONG llSubchunkSeekPos = llSeekPos;
		while (llSubchunkSeekPos < llSeekPos + chunkheader.cbData) {

			// Read subchunk header
			MVE_CHUNK_HEADER subchunkheader = {0};
			hr = pReader->SyncRead(llSubchunkSeekPos, sizeof(subchunkheader), (BYTE*)&subchunkheader);
			if (hr != S_OK)
				break;

			llSubchunkSeekPos += sizeof(subchunkheader);

			// Check the subchunk type
			switch (subchunkheader.bType) {

				case MVE_SUBCHUNK_TIMER:
					pReader->SyncRead(llSubchunkSeekPos, subchunkheader.cbData, (BYTE*)&timerdata);
					if (!bFoundTimerData)
						iFirstVideoFrame = i;
					bFoundTimerData = TRUE;
					break;

				case MVE_SUBCHUNK_AUDIOINFO:
					pReader->SyncRead(llSubchunkSeekPos, subchunkheader.cbData, (BYTE*)&audioinfo);
					bIsAudioCompressed = (subchunkheader.bSubtype >= 1) && (audioinfo.wFlags & MVE_AUDIO_COMPRESSED);
					bFoundAudioInfo = TRUE;
					break;

				case MVE_SUBCHUNK_VIDEOMODE:
					pReader->SyncRead(llSubchunkSeekPos, subchunkheader.cbData, (BYTE*)&videomodeinfo);
					bFoundVideoModeInfo = TRUE;
					break;

				case MVE_SUBCHUNK_VIDEOINFO:
					pReader->SyncRead(llSubchunkSeekPos, subchunkheader.cbData, (BYTE*)&videoinfo);
					bFoundVideoInfo = TRUE;
					break;

				case MVE_SUBCHUNK_AUDIODATA:
					if (iAudioData < 2)
						cbAudioData[iAudioData++] = subchunkheader.cbData;
					break;
			}

			// Advance seek position
			llSubchunkSeekPos += subchunkheader.cbData;
		}

		// If we've got all necessary information, quit
		if (
			(bFoundVideoModeInfo)	&&
			(bFoundVideoInfo)		&&
			(bFoundAudioInfo)		&&
			(bFoundTimerData)
		)
			break;

		// Advance seek position
		llSeekPos += chunkheader.cbData;
	}

	// Check if we have video info
	if ((!bFoundVideoInfo) || (!bFoundVideoModeInfo))
		return VFW_E_INVALID_FILE_FORMAT;

	// Protect the filter data
	CAutoLock datalock(&m_csData);

	// Check if we have timer data
	m_rtFrameDelta = (bFoundTimerData)
					? (timerdata.dwRate * timerdata.wSubdivision * 10)
					: MVE_FRAME_DELTA_DEFAULT;

	// Set file positions
	m_llDefaultStart	= (LONGLONG)sizeof(MVE_HEADER);	// Right after the header
	m_llDefaultStop		= MAXLONGLONG; // Defaults to file end

	// Decide on the input pin properties
	m_cbInputAlign	= 1;
	m_cbInputBuffer	= 0x40000; // TEMP: some reasonably large value

	// Protect the output pins state
	CAutoLock pinlock(&m_csPins);

	// Decide on the output pins count
	m_nOutputPins = 1;		// Video always should be present in the file
	if (bFoundAudioInfo)
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
	pmtVideo->SetSubtype(&MEDIASUBTYPE_MVEVideo);
	pmtVideo->SetSampleSize(0);				// Variable size samples
	pmtVideo->SetTemporalCompression(TRUE);	// Using temporal compression
	pmtVideo->SetFormatType(&FORMAT_MVEVideo);
	BYTE *pbFormat = pmtVideo->AllocFormatBuffer(
		sizeof(videoinfo) +
		sizeof(videomodeinfo) +
		((bFoundTimerData) ? sizeof(timerdata) : 0)
	);
	if (pbFormat == NULL) {
		delete pmtVideo;
		Shutdown();
		return E_FAIL;
	}
	CopyMemory(pbFormat, &videoinfo, sizeof(videoinfo));
	pbFormat += sizeof(videoinfo);
	CopyMemory(pbFormat, &videomodeinfo, sizeof(videomodeinfo));
	pbFormat += sizeof(videomodeinfo);
	if (bFoundTimerData)
		CopyMemory(pbFormat, &timerdata, sizeof(timerdata));

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
	papVideo->cbBuffer	= max(
		2 + 4 + 800,											// Type + subtype + palette header + full (RLE) palette
		max(
			2 + videoinfo.wWidth * videoinfo.wHeight / 2,		// Type + subtype + decode map size
			2 + 2 + videoinfo.wWidth * videoinfo.wHeight * 128	// Type + subtype + offset + raw hi-color pixel data
		)
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
		NAME("MVE Splitter Video Output Pin"),
		this,
		&m_csFilter,
		pmtVideo,
		papVideo,
		dwVideoCapabilities,
		nVideoTimeFormats,
		pVideoTimeFormats,
		&hr,
		wszMVEVideoOutputName
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

	// Protect the filter options
	CAutoLock optionlock(&m_csOptions);

	// Create audio output pin if we have to
	if (bFoundAudioInfo) {

		// Allocate audio media type
		CMediaType *pmtAudio = new CMediaType();
		if (pmtAudio == NULL) {
			Shutdown();
			return E_OUTOFMEMORY;
		}

		// Set the wave format parameters needed for the calculation
		// of sample stream and media duration
		m_nSampleSize		= ((audioinfo.wFlags & MVE_AUDIO_16BIT) ? 2 : 1)
							* ((audioinfo.wFlags & MVE_AUDIO_STEREO) ? 2 : 1);
		m_nAvgBytesPerSec	= m_nSampleSize * audioinfo.wSampleRate;
		m_wCompressionRatio	= (bIsAudioCompressed) ? 2 : 1;
		m_bIs16Bit			= audioinfo.wFlags & MVE_AUDIO_16BIT;

		// Initialize the audio media type
		pmtAudio->InitMediaType();
		pmtAudio->SetType(&MEDIATYPE_Audio);
		pmtAudio->SetSubtype(
			(bIsAudioCompressed)
			? &MEDIASUBTYPE_MVEADPCM
			: &MEDIASUBTYPE_PCM
		);
		pmtAudio->SetSampleSize(m_nSampleSize / m_wCompressionRatio);
		pmtAudio->SetTemporalCompression(bIsAudioCompressed);
		pmtAudio->SetFormatType(
			(bIsAudioCompressed)
			? &FORMAT_MVEADPCM
			: &FORMAT_WaveFormatEx
		);
		if (bIsAudioCompressed) {

			if (!pmtAudio->SetFormat((BYTE*)&audioinfo, sizeof(audioinfo))) {
				delete pmtAudio;
				Shutdown();
				return E_FAIL;
			}

		} else {

			WAVEFORMATEX *pFormat = (WAVEFORMATEX*)pmtAudio->AllocFormatBuffer(sizeof(WAVEFORMATEX));
			if (pFormat == NULL) {
				delete pmtAudio;
				Shutdown();
				return E_OUTOFMEMORY;
			}

			pFormat->wFormatTag			= WAVE_FORMAT_PCM;
			pFormat->nChannels			= (audioinfo.wFlags & MVE_AUDIO_STEREO) ? 2 : 1;
			pFormat->nSamplesPerSec		= audioinfo.wSampleRate;
			pFormat->nAvgBytesPerSec	= m_nAvgBytesPerSec;
			pFormat->nBlockAlign		= m_nSampleSize;
			pFormat->wBitsPerSample		= (audioinfo.wFlags & MVE_AUDIO_16BIT) ? 16 : 8;
			pFormat->cbSize				= 0;

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
		papAudio->cBuffers	= (iFirstVideoFrame != -1) ? (iFirstVideoFrame + 1) : 32;	// All pre-video audio chunks plus one
		papAudio->cbBuffer	= max(cbAudioData[0], cbAudioData[1]);
		if (papAudio->cbBuffer == 0)
			papAudio->cbBuffer = 0x10000;

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
			NAME("MVE Splitter Audio Output Pin"),
			this,
			&m_csFilter,
			pmtAudio,
			papAudio,
			dwAudioCapabilities,
			nAudioTimeFormats,
			pAudioTimeFormats,
			&hr,
			wszMVEAudioOutputName
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
		m_wszAuthorName = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszMVEAuthorName) + 1));
		if (m_wszAuthorName)
			lstrcpyW(m_wszAuthorName, wszMVEAuthorName);
		m_wszDescription = (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR) * (lstrlenW(wszMVEDescription) + 1));
		if (m_wszDescription)
			lstrcpyW(m_wszDescription, wszMVEDescription);
	}

	return NOERROR;
}

HRESULT CMVESplitterFilter::Shutdown(void)
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
		m_rtFrameDelta		= 0;
		m_nSampleSize		= 0;
		m_nAvgBytesPerSec	= 0;
		m_wCompressionRatio	= 0;
		m_bIs16Bit			= TRUE;
	}

	// Call the base-class implementation
	return CBaseParserFilter::Shutdown();
}

HRESULT CMVESplitterFilter::InitializeParser(void)
{
	// Protect the filter data
	CAutoLock datalock(&m_csData);

	HRESULT hr = NOERROR;

	// Create chunk parser if we have to
	if (m_pParser == NULL) {

		m_pParser = new CMVEOuterChunkParser(
			this,
			m_ppOutputPin,
			m_rtFrameDelta,
			m_nSampleSize,
			m_nAvgBytesPerSec,
			m_wCompressionRatio,
			m_bIs16Bit,
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

HRESULT CMVESplitterFilter::ShutdownParser(void)
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

HRESULT CMVESplitterFilter::ResetParser(void)
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

HRESULT CMVESplitterFilter::CheckInputType(const CMediaType* pmt)
{
	// Check and validate the pointer
	CheckPointer(pmt, E_POINTER);
	ValidateReadPtr(pmt, sizeof(CMediaType));

	if (
		(IsEqualGUID(*pmt->Type(),		MEDIATYPE_Stream)) &&	// We accept byte stream
		(IsEqualGUID(*pmt->Subtype(),	MEDIASUBTYPE_MVE))		// ... from MVE file
	)
		return S_OK;
	else
		return S_FALSE;
}

HRESULT CMVESplitterFilter::GetInputType(int iPosition, CMediaType *pMediaType)
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
		pMediaType->SetSubtype(&MEDIASUBTYPE_MVE);	// From MVE file
		pMediaType->SetVariableSize();				// Variable size samples
		pMediaType->SetTemporalCompression(FALSE);	// No temporal compression
		pMediaType->SetFormatType(&FORMAT_None);	// Is it needed ???
	}

	return S_OK;
}
	
HRESULT CMVESplitterFilter::Receive(
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

HRESULT CMVESplitterFilter::ConvertTimeFormat(
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

	if (!lstrcmpW(pPinName, wszMVEVideoOutputName)) {

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
			*pTarget = llSource * m_rtFrameDelta;
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
			*pTarget = llSource / m_rtFrameDelta;
			return NOERROR;
		}

		// If the source/target formats pair does not match one of the 
		// above, we cannot perform convertion
		return E_INVALIDARG;

	} else if (!lstrcmpW(pPinName, wszMVEAudioOutputName)) {

		// Process the audio output pin's request

		// Protect the filter data
		CAutoLock datalock(&m_csData);

		// Check if we have correct wave format parameters
		if (
			(m_nSampleSize			== 0) ||
			(m_nAvgBytesPerSec		== 0) ||
			(m_wCompressionRatio	== 0)
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

STDMETHODIMP CMVESplitterFilter::GetPages(CAUUID *pPages)
{
	// Check and validate the pointer
	CheckPointer(pPages, E_POINTER);
	ValidateWritePtr(pPages, sizeof(CAUUID));
	
	// Fill in the counted array structure
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;
	pPages->pElems[0] = CLSID_MVESplitterPage;
	pPages->pElems[1] = CLSID_BaseParserPage;

	return NOERROR;
}

//==========================================================================
// CMVESplitterPage methods
//==========================================================================

const WCHAR g_wszMVESplitterPageName[] = L"ANX MVE Splitter Property Page";

CMVESplitterPage::CMVESplitterPage(LPUNKNOWN pUnk) :
	CBasePropertyPage(
		NAME("MVE Splitter Property Page"),
		pUnk,
		IDD_MVESPLITTERPAGE,
		IDS_TITLE_MVESPLITTERPAGE
	)
{
}

CUnknown* WINAPI CMVESplitterPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CUnknown* pObject = new CMVESplitterPage(pUnk);
	if (pObject == NULL)
		*phr = E_OUTOFMEMORY;
	else
		*phr = NOERROR;
	return pObject;
}
