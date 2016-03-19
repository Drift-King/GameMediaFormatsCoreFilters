//==========================================================================
//
// File: GMFCore.cpp
//
// Desc: Game Media Formats - Filters' setup stuff 
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

#include "FilterOptions.h"

// Base parser header
#include "BaseParser.h"

// HNM headers
#include "HNMGUID.h"
#include "HNMSplitter.h"
#include "HNMVideoDecompressor.h"

// APC headers
#include "APCGUID.h"
#include "APCParser.h"
#include "ContinuousIMAADPCMDecompressor.h"

// CIN headers
#include "CINGUID.h"
#include "CINSplitter.h"
#include "CINVideoDecompressor.h"

// ROQ headers
#include "ROQGUID.h"
#include "ROQSplitter.h"
#include "ROQADPCMDecompressor.h"
#include "ROQVideoDecompressor.h"

// FST headers
#include "FSTGUID.h"
#include "FSTSplitter.h"

// VQA headers
#include "VQAGUID.h"
#include "VQASplitter.h"
#include "VQAVideoDecompressor.h"
#include "WSADPCMDecompressor.h"
#include "ADPCMInterleaver.h"

// MVE headers
#include "MVEGUID.h"
#include "MVESplitter.h"
#include "MVEVideoDecompressor.h"
#include "MVEADPCMDecompressor.h"

// H263 headers
#include "H263GUID.h"
#include "H263Parser.h"

//==========================================================================
// DirectShow server registration
//==========================================================================

