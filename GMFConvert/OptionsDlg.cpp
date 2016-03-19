// OptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GMFConvert.h"
#include "OptionsDlg.h"
#include "GMFConvertView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog


COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
	// Retrieve the options from the parent view
	m_pView = (CGMFConvertView*)pParent;

	//{{AFX_DATA_INIT(COptionsDlg)
	m_bGetDurationOnAdd = m_pView->m_bGetDurationOnAdd;
	m_bRecursiveAddDir = m_pView->m_bRecursiveAddDir;
	//}}AFX_DATA_INIT
}


void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
	DDX_Check(pDX, IDC_DURATION_ADD, m_bGetDurationOnAdd);
	DDX_Check(pDX, IDC_RECURSIVE_ADDDIR, m_bRecursiveAddDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	//{{AFX_MSG_MAP(COptionsDlg)
	ON_BN_CLICKED(IDC_DEFAULTS, OnDefaults)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg message handlers

void COptionsDlg::OnDefaults() 
{
	CheckDlgButton(IDC_DURATION_ADD, 0);
	CheckDlgButton(IDC_RECURSIVE_ADDDIR, 1);
}

void COptionsDlg::OnOK() 
{
	CDialog::OnOK();

	// Put the options back on the parent view
	ASSERT(m_pView != NULL);
	m_pView->m_bGetDurationOnAdd = m_bGetDurationOnAdd;
	m_pView->m_bRecursiveAddDir = m_bRecursiveAddDir;
}
