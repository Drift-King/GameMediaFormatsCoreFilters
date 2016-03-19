//==========================================================================
//
// File: BaseParser.h
//
// Desc: Game Media Formats - Header file for generic parser filter
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

#ifndef __GMF_BASE_PARSER_H__
#define __GMF_BASE_PARSER_H__

#include <streams.h>
#include <qnetwork.h>
#include <pullpin.h>
#include <tchar.h>

#include "BaseParserConfig.h"

class CParserInputPin;

//==========================================================================
// Helper class for pulling input pin
//
// Pulls data from peer's IAsyncReader and delegates streaming 
// methods to parent input pin
//==========================================================================

class CParserPullPin : public CPullPin 
{

public:

	CParserPullPin(CParserInputPin *pParentPin);

	// Implement pure methods (delegate calls to parent input pin)
	HRESULT Receive(IMediaSample *pSample);
	HRESULT BeginFlush(void);
	HRESULT EndFlush(void);
	HRESULT EndOfStream(void);

	void OnError(HRESULT hr);

protected:

	friend class CParserInputPin;

	CParserInputPin *m_pParentPin; // Parent pin using this pullpin
};

class CBaseParserFilter;

//==========================================================================
// Parser input pin class
// 
// Uses CParserPullPin to pull data from the upstream output pin,
// delegates Receive() to the parent filter and delivers streaming 
// messages to all downstream output pins
//==========================================================================

class CParserInputPin : public CBaseInputPin 
{

public:

	CParserInputPin(
		TCHAR *pObjectName,			// Object name
		CBaseParserFilter *pFilter,	// Parent filter
		CCritSec *pFilterLock,		// Critical section protecting filter state
		CCritSec *pReceiveLock,		// Critical section protecting streaming state
		HRESULT *phr,				// Success or error code
		LPCWSTR pName				// Pin name
	);

	// Overridden to handle pullpin connection/disconnection
	HRESULT CheckConnect(IPin *pPin);
	HRESULT BreakConnect(void);

	// Overridden to handle pullpin activation/deactivation
	HRESULT Active(void);
	HRESULT Inactive(void);	

	// Media type stuff
	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// Implements Receive() method (delegates call to the parent filter)
	STDMETHODIMP Receive(IMediaSample *pSample);

	// Set the start and stop positions for the pullpin
	HRESULT Seek(LONGLONG llStart, LONGLONG llStop);

	// Pass the calls to the parent filter
	STDMETHODIMP BeginFlush(void);
	STDMETHODIMP EndFlush(void);
	STDMETHODIMP NewSegment(
		REFERENCE_TIME tStart, 
		REFERENCE_TIME tStop, 
		double dRate
	);
	STDMETHODIMP EndOfStream(void);

	// Retrieves IAsyncReader from the contained pullpin
	IAsyncReader* GetReader(void) { return m_PullPin.GetReader(); };

protected:

	CCritSec *m_pReceiveLock;	// Critical section protecting streaming state

	CParserPullPin m_PullPin;	// Pullpin used by this input pin
};

class CParserOutputPin;

//==========================================================================
// Parser media seeking class
//
// Helper class which provides IMediaSeeking for output pins of 
// parser filter. In parser filter's Initialize() method create an 
// instance of this class for each output pin (with the exception of 
// pass-through output pins, which have pass-through seeking objects) 
// and pass it to the constructor of the respective output pin.
// Note that this class does not support IMediaPosition -- calls to 
// its methods should be handled by the filter graph manager using 
// methods provided by this class
//==========================================================================

