// OutputFilePathPage.cpp : implementation file
//

#include "stdafx.h"
#include "GMFConvert.h"
#include "OutputFilePathPage.h"
#include "GMFConvertItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COutputFilePathPage property page

COutputFilePathPage::COutputFilePathPage(CGMFConvertView *pParent) :
	CPropertyPage(COutputFilePathPage::IDD),
	m_pView(pParent)
{
	// Load bitmap resources
	m_bmBrowseFolder.LoadBitmap(IDB_FOLDER_BROWSE);
	m_bmBrowseFilename.LoadBitmap(IDB_FILENAME_BROWSE);

	// Initialize control variables from the parent view
	ASSERT(pParent != NULL);
	CListCtrl& list = pParent->GetListCtrl();
	ASSERT(list.GetSelectedCount() > 0);
	if (list.GetSelectedCount() == 1) {

		POSITION pos = list.GetFirstSelectedItemPosition();
		ASSERT(pos != NULL);

		int i = list.GetNextSelectedItem(pos);
		CGMFConvertItem *pItem = (CGMFConvertItem*)list.GetItemData(i);
		ASSERT(pItem != NULL);

		// Get folder
		m_szFolder = list.GetItemText(i, 6);
		if (m_szFolder.Right(1) != _T("\\"))
			m_szFolder += _T("\\");
		CString szSourceFolder = list.GetItemText(i, 2);
		m_iFolder = (m_szFolder == szSourceFolder) ? 0 : 1;

		// Get file name and extension
		m_szName = list.GetItemText(i, 5);
		int iDot = m_szName.ReverseFind(_T('.'));
		if (iDot != -1) {
			m_szExtension = m_szName.Mid(iDot);
			if (m_szExtension[0] == _T('.'))
				m_szExtension.Delete(0);
			m_szName = m_szName.Left(iDot);
		} else
			m_szExtension = _T("");
		CString szSourceName = list.GetItemText(i, 1);
		iDot = szSourceName.ReverseFind(_T('.'));
		if (iDot != -1)
			szSourceName = szSourceName.Left(iDot);
		m_iName = (m_szName == szSourceName) ? 0 : 1;

		// Get file type
		m_iType = pItem->GetType();

		// Are we using a default extension?
		if (
			(m_iType == 0)
				? !m_szExtension.CompareNoCase(_T("avi"))
				: !m_szExtension.CompareNoCase(_T("wav"))
		)
			m_szExtension.Empty();

		// Get AVI multiplexing settings
		m_bCompatibilityIndex = pItem->GetCompatibilityIndex();
		m_iMasterStream = pItem->GetMasterStream();
		m_bMasterStream = (m_iMasterStream != -1);
		if (m_iMasterStream == -1)
			m_iMasterStream = 0;
		m_lInterleavingFrequency = pItem->GetInterleavingFrequency();
		m_lInterleavingPreroll = pItem->GetInterleavingPreroll();
		m_iInterleavingMode = pItem->GetInterleavingMode();

	} else {
		m_szFolder = _T("");
		m_iFolder = 0;
		m_szName = _T("");
		m_iName = 0;
		m_iType = 0;
		m_szExtension = _T("");
		m_bCompatibilityIndex = TRUE;
		m_bMasterStream = FALSE;
		m_iMasterStream = 0;
		m_lInterleavingFrequency = 1000;
		m_lInterleavingPreroll = 0;
		m_iInterleavingMode = 0;
	}

	//{{AFX_DATA_INIT(COutputFilePathPage)
	//}}AFX_DATA_INIT
}

COutputFilePathPage::~COutputFilePathPage()
{
}

void COutputFilePathPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COutputFilePathPage)
	DDX_Text(pDX, IDC_OUTPUT_FILENAME_EDIT, m_szName);
	DDX_Text(pDX, IDC_OUTPUT_FOLDER_EDIT, m_szFolder);
	DDX_Text(pDX, IDC_OUTPUT_FILETYPE_EXT_EDIT, m_szExtension);
	DDX_Radio(pDX, IDC_OUTPUT_FILENAME_SAME, m_iName);
	DDX_Radio(pDX, IDC_OUTPUT_FILETYPE_AVI, m_iType);
	DDX_Radio(pDX, IDC_OUTPUT_FOLDER_SAME, m_iFolder);
	DDX_Check(pDX, IDC_AVI_MUX_INDEX, m_bCompatibilityIndex);
	DDX_Check(pDX, IDC_AVI_MUX_MASTER_CHECK, m_bMasterStream);
	DDX_Text(pDX, IDC_AVI_MUX_MASTER_EDIT, m_iMasterStream);
	DDV_MinMaxLong(pDX, m_iMasterStream, 0, 1000000);
	DDX_Text(pDX, IDC_INTERLEAVING_FREQUENCY, m_lInterleavingFrequency);
	DDV_MinMaxLong(pDX, m_lInterleavingFrequency, 1, 1000000000);
	DDX_Text(pDX, IDC_INTERLEAVING_PREROLL, m_lInterleavingPreroll);
	DDX_Radio(pDX, IDC_INTERLEAVING_MODE_NONE, m_iInterleavingMode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COutputFilePathPage, CPropertyPage)
	//{{AFX_MSG_MAP(COutputFilePathPage)
	ON_BN_CLICKED(IDC_OUTPUT_FILENAME_SAME, OnOutputFilenameSame)
	ON_BN_CLICKED(IDC_OUTPUT_FILENAME_USE, OnOutputFilenameUse)
	ON_BN_CLICKED(IDC_OUTPUT_FOLDER_SAME, OnOutputFolderSame)
	ON_BN_CLICKED(IDC_OUTPUT_FOLDER_USE, OnOutputFolderUse)
	ON_BN_CLICKED(IDC_OUTPUT_FOLDER_BROWSE, OnOutputFolderBrowse)
	ON_BN_CLICKED(IDC_OUTPUT_FILENAME_BROWSE, OnOutputFilenameBrowse)
	ON_BN_CLICKED(IDC_OUTPUT_FILETYPE_AVI, OnOutputFiletypeAvi)
	ON_BN_CLICKED(IDC_OUTPUT_FILETYPE_WAV, OnOutputFiletypeWav)
	ON_BN_CLICKED(IDC_AVI_MUX_MASTER_CHECK, OnAviMuxMasterCheck)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COutputFilePathPage message handlers

BOOL COutputFilePathPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// Set up botton icons
	((CButton*)GetDlgItem(IDC_OUTPUT_FOLDER_BROWSE))->SetBitmap(m_bmBrowseFolder);
	((CButton*)GetDlgItem(IDC_OUTPUT_FILENAME_BROWSE))->SetBitmap(m_bmBrowseFilename);

	// Set initial controls state
	UpdateFolderControls(m_iFolder == 1);
	UpdateFilenameControls(m_iName == 1);
	UpdateAviMuxControls(m_iType == 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void COutputFilePathPage::OnOutputFilenameSame() 
{
	UpdateFilenameControls(FALSE);
}

void COutputFilePathPage::OnOutputFilenameUse() 
{
	UpdateFilenameControls(TRUE);
	GotoDlgCtrl(GetDlgItem(IDC_OUTPUT_FILENAME_EDIT));
}

void COutputFilePathPage::OnOutputFolderSame() 
{
	UpdateFolderControls(FALSE);
}

void COutputFilePathPage::OnOutputFolderUse() 
{
	UpdateFolderControls(TRUE);
	GotoDlgCtrl(GetDlgItem(IDC_OUTPUT_FOLDER_EDIT));
}

void COutputFilePathPage::OnOK() 
{
	ASSERT(m_pView != NULL);
	CListCtrl& list = m_pView->GetListCtrl();
	POSITION pos = list.GetFirstSelectedItemPosition();
	ASSERT(pos != NULL);

	// Set the parameters for all selected list items
	while (pos) {

		int i = list.GetNextSelectedItem(pos);
		CGMFConvertItem *pItem = (CGMFConvertItem*)list.GetItemData(i);
		ASSERT(pItem != NULL);

		// Set the folder
		if (m_szFolder.Right(1) != _T("\\"))
			m_szFolder += _T("\\");
		if (m_iFolder == 0)
			list.SetItemText(i, 6, list.GetItemText(i, 2).GetBuffer(MAX_PATH));
		else
			list.SetItemText(i, 6, m_szFolder.GetBuffer(MAX_PATH));

		// Set the file name
		if (m_iName == 0)
			m_szName = list.GetItemText(i, 1);
		int iDot = m_szName.ReverseFind(_T('.'));
		if (iDot != -1)
			m_szName = m_szName.Left(iDot);
		if (!m_szName.IsEmpty()) {
			if (!m_szExtension.IsEmpty() && m_szExtension[0] != _T('.'))
				m_szExtension.Insert(0, _T('.'));
			m_szName += (m_szExtension.IsEmpty())
							? (m_iType == 0) ? _T(".avi") : _T(".wav")
							: m_szExtension;
		}
		list.SetItemText(i, 5, m_szName.GetBuffer(MAX_PATH));

		list.Update(i);

		// Set the type
		pItem->SetType(m_iType);

		// Set AVI multiplexing settings
		pItem->SetCompatibilityIndex(m_bCompatibilityIndex);
		pItem->SetMasterStream((m_bMasterStream) ? (m_iMasterStream) : -1);
		pItem->SetInterleavingFrequency(m_lInterleavingFrequency);
		pItem->SetInterleavingPreroll(m_lInterleavingPreroll);
		pItem->SetInterleavingMode(m_iInterleavingMode);
	}
	
	CPropertyPage::OnOK();
}

void COutputFilePathPage::UpdateFilenameControls(BOOL bEnable)
{
	GetDlgItem(IDC_OUTPUT_FILENAME_EDIT)->EnableWindow(bEnable);
	GetDlgItem(IDC_OUTPUT_FILENAME_BROWSE)->EnableWindow(bEnable);
}

void COutputFilePathPage::UpdateFolderControls(BOOL bEnable)
{
	GetDlgItem(IDC_OUTPUT_FOLDER_EDIT)->EnableWindow(bEnable);
	GetDlgItem(IDC_OUTPUT_FOLDER_BROWSE)->EnableWindow(bEnable);
}

void COutputFilePathPage::OnOutputFolderBrowse() 
{
	// Prepare for browsing
	BROWSEINFO bi = {0};
	CString szFolder;
	bi.hwndOwner		= this->m_hWnd;
	bi.pidlRoot			= NULL;
	bi.pszDisplayName	= szFolder.GetBuffer(MAX_PATH);
	bi.lpszTitle		= CString((LPCSTR)IDS_BROWSE_DIR_DEST);
	bi.ulFlags			= BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	// Browse for folder
	LPITEMIDLIST pIDL = SHBrowseForFolder(&bi);
	if (pIDL != NULL) {

		// Get actual browsed path
		SHGetPathFromIDList(pIDL, szFolder.GetBuffer(MAX_PATH));
		szFolder.ReleaseBuffer();

		// Set edit control text
		SetDlgItemText(IDC_OUTPUT_FOLDER_EDIT, szFolder);
		
		// Free the IDL
		IMalloc *pMalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
			pMalloc->Free(pIDL);
			pMalloc->Release();
		}

	} else
		szFolder.ReleaseBuffer();
}

