#if !defined(AFX_VIDEOCOMPRESSIONPAGE_H__7D1D4552_0429_4CFD_AEF9_7D14A9E67657__INCLUDED_)
#define AFX_VIDEOCOMPRESSIONPAGE_H__7D1D4552_0429_4CFD_AEF9_7D14A9E67657__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VideoCompressionPage.h : header file
//

#include "FilterConfigurator.h"
#include "GMFConvertView.h"
#include "OutputFilePathPage.h"

/////////////////////////////////////////////////////////////////////////////
// CVideoCompressionPage dialog

class CVideoCompressionPage : public CPropertyPage
{
	CGMFConvertView *m_pView;
	CFilterConfigurator *m_pFilterConfigurator;

// Construction
public:
	CVideoCompressionPage(CGMFConvertView *pParent);
	~CVideoCompressionPage();

// Dialog Data
	//{{AFX_DATA(CVideoCompressionPage)
	enum { IDD = IDD_CONFIGURE_VIDEO };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CVideoCompressionPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CVideoCompressionPage)
	afx_msg void OnVideoBitrateCheck();
	afx_msg void OnVideoQualityCheck();
	afx_msg void OnVideoKeyframeCheck();
	afx_msg void OnVideoPframeCheck();
	afx_msg void OnVideoWindowsizeCheck();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelChangeVideoFilters();
	afx_msg void OnVcmConfigure();
	afx_msg void OnVcmAbout();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnChangeVideoQualityEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void GetCodecState(IMoniker *pMoniker, LPVOID& pState, int& cbState);
	void SetCodecState(IMoniker *pMoniker, LPVOID pState, int cbState);
	void ResetVideoControls(void);
	void UpdateVideoBitrateControls(BOOL bEnable);
	void UpdateVideoQualityControls(BOOL bEnable);
	void UpdateVideoKeyframeControl(BOOL bEnable);
	void UpdateVideoPframeControl(BOOL bEnable);
	void UpdateVideoWindowsizeControl(BOOL bEnable);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIDEOCOMPRESSIONPAGE_H__7D1D4552_0429_4CFD_AEF9_7D14A9E67657__INCLUDED_)
