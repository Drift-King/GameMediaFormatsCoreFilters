// AMErrorText.h: interface for the CAMErrorText class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AMERRORTEXT_H__ACF5B32B_7706_4C9B_825F_A24CACC1F3A9__INCLUDED_)
#define AFX_AMERRORTEXT_H__ACF5B32B_7706_4C9B_825F_A24CACC1F3A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <dshow.h>

class CAMErrorText : public CString  
{
public:
	CAMErrorText(HRESULT hr, UINT nFormatID);
	CAMErrorText(HRESULT hr, LPCTSTR lpszFormat = _T("%s"));
	virtual ~CAMErrorText();

};

#endif // !defined(AFX_AMERRORTEXT_H__ACF5B32B_7706_4C9B_825F_A24CACC1F3A9__INCLUDED_)
