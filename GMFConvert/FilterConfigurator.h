// FilterConfigurator.h: interface for the CFilterConfigurator class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILTERCONFIGURATOR_H__72BA0DB6_5A0C_4601_BDD8_2E45D9446CE1__INCLUDED_)
#define AFX_FILTERCONFIGURATOR_H__72BA0DB6_5A0C_4601_BDD8_2E45D9446CE1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <dshow.h>

class CFilterConfigurator  
{
	CListBox *m_pFilterList;
	CListBox *m_pFormatList;

public:
	void ResetFilterList(void);
	void ResetFormatList(void);
	HRESULT PopulateFilterList(REFCLSID clsidCategory);
	HRESULT PopulateFormatList(IMoniker *pMoniker);
	CFilterConfigurator(CListBox *pFilterList, CListBox *pFormatList);
	virtual ~CFilterConfigurator();

};

#endif // !defined(AFX_FILTERCONFIGURATOR_H__72BA0DB6_5A0C_4601_BDD8_2E45D9446CE1__INCLUDED_)
