// AMErrorText.cpp: implementation of the CAMErrorText class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GMFConvert.h"
#include "AMErrorText.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAMErrorText::CAMErrorText(HRESULT hr, UINT nFormatID)
{
	Empty();
	TCHAR szAMErrorText[MAX_ERROR_TEXT_LEN];
	if (AMGetErrorText(hr, szAMErrorText, MAX_ERROR_TEXT_LEN))
		Format(nFormatID, szAMErrorText);
}

CAMErrorText::CAMErrorText(HRESULT hr, LPCTSTR lpszFormat)
{
	Empty();
	TCHAR szAMErrorText[MAX_ERROR_TEXT_LEN];
	if (AMGetErrorText(hr, szAMErrorText, MAX_ERROR_TEXT_LEN))
		Format(lpszFormat, szAMErrorText);
}


CAMErrorText::~CAMErrorText()
{
}
