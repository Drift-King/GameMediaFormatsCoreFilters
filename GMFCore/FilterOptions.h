//==========================================================================
//
// File: FilterOptions.h
//
// Desc: Game Media Formats - Operations with filter options 
//
// Copyright (C) 2004 ANX Software.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License 
// along with this program; if not, write to the Free Software 
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//==========================================================================

#ifndef __GMF_FILTER_OPTIONS_H__
#define __GMF_FILTER_OPTIONS_H__

#include <windows.h>
#include <tchar.h>
#include <wchar.h>

// Root registry path for filter options
extern const TCHAR g_szFilterRootName[];

// Retrieve filter option value from the registry
HRESULT RegGetFilterOptionValue(
	LPCWSTR	pszFilterName,	// Filter name
	LPCTSTR	pszOptionName,	// Option name
	LPDWORD	lpOptionType,	// Option value type
	LPBYTE	lpOptionData,	// Option value data
	LPDWORD	lpcbOptionData	// Size of option value data
);

// Get filter DWORD option or set it to default value if it's 
// not present in the registry (the default value is supplied 
// by the caller in the variable which is meant to receive the 
// obtained option value)
HRESULT RegGetFilterOptionDWORD(
	LPCWSTR	pszFilterName,
	LPCTSTR	pszOptionName,
	DWORD *pdwValue
);

// Store filter option value in the registry
HRESULT RegSetFilterOptionValue(
	LPCWSTR	pszFilterName,	// Filter name
	LPCTSTR	pszOptionName,	// Option name
	DWORD	dwOptionType,	// Option value type
	LPCBYTE	lpOptionData,	// Option value data
	DWORD	cbOptionData	// Size of option value data
);

// Wipe out the filter options from the registry
HRESULT DeleteFilterOptions(LPCWSTR	pszFilterName);

#endif