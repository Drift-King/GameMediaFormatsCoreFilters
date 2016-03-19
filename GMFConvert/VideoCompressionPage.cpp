// VideoCompressionPage.cpp : implementation file
//

#include "stdafx.h"
#include "GMFConvert.h"
#include "GMFConvertItem.h"
#include "VideoCompressionPage.h"

#include <streams.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVideoCompressionPage property page

CVideoCompressionPage::CVideoCompressionPage(CGMFConvertView *pParent) :
	CPropertyPage(CVideoCompressionPage::IDD),
	m_pView(pParent),
	m_pFilterConfigurator(NULL)
{
	ASSERT(pParent != NULL);

	//{{AFX_DATA_INIT(CVideoCompressionPage)
	//}}AFX_DATA_INIT
}

CVideoCompressionPage::~CVideoCompressionPage()
{
}

void CVideoCompressionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVideoCompressionPage)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVideoCompressionPage, CPropertyPage)
	//{{AFX_MSG_MAP(CVideoCompressionPage)
	ON_BN_CLICKED(IDC_VIDEO_BITRATE_CHECK, OnVideoBitrateCheck)
	ON_BN_CLICKED(IDC_VIDEO_QUALITY_CHECK, OnVideoQualityCheck)
	ON_BN_CLICKED(IDC_VIDEO_KEYFRAME_CHECK, OnVideoKeyframeCheck)
	ON_BN_CLICKED(IDC_VIDEO_PFRAME_CHECK, OnVideoPframeCheck)
	ON_BN_CLICKED(IDC_VIDEO_WINDOWSIZE_CHECK, OnVideoWindowsizeCheck)
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(IDC_VIDEO_FILTERS, OnSelChangeVideoFilters)
	ON_BN_CLICKED(IDC_VCM_CONFIGURE, OnVcmConfigure)
	ON_BN_CLICKED(IDC_VCM_ABOUT, OnVcmAbout)
	ON_WM_HSCROLL()
	ON_EN_CHANGE(IDC_VIDEO_QUALITY_EDIT, OnChangeVideoQualityEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVideoCompressionPage message handlers

void CVideoCompressionPage::OnVideoBitrateCheck() 
{
	UpdateVideoBitrateControls(TRUE);
	if (GetDlgButtonCheck(IDC_VIDEO_BITRATE_CHECK))
		GotoDlgCtrl(GetDlgItem(IDC_VIDEO_BITRATE_EDIT));
}

void CVideoCompressionPage::OnVideoQualityCheck() 
{
	UpdateVideoQualityControls(TRUE);
	if (GetDlgButtonCheck(IDC_VIDEO_QUALITY_CHECK))
		GotoDlgCtrl(GetDlgItem(IDC_VIDEO_QUALITY_SLIDER));
}

void CVideoCompressionPage::OnVideoKeyframeCheck() 
{
	UpdateVideoKeyframeControl(TRUE);
	if (GetDlgButtonCheck(IDC_VIDEO_KEYFRAME_CHECK))
		GotoDlgCtrl(GetDlgItem(IDC_VIDEO_KEYFRAME_EDIT));
}

void CVideoCompressionPage::OnVideoPframeCheck() 
{
	UpdateVideoPframeControl(TRUE);
	if (GetDlgButtonCheck(IDC_VIDEO_PFRAME_CHECK))
		GotoDlgCtrl(GetDlgItem(IDC_VIDEO_PFRAME_EDIT));
}

void CVideoCompressionPage::OnVideoWindowsizeCheck() 
{
	UpdateVideoWindowsizeControl(TRUE);
	if (GetDlgButtonCheck(IDC_VIDEO_WINDOWSIZE_CHECK))
		GotoDlgCtrl(GetDlgItem(IDC_VIDEO_WINDOWSIZE_EDIT));
}

BOOL CVideoCompressionPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// Create and populate the filter list
	CListBox *pFilterList = (CListBox*)GetDlgItem(IDC_VIDEO_FILTERS);
	if (m_pFilterConfigurator == NULL)
		m_pFilterConfigurator = new CFilterConfigurator(pFilterList, NULL);
	m_pFilterConfigurator->PopulateFilterList(CLSID_VideoCompressorCategory);

	// Add a null filter item
	int i = pFilterList->InsertString(0, CString((LPCSTR)IDS_VIDEO_NULL_FILTER));
	pFilterList->SetItemDataPtr(i, NULL);
	pFilterList->UpdateWindow();

	// Add a raw filter item
	i = pFilterList->InsertString(1, CString((LPCSTR)IDS_VIDEO_RAW_FILTER));
	pFilterList->SetItemDataPtr(i, (void*)-1);
	pFilterList->UpdateWindow();

	IMoniker* pMoniker			= NULL;
	int		iVideoFilter		= -1;
	int		iVideoQuality		= -1;
	BOOL	bVideoQuality		= FALSE;
	DWORD	dwBitRate			= -1;
	BOOL	bBitRate			= FALSE;
	long	lKeyFrameRate		= -1;
	BOOL	bKeyFrameRate		= FALSE;
	long	lPFramesPerKeyFrame	= -1;
	BOOL	bPFramesPerKeyFrame	= FALSE;
	DWORD	dwWindowSize		= -1;
	BOOL	bWindowSize			= FALSE;
	LPVOID	pVideoCodecState	= NULL;
	int		cbVideoCodecState	= 0;

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
		pMoniker = pItem->GetVideoFilterMoniker();

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
				iVideoFilter = i;
				break;
			}
		}

		// Get video compression parameters
		iVideoQuality		= (int)(pItem->GetVideoQuality() * 100);
		bVideoQuality		= iVideoQuality >= 0;
		dwBitRate			= pItem->GetBitRate();
		bBitRate			= dwBitRate != -1;
		lKeyFrameRate		= pItem->GetKeyFrameRate();
		bKeyFrameRate		= lKeyFrameRate >= 0;
		lPFramesPerKeyFrame	= pItem->GetPFramesPerKeyFrame();
		bPFramesPerKeyFrame	= lPFramesPerKeyFrame >= 0;
		dwWindowSize		= (DWORD)pItem->GetWindowSize();
		bWindowSize			= dwWindowSize != -1;
		pVideoCodecState	= pItem->GetVideoCodecState();
		cbVideoCodecState	= pItem->GetVideoCodecStateSize();
	}

	pFilterList->SetCurSel(iVideoFilter);
	OnSelChangeVideoFilters();

	// ---- Set up the video compression controls ----

	CheckDlgButton(IDC_VIDEO_QUALITY_CHECK, bVideoQuality ? 1 : 0);
	UpdateVideoQualityControls(TRUE);
	if (bVideoQuality) {
		SetDlgItemInt(IDC_VIDEO_QUALITY_EDIT, iVideoQuality, FALSE);
		OnChangeVideoQualityEdit();
	}

	CheckDlgButton(IDC_VIDEO_BITRATE_CHECK, bBitRate ? 1 : 0);
	UpdateVideoBitrateControls(TRUE);
	if (bBitRate)
		SetDlgItemInt(IDC_VIDEO_BITRATE_EDIT, dwBitRate, FALSE);

	CheckDlgButton(IDC_VIDEO_KEYFRAME_CHECK, bKeyFrameRate ? 1 : 0);
	UpdateVideoKeyframeControl(TRUE);
	if (bKeyFrameRate)
		SetDlgItemInt(IDC_VIDEO_KEYFRAME_EDIT, lKeyFrameRate, FALSE);

	CheckDlgButton(IDC_VIDEO_PFRAME_CHECK, bPFramesPerKeyFrame ? 1 : 0);
	UpdateVideoPframeControl(TRUE);
	if (bPFramesPerKeyFrame)
		SetDlgItemInt(IDC_VIDEO_PFRAME_EDIT, lPFramesPerKeyFrame, FALSE);

	CheckDlgButton(IDC_VIDEO_WINDOWSIZE_CHECK, bWindowSize ? 1 : 0);
	UpdateVideoWindowsizeControl(TRUE);
	if (bWindowSize)
		SetDlgItemInt(IDC_VIDEO_WINDOWSIZE_EDIT, dwWindowSize, FALSE);

	if (CHECKMONIKER(pMoniker))
		SetCodecState(pMoniker, pVideoCodecState, cbVideoCodecState);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CVideoCompressionPage::UpdateVideoBitrateControls(BOOL bEnable)
{
	BOOL bReallyEnable = bEnable && GetDlgButtonCheck(IDC_VIDEO_BITRATE_CHECK);
	GetDlgItem(IDC_VIDEO_BITRATE_EDIT)->EnableWindow(bReallyEnable);
	GetDlgItem(IDC_VIDEO_BITRATE_UNITS)->EnableWindow(bReallyEnable);
}

