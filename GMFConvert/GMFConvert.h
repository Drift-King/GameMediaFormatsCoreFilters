// GMFConvert.h : main header file for the GMFCONVERT application
//

#if !defined(AFX_GMFCONVERT_H__2A5415D1_0CA8_4F98_9160_8185B7385609__INCLUDED_)
#define AFX_GMFCONVERT_H__2A5415D1_0CA8_4F98_9160_8185B7385609__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertApp:
// See GMFConvert.cpp for the implementation of this class
//

class CGMFConvertApp : public CWinApp
{
public:
	CGMFConvertApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMFConvertApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CGMFConvertApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMFCONVERT_H__2A5415D1_0CA8_4F98_9160_8185B7385609__INCLUDED_)