class CParserSeeking :	public CUnknown,
						public IMediaSeeking
{

public:

	CParserSeeking(
		const TCHAR *pName,			// Object name
		LPUNKNOWN pUnk,				// Controlling IUnknown
		CBaseParserFilter *pFilter,	// Parent filter
		CParserOutputPin *pPin,		// Parent output pin
		DWORD dwCapabilities,		// Seeking capabilities
		int nTimeFormats,			// Number of supported time formats
		GUID *pTimeFormats			// Array of supported time formats
	);
	virtual ~CParserSeeking();

	DECLARE_IUNKNOWN

	// Overridden to expose IMediaSeeking
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// ---- IMediaSeeking methods ----
	STDMETHODIMP GetCapabilities(DWORD *pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD *pCapabilities);
	STDMETHODIMP SetTimeFormat(const GUID *pFormat);
	STDMETHODIMP GetTimeFormat(GUID *pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat);
	STDMETHODIMP IsFormatSupported(const GUID *pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
	STDMETHODIMP ConvertTimeFormat(
		LONGLONG *pTarget,
		const GUID *pTargetFormat,
		LONGLONG llSource,
		const GUID *pSourceFormat
	);
	STDMETHODIMP SetPositions(
		LONGLONG *pllCurrent,
		DWORD dwCurrentFlags,
		LONGLONG *pllStop,
		DWORD dwStopFlags
	);
	STDMETHODIMP GetPositions(LONGLONG *pCurrent, LONGLONG *pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
	STDMETHODIMP GetStopPosition(LONGLONG *pStop);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double *pdRate);
	STDMETHODIMP GetDuration(LONGLONG *pDuration);
	STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
	STDMETHODIMP GetPreroll(LONGLONG *pllPreroll);

	// This method is used internally by the containing output pin 
	// to advance the current position (called after successful 
	// Deliver()). The delta argument is the delivered media sample 
	// duration expressed in samples in the stream (TIME_FORMAT_SAMPLE) 
	// -- the method converts the value to the current time format
	HRESULT AdvanceCurrentPosition(LONGLONG llDelta);

	// Reset the seeker to the default position parameters
	void Reset(void);

	// Retrieve the current segment parameters (the rate and the 
	// start/stop times in reference units)
	HRESULT GetSegmentParameters(
		REFERENCE_TIME *prtStart,
		REFERENCE_TIME *prtStop,
		double *pdRate
	);

protected:

	CCritSec m_csLock;				// Critical section protecting shared data

	CBaseParserFilter *m_pFilter;	// Parent filter
	CParserOutputPin *m_pPin;		// Parent output pin

	// Seeking stuff
	DWORD m_dwCapabilities;			// Seeking capabilities
	int m_nTimeFormats;				// Number of supported time formats
	GUID *m_pTimeFormats;			// Array of supported time formats
	int m_iTimeFormat;				// Index of the current time format
	LONGLONG m_llCurrentPosition;	// Current position
	LONGLONG m_llStopPosition;		// Stop position

};

//==========================================================================
// Parser output pin class
// 
// Delivers media samples downstream (using COutputQueue).
// Media type is assigned by the parent filter at construction time
// and never changes during the lifetime of the pin
//==========================================================================

class CParserOutputPin : public CBaseOutputPin
{

public:

	CParserOutputPin(
		TCHAR *pObjectName,			// Object name
		CBaseParserFilter *pFilter,	// Parent filter
		CCritSec *pFilterLock,		// Critical section protecting filter state
		CMediaType *pmt,			// Media type assigned to the pin
		ALLOCATOR_PROPERTIES *pap,	// Allocator properties for the pin
		DWORD dwCapabilities,		// Seeker capabilities
		int nTimeFormats,			// Number of time formats supported by the seeker
		GUID *pTimeFormats,			// Array of time formats supported by the seeker
		HRESULT *phr,				// Success or error code
		LPCWSTR pName				// Pin name
	);
	virtual ~CParserOutputPin();

	DECLARE_IUNKNOWN

	// Overridden to expose IMediaSeeking
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// Overridden since the lifetimeû of pins and filters are not the same
	STDMETHODIMP_(ULONG) NonDelegatingAddRef(void);
	STDMETHODIMP_(ULONG) NonDelegatingRelease(void); 

	// Overridden to support quality control
	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);

	// Media type stuff
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// Decides the allocator properties
	HRESULT DecideBufferSize(
		IMemAllocator *pMemAllocator, 
		ALLOCATOR_PROPERTIES *pRequest
	);

	// Overridden to create/delete output queue object
	HRESULT Active(void);
	HRESULT Inactive(void);

	// Public delivery method. Requires sample durations in stream 
	// and media times to be passed as arguments. This method sets 
	// the stream and media times on the sample, so it's no use to 
	// set the times prematurely. If you need to implement different 
	// time setting scheme, just set the stream and media times 
	// directly on the output pin (using SetTime() and SetMediaTime()).
	// Note that you can alternatively use regular Deliver() which
	// does not set times on the sample, but it also does not change 
	// sample playback rate, so be sure to call this version of 
	// Deliver() to keep the filter responsive to the rate changes
	HRESULT Deliver(
		IMediaSample *pMediaSample,
		REFERENCE_TIME rtDelta,
		LONGLONG llDelta
	);

	// Overridden to pass data to the output queue
	HRESULT Deliver(IMediaSample *pMediaSample);
	HRESULT DeliverEndOfStream(void);
	HRESULT DeliverBeginFlush(void);
	HRESULT DeliverEndFlush(void);
	HRESULT DeliverNewSegment(
		REFERENCE_TIME tStart,
		REFERENCE_TIME tStop,
		double dRate
	);

	// Performs Disconnect() on both connected pins
	HRESULT DisconnectBoth(void);

	// Methods to set/get stream time
	void SetTime(REFERENCE_TIME rtStreamTime) { m_rtStreamTime = rtStreamTime; };
	REFERENCE_TIME GetTime(void) { return m_rtStreamTime; };

	// Methods to set/get media time
	void SetMediaTime(LONGLONG llMediaTime) { m_llMediaTime = llMediaTime; };
	LONGLONG GetMediaTime(void) { return m_llMediaTime; };

	// Methods to set and get a discontinuity property
	void SetDiscontinuity(BOOL bDiscontinuity) { m_bIsDiscontinuity = bDiscontinuity; };
	BOOL IsDiscontinuity(void) { return m_bIsDiscontinuity; };

	// Overridden to set default sample properties:
	// (1) set syncpoint to TRUE for all samples if 
	// bTemporalCompression is FALSE, otherwise set syncpoint 
	// to TRUE only for the first sample (and FALSE for all 
	// further samples);
	// (2) set preroll property to FALSE for all samples;
	// (3) set the discontinuity property to TRUE for the first sample
	// and FALSE for all further samples (seeking and quality control 
	// methods can also set this property at the appropriate times).
	HRESULT GetDeliveryBuffer(
		IMediaSample **ppSample,
		REFERENCE_TIME *pStartTime,
		REFERENCE_TIME *pEndTime,
		DWORD dwFlags
	);

	// Pin options access methods
	long get_BuffersNumber(void);
	BOOL get_AutoQueue(void);
	BOOL get_Queue(void);
	LONG get_BatchSize(void);
	BOOL get_BatchExact(void);
	void put_BuffersNumber(long nBuffers);
	void put_AutoQueue(BOOL bAutoQueue);
	void put_Queue(BOOL bQueue);
	void put_BatchSize(LONG lBatchSize);
	void put_BatchExact(BOOL bBatchExact);

protected:

	// Pin options
	BOOL m_bAutoQueue;		// Output queue is created only if there's a need
	BOOL m_bQueue;			// Create output queue (if not auto)
	LONG m_lBatchSize;		// Batch size for output queue
	BOOL m_bBatchExact;		// Use exact batch size

	CCritSec m_csOptions;	// Option values protection

	// Current stream and media time
	REFERENCE_TIME	m_rtStreamTime;	// Stream time
	LONGLONG		m_llMediaTime;	// Media time

	// Is the next sample a discontinuity?
	BOOL m_bIsDiscontinuity;

	// Media type assigned to the pin
	CMediaType *m_pMediaType;

	// Allocator properties for the pin
	ALLOCATOR_PROPERTIES *m_pAllocatorProperties;

	// Parser seeker of the pin
	CParserSeeking *m_pParserSeeking;

	// Streams data to the peer pin
	COutputQueue *m_pOutputQueue;

};

