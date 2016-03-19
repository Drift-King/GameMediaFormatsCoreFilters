#if !defined(AFX_OUTPUTFILEPATHPAGE_H__6DCD1F4D_F66C_4C7F_A7DD_B2A31F36BA39__INCLUDED_)
#define AFX_OUTPUTFILEPATHPAGE_H__6DCD1F4D_F66C_4C7F_A7DD_B2A31F36BA39__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OutputFilePathPage.h : header file
//

#include "GMFConvertView.h"

/////////////////////////////////////////////////////////////////////////////
// COutputFilePathPage dialog

class COutputFilePathPage : public CPropertyPage
{
	CGMFConvertView *m_pView;
	CBitmap m_bmBrowseFolder;
	CBitmap m_bmBrowseFilename;

// Construction
public:
	COutputFilePathPage(CGMFConvertView *pParentView);
	~COutputFilePathPage();

// Dialog Data
	//{{AFX_DATA(COutputFilePathPage)
	enum { IDD = IDD_CONFIGURE_FILE };
	CString	m_szName;
	CString	m_szFolder;
	CString	m_szExtension;
	int		m_iName;
	int		m_iType;
	int		m_iFolder;
	BOOL	m_bCompatibilityIndex;
	BOOL	m_bMasterStream;
	long	m_iMasterStream;
	long	m_lInterleavingFrequency;
	long	m_lInterleavingPreroll;
	int		m_iInterleavingMode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COutputFilePathPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COutputFilePathPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnOutputFilenameSame();
	afx_msg void OnOutputFilenameUse();
	afx_msg void OnOutputFolderSame();
	afx_msg void OnOutputFolderUse();
	afx_msg void OnOutputFolderBrowse();
	afx_msg void OnOutputFilenameBrowse();
	afx_msg void OnOutputFiletypeAvi();
	afx_msg void OnOutputFiletypeWav();
	afx_msg void OnAviMuxMasterCheck();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void UpdateFolderControls(BOOL bEnable);
	void UpdateFilenameControls(BOOL bEnable);
	void UpdateAviMuxControls(BOOL bEnable);
	void UpdateMasterStreamControl(BOOL bEnable);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUTPUTFILEPATHPAGE_H__6DCD1F4D_F66C_4C7F_A7DD_B2A31F36BA39__INCLUDED_)
