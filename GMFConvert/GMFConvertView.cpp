// GMFConvertView.cpp : implementation of the CGMFConvertView class
//

#include "stdafx.h"
#include "GMFConvert.h"

#include "GMFConvertItem.h"
#include "GMFConvertView.h"

#include "OutputFilePathPage.h"
#include "AudioCompressionPage.h"
#include "VideoCompressionPage.h"

#include <stdlib.h>
#include <dshow.h>
#include <streams.h>
#include <shlobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertView

IMPLEMENT_DYNCREATE(CGMFConvertView, CListView)

BEGIN_MESSAGE_MAP(CGMFConvertView, CListView)
	//{{AFX_MSG_MAP(CGMFConvertView)
	ON_UPDATE_COMMAND_UI(ID_ADD_DIR, OnUpdateAddDir)
	ON_UPDATE_COMMAND_UI(ID_ADD_FILE, OnUpdateAddFile)
	ON_UPDATE_COMMAND_UI(ID_CLEAR_LIST, OnUpdateClearList)
	ON_COMMAND(ID_ADD_FILE, OnAddFile)
	ON_COMMAND(ID_CLEAR_LIST, OnClearList)
	ON_UPDATE_COMMAND_UI(ID_CONFIG_CONVERT, OnUpdateConfigConvert)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_INVERT_SEL, OnUpdateInvertSel)
	ON_UPDATE_COMMAND_UI(ID_PAUSE_CONVERT, OnUpdatePauseConvert)
	ON_UPDATE_COMMAND_UI(ID_SELECT_ALL, OnUpdateSelectAll)
	ON_UPDATE_COMMAND_UI(ID_UNSELECT_ALL, OnUpdateUnselectAll)
	ON_UPDATE_COMMAND_UI(ID_RUN_CONVERT, OnUpdateRunConvert)
	ON_UPDATE_COMMAND_UI(ID_STOP_CONVERT, OnUpdateStopConvert)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_INVERT_SEL, OnInvertSel)
	ON_COMMAND(ID_SELECT_ALL, OnSelectAll)
	ON_COMMAND(ID_UNSELECT_ALL, OnUnselectAll)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS, OnUpdateOptions)
	ON_COMMAND(ID_ADD_DIR, OnAddDir)
	ON_COMMAND(ID_OPTIONS, OnOptions)
	ON_NOTIFY_REFLECT(HDN_ITEMCLICK, OnItemClick)
	ON_COMMAND(ID_CONFIG_CONVERT, OnConfigConvert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertView construction/destruction

CGMFConvertView::CGMFConvertView() :
	m_iSortItem(-1),		// No sorting at this time
	m_bSortDescending(TRUE)	// Default is descending sorting
{
	CWinApp *pApp = AfxGetApp();

	// -- Retrieve the options from the registry --

	m_bGetDurationOnAdd = (BOOL)pApp->GetProfileInt(_T(""), CString((LPCSTR)IDS_OPTION_DURATION_ADD), -1);
	if (m_bGetDurationOnAdd == -1) {
		pApp->WriteProfileInt(_T(""), CString((LPCSTR)IDS_OPTION_DURATION_ADD), FALSE);
		m_bGetDurationOnAdd = FALSE;
	}
	m_bRecursiveAddDir = (BOOL)pApp->GetProfileInt(_T(""), CString((LPCSTR)IDS_OPTION_RECURSIVE_ADDDIR), -1);
	if (m_bRecursiveAddDir == -1) {
		pApp->WriteProfileInt(_T(""), CString((LPCSTR)IDS_OPTION_RECURSIVE_ADDDIR), TRUE);
		m_bRecursiveAddDir = TRUE;
	}
}

CGMFConvertView::~CGMFConvertView()
{
	CWinApp *pApp = AfxGetApp();

	// -- Store the options in the registry -- 

	pApp->WriteProfileInt(_T(""), CString((LPCSTR)IDS_OPTION_DURATION_ADD), (int)m_bGetDurationOnAdd);
	pApp->WriteProfileInt(_T(""), CString((LPCSTR)IDS_OPTION_RECURSIVE_ADDDIR), (int)m_bRecursiveAddDir);
}

BOOL CGMFConvertView::PreCreateWindow(CREATESTRUCT& cs)
{
	// Report list view style
	cs.style |= LVS_REPORT | LVS_SHOWSELALWAYS;

	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertView drawing

void CGMFConvertView::OnDraw(CDC* pDC)
{
	CGMFConvertDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

void CGMFConvertView::OnInitialUpdate()
{
	CListView::OnInitialUpdate();

	CListCtrl& list = GetListCtrl();

	list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	// Set image list for the list control
	m_ListImageList.Create(IDB_STATUS_SMALL, 16, 1, RGB(0xFF, 0xFF, 0xFF));
	list.SetImageList(&m_ListImageList, LVSIL_SMALL);

	// Modify the header control style
	CHeaderCtrl *pHeader = list.GetHeaderCtrl();
	pHeader->ModifyStyle(0, HDS_HOTTRACK | HDS_DRAGDROP | HDS_FULLDRAG | HDS_BUTTONS);

	// Set image list for the header control
	m_HeaderImageList.Create(IDB_HEADERSORT_SMALL, 16, 1, RGB(0xFF, 0xFF, 0xFF));
	pHeader->SetImageList(&m_HeaderImageList);

	// Set up the columns of the list control
	list.InsertColumn(0, _T("Status"), LVCFMT_LEFT, 100, 0);
	list.InsertColumn(1, _T("Source file"), LVCFMT_LEFT, 150, 1);
	list.InsertColumn(2, _T("Source folder"), LVCFMT_LEFT, 150, 2);
	list.InsertColumn(3, _T("Duration"), LVCFMT_RIGHT, 90, 3);
	list.InsertColumn(4, _T("Size"), LVCFMT_RIGHT, 80, 4);
	list.InsertColumn(5, _T("Destination file"), LVCFMT_LEFT, 120, 5);
	list.InsertColumn(6, _T("Destination folder"), LVCFMT_LEFT, 150, 6);	
}

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertView diagnostics

#ifdef _DEBUG
void CGMFConvertView::AssertValid() const
{
	CListView::AssertValid();
}

void CGMFConvertView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CGMFConvertDoc* CGMFConvertView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGMFConvertDoc)));
	return (CGMFConvertDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertView message handlers

void CGMFConvertView::OnItemClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMHEADER *phdn = (NMHEADER*)pNMHDR;

	*pResult = 0;

	// React only on the left mouse button
	if (phdn->iButton == 0) {

		// If the list is empty, no sorting
		CListCtrl& list = GetListCtrl();
		if (list.GetItemCount() == 0)
			return;

		CHeaderCtrl *pHeader = list.GetHeaderCtrl();
		HDITEM hditem = {0};

		// If the clicked item is already the sorting item, change sort order
		if (phdn->iItem == m_iSortItem) {

			// Perform sorting
			if (SortList(phdn->iItem, !m_bSortDescending)) {

				// Invert sort order
				m_bSortDescending = !m_bSortDescending;

				// Set the item's image
				hditem.mask = HDI_IMAGE;
				hditem.iImage = (m_bSortDescending) ? 0 : 1;
				pHeader->SetItem(phdn->iItem, &hditem);
			}

		} else { // Sort descending

			// Perform sorting
			if (SortList(phdn->iItem, TRUE)) {

				// Clear image display for the previous sort item
				if (m_iSortItem != -1) {
					hditem.mask = HDI_FORMAT;
					pHeader->GetItem(m_iSortItem, &hditem);
					hditem.fmt &= ~HDF_IMAGE;
					pHeader->SetItem(m_iSortItem, &hditem);
				}

				// Set the sort parameters
				m_iSortItem = phdn->iItem;
				m_bSortDescending = TRUE;

				// Instruct the item to diplay image
				hditem.mask = HDI_FORMAT;
				pHeader->GetItem(phdn->iItem, &hditem);
				hditem.mask = HDI_FORMAT | HDI_IMAGE;
				BOOL bBitmapOnRight = (phdn->iItem != 3) && (phdn->iItem != 4);
				hditem.fmt |= HDF_IMAGE | ((bBitmapOnRight) ? HDF_BITMAP_ON_RIGHT : 0);
				hditem.iImage = 0;
				pHeader->SetItem(phdn->iItem, &hditem);
			}
		}
	}
}