class CPassThruInputPin;

//==========================================================================
// Pass-through output pin class
// 
// Helper output pin for splitters. Media type and allocator are  
// assigned by the parent input pin which also calls media delivery 
// methods. See comments to the pass-through input pin class for 
// an example of pass-through pins usage
//==========================================================================

class CPassThruOutputPin : public CBaseOutputPin
{

public:

	CPassThruOutputPin(
		CBaseFilter *pFilter,			// Parent filter
		CPassThruInputPin *pParentPin,	// Parent input pin
		CCritSec *pFilterLock,			// Critical section protecting filter state
		HRESULT *phr,					// Success or error code
		LPCWSTR pName					// Pin name
	);
	virtual ~CPassThruOutputPin();

	DECLARE_IUNKNOWN

	// Overridden to expose IMediaSeeking
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppvoid);

	// Overridden to support quality control
	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);

	// Accept and report only one media type (assigned by the inpit pin)
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// Refuse connection if the input pin is not connected
	HRESULT CheckConnect(IPin *pPin);

	// Overridden to propose our allocator to the peer pin
	HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **pAlloc);

	// Empty method
	HRESULT DecideBufferSize(IMemAllocator *pMemAllocator, ALLOCATOR_PROPERTIES *pRequest);

	// Disconnect both pins (this and the peer pin)
	HRESULT DisconnectBoth(void);

protected:

	CPassThruInputPin *m_pParentPin; // Parent pass-through input pin

	IUnknown *m_pPosition; // Pass-through media seeking and position

};

//==========================================================================
// Pass-through input pin class
// 
// Helper input pin for splitters. Creates a companion pass-through  
// output pin, then passes media samples through to that pin.  
// Pass-through pins use the same media type and allocator (provided 
// by the upstream pin) and are created and destroyed together.
// Pass-through pins can be used for external soundtrack which is 
// handled by separate source filter, proper parser filter the output
// of which we can direct to pass-through input pin, so that both 
// audio and video streams will flow from the splitter output pins
//==========================================================================

class CPassThruInputPin : public CBaseInputPin 
{

public:

	CPassThruInputPin(
		CBaseFilter *pFilter,				// Parent filter
		CCritSec *pFilterLock,				// Critical section protecting filter state
		CCritSec *pReceiveLock,				// Critical section protecting streaming state
		HRESULT *phr,						// Success or error code
		LPCWSTR pInputName,					// Input pin name
		LPCWSTR pOutputName,				// Output pin name
		CPassThruOutputPin **ppOutputPin	// Output pin created as a by-product
	);

	DECLARE_IUNKNOWN

	// Overridden as the lifetimes of pin and filter are not the same
	STDMETHODIMP_(ULONG) NonDelegatingAddRef(void);
	STDMETHODIMP_(ULONG) NonDelegatingRelease(void); 

	// Just accept any type
	HRESULT CheckMediaType(const CMediaType* pmt);

	// Break the output pin connection
	HRESULT BreakConnect(void);

	// Performs Disconnect() on both connected pins
	HRESULT DisconnectBoth(void);
	
	// Flushing is delegated to the output pin
	STDMETHODIMP BeginFlush(void);
	STDMETHODIMP EndFlush(void);

	// Streaming methods are delegated to the output pin
	STDMETHODIMP Receive(IMediaSample *pSample);
	STDMETHODIMP NewSegment(
		REFERENCE_TIME tStart, 
		REFERENCE_TIME tStop, 
		double dRate
	);
	STDMETHODIMP EndOfStream(void);

protected:

	CCritSec *m_pReceiveLock;		// Critical section protecting streaming state

	CPassThruOutputPin m_OutputPin;	// Output pin for this pin

};

