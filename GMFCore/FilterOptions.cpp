//==========================================================================
//
// File: FilterOptions.cpp
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

#include <streams.h>

#include "FilterOptions.h"

const TCHAR g_szFilterRootName[] = 
	TEXT("Software\\ANX Software\\Game Media Formats Suit\\Filters");

HRESULT RegGetFilterOptionValue(
	LPCWSTR	pszFilterName,
	LPCTSTR	pszOptionName,
	LPDWORD	lpOptionType,
	LPBYTE	lpOptionData,
	LPDWORD	lpcbOptionData
)
{
	// Compose complete path to the filter options
	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, TEXT("%s\\%ls"), g_szFilterRootName, pszFilterName);

	// Open registry key (or create it if not present)
	HKEY hk;
	DWORD dwDisposition;
	LONG lr = RegCreateKeyEx(
		HKEY_CURRENT_USER,
		(LPCTSTR)szPath,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_QUERY_VALUE,
		NULL,
		&hk,
		&dwDisposition
	);
	if (lr != ERROR_SUCCESS)
		return AmHresultFromWin32(lr);

	// Query the option value
	lr = RegQueryValueEx(
		hk,
		pszOptionName,
		NULL,
		lpOptionType,
		lpOptionData,
		lpcbOptionData
	);

	RegCloseKey(hk);

	return (lr != ERROR_SUCCESS) ? AmHresultFromWin32(lr) : NOERROR;
}

HRESULT RegSetFilterOptionValue(
	LPCWSTR	pszFilterName,
	LPCTSTR	pszOptionName,
	DWORD	dwOptionType,
	LPCBYTE	lpOptionData,
	DWORD	cbOptionData
)
{
	// Compose complete path to the filter options
	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, TEXT("%s\\%ls"), g_szFilterRootName, pszFilterName);

	// Open registry key (or create it if not present)
	HKEY hk;
	DWORD dwDisposition;
	LONG lr = RegCreateKeyEx(
		HKEY_CURRENT_USER,
		(LPCTSTR)szPath,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_SET_VALUE,
		NULL,
		&hk,
		&dwDisposition
	);
	if (lr != ERROR_SUCCESS)
		return AmHresultFromWin32(lr);

	// Set the option value
	lr = RegSetValueEx(
		hk,
		pszOptionName,
		0,
		dwOptionType,
		lpOptionData,
		cbOptionData
	);

	RegCloseKey(hk);

	return (lr != ERROR_SUCCESS) ? AmHresultFromWin32(lr) : NOERROR;
}

HRESULT DeleteFilterOptions(LPCWSTR	pszFilterName)
{
	// Compose complete path to the filter options
	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, TEXT("%s\\%ls"), g_szFilterRootName, pszFilterName);

	// Try to open the key
	HKEY hk;
	LONG lr = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		szPath,
		0,
		KEY_READ,
		&hk
	);
	// Delete the key if it exists
	if (lr == ERROR_SUCCESS) {
		RegCloseKey(hk);
		// Delete the filter options section
		lr = RegDeleteKey(HKEY_CURRENT_USER, szPath);
		if (lr != ERROR_SUCCESS)
			return AmHresultFromWin32(lr);
	}

	return NOERROR;
}

HRESULT RegGetFilterOptionDWORD(
	LPCWSTR	pszFilterName,
	LPCTSTR	pszOptionName,
	DWORD *pdwValue
)
{
	// Get the option value
	DWORD dwType, cbSize;
	HRESULT hr = RegGetFilterOptionValue(
		pszFilterName,
		pszOptionName,
		&dwType,
		(LPBYTE)pdwValue,
		&cbSize
	);
	if (FAILED(hr)) {
			
		// Set the default option value
		hr = RegSetFilterOptionValue(
			pszFilterName,
			pszOptionName,
			REG_DWORD,
			(LPBYTE)pdwValue,
			sizeof(DWORD)
		);
		if (FAILED(hr))
			return hr;

	} else {

		// Check if the returned value is of the right size
		if (
			(dwType != REG_DWORD) || 
			(cbSize != sizeof(DWORD))
		)
			return E_UNEXPECTED;
	}

	return NOERROR;
}
