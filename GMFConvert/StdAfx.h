// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__46D5594D_1EE4_4FE5_B438_2991B8CFBDBA__INCLUDED_)
#define AFX_STDAFX_H__46D5594D_1EE4_4FE5_B438_2991B8CFBDBA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define GetDlgButtonCheck(id) (((CButton*)GetDlgItem(id))->GetCheck() == 1)

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxtempl.h>		// MFC templates
#include <afxdlgs.h>		// MFC common dialogs
#include <afxcview.h>
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__46D5594D_1EE4_4FE5_B438_2991B8CFBDBA__INCLUDED_)
