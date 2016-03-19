// FilterList.h: interface for the CFilterList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILTERLIST_H__72BA0DB6_5A0C_4601_BDD8_2E45D9446CE1__INCLUDED_)
#define AFX_FILTERLIST_H__72BA0DB6_5A0C_4601_BDD8_2E45D9446CE1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFilterList  
{
	CListBox *m_pList;

public:
	void Reset(void);
	HRESULT Populate(CLSID clsidCategory, CString szNullFilterName);
	CFilterList(CListBox *pList);
	virtual ~CFilterList();

};

#endif // !defined(AFX_FILTERLIST_H__72BA0DB6_5A0C_4601_BDD8_2E45D9446CE1__INCLUDED_)
