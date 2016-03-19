// GMFConvertDoc.cpp : implementation of the CGMFConvertDoc class
//

#include "stdafx.h"
#include "GMFConvert.h"

#include "GMFConvertDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertDoc

IMPLEMENT_DYNCREATE(CGMFConvertDoc, CDocument)

BEGIN_MESSAGE_MAP(CGMFConvertDoc, CDocument)
	//{{AFX_MSG_MAP(CGMFConvertDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertDoc construction/destruction

CGMFConvertDoc::CGMFConvertDoc()
{
	// TODO: add one-time construction code here

}

CGMFConvertDoc::~CGMFConvertDoc()
{
}


/////////////////////////////////////////////////////////////////////////////
// CGMFConvertDoc serialization

void CGMFConvertDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertDoc diagnostics

#ifdef _DEBUG
void CGMFConvertDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CGMFConvertDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGMFConvertDoc commands
