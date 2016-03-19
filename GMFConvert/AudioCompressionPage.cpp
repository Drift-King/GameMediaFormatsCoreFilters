// AudioCompressionPage.cpp : implementation file
//

#include "stdafx.h"
#include "GMFConvert.h"
#include "GMFConvertItem.h"
#include "AudioCompressionPage.h"
#include "GUIDMap.h"

#include <mmreg.h>
#include <msacm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAudioCompressionPage property page

CAudioCompressionPage::CAudioCompressionPage(CGMFConvertView *pParent) :
	CPropertyPage(CAudioCompressionPage::IDD),
	m_pView(pParent),
	m_pFilterConfigurator(NULL)
{
	ASSERT(pParent != NULL);

	//{{AFX_DATA_INIT(CAudioCompressionPage)
	//}}AFX_DATA_INIT
}

CAudioCompressionPage::~CAudioCompressionPage()
{
}

void CAudioCompressionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAudioCompressionPage)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAudioCompressionPage, CPropertyPage)
	//{{AFX_MSG_MAP(CAudioCompressionPage)
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(IDC_AUDIO_FILTERS, OnSelChangeAudioFilters)
	ON_LBN_SELCHANGE(IDC_AUDIO_FORMATS, OnSelChangeAudioFormats)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAudioCompressionPage message handlers

BOOL CAudioCompressionPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// Populate the filter list
	CListBox *pFilterList = (CListBox*)GetDlgItem(IDC_AUDIO_FILTERS);
	CListBox *pFormatList = (CListBox*)GetDlgItem(IDC_AUDIO_FORMATS);
	if (m_pFilterConfigurator == NULL)
		m_pFilterConfigurator = new CFilterConfigurator(pFilterList, pFormatList);
	m_pFilterConfigurator->PopulateFilterList(CLSID_AudioCompressorCategory);

	// Add a null filter item
	int i = pFilterList->InsertString(0, CString((LPCSTR)IDS_AUDIO_NULL_FILTER));
	pFilterList->SetItemDataPtr(i, NULL);
	pFilterList->UpdateWindow();

	// Add a raw filter item
	i = pFilterList->InsertString(1, CString((LPCSTR)IDS_AUDIO_RAW_FILTER));
	pFilterList->SetItemDataPtr(i, (void*)-1);
	pFilterList->UpdateWindow();

	// Reset the format list and info
	m_pFilterConfigurator->ResetFormatList();
	ResetFormatInfo(_T(""));

	int iAudioFilter = -1;
	int iAudioFormat = -1;

	// Initialize control variables from the parent view
	CListCtrl& list = m_pView->GetListCtrl();
	ASSERT(list.GetSelectedCount() > 0);
	if (list.GetSelectedCount() == 1) {

		POSITION pos = list.GetFirstSelectedItemPosition();
		ASSERT(pos != NULL);

		int i = list.GetNextSelectedItem(pos);
		CGMFConvertItem *pItem = (CGMFConvertItem*)list.GetItemData(i);
		ASSERT(pItem != NULL);

		// Get filter moniker
		IMoniker *pMoniker = pItem->GetAudioFilterMoniker();

		// Try to find identical moniker in the filter list
		for (i = 0; i < pFilterList->GetCount(); i++) {
			IMoniker *pFilterMoniker = (IMoniker*)pFilterList->GetItemDataPtr(i);
			if (
				(pMoniker == pFilterMoniker) ||
				(
					(pMoniker		!= NULL)			&&
					(pMoniker		!= (IMoniker*)-1)	&&
					(pFilterMoniker	!= NULL)			&&
					(pFilterMoniker	!= (IMoniker*)-1)	&&
					(pMoniker->IsEqual(pFilterMoniker) == S_OK)
				)
			) {
				iAudioFilter = i;
				break;
			}
		}

		// Get the format media type index
		iAudioFormat = pItem->GetAudioMediaType();

	}

	pFilterList->SetCurSel(iAudioFilter);
	OnSelChangeAudioFilters();
	pFormatList->SetCurSel(iAudioFormat);
	OnSelChangeAudioFormats();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAudioCompressionPage::OnDestroy() 
{
	CPropertyPage::OnDestroy();
	
	// Delete the filter list
	if (m_pFilterConfigurator != NULL) {
		delete m_pFilterConfigurator;
		m_pFilterConfigurator = NULL;
	}
}