void CVideoCompressionPage::UpdateVideoQualityControls(BOOL bEnable)
{
	BOOL bReallyEnable = bEnable && GetDlgButtonCheck(IDC_VIDEO_QUALITY_CHECK);
	GetDlgItem(IDC_VIDEO_QUALITY_SLIDER)->EnableWindow(bReallyEnable);
	GetDlgItem(IDC_VIDEO_QUALITY_EDIT)->EnableWindow(bReallyEnable);
	GetDlgItem(IDC_VIDEO_QUALITY_UNITS)->EnableWindow(bReallyEnable);
}

void CVideoCompressionPage::UpdateVideoKeyframeControl(BOOL bEnable)
{
	GetDlgItem(IDC_VIDEO_KEYFRAME_EDIT)->EnableWindow(
		bEnable && GetDlgButtonCheck(IDC_VIDEO_KEYFRAME_CHECK)
	);
}

void CVideoCompressionPage::UpdateVideoPframeControl(BOOL bEnable)
{
	GetDlgItem(IDC_VIDEO_PFRAME_EDIT)->EnableWindow(
		bEnable && GetDlgButtonCheck(IDC_VIDEO_PFRAME_CHECK)
	);
}

void CVideoCompressionPage::UpdateVideoWindowsizeControl(BOOL bEnable)
{
	GetDlgItem(IDC_VIDEO_WINDOWSIZE_EDIT)->EnableWindow(
		bEnable && GetDlgButtonCheck(IDC_VIDEO_WINDOWSIZE_CHECK)
	);
}

