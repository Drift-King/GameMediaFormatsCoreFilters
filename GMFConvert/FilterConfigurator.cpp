// FilterConfigurator.cpp: implementation of the CFilterConfigurator class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GMFConvert.h"
#include "FilterConfigurator.h"
#include "AMErrorText.h"
#include "GUIDMap.h"

#include <streams.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFilterConfigurator::CFilterConfigurator(CListBox *pFilterList, CListBox *pFormatList) :
	m_pFilterList(pFilterList),
	m_pFormatList(pFormatList)
{
	ASSERT(pFilterList != NULL);
}

CFilterConfigurator::~CFilterConfigurator()
{
	ResetFormatList();
	ResetFilterList();
}

HRESULT CFilterConfigurator::PopulateFilterList(REFCLSID clsidCategory)
{
	ResetFormatList();
	ResetFilterList();

	// Create an instance of system device enumerator
	ICreateDevEnum *pSysDevEnum = NULL;
	HRESULT hr = CoCreateInstance(
		CLSID_SystemDeviceEnum,
		NULL,
		CLSCTX_INPROC,
		IID_ICreateDevEnum,
		(void**)&pSysDevEnum
	);
	if (FAILED(hr))
		return hr;

	// Create a class enumerator
	IEnumMoniker *pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(clsidCategory, &pEnumCat, 0);
	if (hr != S_OK) {
		pSysDevEnum->Release();
		return hr;
	}

	// Enumerate the filters
	IMoniker *pMoniker = NULL;
	while (pEnumCat->Next(1, &pMoniker, NULL) == S_OK) {

		CString szFilterDesc = _T("");

		// Get the property bag of the filter
		IPropertyBag *pPropBag = NULL;
		hr = pMoniker->BindToStorage(NULL, NULL, IID_IPropertyBag, (void**)&pPropBag);
		if (FAILED(hr))
			szFilterDesc.LoadString(IDS_FILTER_NO_PROPERTY_BAG);
		else {

			// Get the friendly name of the filter
			VARIANT varName;
			varName.vt = VT_BSTR;
			hr = pPropBag->Read(L"FriendlyName", &varName, NULL);
			pPropBag->Release();
			if (SUCCEEDED(hr)) {
#ifdef _UNICODE
				szFilterDesc.Format(_T("%ls"), varName.bstrVal);
#else
				WideCharToMultiByte(
					CP_ACP,
					0,
					varName.bstrVal,
					-1,
					szFilterDesc.GetBuffer(lstrlenW(varName.bstrVal)),
					lstrlenW(varName.bstrVal),
					NULL,
					NULL
				);
				szFilterDesc.ReleaseBuffer();
#endif
				SysFreeString(varName.bstrVal);
			} else
				szFilterDesc.LoadString(IDS_FILTER_NO_FRIENDLY_NAME);
		}

		// Add filter description string and data to the list box
		int i = m_pFilterList->AddString(szFilterDesc);
		m_pFilterList->SetItemDataPtr(i, pMoniker);
		m_pFilterList->UpdateWindow();
	}

	// Release the enumerators
	pEnumCat->Release();
	pSysDevEnum->Release();

	return NOERROR;
}