void CGMFConvertView::OnUpdateAddDir(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);	
}

void CGMFConvertView::OnUpdateAddFile(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);	
}

void CGMFConvertView::OnUpdateClearList(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	pCmdUI->Enable(list.GetItemCount() > 0);
}

void CGMFConvertView::OnAddFile() 
{
	// Create and configure open file dialog
	CFileDialog dlgAddFile(
		TRUE,
		NULL,
		_T(""),
		OFN_HIDEREADONLY		| 
		OFN_ALLOWMULTISELECT	| 
		OFN_NODEREFERENCELINKS	|
		OFN_SHAREAWARE			|
		//OFN_PATHMUSTEXIST		|
		//OFN_FILEMUSTEXIST		|
		OFN_EXPLORER,
		_T("All files (*.*)\0*.*\0")
	);
	// File name buffer fits 256 names each being 256 chars large
	dlgAddFile.m_ofn.nMaxFile = 0xFFFF;
	dlgAddFile.m_ofn.lpstrFile = new TCHAR[dlgAddFile.m_ofn.nMaxFile];
	lstrcpy(dlgAddFile.m_ofn.lpstrFile, _T("*.*"));
	dlgAddFile.m_ofn.lpstrFileTitle = NULL;
	TCHAR szDirectory[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szDirectory);
	dlgAddFile.m_ofn.lpstrInitialDir = szDirectory;

	if (dlgAddFile.DoModal() == IDOK) {

		CListCtrl& list = GetListCtrl();
		int nOldItemCount = list.GetItemCount();

		POSITION pos = dlgAddFile.GetStartPosition();
		while (pos != NULL)
			AddFile(dlgAddFile.GetNextPathName(pos));

		if (list.GetItemCount() > nOldItemCount)
			ResetSort();
	}

	// Clean up
	delete[] dlgAddFile.m_ofn.lpstrFile;
}