void CAudioCompressionPage::OnSelChangeAudioFilters() 
{
	// Get current selection
	CListBox *pMainList = (CListBox*)GetDlgItem(IDC_AUDIO_FILTERS);
	int i = pMainList->GetCurSel();
	if (i == LB_ERR)
		return;

	// Get the moniker of the selected filter
	IMoniker *pMoniker = (IMoniker*)pMainList->GetItemDataPtr(i);

	// Reset the format list and info
	CListBox *pFormatList = (CListBox*)GetDlgItem(IDC_AUDIO_FORMATS);
	m_pFilterConfigurator->ResetFormatList();
	ResetFormatInfo(_T(""));

	// Check the moniker
	if (pMoniker == NULL) {
		// No moniker means there's no need to list any formats
		i = pFormatList->AddString(CString((LPCSTR)IDS_AUDIO_NULL_FILTER_DESC));
		pFormatList->SetItemDataPtr(i, NULL);
		pFormatList->EnableWindow(FALSE);
	} else if (pMoniker == (IMoniker*)-1) {
		// -1 means raw uncompressed stream
		i = pFormatList->AddString(CString((LPCSTR)IDS_AUDIO_RAW_FILTER_DESC));
		pFormatList->SetItemDataPtr(i, NULL);
		pFormatList->EnableWindow(FALSE);
	} else
		m_pFilterConfigurator->PopulateFormatList(pMoniker);
}

void CAudioCompressionPage::ResetFormatInfo(CString szNoInfo)
{
	SetDlgItemText(IDC_AUDIO_FORMAT_TAG, szNoInfo);
	SetDlgItemText(IDC_AUDIO_FORMAT_DESC, szNoInfo);
	SetDlgItemText(IDC_BLOCK_ALIGN, szNoInfo);
	SetDlgItemText(IDC_DATA_RATE, szNoInfo);
}

void CAudioCompressionPage::OnSelChangeAudioFormats() 
{
	// Get current selection
	CListBox *pList = (CListBox*)GetDlgItem(IDC_AUDIO_FORMATS);
	int i = pList->GetCurSel();
	if (i == LB_ERR)
		return;

	// Get the data of the selected format
	AM_MEDIA_TYPE *pmt = (AM_MEDIA_TYPE*)pList->GetItemDataPtr(i);

	// ---- Set format parameters text controls ----

	DWORD dwFormatTag = CGUIDMap(pmt->subtype);
	CString szTemp = _T("N/A");
	if (dwFormatTag != 0) 
		szTemp.Format(_T("0x%04X"), dwFormatTag);
	SetDlgItemText(IDC_AUDIO_FORMAT_TAG, szTemp);

	szTemp = _T("N/A");
	if (dwFormatTag != 0) {
		ACMFORMATTAGDETAILS aftd = {0};
		aftd.cbStruct = sizeof(ACMFORMATTAGDETAILS);
		aftd.dwFormatTag = dwFormatTag;
		if (!acmFormatTagDetails(NULL, &aftd, ACM_FORMATTAGDETAILSF_FORMATTAG))
			szTemp = aftd.szFormatTag;
	}
	SetDlgItemText(IDC_AUDIO_FORMAT_DESC, szTemp);

	CString szBlockAlign = _T("N/A"), szDataRate = _T("N/A");
	if (pmt->formattype == FORMAT_WaveFormatEx) {
		WAVEFORMATEX *pwfex = (WAVEFORMATEX*)pmt->pbFormat;
		szBlockAlign.Format(_T("%u bytes"), pwfex->nBlockAlign);
		szDataRate.Format(_T("%lu bytes/sec"), pwfex->nAvgBytesPerSec);
	}
	SetDlgItemText(IDC_BLOCK_ALIGN, szBlockAlign);
	SetDlgItemText(IDC_DATA_RATE, szDataRate);
}


void CAudioCompressionPage::OnOK() 
{
	ASSERT(m_pView != NULL);
	CListCtrl& list = m_pView->GetListCtrl();
	POSITION pos = list.GetFirstSelectedItemPosition();
	ASSERT(pos != NULL);

	CListBox *pFilterList = (CListBox*)GetDlgItem(IDC_AUDIO_FILTERS);
	CListBox *pFormatList = (CListBox*)GetDlgItem(IDC_AUDIO_FORMATS);

	// Get the selected filter moniker
	IMoniker *pMoniker = NULL;
	int i = pFilterList->GetCurSel();
	if (i != LB_ERR)
		pMoniker = (IMoniker*)pFilterList->GetItemDataPtr(i);

	// Get the selected format media type index
	int iAudioFormat = pFormatList->GetCurSel();
	if (iAudioFormat == LB_ERR)
		iAudioFormat = -1;

	// Set the parameters for all selected list items
	while (pos) {

		i = list.GetNextSelectedItem(pos);
		CGMFConvertItem *pItem = (CGMFConvertItem*)list.GetItemData(i);
		ASSERT(pItem != NULL);

		pItem->SetAudioFilterMoniker(pMoniker);
		pItem->SetAudioMediaType(iAudioFormat);

	}

	CPropertyPage::OnOK();
}