void COutputFilePathPage::OnOutputFilenameBrowse() 
{
	// Create and configure save file dialog
	CFileDialog dlgOutputFile(
		FALSE,
		NULL,
		_T(""),
		OFN_HIDEREADONLY		| 
		OFN_NODEREFERENCELINKS	|
		OFN_SHAREAWARE			|
		OFN_EXPLORER,
		_T("All files (*.*)\0*.*\0")
	);
	dlgOutputFile.m_ofn.nMaxFile = MAX_PATH;
	dlgOutputFile.m_ofn.lpstrFile = new TCHAR[dlgOutputFile.m_ofn.nMaxFile];
	lstrcpy(dlgOutputFile.m_ofn.lpstrFile, _T("*.*"));
	dlgOutputFile.m_ofn.nMaxFileTitle = MAX_PATH;
	dlgOutputFile.m_ofn.lpstrFileTitle = new TCHAR[dlgOutputFile.m_ofn.nMaxFileTitle];

	if (dlgOutputFile.DoModal() == IDOK)
		SetDlgItemText(IDC_OUTPUT_FILENAME_EDIT, dlgOutputFile.GetFileTitle());

	// Clean up
	delete[] dlgOutputFile.m_ofn.lpstrFile;
	delete[] dlgOutputFile.m_ofn.lpstrFileTitle;
}

void COutputFilePathPage::UpdateAviMuxControls(BOOL bEnable)
{
	GetDlgItem(IDC_STATIC_AVI_MUX_SETTINGS)->EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC_INTERLEAVING_MODE)->EnableWindow(bEnable);
	GetDlgItem(IDC_INTERLEAVING_MODE_NONE)->EnableWindow(bEnable);
	GetDlgItem(IDC_INTERLEAVING_MODE_CAPTURE)->EnableWindow(bEnable);
	GetDlgItem(IDC_INTERLEAVING_MODE_FULL)->EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC_INTERLEAVING_PARAMS)->EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC_INTERLEAVING_FREQUENCY)->EnableWindow(bEnable);
	GetDlgItem(IDC_INTERLEAVING_FREQUENCY)->EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC_INTERLEAVING_PREROLL)->EnableWindow(bEnable);
	GetDlgItem(IDC_INTERLEAVING_PREROLL)->EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC_AVI_SPECIFIC_SETTINGS)->EnableWindow(bEnable);
	GetDlgItem(IDC_AVI_MUX_MASTER_CHECK)->EnableWindow(bEnable);
	UpdateMasterStreamControl(bEnable);
	GetDlgItem(IDC_AVI_MUX_INDEX)->EnableWindow(bEnable);
}

void COutputFilePathPage::OnOutputFiletypeAvi() 
{
	UpdateAviMuxControls(TRUE);
}

void COutputFilePathPage::OnOutputFiletypeWav() 
{
	UpdateAviMuxControls(FALSE);
}

void COutputFilePathPage::UpdateMasterStreamControl(BOOL bEnable)
{
	GetDlgItem(IDC_AVI_MUX_MASTER_EDIT)->EnableWindow(
		bEnable && GetDlgButtonCheck(IDC_AVI_MUX_MASTER_CHECK)
	);
}

void COutputFilePathPage::OnAviMuxMasterCheck() 
{
	UpdateMasterStreamControl(TRUE);
	if (GetDlgButtonCheck(IDC_AVI_MUX_MASTER_CHECK))
		GotoDlgCtrl(GetDlgItem(IDC_AVI_MUX_MASTER_EDIT));
}
