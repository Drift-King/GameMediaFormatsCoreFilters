#if !defined(AFX_OPTIONSDLG_H__96187529_5866_4CD3_8ED5_BDAD755A2DD5__INCLUDED_)
#define AFX_OPTIONSDLG_H__96187529_5866_4CD3_8ED5_BDAD755A2DD5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsDlg.h : header file
//

#include "GMFConvertView.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog

class COptionsDlg : public CDialog
{
// Construction
public:
	COptionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COptionsDlg)
	enum { IDD = IDD_OPTIONS };
	BOOL	m_bGetDurationOnAdd;
	BOOL	m_bRecursiveAddDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COptionsDlg)
	afx_msg void OnDefaults();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	class CGMFConvertView *m_pView;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSDLG_H__96187529_5866_4CD3_8ED5_BDAD755A2DD5__INCLUDED_)
