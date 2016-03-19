// GUIDMap.h: interface for the CGUIDMap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUIDMAP_H__9CE66864_77B8_44F1_A1EE_0360DC274F7E__INCLUDED_)
#define AFX_GUIDMAP_H__9CE66864_77B8_44F1_A1EE_0360DC274F7E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <uuids.h>

class CGUIDMap : 
	public CString,
	public GUID
{

#define GUID_Data2		0
#define GUID_Data3		0x10
#define GUID_Data4_1	0xaa000080
#define GUID_Data4_2	0x719b3800

public:
	CString GetFOURCCString(void);
	CGUIDMap(REFGUID guid);
	CGUIDMap(LPOLESTR wszGUID);
	CGUIDMap(DWORD dwFCC);
	operator DWORD() const {
		return (
			(Data2 == GUID_Data2) &&
			(Data3 == GUID_Data3) &&
			(((DWORD*)Data4)[0] == GUID_Data4_1) &&
			(((DWORD*)Data4)[1] == GUID_Data4_2)
		) ? Data1 : 0;
	};
	virtual ~CGUIDMap();

private:
	void StringFromGUID(void);
};

#endif // !defined(AFX_GUIDMAP_H__9CE66864_77B8_44F1_A1EE_0360DC274F7E__INCLUDED_)