void CVideoCompressionPage::OnDestroy() 
{
	CPropertyPage::OnDestroy();
	
	// Delete the filter list
	if (m_pFilterConfigurator != NULL) {
		delete m_pFilterConfigurator;
		m_pFilterConfigurator = NULL;
	}
}

void CVideoCompressionPage::OnSelChangeVideoFilters() 
{
	// Reset all video controls (so that later we can enable and set up 
	// only those which are needed)
	ResetVideoControls();

	// Get current selection
	CListBox *pMainList = (CListBox*)GetDlgItem(IDC_VIDEO_FILTERS);
	int i = pMainList->GetCurSel();
	if (i == LB_ERR)
		return;

	// Get moniker of the selected filter
	IMoniker *pMoniker = (IMoniker*)pMainList->GetItemDataPtr(i);

	// Check the moniker
	if ((pMoniker == NULL) || (pMoniker == (IMoniker*)-1)) 
		return;

	// Instantiate a filter
	IBaseFilter *pBaseFilter = NULL;
	HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pBaseFilter);
	if (FAILED(hr))
		return;

	// Get IAMVfwCompressDialogs interface on the filter
	IAMVfwCompressDialogs *pAMVfwCompressDialogs = NULL;
	hr = pBaseFilter->QueryInterface(IID_IAMVfwCompressDialogs, (void**)&pAMVfwCompressDialogs);
	if (SUCCEEDED(hr)) {
		GetDlgItem(IDC_VCM_CONFIGURE)->EnableWindow(
			pAMVfwCompressDialogs->ShowDialog(VfwCompressDialog_QueryConfig, NULL) == S_OK
		);
		GetDlgItem(IDC_VCM_ABOUT)->EnableWindow(
			pAMVfwCompressDialogs->ShowDialog(VfwCompressDialog_QueryAbout, NULL) == S_OK
		);
		pAMVfwCompressDialogs->Release();
	}

	// Enumerate the pins on the filter
	IEnumPins *pEnumPins = NULL;
	hr = pBaseFilter->EnumPins(&pEnumPins);
	if (FAILED(hr)) {
		pBaseFilter->Release();
		return;
	}

	// Find a pin providing IAMVideoCompression interface
	IPin *pPin = NULL;
	IAMVideoCompression *pAMVideoCompression = NULL;
	while (pEnumPins->Next(1, &pPin, NULL) == S_OK) {
		// Try to retrieve IAMVideoCompression
		hr = pPin->QueryInterface(IID_IAMVideoCompression, (void**)&pAMVideoCompression);
		pPin->Release();
		if (SUCCEEDED(hr))
			break;
	}
	// Release the pin enumerator
	pEnumPins->Release();

	// If the filter has no pin with required interface, we're stuck
	if (pAMVideoCompression == NULL) {
		pBaseFilter->Release();
		return;
	}

	WCHAR wszVersion[1024];
	int cbVersion = 1024;
	WCHAR wszDescription[1024];
	int cbDescription = 1024;
	long lDefaultKeyFrameRate = -1, lDefaultPFramesPerKey = -1;
	double dDefaultQuality = 1.0;
	long lCapabilities = 0;
	hr = pAMVideoCompression->GetInfo(
		wszVersion,
		&cbVersion,
		wszDescription,
		&cbDescription,
		&lDefaultKeyFrameRate,
		&lDefaultPFramesPerKey,
		&dDefaultQuality,
		&lCapabilities
	);
	if (SUCCEEDED(hr)) {

		// ---- Update video controls ----

		GetDlgItem(IDC_VIDEO_BITRATE_CHECK)->EnableWindow(
			lCapabilities & CompressionCaps_CanCrunch
		);

		BOOL bSupported = lCapabilities & CompressionCaps_CanQuality;
		GetDlgItem(IDC_VIDEO_QUALITY_CHECK)->EnableWindow(bSupported);
		if (bSupported) {
			CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(IDC_VIDEO_QUALITY_SLIDER);
			pSlider->SetRange(0, 100, TRUE);
			pSlider->SetPos((int)(dDefaultQuality * 100.));
			SetDlgItemInt(IDC_VIDEO_QUALITY_EDIT, (UINT)(dDefaultQuality * 100.), FALSE);
		}

		bSupported = lCapabilities & CompressionCaps_CanKeyFrame;
		GetDlgItem(IDC_VIDEO_KEYFRAME_CHECK)->EnableWindow(bSupported);
		if ((bSupported) && (lDefaultKeyFrameRate != -1))
			SetDlgItemInt(IDC_VIDEO_KEYFRAME_EDIT, lDefaultKeyFrameRate, FALSE);

		bSupported = lCapabilities & CompressionCaps_CanBFrame;
		GetDlgItem(IDC_VIDEO_PFRAME_CHECK)->EnableWindow(bSupported);
		if ((bSupported) && (lDefaultPFramesPerKey != -1))
			SetDlgItemInt(IDC_VIDEO_PFRAME_EDIT, lDefaultPFramesPerKey, FALSE);

		GetDlgItem(IDC_VIDEO_WINDOWSIZE_CHECK)->EnableWindow(
			lCapabilities & CompressionCaps_CanWindow
		);

	}
	pAMVideoCompression->Release();
	pBaseFilter->Release();
}