void CGMFConvertView::AddFile(CString szFileName)
{
	CListCtrl& list = GetListCtrl();
	int i = list.GetItemCount();
	CString szTemp;

	// -- Status subitem --
	list.InsertItem(
		LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM,
		i,
		CString((LPCSTR)IDS_WAITING),
		0,
		0,
		STATUS_IMAGE_WAITING,
		(LPARAM)(new CGMFConvertItem())
	);

	// -- Source file title subitem --
	list.SetItem(
		i,
		1,
		LVIF_TEXT,
		szFileName.Mid(szFileName.ReverseFind(_T('\\')) + 1),
		0,
		0,
		0,
		0
	);

	// -- Source file folder subitem --
	list.SetItem(
		i,
		2,
		LVIF_TEXT,
		szFileName.Left(szFileName.ReverseFind(_T('\\')) + 1),
		0,
		0,
		0,
		0
	);

	// -- File duration subitem --
	
	DWORD dwDuration = (DWORD)-1;
	if (m_bGetDurationOnAdd) {
		
		// Get a graph builder interface
		IGraphBuilder *pGraphBuilder = NULL;
		HRESULT hr = CoCreateInstance(
			CLSID_FilterGraph,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IGraphBuilder,
			(void**)&pGraphBuilder
		);
		if (SUCCEEDED(hr)) {

			// Deal with the file name properly
			WCHAR wszFileName[MAX_PATH];
#ifdef _UNICODE 
			lstrcpyW(wszFileName, szFileName);
#else
			MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wszFileName, MAX_PATH);
#endif

			// Build a filter graph
			hr = pGraphBuilder->RenderFile(wszFileName, NULL);
			if (SUCCEEDED(hr))
				dwDuration = GetDuration(pGraphBuilder);
			pGraphBuilder->Release();
		}
	}

	// Make the duration string
	CString szDuration;
	MakeDurationString(dwDuration, szDuration);

	// Set the subitem
	list.SetItem(i, 3, LVIF_TEXT, szDuration, 0, 0, 0, 0);

	// -- File size subitem --

	// Open the file and get its length
	CFile file;
	CString szSize(_T(""));
	DWORD dwSize;
	if (file.Open(szFileName, CFile::modeRead | CFile::shareDenyNone)) {
		for (dwSize = file.GetLength(); dwSize > 0; dwSize /= 1000) {
			szTemp.Format(_T(" %.3u"), dwSize % 1000);
			szSize = szTemp + szSize;
		}
		szSize.TrimLeft(_T(" 0"));
		if (szSize == _T(""))
			szSize = _T("0");
		file.Close();
	} else
		szSize = _T("N/A");
	
	// Set the subitem
	list.SetItem(i, 4, LVIF_TEXT, szSize, 0, 0, 0, 0);

	// -- Destination file title subitem --
	szTemp = list.GetItemText(i, 1);
	int iDot = szTemp.ReverseFind(_T('.'));
	if (iDot != -1)
		szTemp = szTemp.Left(iDot);
	if (!szTemp.IsEmpty())
		szTemp += _T(".avi");
	list.SetItem(
		i,
		5,
		LVIF_TEXT,
		szTemp,
		0,
		0,
		0,
		0
	);

	// -- Destination file folder subitem --
	list.SetItem(
		i,
		6,
		LVIF_TEXT,
		szFileName.Left(szFileName.ReverseFind(_T('\\')) + 1),
		0,
		0,
		0,
		0
	);

	// Force the item to be redrawn
	list.Update(i);
}

