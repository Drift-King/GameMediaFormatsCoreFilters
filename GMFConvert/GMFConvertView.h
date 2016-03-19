// GMFConvertView.h : interface of the CGMFConvertView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GMFCONVERTVIEW_H__4138A86C_C77E_4881_9448_BBBE0D52704A__INCLUDED_)
#define AFX_GMFCONVERTVIEW_H__4138A86C_C77E_4881_9448_BBBE0D52704A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GMFConvertDoc.h"
#include "OptionsDlg.h"

#define STATUS_IMAGE_FAILURE	0
#define STATUS_IMAGE_PROCESSING	5
#define STATUS_IMAGE_SUCCESS	7
#define STATUS_IMAGE_WAITING	4
#define STATUS_IMAGE_STOPPED	8
#define STATUS_IMAGE_PAUSED		1

class CGMFConvertView : public CListView
{
protected: // create from serialization only
	CGMFConvertView();
	DECLARE_DYNCREATE(CGMFConvertView)

// Data fields
private:
	void ResetSort(void);
	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	BOOL SortList(int iSortItem, BOOL bSortDescending);
	BOOL BrowseForFolder(CString szTitle, CString& szFolder);
	void MakeDurationString(DWORD dwDuration, CString& szDuration);
	DWORD GetDuration(IUnknown *pGraph);
	void AddFile(CString szFileName);
	void AddDir(CString szFolder);

	CImageList m_ListImageList;
	CImageList m_HeaderImageList;

	BOOL m_bGetDurationOnAdd;
	BOOL m_bRecursiveAddDir;
	
	typedef struct tagSORT_LIST_DATA {
		CListCtrl *pList;
		int iSortItem;
		BOOL bSortDescending;
	} SORT_LIST_DATA;

	int m_iSortItem;
	BOOL m_bSortDescending;

	friend class COptionsDlg;

// Attributes
public:
	CGMFConvertDoc* GetDocument();

// Operations
public:

	void OnItemClick(NMHDR* pNMHDR, LRESULT* pResult);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMFConvertView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGMFConvertView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CGMFConvertView)
	afx_msg void OnUpdateAddDir(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAddFile(CCmdUI* pCmdUI);
	afx_msg void OnUpdateClearList(CCmdUI* pCmdUI);
	afx_msg void OnAddFile();
	afx_msg void OnClearList();
	afx_msg void OnUpdateConfigConvert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateInvertSel(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePauseConvert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSelectAll(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUnselectAll(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRunConvert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStopConvert(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnInvertSel();
	afx_msg void OnSelectAll();
	afx_msg void OnUnselectAll();
	afx_msg void OnUpdateOptions(CCmdUI* pCmdUI);
	afx_msg void OnAddDir();
	afx_msg void OnOptions();
	afx_msg void OnConfigConvert();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in GMFConvertView.cpp
inline CGMFConvertDoc* CGMFConvertView::GetDocument()
   { return (CGMFConvertDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMFCONVERTVIEW_H__4138A86C_C77E_4881_9448_BBBE0D52704A__INCLUDED_)