void CVideoCompressionPage::OnVcmConfigure() 
{
	// Get current selection
	CListBox *pMainList = (CListBox*)GetDlgItem(IDC_VIDEO_FILTERS);
	int i = pMainList->GetCurSel();
	if (i == LB_ERR)
		return;

	// Get moniker of the selected filter
	IMoniker *pMoniker = (IMoniker*)pMainList->GetItemDataPtr(i);
	if ((pMoniker == NULL) || (pMoniker == (IMoniker*)-1))
		return;

	// Instantiate a filter
	IBaseFilter *pBaseFilter = NULL;
	HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pBaseFilter);
	if (FAILED(hr))
		return;

	// Get IAMVfwCompressDialogs interface on the filter
	IAMVfwCompressDialogs *pAMVfwCompressDialogs = NULL;
	hr = pBaseFilter->QueryInterface(IID_IAMVfwCompressDialogs, (void**)&pAMVfwCompressDialogs);
	pBaseFilter->Release();
	if (FAILED(hr))
		return;

	// Bring up the dialog
	pAMVfwCompressDialogs->ShowDialog(VfwCompressDialog_Config, this->m_hWnd);

	// Release the interface
	pAMVfwCompressDialogs->Release();
}

void CVideoCompressionPage::OnVcmAbout() 
{
	// Get current selection
	CListBox *pMainList = (CListBox*)GetDlgItem(IDC_VIDEO_FILTERS);
	int i = pMainList->GetCurSel();
	if (i == LB_ERR)
		return;

	// Get moniker of the selected filter
	IMoniker *pMoniker = (IMoniker*)pMainList->GetItemDataPtr(i);
	if ((pMoniker == NULL) || (pMoniker == (IMoniker*)-1))
		return;

	// Instantiate a filter
	IBaseFilter *pBaseFilter = NULL;
	HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pBaseFilter);
	if (FAILED(hr))
		return;

	// Get IAMVfwCompressDialogs interface on the filter
	IAMVfwCompressDialogs *pAMVfwCompressDialogs = NULL;
	hr = pBaseFilter->QueryInterface(IID_IAMVfwCompressDialogs, (void**)&pAMVfwCompressDialogs);
	pBaseFilter->Release();
	if (FAILED(hr))
		return;

	// Bring up the dialog
	pAMVfwCompressDialogs->ShowDialog(VfwCompressDialog_About, this->m_hWnd);
	pAMVfwCompressDialogs->Release();
}