void CGMFConvertView::OnClearList() 
{
	CListCtrl& list = GetListCtrl();

	// Delete list items' data
	for (int i = 0; i < list.GetItemCount(); i++) {
		CGMFConvertItem *pItem = (CGMFConvertItem*)list.GetItemData(i);
		ASSERT(pItem != NULL);
		delete pItem;
	}

	// Clear the list
	list.DeleteAllItems();

	ResetSort();
}

DWORD CGMFConvertView::GetDuration(IUnknown *pGraph)
{
	// Get a video window interface
	IVideoWindow *pVideoWindow = NULL;
	HRESULT hr = pGraph->QueryInterface(IID_IVideoWindow, (void**)&pVideoWindow);
	if (SUCCEEDED(hr)) {
		// Make the video window invisible
		pVideoWindow->put_Visible(OAFALSE);
		pVideoWindow->Release();
	}

	DWORD dwDuration = (DWORD)-1;
	IMediaSeeking *pMediaSeeking = NULL;
	try {

		// Get a media seeking interface
		hr = pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pMediaSeeking);
		if (FAILED(hr)) 
			throw (int)0;

		// Get the capabilties
		DWORD dwCaps = 0;
		hr = pMediaSeeking->GetCapabilities(&dwCaps);
		if (FAILED(hr))
			throw (int)0;

		// Find out if we can get duration
		if (!(dwCaps & AM_SEEKING_CanGetDuration))
			throw (int)0;

		// Set the convenient time format
		hr = pMediaSeeking->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
		if (FAILED(hr))
			throw (int)0;

		// Get the duration
		LONGLONG llDuration = 0;
		hr = pMediaSeeking->GetDuration(&llDuration);
		if (FAILED(hr))
			throw (int)0;

		pMediaSeeking->Release();

		// Convert the duration to DWORD
		dwDuration = (DWORD)(llDuration / UNITS);
	}
	catch (int) {

		// Release the media seeking interface
		if (pMediaSeeking)
			pMediaSeeking->Release();

		// Get a media position interface
		IMediaPosition *pMediaPosition = NULL;
		hr = pGraph->QueryInterface(IID_IMediaPosition, (void**)&pMediaPosition);
		if (SUCCEEDED(hr)) {
			// Get the duration and convert it to DWORD
			REFTIME rtDuration = 0;
			hr = pMediaPosition->get_Duration(&rtDuration);
			pMediaPosition->Release();
			if (SUCCEEDED(hr))
				dwDuration = (DWORD)rtDuration;
		}
	}

	return dwDuration;
}

