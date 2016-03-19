// FilterList.cpp: implementation of the CFilterList class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GMFConvert.h"
#include "FilterList.h"

#include <dshow.h>
//TEMP: #include <streams.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFilterList::CFilterList(CListBox *pList) :
	m_pList(pList)
{
	ASSERT(pList != NULL);	
}

CFilterList::~CFilterList()
{
	Reset();
}

HRESULT CFilterList::Populate(CLSID clsidCategory, CString szNullFilterName)
{
	Reset();

	// Add a null filter item
	int i = m_pList->AddString(szNullFilterName);
	m_pList->SetItemDataPtr(i, NULL);

	HRESULT hr = NOERROR;
	ICreateDevEnum *pSysDevEnum;
	IEnumMoniker *pEnumCat = NULL;
	try {

		// Create an instance of system device enumerator
		pSysDevEnum = NULL;
		hr = CoCreateInstance(
			CLSID_SystemDeviceEnum,
			NULL,
			CLSCTX_INPROC,
			IID_ICreateDevEnum,
			(void**)&pSysDevEnum
		);
		if (FAILED(hr))
			throw 0;

		// Create a class enumerator
		pEnumCat = NULL;
		hr = pSysDevEnum->CreateClassEnumerator(
			clsidCategory,
			&pEnumCat,
			0
		);
		if (hr != S_OK)
			throw 0;

		// Enumerate the filters
		IMoniker *pMoniker = NULL;
		ULONG nFetched;
		while (pEnumCat->Next(1, &pMoniker, &nFetched) == S_OK) {

			CString szFilterDesc, szTemp;
			CLSID *pFilterCLSID;

			// Get the property bag of the filter
			IPropertyBag *pPropBag = NULL;
			hr = pMoniker->BindToStorage(NULL, NULL, IID_IPropertyBag, (void**)&pPropBag);
			if (FAILED(hr)) {
				szFilterDesc.LoadString(IDS_FILTER_NO_PROPERTY_BAG);
				i = m_pList->AddString(szFilterDesc);
				m_pList->SetItemDataPtr(i, -1);
				pMoniker->Release();
				continue;
			}

			// Get the CLSID of the filter
			VARIANT varCLSID;
			varCLSID.vt = VT_BSTR;
			hr = pPropBag->Read(L"GUID", &varCLSID, NULL);
			if (SUCCEEDED(hr)) {
				pFilterCLSID = new CLSID;
				CLSIDFromString(varCLSID.bstrVal, pFilterCLSID);
				szFilterDesc.Format(_T("%ls"), varCLSID.bstrVal);
				SysFreeString(varCLSID.bstrVal);
			} else {
				pFilterCLSID = -1;
				szFilterDesc = _T("<Failed to get GUID>");
			}

			// Get the friendly name of the filter
			VARIANT varName;
			varName.vt = VT_BSTR;
			hr = pPropBag->Read(L"FriendlyName", &varName, NULL);
			if (SUCCEEDED(hr)) {
				szTemp.Format(_T("%ls"), varName.bstrVal);
				szFilterDesc += _T(" ") + szTemp;
				SysFreeString(varName.bstrVal);
			} else
				szFilterDesc += _T(" ") + CString((LPCSTR)IDS_FILTER_NO_FRIENDLY_NAME);

			// Get the vendor info of the filter
			IBaseFilter *pBaseFilter = NULL;
			hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pBaseFilter);
			if (SUCCEEDED(hr)) {
				LPWSTR wszVendorInfo = NULL;
				hr = pBaseFilter->QueryVendorInfo(&wszVendorInfo);
				if (SUCCEEDED(hr)) {
					szTemp.Format(_T("%ls"), wszVendorInfo);
					szFilterDesc += _T(" ") + szTemp;
					CoTaskMemFree(wszVendorInfo);
				} else
					szFilterDesc += _T(" ") + CString((LPCSTR)IDS_FILTER_NO_VENDOR_INFO);
			} else
				szFilterDesc += _T(" ") + CString((LPCSTR)IDS_FILTER_NO_VENDOR_INFO);

			i = m_pList->AddString(szFilterDesc);
			m_pList->SetItemDataPtr(i, pFilterCLSID);
			
			pPropBag->Release();
			pMoniker->Release();
		}
	}
	catch (int) {
		pEnumCat->Release();
		pSysDevEnum->Release();
	}
	pEnumCat->Release();
	pSysDevEnum->Release();
}

void CFilterList::Reset()
{
	// Free the data associated with the list box items
	for (int i = 0; i < m_pList->GetCount(); i++) {
		CLSID *pclsid = (CLSID*)m_pList->GetItemDataPtr(i);
		if ((pclsid != -1) && (pclsid != NULL))
			delete pclsid;
	}

	// Clear the list box
	m_pList->ResetContent();
}