//==========================================================================
// Base chunk parser class
// 
// A helper class to parse chunkwisely-organized formats. A chunk
// consists of a header (fixed-size structure from which the chunk size 
// could be extracted) immediately followed by data.
// To parse the group of chunks we should know the following:
// (1) size of chunk header (should be constant!)
// (2) alignment of the chunk beginning
// (3) starting position of chunk group (used to maintain alignment)
// While the first 2 parameters should be set when constructing the 
// object, the last one may be set several times for the same object 
// when we're parsing nested chunks.
// 
// To use this class you should derive your class from this one and 
// implement pure virtual methods ParseChunkXXXX() in the derived class
// then include an object of the derived class in your parser filter
// and call its Receive() method from within filter's Receive().
// To implement nested chunks parsing you should nest parsers so that 
// the outer parser calls Receive() method of the inner parser from  
// within its ParseChunkData().
//
// Note that chunk parser need not (in general) be aware of any seek
// operations, since after correct seeking it should be ready to accept 
// incoming data from containing filter and that data should be parseable 
// right from the seeking point. In case the parser maintains variables 
// which may be rendered incorrect by the seeking, you should supply
// appropriate ResetParser() (it's called during flush operation)
//==========================================================================

class CBaseChunkParser : public CBaseObject
{

public:

	CBaseChunkParser(
		TCHAR *pName,						// Object name
		CBaseParserFilter *pFilter,			// Parent filter
		CParserOutputPin **ppOutputPin,	// Output pins array
		LONG lHeaderSize,					// Size of the chunk header 
		LONG lAlignment,					// Alignment of the chunk beginning
		HRESULT *phr						// Success or error code
	);
	virtual ~CBaseChunkParser();

	// Cleanup whatever temporary objects parser has obtained during
	// the parsing process and reset the parser state -- this is used 
	// during initialization, flush and may be used in any other method 
	// which requires the parser to be clean for the forthcoming data.
	// Note that you MUST call this in ParseChunkHeader() of outer
	// parser (when parsing nested chunks) for inner parser object
	// in order to reset the inner parser state at the start of outer 
	// chunk -- it's possible that previous outer chunk contained 
	// incomplete inner chunk at the end, this would confuse inner 
	// parser unless it's reset at the beginning of next outer chunk.
	// Be sure to call base-class method in the derived class -- 
	// it resets base-class members responsible for the parser state
	virtual HRESULT ResetParser(void);

	// Receive the data and parse it. Parsing proceeds as follows: 
	// (1) receive header completely and call ParseChunkHeader()
	// (2) receive data portion and call ParseChunkData()
	// (3) when the chunk is over go back to the first step
	virtual HRESULT Receive(
		LONGLONG llStartPosition,	// Data start position
		BYTE *pbData,				// Data buffer
		LONG lDataSize				// Size of data in the buffer
	);

protected:

	CBaseParserFilter *m_pFilter;		// Parent filter
	CParserOutputPin **m_ppOutputPin;	// Output pins array

	CCritSec m_csLock;	// Critical section protecting parser state

	enum EParserState { 
		CPS_CHUNK_COMPLETE,		// We've got complete chunk:
								// ready to parse the next one
		CPS_HEADER_PARTIAL,		// We've got partial header:
								// waiting to receive the rest of header and all the data
		CPS_HEADER_COMPLETE,	// We've got complete header:
								// receiving data
	};

	// Parser configuration data (set once for an object in its constructor)
	LONG m_lHeaderSize;			// Size of the chunk header 
	LONG m_lAlignment;			// Alignment of the chunk beginning

	// Parser state
	EParserState m_State;		// Current state of the parser

	// Storage for incompletely-received data
	BYTE *m_pbHeader;			// Current (incomplete) chunk header
	LONG m_lHeaderLength;		// Actual length of header info stored in m_pbHeader
	LONG m_lChunkDataSize;		// Size of the chunk data following the header

	// Parse chunk header (called from within Receive()) ---- PURE
	// In a case of simple (non-nested) chunk the typical 
	// implementation would be to determine the type of data stored 
	// in chunk, choose the output pin appropriate for that type 
	// and get empty media sample from that pin's allocator.
	// When parsing nested chunk the implementation MUST call
	// ResetParser() of the nested (contained) parser object.
	// Note that this method MUST fill the variable pointed to by
	// plDataSize by the size of chunk data following the header
	// (that size does NOT include the header size, of course)
	virtual HRESULT ParseChunkHeader(
		LONGLONG llStartPosition,	// Header start position
		BYTE *pbHeader,				// Header buffer
		LONG *plDataSize			// Size of chunk data following the header
	) PURE;

	// Parse chunk data (called from within Receive()) ---- PURE
	// In a case of simple (non-nested) chunk the typical 
	// implementation would copy data to media sample's buffer.
	// When parsing nested chunk the implementation most likely calls
	// Receive() of the nested (contained) parser object
	virtual HRESULT ParseChunkData(
		LONGLONG llStartPosition,	// Data start position
		BYTE *pbData,				// Data buffer
		LONG lDataSize				// Size of data in the buffer
	) PURE;

	// Parse chunk end (called from within Receive()) ---- PURE
	// In a case of simple (non-nested) chunk the typical 
	// implementation would deliver media sample downstream.
	// When parsing nested chunk the implementation most likely calls
	// ParseChunkEnd() of the nested (contained) parser object
	virtual HRESULT ParseChunkEnd(void) PURE;

};

//==========================================================================
// Patterns and extensions registration stuff
//==========================================================================

typedef struct tagGMF_FILETYPE_PATTERN {
	LONG	lOffset;	// Pattern offset
	LONG	cbPattern;	// Pattern length
	LPCSTR	szValue;	// Pattern value
} GMF_FILETYPE_PATTERN;