void CGMFConvertView::MakeDurationString(DWORD dwDuration, CString& szDuration)
{
	CString szTemp;

	// Convert the duration value to string
	if (dwDuration != (DWORD)-1) {

		// Seconds
		szTemp.Format(_T(":%.2u"), dwDuration % 60);
		szDuration = szTemp;
		dwDuration /= 60;
		
		// Minutes
		szTemp.Format(_T(":%.2u"), dwDuration % 60);
		szDuration = szTemp + szDuration;
		dwDuration /= 60;

		// Hours
		if (dwDuration > 0) {
			szTemp.Format(_T("%u"), dwDuration);
			szDuration = szTemp + szDuration;
		} else {
			szDuration.Delete(0);
			if (szDuration[0] == _T('0'))
				szDuration.Delete(0);
		}
	} else
		szDuration = _T("N/A");
}

void CGMFConvertView::OnUpdateConfigConvert(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	pCmdUI->Enable(list.GetSelectedCount() > 0);
}

void CGMFConvertView::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	pCmdUI->Enable(list.GetSelectedCount() > 0);
}

void CGMFConvertView::OnUpdateInvertSel(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	pCmdUI->Enable(list.GetItemCount() > 0);
}

void CGMFConvertView::OnUpdatePauseConvert(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CGMFConvertView::OnUpdateSelectAll(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	pCmdUI->Enable(
		(list.GetItemCount() > 0) && 
		(list.GetSelectedCount() < (UINT)list.GetItemCount())
	);
}

void CGMFConvertView::OnUpdateUnselectAll(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	pCmdUI->Enable(list.GetSelectedCount() > 0);
}

void CGMFConvertView::OnUpdateRunConvert(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	pCmdUI->Enable(list.GetItemCount() > 0);
}

void CGMFConvertView::OnUpdateStopConvert(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CGMFConvertView::OnEditCut() 
{
	CListCtrl& list = GetListCtrl();

	while (list.GetSelectedCount() > 0) {
		POSITION pos = list.GetFirstSelectedItemPosition();
		if (pos != NULL) {
			int i = list.GetNextSelectedItem(pos);
			CGMFConvertItem *pItem = (CGMFConvertItem*)list.GetItemData(i);
			ASSERT(pItem != NULL);
			delete pItem;
			list.DeleteItem(i);
		}
	}

	if (list.GetItemCount() == 0)
		ResetSort();
}

void CGMFConvertView::OnInvertSel() 
{
	CListCtrl& list = GetListCtrl();

	for (int i = 0; i < list.GetItemCount(); i++)
		list.SetItemState(
			i,
			~list.GetItemState(i, LVIS_SELECTED),
			LVIS_SELECTED
		);
}

void CGMFConvertView::OnSelectAll() 
{
	CListCtrl& list = GetListCtrl();

	for (int i = 0; i < list.GetItemCount(); i++)
		list.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
}

void CGMFConvertView::OnUnselectAll() 
{
	CListCtrl& list = GetListCtrl();

	for (int i = 0; i < list.GetItemCount(); i++)
		list.SetItemState(i, 0, LVIS_SELECTED);
}

void CGMFConvertView::OnUpdateOptions(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();	
}

BOOL CGMFConvertView::BrowseForFolder(CString szTitle, CString& szFolder)
{
	// Prepare for browsing
	BROWSEINFO bi = {0};
	bi.hwndOwner		= this->m_hWnd;
	bi.pidlRoot			= NULL;
	bi.pszDisplayName	= szFolder.GetBuffer(MAX_PATH);
	bi.lpszTitle		= szTitle;
	bi.ulFlags			= BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	// Browse for folder
	LPITEMIDLIST pIDL = SHBrowseForFolder(&bi);
	if (pIDL != NULL) {

		// Get actual browsed path
		SHGetPathFromIDList(pIDL, szFolder.GetBuffer(MAX_PATH));
		szFolder.ReleaseBuffer();
		
		// Free the IDL
		IMalloc *pMalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
			pMalloc->Free(pIDL);
			pMalloc->Release();
		}

		return TRUE;
	} else {
		szFolder.ReleaseBuffer();
		return FALSE;
	}
}

void CGMFConvertView::OnAddDir() 
{
	// Browse for folder
	CString szFolder;
	CString szTitle((LPCSTR)IDS_BROWSE_DIR_ADD);
	if (BrowseForFolder(szTitle, szFolder)) {

		CListCtrl& list = GetListCtrl();
		int nOldItemCount = list.GetItemCount();

		AddDir(szFolder);

		if (list.GetItemCount() > nOldItemCount)
			ResetSort();
	}
}

void CGMFConvertView::AddDir(CString szFolder)
{
	// Append backslash if necessary
	if (szFolder.Right(1) != _T("\\"))
		szFolder += _T("\\");

	// Iterate over the files/subdirectories in the folder
	CFileFind finder;
	BOOL bWorking = finder.FindFile(szFolder + _T("*"));
	while (bWorking) {
		bWorking = finder.FindNextFile();
		if (finder.IsDirectory()) {
			if (!finder.IsDots() && m_bRecursiveAddDir)
				AddDir(finder.GetFilePath());
		} else if (!finder.IsHidden() && !finder.IsTemporary())
			AddFile(finder.GetFilePath());
	}
	finder.Close();
}

void CGMFConvertView::OnOptions() 
{
	COptionsDlg optionsDlg(this);
	optionsDlg.DoModal();
}

int CALLBACK CGMFConvertView::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	SORT_LIST_DATA *pData = (SORT_LIST_DATA*)lParamSort;

	// Find the items to be compared
	LVFINDINFO lvfi;
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = lParam1;
	int iItem1 = pData->pList->FindItem(&lvfi);
	ASSERT(iItem1 != -1);
	lvfi.lParam = lParam2;
	int iItem2 = pData->pList->FindItem(&lvfi);
	ASSERT(iItem2 != -1);

	// Get the strings from the items
	CString szItem1 = pData->pList->GetItemText(iItem1, pData->iSortItem);
	CString szItem2 = pData->pList->GetItemText(iItem2, pData->iSortItem);

	if ((pData->iSortItem == 3) || (pData->iSortItem == 4)) {

		// Clean the strings from the non-digit characters
		szItem1.Remove(_T(' '));
		szItem2.Remove(_T(' '));
		szItem1.Remove(_T(':'));
		szItem2.Remove(_T(':'));

		// Convert the strings to numbers
		long lItem1 = atol(szItem1);
		long lItem2 = atol(szItem2);

		return ((pData->bSortDescending) ? -1 : 1) * (lItem1 - lItem2);

	} else
		return ((pData->bSortDescending) ? -1 : 1) * szItem1.Compare(szItem2);
}