void CVideoCompressionPage::ResetVideoControls()
{
	// Disable all controls except filter list
	GetDlgItem(IDC_VIDEO_BITRATE_CHECK)->EnableWindow(FALSE);
	UpdateVideoBitrateControls(FALSE);
	GetDlgItem(IDC_VIDEO_QUALITY_CHECK)->EnableWindow(FALSE);
	UpdateVideoQualityControls(FALSE);
	GetDlgItem(IDC_VIDEO_KEYFRAME_CHECK)->EnableWindow(FALSE);
	UpdateVideoKeyframeControl(FALSE);
	GetDlgItem(IDC_VIDEO_PFRAME_CHECK)->EnableWindow(FALSE);
	UpdateVideoPframeControl(FALSE);
	GetDlgItem(IDC_VIDEO_WINDOWSIZE_CHECK)->EnableWindow(FALSE);
	UpdateVideoWindowsizeControl(FALSE);
	GetDlgItem(IDC_VCM_CONFIGURE)->EnableWindow(FALSE);
	GetDlgItem(IDC_VCM_ABOUT)->EnableWindow(FALSE);

	// Reset the values of video compression settings
	CheckDlgButton(IDC_VIDEO_BITRATE_CHECK, 0);
	SetDlgItemText(IDC_VIDEO_BITRATE_EDIT, _T(""));
	CheckDlgButton(IDC_VIDEO_QUALITY_CHECK, 0);
	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(IDC_VIDEO_QUALITY_SLIDER);
	pSlider->SetRange(0, 0, TRUE);
	pSlider->SetPos(0);
	SetDlgItemText(IDC_VIDEO_QUALITY_EDIT, _T(""));
	CheckDlgButton(IDC_VIDEO_KEYFRAME_CHECK, 0);
	SetDlgItemText(IDC_VIDEO_KEYFRAME_EDIT, _T(""));
	CheckDlgButton(IDC_VIDEO_PFRAME_CHECK, 0);
	SetDlgItemText(IDC_VIDEO_PFRAME_EDIT, _T(""));
	CheckDlgButton(IDC_VIDEO_WINDOWSIZE_CHECK, 0);
	SetDlgItemText(IDC_VIDEO_WINDOWSIZE_EDIT, _T(""));
}

void CVideoCompressionPage::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(IDC_VIDEO_QUALITY_SLIDER);
	if (pScrollBar == (CWnd*)pSlider)
		SetDlgItemInt(IDC_VIDEO_QUALITY_EDIT, pSlider->GetPos(), FALSE);

	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CVideoCompressionPage::OnChangeVideoQualityEdit() 
{
	int iVideoQuality = GetDlgItemInt(IDC_VIDEO_QUALITY_EDIT, NULL, FALSE);
	if (iVideoQuality > 100) {
		iVideoQuality = 100;
		SetDlgItemInt(IDC_VIDEO_QUALITY_EDIT, iVideoQuality, FALSE);
	}
	((CSliderCtrl*)GetDlgItem(IDC_VIDEO_QUALITY_SLIDER))->SetPos(iVideoQuality);
}

