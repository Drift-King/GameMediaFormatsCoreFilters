#if !defined(AFX_AUDIOCOMPRESSIONPAGE_H__E13D82CE_C00D_4789_B842_4D3D16366620__INCLUDED_)
#define AFX_AUDIOCOMPRESSIONPAGE_H__E13D82CE_C00D_4789_B842_4D3D16366620__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AudioCompressionPage.h : header file
//

#include "FilterConfigurator.h"
#include "GMFConvertView.h"

/////////////////////////////////////////////////////////////////////////////
// CAudioCompressionPage dialog

class CAudioCompressionPage : public CPropertyPage
{
	CGMFConvertView *m_pView;
	CFilterConfigurator *m_pFilterConfigurator;

// Construction
public:
	CAudioCompressionPage(CGMFConvertView *pParent);
	~CAudioCompressionPage();

// Dialog Data
	//{{AFX_DATA(CAudioCompressionPage)
	enum { IDD = IDD_CONFIGURE_AUDIO };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAudioCompressionPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAudioCompressionPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelChangeAudioFilters();
	afx_msg void OnSelChangeAudioFormats();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void ResetFormatInfo(CString szNoInfo);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUDIOCOMPRESSIONPAGE_H__E13D82CE_C00D_4789_B842_4D3D16366620__INCLUDED_)