BOOL CGMFConvertView::SortList(int iSortItem, BOOL bSortDescending)
{
	CListCtrl& list = GetListCtrl();

	SORT_LIST_DATA SortData;
	SortData.pList = &list;
	SortData.iSortItem = iSortItem;
	SortData.bSortDescending = bSortDescending;

	return list.SortItems((PFNLVCOMPARE)CompareFunc, (DWORD)&SortData);
}

void CGMFConvertView::ResetSort()
{
	// Clear image display for the current sort item
	if (m_iSortItem != -1) {
		CListCtrl& list = GetListCtrl();
		CHeaderCtrl *pHeader = list.GetHeaderCtrl();
		HDITEM hditem = {0};
		hditem.mask = HDI_FORMAT;
		pHeader->GetItem(m_iSortItem, &hditem);
		hditem.fmt &= ~HDF_IMAGE;
		pHeader->SetItem(m_iSortItem, &hditem);
	}

	// Reset the sort parameters
	m_iSortItem = -1;
	m_bSortDescending = TRUE;
}

void CGMFConvertView::OnConfigConvert() 
{
	CPropertySheet psConfig(IDS_CONFIGURE_TITLE, this);
	COutputFilePathPage pspOutputFilePath(this);
	CAudioCompressionPage pspAudioCompression(this);
	CVideoCompressionPage pspVideoCompression(this);
	psConfig.AddPage(&pspOutputFilePath);
	psConfig.AddPage(&pspAudioCompression);
	psConfig.AddPage(&pspVideoCompression);
	psConfig.m_psh.dwFlags |= PSH_NOAPPLYNOW;
	psConfig.DoModal();
}
