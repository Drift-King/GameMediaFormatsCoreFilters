// GUIDMap.cpp: implementation of the CGUIDMap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GMFConvert.h"
#include "GUIDMap.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGUIDMap::CGUIDMap(REFGUID guid)
{
	Data1 = guid.Data1;
	Data2 = guid.Data2;
	Data3 = guid.Data3;
	((DWORD*)Data4)[0] = ((DWORD*)guid.Data4)[0];
    ((DWORD*)Data4)[1] = ((DWORD*)guid.Data4)[1];
	StringFromGUID();
}

CGUIDMap::CGUIDMap(LPOLESTR wszGUID)
{
	Format(_T("%ls"), wszGUID);
	CLSIDFromString(wszGUID, this);
}

CGUIDMap::CGUIDMap(DWORD dwFCC)
{
	Data1 = dwFCC;
	Data2 = GUID_Data2;
    Data3 = GUID_Data3;
    ((DWORD*)Data4)[0] = GUID_Data4_1;
    ((DWORD*)Data4)[1] = GUID_Data4_2;
	StringFromGUID();
}

CGUIDMap::~CGUIDMap()
{
}

void CGUIDMap::StringFromGUID()
{
	LPOLESTR wszGUID;
	StringFromCLSID(*this, &wszGUID);
	Format(_T("%ls"), wszGUID);
	CoTaskMemFree(wszGUID);
}

CString CGUIDMap::GetFOURCCString()
{
	return CString((LPCTSTR)&Data1, 4);
}