typedef struct tagGMF_FILETYPE_ENTRY {
	UINT nPatterns;							// Number of patterns
	const GMF_FILETYPE_PATTERN *pPattern;	// Array of patterns
} GMF_FILETYPE_ENTRY;

typedef struct tagGMF_FILETYPE_REGINFO {
	LPCTSTR szTypeName;
	const CLSID *clsMajorType;			// Media type CLSID
	const CLSID *clsMinorType;			// Media subtype CLSID
	const CLSID *clsSourceFilter;		// Source filter CLSID
	UINT nEntries;						// Number of entries
	const GMF_FILETYPE_ENTRY *pEntry;	// Array of entries
	UINT nExtensions;					// Number of extensions
	LPCTSTR *pExtension;				// Array of extensions (including dot!)
} GMF_FILETYPE_REGINFO;

// Patterns and extensions registration members. Use this in the 
// declaration of your derived class (preferrably in the protected or 
// private section), set the static members and include 
// IMPLEMENT_FILETYPE in the implementation of the derived class
#define DECLARE_FILETYPE			\
	static const int g_nRegInfo;	\
	static const GMF_FILETYPE_REGINFO g_pRegInfo[];

// Patterns and extensions registration method. Use this in the 
// declaration of your derived class in the public section
#define DECLARE_FILETYPE_METHODS											\
	static HRESULT RegisterFileType(BOOL bRegister);						\
	STDMETHODIMP GetFileTypeCount(int *pnCount);							\
	STDMETHODIMP GetFileTypeName(int iType, LPTSTR szName);					\
	STDMETHODIMP GetFileTypeExtensions(										\
		int iType,															\
		UINT *pnExtensions,													\
		LPCTSTR **ppExtension												\
	);																		\
	STDMETHODIMP RegisterFileTypePatterns(int iType, BOOL bRegister);		\
	STDMETHODIMP RegisterFileTypeExtensions(int iType, BOOL bRegister);

#define IMPLEMENT_FILETYPE(ClassName)										\
HRESULT ClassName::RegisterFileType(BOOL bRegister)							\
{																			\
	for (int i = 0; i < g_nRegInfo; i++) {									\
		HRESULT hr = RegisterPatterns(&g_pRegInfo[i], bRegister);			\
		if (FAILED(hr))														\
			return hr;														\
		hr = RegisterExtensions(&g_pRegInfo[i], bRegister);					\
		if (FAILED(hr))														\
			return hr;														\
	}																		\
	return NOERROR;															\
}																			\
STDMETHODIMP ClassName::GetFileTypeCount(int *pnCount)						\
{																			\
	CheckPointer(pnCount, E_POINTER);										\
	ValidateWritePtr(pnCount, sizeof(int));									\
	*pnCount = g_nRegInfo;													\
	return NOERROR;															\
}																			\
STDMETHODIMP ClassName::GetFileTypeName(int iType, LPTSTR szName)			\
{																			\
	CheckPointer(szName, E_POINTER);										\
	if ((iType < 0) || (iType >= g_nRegInfo))								\
		return E_INVALIDARG;												\
	lstrcpy(szName, g_pRegInfo[iType].szTypeName);							\
	return NOERROR;															\
}																			\
STDMETHODIMP ClassName::GetFileTypeExtensions(								\
	int iType,																\
	UINT *pnExtensions,														\
	LPCTSTR **ppExtension													\
)																			\
{																			\
	CheckPointer(pnExtensions, E_POINTER);									\
	ValidateWritePtr(pnExtensions, sizeof(UINT));							\
	CheckPointer(ppExtension, E_POINTER);									\
	ValidateWritePtr(ppExtension, sizeof(LPCTSTR*));						\
	if ((iType < 0) || (iType >= g_nRegInfo))								\
		return E_INVALIDARG;												\
	*pnExtensions	= g_pRegInfo[iType].nExtensions;						\
	*ppExtension	= g_pRegInfo[iType].pExtension;							\
	return NOERROR;															\
}																			\
STDMETHODIMP ClassName::RegisterFileTypePatterns(							\
	int iType,																\
	BOOL bRegister															\
)																			\
{																			\
	if ((iType < 0) || (iType >= g_nRegInfo))								\
		return E_INVALIDARG;												\
	return RegisterPatterns(&g_pRegInfo[iType], bRegister);					\
}																			\
STDMETHODIMP ClassName::RegisterFileTypeExtensions(							\
	int iType,																\
	BOOL bRegister															\
)																			\
{																			\
	if ((iType < 0) || (iType >= g_nRegInfo))								\
		return E_INVALIDARG;												\
	return RegisterExtensions(&g_pRegInfo[iType], bRegister);				\
}

//==========================================================================
// Base parser filter class
// 
// This base class does not need to have CreateInstance() as we're 
// not going to instantiate it. This class serves only to provide 
// a common environment for parser filters: 
// (1) input & output pins creating, deleting and reporting
// (2) pin options handling (configuring, storing/retrieving)
// (3) interfaces exposure
// Note that IAMMediaContent methods are implemented in the base class
// but all return pointers to the member BSTR's which are to be set 
// by the Initialize() in the derived class (in the base class all 
// media content strings are NULLs)
//==========================================================================