CFactoryTemplate g_Templates[] = {
	{	// Base parser property page
		g_wszBaseParserPageName,					// Name
		&CLSID_BaseParserPage,						// CLSID
		CBaseParserPage::CreateInstance				// Creation function
	},
	{	// HNM splitter
		g_wszHNMSplitterName,						// Name
		&CLSID_HNMSplitter,							// CLSID
		CHNMSplitterFilter::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudHNMSplitter							// Setup data
	},
	{	// HNM splitter property page
		g_wszHNMSplitterPageName,					// Name
		&CLSID_HNMSplitterPage,						// CLSID
		CHNMSplitterPage::CreateInstance			// Creation function
	},
	{	// APC parser
		g_wszAPCParserName,							// Name
		&CLSID_APCParser,							// CLSID
		CAPCParserFilter::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudAPCParser								// Setup data
	},
	{	// APC parser property page
		g_wszAPCParserPageName,						// Name
		&CLSID_APCParserPage,						// CLSID
		CAPCParserPage::CreateInstance				// Creation function
	},
	{	// Continuous IMA ADPCM decompressor
		g_wszCIMAADPCMDecompressorName,				// Name
		&CLSID_CIMAADPCMDecompressor,				// CLSID
		CCIMAADPCMDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudCIMAADPCMDecompressor					// Setup data
	},
	{	// Continuous IMA ADPCM decompressor property page
		g_wszCIMAADPCMDecompressorPageName,			// Name
		&CLSID_CIMAADPCMDecompressorPage,			// CLSID
		CCIMAADPCMDecompressorPage::CreateInstance	// Creation function
	},
	{	// HNM video decompressor
		g_wszHNMVideoDecompressorName,				// Name
		&CLSID_HNMVideoDecompressor,				// CLSID
		CHNMVideoDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudHNMVideoDecompressor					// Setup data
	},
	{	// HNM video decompressor property page
		g_wszHNMVideoDecompressorPageName,			// Name
		&CLSID_HNMVideoDecompressorPage,			// CLSID
		CHNMVideoDecompressorPage::CreateInstance	// Creation function
	},
	{	// CIN splitter
		g_wszCINSplitterName,						// Name
		&CLSID_CINSplitter,							// CLSID
		CCINSplitterFilter::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudCINSplitter							// Setup data
	},
	{	// CIN splitter property page
		g_wszCINSplitterPageName,					// Name
		&CLSID_CINSplitterPage,						// CLSID
		CCINSplitterPage::CreateInstance			// Creation function
	},
	{	// CIN video decompressor
		g_wszCINVideoDecompressorName,				// Name
		&CLSID_CINVideoDecompressor,				// CLSID
		CCINVideoDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudCINVideoDecompressor					// Setup data
	},
	{	// CIN video decompressor property page
		g_wszCINVideoDecompressorPageName,			// Name
		&CLSID_CINVideoDecompressorPage,			// CLSID
		CCINVideoDecompressorPage::CreateInstance	// Creation function
	},
	{	// ROQ splitter
		g_wszROQSplitterName,						// Name
		&CLSID_ROQSplitter,							// CLSID
		CROQSplitterFilter::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudROQSplitter							// Setup data
	},
	{	// ROQ splitter property page
		g_wszROQSplitterPageName,					// Name
		&CLSID_ROQSplitterPage,						// CLSID
		CROQSplitterPage::CreateInstance			// Creation function
	},
	{	// ROQ ADPCM decompressor
		g_wszROQADPCMDecompressorName,				// Name
		&CLSID_ROQADPCMDecompressor,				// CLSID
		CROQADPCMDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudROQADPCMDecompressor					// Setup data
	},
	{	// ROQ ADPCM decompressor property page
		g_wszROQADPCMDecompressorPageName,			// Name
		&CLSID_ROQADPCMDecompressorPage,			// CLSID
		CROQADPCMDecompressorPage::CreateInstance	// Creation function
	},
	{	// ROQ video decompressor
		g_wszROQVideoDecompressorName,				// Name
		&CLSID_ROQVideoDecompressor,				// CLSID
		CROQVideoDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudROQVideoDecompressor					// Setup data
	},
	{	// ROQ video decompressor property page
		g_wszROQVideoDecompressorPageName,			// Name
		&CLSID_ROQVideoDecompressorPage,			// CLSID
		CROQVideoDecompressorPage::CreateInstance	// Creation function
	},
	{	// FST splitter
		g_wszFSTSplitterName,						// Name
		&CLSID_FSTSplitter,							// CLSID
		CFSTSplitterFilter::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudFSTSplitter							// Setup data
	},
	{	// FST splitter property page
		g_wszFSTSplitterPageName,					// Name
		&CLSID_FSTSplitterPage,						// CLSID
		CFSTSplitterPage::CreateInstance			// Creation function
	},
	{	// VQA splitter
		g_wszVQASplitterName,						// Name
		&CLSID_VQASplitter,							// CLSID
		CVQASplitterFilter::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudVQASplitter							// Setup data
	},
	{	// VQA splitter property page
		g_wszVQASplitterPageName,					// Name
		&CLSID_VQASplitterPage,						// CLSID
		CVQASplitterPage::CreateInstance			// Creation function
	},
	{	// WS ADPCM decompressor
		g_wszWSADPCMDecompressorName,				// Name
		&CLSID_WSADPCMDecompressor,					// CLSID
		CWSADPCMDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudWSADPCMDecompressor					// Setup data
	},
	{	// WS ADPCM decompressor property page
		g_wszWSADPCMDecompressorPageName,			// Name
		&CLSID_WSADPCMDecompressorPage,				// CLSID
		CWSADPCMDecompressorPage::CreateInstance	// Creation function
	},
	{	// ADPCM interleaver
		g_wszADPCMInterleaverName,					// Name
		&CLSID_ADPCMInterleaver,					// CLSID
		CADPCMInterleaver::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudADPCMInterleaver						// Setup data
	},
	{	// ADPCM interleaver property page
		g_wszADPCMInterleaverPageName,				// Name
		&CLSID_ADPCMInterleaverPage,				// CLSID
		CADPCMInterleaverPage::CreateInstance		// Creation function
	},
	{	// VQA video decompressor
		g_wszVQAVideoDecompressorName,				// Name
		&CLSID_VQAVideoDecompressor,				// CLSID
		CVQAVideoDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudVQAVideoDecompressor					// Setup data
	},
	{	// VQA video decompressor property page
		g_wszVQAVideoDecompressorPageName,			// Name
		&CLSID_VQAVideoDecompressorPage,			// CLSID
		CVQAVideoDecompressorPage::CreateInstance	// Creation function
	},
	{	// MVE splitter
		g_wszMVESplitterName,						// Name
		&CLSID_MVESplitter,							// CLSID
		CMVESplitterFilter::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudMVESplitter							// Setup data
	},
	{	// MVE splitter property page
		g_wszMVESplitterPageName,					// Name
		&CLSID_MVESplitterPage,						// CLSID
		CMVESplitterPage::CreateInstance			// Creation function
	},
	{	// MVE ADPCM decompressor
		g_wszMVEADPCMDecompressorName,				// Name
		&CLSID_MVEADPCMDecompressor,				// CLSID
		CMVEADPCMDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudMVEADPCMDecompressor					// Setup data
	},
	{	// MVE ADPCM decompressor property page
		g_wszMVEADPCMDecompressorPageName,			// Name
		&CLSID_MVEADPCMDecompressorPage,			// CLSID
		CMVEADPCMDecompressorPage::CreateInstance	// Creation function
	},
	{	// MVE video decompressor
		g_wszMVEVideoDecompressorName,				// Name
		&CLSID_MVEVideoDecompressor,				// CLSID
		CMVEVideoDecompressor::CreateInstance,		// Creation function
		NULL,										// Init function
		&g_sudMVEVideoDecompressor					// Setup data
	},
	{	// MVE video decompressor property page
		g_wszMVEVideoDecompressorPageName,			// Name
		&CLSID_MVEVideoDecompressorPage,			// CLSID
		CMVEVideoDecompressorPage::CreateInstance	// Creation function
	},
	{	// H263 parser
		g_wszH263ParserName,						// Name
		&CLSID_H263Parser,							// CLSID
		CH263ParserFilter::CreateInstance,			// Creation function
		NULL,										// Init function
		&g_sudH263Parser							// Setup data
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer(void)
{
	// Register DLL server
	HRESULT hr = AMovieDllRegisterServer2(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	// Register file type for HNM splitter
	hr = CHNMSplitterFilter::RegisterFileType(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	// Register file type for APC parser
	hr = CAPCParserFilter::RegisterFileType(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	// Register file type for CIN splitter
	hr = CCINSplitterFilter::RegisterFileType(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	// Register file type for ROQ splitter
	hr = CROQSplitterFilter::RegisterFileType(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	// Register file type for FST splitter
	hr = CFSTSplitterFilter::RegisterFileType(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	// Register file type for VQA splitter
	hr = CVQASplitterFilter::RegisterFileType(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	// Register file type for MVE splitter
	hr = CMVESplitterFilter::RegisterFileType(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	// Register file type for H263 parser
	hr = CH263ParserFilter::RegisterFileType(TRUE);
	if (FAILED(hr)) {
		DllUnregisterServer();
		return hr;
	}

	return NOERROR;
}

STDAPI DllUnregisterServer(void)
{
	// Unregister DLL server
	HRESULT hr = AMovieDllRegisterServer2(FALSE);
	if (FAILED(hr))
		return hr;

	// Unregister file type for HNM splitter
	hr = CHNMSplitterFilter::RegisterFileType(FALSE);
	if (FAILED(hr))
		return hr;

	// Wipe out HNM splitter options from the registry
	hr = DeleteFilterOptions(g_wszHNMSplitterName);
	if (FAILED(hr))
		return hr;

	// Unregister file type for APC parser
	hr = CAPCParserFilter::RegisterFileType(FALSE);
	if (FAILED(hr))
		return hr;

	// Wipe out APC parser options from the registry
	hr = DeleteFilterOptions(g_wszAPCParserName);
	if (FAILED(hr))
		return hr;

	// Wipe out HNM video decompressor options from the registry
	hr = DeleteFilterOptions(g_wszHNMVideoDecompressorName);
	if (FAILED(hr))
		return hr;

	// Unregister file type for CIN splitter
	hr = CCINSplitterFilter::RegisterFileType(FALSE);
	if (FAILED(hr))
		return hr;

	// Wipe out CIN splitter options from the registry
	hr = DeleteFilterOptions(g_wszCINSplitterName);
	if (FAILED(hr))
		return hr;

	// Unregister file type for ROQ splitter
	hr = CROQSplitterFilter::RegisterFileType(FALSE);
	if (FAILED(hr))
		return hr;

	// Wipe out ROQ splitter options from the registry
	hr = DeleteFilterOptions(g_wszROQSplitterName);
	if (FAILED(hr))
		return hr;

	// Unregister file type for FST splitter
	hr = CFSTSplitterFilter::RegisterFileType(FALSE);
	if (FAILED(hr))
		return hr;

	// Wipe out FST splitter options from the registry
	hr = DeleteFilterOptions(g_wszFSTSplitterName);
	if (FAILED(hr))
		return hr;

	// Unregister file type for VQA splitter
	hr = CVQASplitterFilter::RegisterFileType(FALSE);
	if (FAILED(hr))
		return hr;

	// Wipe out VQA splitter options from the registry
	hr = DeleteFilterOptions(g_wszVQASplitterName);
	if (FAILED(hr))
		return hr;

	// Unregister file type for MVE splitter
	hr = CMVESplitterFilter::RegisterFileType(FALSE);
	if (FAILED(hr))
		return hr;

	// Wipe out MVE splitter options from the registry
	hr = DeleteFilterOptions(g_wszMVESplitterName);
	if (FAILED(hr))
		return hr;

	// Unregister file type for H263 parser
	hr = CH263ParserFilter::RegisterFileType(FALSE);
	if (FAILED(hr))
		return hr;

	// Wipe out H263 parser options from the registry
	hr = DeleteFilterOptions(g_wszH263ParserName);
	if (FAILED(hr))
		return hr;

	return NOERROR;
}