HRESULT CFilterConfigurator::PopulateFormatList(IMoniker *pMoniker)
{
	if (m_pFormatList == NULL)
		return E_UNEXPECTED;

	// Instantiate a filter
	IBaseFilter *pBaseFilter = NULL;
	HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pBaseFilter);
	if (FAILED(hr)) {
		m_pFormatList->AddString(CAMErrorText(hr, IDS_FILTER_NO_OBJECT));
		m_pFormatList->EnableWindow(FALSE);
		return hr;
	}

	// Enumerate the pins on the filter
	IEnumPins *pEnumPins = NULL;
	hr = pBaseFilter->EnumPins(&pEnumPins);
	if (FAILED(hr)) {
		pBaseFilter->Release();
		m_pFormatList->AddString(CAMErrorText(hr, IDS_FILTER_NO_PIN_ENUM));
		m_pFormatList->EnableWindow(FALSE);
		return hr;
	}

	// Find a pin providing IAMStreamConfig interface
	IPin *pPin = NULL;
	IAMStreamConfig *pAMStreamConfig = NULL;
	while (pEnumPins->Next(1, &pPin, NULL) == S_OK) {
		// Try to retrieve IAMStreamConfig
		hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**)&pAMStreamConfig);
		pPin->Release();
		if (SUCCEEDED(hr))
			break;
	}
	// Release the pin enumerator
	pEnumPins->Release();

	// If the filter has no pin with required interface, we're stuck
	if (pAMStreamConfig == NULL) {
		pBaseFilter->Release();
		m_pFormatList->AddString(CString((LPCSTR)IDS_FILTER_NO_CONFIG));
		m_pFormatList->EnableWindow(FALSE);
		return E_UNEXPECTED;
	}

	// Get the number of capabilities
	int iCount, iSize;
	hr = pAMStreamConfig->GetNumberOfCapabilities(&iCount, &iSize);
	if (FAILED(hr)) {
		pAMStreamConfig->Release();
		pBaseFilter->Release();
		m_pFormatList->AddString(CAMErrorText(hr, IDS_FILTER_NO_CAPS));
		m_pFormatList->EnableWindow(FALSE);
		return hr;
	}

	m_pFormatList->EnableWindow(TRUE);

	// Enumerate the capabilities
	for (int i = 0; i < iCount; i++) {

		// Get the capabilities
		AM_MEDIA_TYPE *pmt = NULL;
		BYTE *pSCC = new BYTE[iSize];
		hr = pAMStreamConfig->GetStreamCaps(i, &pmt, pSCC);
		delete[] pSCC;
		if (FAILED(hr))
			continue;

		// Add the format to the list
		CString szFormat;
		if (pmt->majortype == MEDIATYPE_Audio) {
			if (pmt->formattype == FORMAT_WaveFormatEx) {
				WAVEFORMATEX *pwfex = (WAVEFORMATEX*)pmt->pbFormat;
				szFormat.Format(
					_T("%3lu kbps, %6lu Hz, %s"),
					(pwfex->nAvgBytesPerSec * 8) / 1000,
					pwfex->nSamplesPerSec,
					(pwfex->nChannels == 2) ? "stereo" : "mono"
				);
			} else
				szFormat.LoadString(IDS_FORMAT_UNEXPECTED_FORMAT_TYPE);
		} else if (pmt->majortype == MEDIATYPE_Video) {
			szFormat.Format(_T("Format #%d"), i);
		} else
			szFormat.LoadString(IDS_FORMAT_UNEXPECTED_MEDIA_TYPE);
		int i = m_pFormatList->AddString(szFormat);
		m_pFormatList->SetItemDataPtr(i, pmt);
	}

	// Release the interfaces
	pAMStreamConfig->Release();
	pBaseFilter->Release();

	return NOERROR;
}

void CFilterConfigurator::ResetFilterList()
{
	// Release the monikers associated with the list box items
	for (int i = 0; i < m_pFilterList->GetCount(); i++) {
		IMoniker *pMoniker = (IMoniker*)m_pFilterList->GetItemDataPtr(i);
		if ((pMoniker != (IMoniker*)-1) && (pMoniker != NULL))
			pMoniker->Release();
	}

	// Clear the list box
	m_pFilterList->ResetContent();
}

void CFilterConfigurator::ResetFormatList()
{
	if (m_pFormatList == NULL)
		return;

	// Free the data associated with the list box items
	for (int i = 0; i < m_pFormatList->GetCount(); i++) {
		AM_MEDIA_TYPE *pmt = (AM_MEDIA_TYPE*)m_pFormatList->GetItemDataPtr(i);
		if (pmt != NULL)
			DeleteMediaType(pmt);
	}

	// Clear the list box
	m_pFormatList->ResetContent();
	m_pFormatList->EnableWindow(TRUE);
}

