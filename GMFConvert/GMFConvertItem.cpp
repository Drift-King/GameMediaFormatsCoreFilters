// GMFConvertItem.cpp: implementation of the CGMFConvertItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GMFConvert.h"
#include "GMFConvertItem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CGMFConvertItem, CObject)

CGMFConvertItem::CGMFConvertItem() :
	m_iType(0),
	m_bCompatibilityIndex(TRUE),
	m_iMasterStream(-1),
	m_lInterleavingFrequency(1000),
	m_lInterleavingPreroll(0),
	m_iInterleavingMode(0),
	m_pAudioFilterMoniker(NULL),
	m_iAudioMediaType(-1),
	m_pVideoFilterMoniker(NULL),
	m_dVideoQuality(-1.0),
	m_dwBitRate(-1),
	m_lKeyFrameRate(-1),
	m_lPFramesPerKeyFrame(-1),
	m_dwlWindowSize((DWORDLONG)-1),
	m_pVideoCodecState(NULL),
	m_cbVideoCodecState(0)
{
}

CGMFConvertItem::~CGMFConvertItem()
{
	// Release the audio filter moniker
	if (CHECKMONIKER(m_pAudioFilterMoniker))
		m_pAudioFilterMoniker->Release();

	// Release the video filter moniker
	if (CHECKMONIKER(m_pVideoFilterMoniker))
		m_pVideoFilterMoniker->Release();

	// Free the video codec state buffer
	if (m_pVideoCodecState != NULL)
		CoTaskMemFree(m_pVideoCodecState);
}

void CGMFConvertItem::SetAudioFilterMoniker(IMoniker* pMoniker)
{
	// Release the old moniker
	if (CHECKMONIKER(m_pAudioFilterMoniker))
		m_pAudioFilterMoniker->Release();

	// Set and addref the new moniker
	m_pAudioFilterMoniker = pMoniker;
	if (CHECKMONIKER(m_pAudioFilterMoniker))
		m_pAudioFilterMoniker->AddRef();
}

void CGMFConvertItem::SetVideoFilterMoniker(IMoniker* pMoniker)
{
	// Release the old moniker
	if (CHECKMONIKER(m_pVideoFilterMoniker))
		m_pVideoFilterMoniker->Release();

	// Set and addref the new moniker
	m_pVideoFilterMoniker = pMoniker;
	if (CHECKMONIKER(m_pVideoFilterMoniker))
		m_pVideoFilterMoniker->AddRef();
}

void CGMFConvertItem::SetVideoCodecState(LPVOID pState, int cbState)
{
	// Free the old codec state buffer
	if (m_pVideoCodecState != NULL)
		CoTaskMemFree(m_pVideoCodecState);

	m_pVideoCodecState = pState;
	m_cbVideoCodecState = cbState;
}