void CVideoCompressionPage::OnOK() 
{
	ASSERT(m_pView != NULL);
	CListCtrl& list = m_pView->GetListCtrl();
	POSITION pos = list.GetFirstSelectedItemPosition();
	ASSERT(pos != NULL);

	CListBox *pFilterList = (CListBox*)GetDlgItem(IDC_VIDEO_FILTERS);

	// Get the selected filter moniker
	IMoniker *pMoniker = NULL;
	int i = pFilterList->GetCurSel();
	if (i != LB_ERR)
		pMoniker = (IMoniker*)pFilterList->GetItemDataPtr(i);

	// ---- Get the video compression parameters ----

	int		iVideoQuality = GetDlgItemInt(IDC_VIDEO_QUALITY_EDIT);
	BOOL	bVideoQuality = GetDlgButtonCheck(IDC_VIDEO_QUALITY_CHECK);
	DWORD	dwBitRate = GetDlgItemInt(IDC_VIDEO_BITRATE_EDIT, NULL, FALSE);
	BOOL	bBitRate = GetDlgButtonCheck(IDC_VIDEO_BITRATE_CHECK);
	long	lKeyFrameRate = GetDlgItemInt(IDC_VIDEO_KEYFRAME_EDIT);
	BOOL	bKeyFrameRate = GetDlgButtonCheck(IDC_VIDEO_KEYFRAME_CHECK);
	long	lPFramesPerKeyFrame = GetDlgItemInt(IDC_VIDEO_PFRAME_EDIT);
	BOOL	bPFramesPerKeyFrame = GetDlgButtonCheck(IDC_VIDEO_PFRAME_CHECK);
	DWORD	dwWindowSize = GetDlgItemInt(IDC_VIDEO_WINDOWSIZE_EDIT, NULL, FALSE);
	BOOL	bWindowSize = GetDlgButtonCheck(IDC_VIDEO_WINDOWSIZE_CHECK);

	// Set the parameters for all selected list items
	while (pos) {

		i = list.GetNextSelectedItem(pos);
		CGMFConvertItem *pItem = (CGMFConvertItem*)list.GetItemData(i);
		ASSERT(pItem != NULL);

		pItem->SetVideoFilterMoniker(pMoniker);
		pItem->SetBitRate(bBitRate ? dwBitRate : -1);
		pItem->SetKeyFrameRate(bKeyFrameRate ? lKeyFrameRate : -1);
		pItem->SetPFramesPerKeyFrame(bPFramesPerKeyFrame ? lPFramesPerKeyFrame : -1);
		pItem->SetVideoQuality(bVideoQuality ? ((double)iVideoQuality / 100) : -1.0);
		pItem->SetWindowSize(bWindowSize ? dwWindowSize : (DWORDLONG)-1);
		if (CHECKMONIKER(pMoniker)) {
			LPVOID	pVideoCodecState	= NULL;
			int		cbVideoCodecState	= 0;
			GetCodecState(pMoniker, pVideoCodecState, cbVideoCodecState);
			pItem->SetVideoCodecState(pVideoCodecState, cbVideoCodecState);
		} else
			pItem->SetVideoCodecState(NULL, 0);
	}

	CPropertyPage::OnOK();
}

void CVideoCompressionPage::SetCodecState(IMoniker *pMoniker, LPVOID pState, int cbState)
{
	// Instantiate a filter
	IBaseFilter *pBaseFilter = NULL;
	HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pBaseFilter);
	if (FAILED(hr))
		return;

	// Get IAMVfwCompressDialogs interface on the filter
	IAMVfwCompressDialogs *pAMVfwCompressDialogs = NULL;
	hr = pBaseFilter->QueryInterface(IID_IAMVfwCompressDialogs, (void**)&pAMVfwCompressDialogs);
	pBaseFilter->Release();
	if (FAILED(hr))
		return;

	CString str;
	str.Format("SetState: p=0x%lX cb=%d", pState, cbState);
	AfxMessageBox(str);

	// Set the codec state (if any)
	if ((pState != NULL) && (cbState != 0))
		pAMVfwCompressDialogs->SetState(pState, cbState);

	// Release the interface
	pAMVfwCompressDialogs->Release();
}

void CVideoCompressionPage::GetCodecState(IMoniker *pMoniker, LPVOID& pState, int& cbState)
{
	// Instantiate a filter
	IBaseFilter *pBaseFilter = NULL;
	HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pBaseFilter);
	if (FAILED(hr))
		return;

	// Get IAMVfwCompressDialogs interface on the filter
	IAMVfwCompressDialogs *pAMVfwCompressDialogs = NULL;
	hr = pBaseFilter->QueryInterface(IID_IAMVfwCompressDialogs, (void**)&pAMVfwCompressDialogs);
	pBaseFilter->Release();
	if (FAILED(hr))
		return;

	// Free the old codec state buffer
	if (pState != NULL) {
		CoTaskMemFree(pState);
		pState = NULL;
	}
	cbState = 0;

	// Find out how large the new state buffer should be
	hr = pAMVfwCompressDialogs->GetState(NULL, &cbState);

	// Allocate and fill the new state buffer
	if (SUCCEEDED(hr)) {
		pState = CoTaskMemAlloc(cbState);
		if (pState != NULL) {
			hr = pAMVfwCompressDialogs->GetState(pState, &cbState);
			if (FAILED(hr)) {
				CoTaskMemFree(pState);
				pState = NULL;
				cbState = 0;
			}
		} else
			cbState = 0;
	}

	// Release the interface
	pAMVfwCompressDialogs->Release();
}
