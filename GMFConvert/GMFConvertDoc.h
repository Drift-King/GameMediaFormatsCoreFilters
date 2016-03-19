// GMFConvertDoc.h : interface of the CGMFConvertDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GMFCONVERTDOC_H__ED7A2AA8_CFDB_4377_8578_860DF8D188CF__INCLUDED_)
#define AFX_GMFCONVERTDOC_H__ED7A2AA8_CFDB_4377_8578_860DF8D188CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGMFConvertDoc : public CDocument
{
protected: // create from serialization only
	CGMFConvertDoc();
	DECLARE_DYNCREATE(CGMFConvertDoc)

// Data fields
private:

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMFConvertDoc)
	public:
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGMFConvertDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CGMFConvertDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMFCONVERTDOC_H__ED7A2AA8_CFDB_4377_8578_860DF8D188CF__INCLUDED_)