class CBaseParserFilter :	public CBaseFilter,
							public IAMMediaContent,
							public IConfigBaseParser,
							public ISpecifyPropertyPages
{
	
	CBaseDispatch m_basedisp;

public:

	DECLARE_IUNKNOWN

	// Overriden to expose IAMMediaContent, IConfigBaseParser and 
	// ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// ---- IDispatch methods ----
	STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
	STDMETHODIMP GetTypeInfo(
		UINT itinfo,
		LCID lcid,
		ITypeInfo ** pptinfo
	);
	STDMETHODIMP GetIDsOfNames(
		REFIID riid,
		OLECHAR  ** rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID * rgdispid
	);
	STDMETHODIMP Invoke(
		DISPID dispidMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS * pdispparams,
		VARIANT * pvarResult,
		EXCEPINFO * pexcepinfo,
		UINT * puArgErr
	);

	// ---- IAMMediaContent methods ----
	STDMETHODIMP get_AuthorName(BSTR *pbstrAuthorName);
	STDMETHODIMP get_Title(BSTR *pbstrTitle);
	STDMETHODIMP get_Rating(BSTR *pbstrRating);
	STDMETHODIMP get_Description(BSTR *pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR *pbstrCopyright);
	STDMETHODIMP get_BaseURL(BSTR *pbstrBaseURL);
	STDMETHODIMP get_LogoURL(BSTR *pbstrLogoURL);
	STDMETHODIMP get_LogoIconURL(BSTR *pbstrLogoURL);
	STDMETHODIMP get_WatermarkURL(BSTR *pbstrWatermarkURL);
	STDMETHODIMP get_MoreInfoURL(BSTR *pbstrMoreInfoURL);
	STDMETHODIMP get_MoreInfoBannerImage(BSTR *pbstrMoreInfoBannerImage);
	STDMETHODIMP get_MoreInfoBannerURL(BSTR *pbstrMoreInfoBannerURL);
	STDMETHODIMP get_MoreInfoText(BSTR *pbstrMoreInfoText);

	// Pin access
	CBasePin *GetPin(int n);
	int GetPinCount(void);

	// Check if the input pin is currently flushing
	BOOL IsFlushing(void);

	// Per-file initialization. Carefully check if the file is 
	// suitable and create output pins. This method finds out which 
	// streams are present in the file and creates corresponding 
	// output pins. The method also sets the media content strings and
	// the default start/stop positions (for later use). If you require
	// the async reader for later use, addref it, store in the 
	// member variable and release in the Shutdown() method
	virtual HRESULT Initialize(IPin *pPin, IAsyncReader *pReader) PURE;
	
	// Destroy the output pins created by Initialize() (and also 
	// free any other resources which Initialize() might have allocated
	// and reset any variables it might have set)
	virtual HRESULT Shutdown(void);

	// The method to set the default start/stop positions. Be sure to 
	// set them in Initialize() or else the playback after stop can
	// start from the last seek positions and not from the beginning
	HRESULT SetDefaultPositions(void);

	// Per-play initialization. Initialize parser for the playback
	virtual HRESULT InitializeParser(void) PURE;
	
	// Free any resources allocated by InitializeParser() and
	// shutdown the parser
	virtual HRESULT ShutdownParser(void) { return NOERROR; };

	// Reset parser. This method should discard the unprocessed data 
	// and reset the parser state so that it's ready to accept data 
	// after some discontinuity occurs. An example of such a 
	// discontinuity is a seeking, so this method is called in EndFlush()
	virtual HRESULT ResetParser(void) PURE;

	// FIXUP: Due to odd IAsyncReader allocator alignment we may find 
	// ourselves ahead of the desired position after Seek(). 
	// Call this method after pullpin's Seek() to set the adjustment 
	// for file positions which are retrieved from the media samples
	// coming from the pullpin
	void SetAdjustment(LONGLONG llAdjustment);

	// Method which handles quality notifications from output pins.
	// This methods just calls AlterQuality() and depending on the
	// return value proceeds as follows:
	// S_OK    -> returns with success code
	// S_FALSE -> passes notification upstream
	// error   -> returns error code
	HRESULT NotifyQuality(LPCWSTR pPinName, Quality q);

	// Actual quality notification handler. The base-class method
	// does nothing and returns S_FALSE
	virtual HRESULT AlterQuality(LPCWSTR pPinName, Quality q);

	// ---- Media seeking methods ----
	// The first argument is usually the name of the output pin
	// whose seeker calls the method (the implementation of the 
	// method may depend on the output pin's stream specifics).
	// The second argument for some methods is the current time 
	// format. 
	
	// The base-class implementation returns E_NOTIMPL
	virtual HRESULT ConvertTimeFormat(
		LPCWSTR pPinName,
		LONGLONG *pTarget,
		const GUID *pTargetFormat,
		LONGLONG llSource,
		const GUID *pSourceFormat
	) { return E_NOTIMPL; };

	// Convert media position (expressed in units of specified time 
	// format) to file position. Source position is absolute. Disregard 
	// all flags in dwSourceFlags except AM_SEEKING_SeekToKeyFrame.
	// Do not forget to return the actual source position which may be
	// different from the requested source position.
	// Note that AM_SEEKING_Segment and AM_SEEKING_NoFlush can be
	// implemented if you introduce the flags responsible for cancelling
	// EndOfStream() and BeginFlush()/EndFlush() delivery downstream 
	// when the pullpin calls these methods (but that's NOT done here).
	// The base-class implementation returns E_NOTIMPL
	virtual HRESULT ConvertPosition(
		LONGLONG *pllTarget,
		LPCWSTR pPinName,
		const GUID *pSourceFormat,
		LONGLONG *pllSource,
		DWORD dwSourceFlags
	) { return E_NOTIMPL; };

	// Set start/stop positions (called from the seeker's SetPositions()).
	// The base-class method calls ConvertPosition() to get file 
	// positions from media positions and then calls Seek() on the 
	// input pin. You can call the base-class method from your 
	// implementation to do that routine work. Some video formats also 
	// require additional work to do (e.g. setting file position
	// ahead of the requested one and then collect the portions of the 
	// keyframe for the position to start playback from).
	// Note that the base-class method accepts requests only from the 
	// first (zero index) output pin. You can change this behavior in 
	// your implementation by providing the base-class method with the 
	// name of the first pin even for the request that came from the 
	// different pin.
	// Note also that if you use the input pin's Seek() method to set 
	// the file positions, you don't need to call BeginFlush()/EndFlush()
	// because the pullpin's Seek() does that for you in a proper way
	virtual HRESULT SetPositions(
		LPCWSTR pPinName,
		const GUID *pCurrentFormat,
		LONGLONG *pllCurrent,
		DWORD dwCurrentFlags,
		LONGLONG *pllStop,
		DWORD dwStopFlags
	);

	// Set playback rate (called from the seeker's SetRate()).
	// The base-class method calls NewSegment() using the specified 
	// rate and the start/stop positions as the parameters.
	// Note that the base-class method accepts requests only from the 
	// first (zero index) output pin. You can change this behavior in 
	// your implementation by providing the base-class method with the 
	// name of the first pin even for the request that came from the 
	// different pin
	virtual HRESULT SetRate(
		LPCWSTR pPinName,
		REFERENCE_TIME rtCurrent,
		REFERENCE_TIME rtStop,
		double dRate
	);

	// The base-class implementation returns E_NOTIMPL
	virtual HRESULT GetDuration(
		LPCWSTR pPinName,
		const GUID *pCurrentFormat,
		LONGLONG *pDuration
	) { return E_NOTIMPL; };

	// The base-class implementation returns E_NOTIMPL
	virtual HRESULT GetAvailable(
		LPCWSTR pPinName,
		const GUID *pCurrentFormat,
		LONGLONG *pEarliest,
		LONGLONG *pLatest
	) { return E_NOTIMPL; };

	// The base-class implementation sets zero preroll time
	virtual HRESULT GetPreroll(
		LPCWSTR pPinName,
		const GUID *pCurrentFormat,
		LONGLONG *pllPreroll
	);
	
	// Receive the sample
	HRESULT Receive(IMediaSample *pSample);

	// Overridden to synchronize with Receive() calls
	STDMETHODIMP Stop(void);

	// Pass the calls to the output pins
	HRESULT BeginFlush(void);
	HRESULT EndFlush(void);
	HRESULT NewSegment(
		REFERENCE_TIME tStart, 
		REFERENCE_TIME tStop, 
		double dRate
	);
	HRESULT EndOfStream(void);

	// Input stream media type stuff
	virtual HRESULT CheckInputType(const CMediaType* pmt) PURE;
	virtual HRESULT GetInputType(int iPosition, CMediaType *pMediaType) PURE;

	// ---- Member fields access methods ----

	// The following methods provide access to the input pin options
	BOOL IsSyncInput(void);
	HRESULT GetInputAllocatorProperties(ALLOCATOR_PROPERTIES *pRequest);

	// Filter name access method
	LPCWSTR Name(void) { return m_wszFilterName; };

	// IConfigBaseParser methods
	STDMETHODIMP GetOutputPinCount(int *pnCount);
	STDMETHODIMP GetOutputPinName(int iPin, LPWSTR wszName);
	STDMETHODIMP get_SyncInput(BOOL *pbSyncInput);
	STDMETHODIMP get_InputBuffersNumber(long *pnBuffers);
	STDMETHODIMP get_OutputBuffersNumber(int iPin, long *pnBuffers);
	STDMETHODIMP get_AutoQueue(int iPin, BOOL *pbAutoQueue);
	STDMETHODIMP get_Queue(int iPin, BOOL *pbQueue);
	STDMETHODIMP get_BatchSize(int iPin, LONG *plBatchSize);
	STDMETHODIMP get_BatchExact(int iPin, BOOL *pbBatchExact);
	STDMETHODIMP put_SyncInput(BOOL bSyncInput);
	STDMETHODIMP put_InputBuffersNumber(long nBuffers);
	STDMETHODIMP put_OutputBuffersNumber(int iPin, long nBuffers);
	STDMETHODIMP put_AutoQueue(int iPin, BOOL bAutoQueue);
	STDMETHODIMP put_Queue(int iPin, BOOL bQueue);
	STDMETHODIMP put_BatchSize(int iPin, LONG lBatchSize);
	STDMETHODIMP put_BatchExact(int iPin, BOOL bBatchExact);
	virtual STDMETHODIMP GetFileTypeCount(int *pnCount) PURE;
	virtual STDMETHODIMP GetFileTypeName(int iType, LPTSTR szName) PURE;
	virtual STDMETHODIMP GetFileTypeExtensions(int iType, UINT *pnExtensions, LPCTSTR **ppExtension) PURE;
	virtual STDMETHODIMP RegisterFileTypePatterns(int iType, BOOL bRegister) PURE;
	virtual STDMETHODIMP RegisterFileTypeExtensions(int iType, BOOL bRegister) PURE;

	// ISpecifyPropertyPages method.
	// The base-class implemenation creates an array consisting of a
	// base filter property page CLSID. If your derived filter has no 
	// additional pages besides that, you can leave the method
	// alone, otherwise override this method and do not call the 
	// base-class method at all
    STDMETHODIMP GetPages(CAUUID *pPages);

protected:

	// Filter name (used for filter options in the registry)
	const WCHAR *m_wszFilterName;

	// ---- Critical sections ----
	// In fact, there's no need to keep that many critical sections.
	// Anyway, for the sake of code robustness, readability (and 
	// understandability) you should use the following policy:
	// (1) Hold data section when changing/reading filter data member 
	// variables (positions, adjustment, etc);
	// (2) Hold info section when changing/reading media content 
	// information fields;
	// (3) Hold pin state section when changing/reading output pins
	// state (number of pins, adding/removing pins, etc);
	// (4) Hold options section when changing/reading option values
	CCritSec m_csFilter;	// Filter state protection
	CCritSec m_csReceive;	// Streaming state protection
	CCritSec m_csData;		// Filter data protection
	CCritSec m_csInfo;		// Media content information protection
	CCritSec m_csPins;		// Output pins state protection
	CCritSec m_csOptions;	// Option values protectiion

	// ---- Media content information ----

	OLECHAR *m_wszAuthorName;
	OLECHAR *m_wszTitle;
	OLECHAR *m_wszRating;
	OLECHAR *m_wszDescription;
	OLECHAR *m_wszCopyright;
	OLECHAR *m_wszBaseURL;
	OLECHAR *m_wszLogoURL;
	OLECHAR *m_wszLogoIconURL;
	OLECHAR *m_wszWatermarkURL;
	OLECHAR *m_wszMoreInfoURL;
	OLECHAR *m_wszMoreInfoBannerImage;
	OLECHAR *m_wszMoreInfoBannerURL;
	OLECHAR *m_wszMoreInfoText;

	// ---- Filter options ----

	// Input pin options
	BOOL m_bSyncInput;		// Use synchronous reads in pullpin
	long m_nInputBuffers;	// Number of input buffers
	long m_cbInputBuffer;	// Size of each input buffer (determined in Initialize())
	long m_cbInputAlign;	// Alignment for input buffers (determined in Initialize())

	// ---- Input & output pins of the filter ----

	CParserInputPin		m_InputPin;		// Input pin of the filter
	int					m_nOutputPins;	// Number of output pins
	CParserOutputPin  **m_ppOutputPin;	// Output pins array

	// ---- File position stuff ----

	LONGLONG m_llStartPosition;	// Start position to start parsing from
	LONGLONG m_llStopPosition;	// Stop position to stop parsing at
	LONGLONG m_llAdjustment;	// File position adjustment
	LONGLONG m_llDefaultStart;	// Default start position (set in Initialize())
	LONGLONG m_llDefaultStop;	// Default stop position (set in Initialize())

	// Construction/destruction
	CBaseParserFilter(
		TCHAR *pName,				// Object name
		LPUNKNOWN pUnk,				// Controlling IUnknown
		REFCLSID clsid,				// CLSID of the filter
		const WCHAR *wszFilterName,	// Filter name
		HRESULT *phr				// Success or error code
	);
	virtual ~CBaseParserFilter();

	// Receive the data and parse it
	virtual HRESULT Receive(
		LONGLONG llStartPosition,
		BYTE *pbData,
		LONG lDataSize
	) PURE;

	// Register/unregister file type patterns
	static HRESULT RegisterPatterns(
		const GMF_FILETYPE_REGINFO *pRegInfo,
		BOOL bRegister
	);

	// Register/unregister file type extensions
	static HRESULT RegisterExtensions(
		const GMF_FILETYPE_REGINFO *pRegInfo,
		BOOL bRegister
	);

};

//==========================================================================
// Base parser property page class
//
// Configures the input and output pin options for the parsers
//==========================================================================

extern const WCHAR g_wszBaseParserPageName[];

class CBaseParserPage : public CBasePropertyPage
{

	CBaseParserPage(LPUNKNOWN pUnk);

	BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect(void);
	HRESULT OnActivate(void);
	HRESULT OnDeactivate(void);
	HRESULT OnApplyChanges(void);

	// Utility (dialog controls handling) methods
	void SetDirty(void);
	UINT GetTrackBarPos(int nTrackBarID, int nTextID);
	int GetComboBoxCurSel(int nID);
	void UpdateOutputPinGroup(int iSel);
	void UpdateFileTypeExtensions(int iSel);
	BOOL GetCheckBox(int nID);
	void FreeOutputPinOptions(void);

	// Configuration interface
	IConfigBaseParser *m_pConfig;

	// Number of the output pins
	int m_nOutputPins;

	// Array of the output pin options
	struct tagOutputPinOptions {
		long nOutputBuffers;	// Number of output buffers
		BOOL bAutoQueue;		// Auto queue option
		BOOL bQueue;			// Queue option
		LONG lBatchSize;		// Batch size
		BOOL bBatchExact;		// Batch exact option
	} *m_pOutputPinOptions;

public:

	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

};

#endif