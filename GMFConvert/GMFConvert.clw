; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CVideoCompressionPage
LastTemplate=CPropertyPage
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "GMFConvert.h"
LastPage=0

ClassCount=9
Class1=CGMFConvertApp
Class2=CGMFConvertDoc
Class3=CGMFConvertView
Class4=CMainFrame

ResourceCount=7
Resource1=IDD_ABOUTBOX
Resource2=IDD_CONFIGURE_AUDIO (English (U.S.))
Class5=CAboutDlg
Resource3=IDR_MAINFRAME (English (U.S.))
Class6=COptionsDlg
Resource4=IDD_CONFIGURE_FILE (English (U.S.))
Class7=COutputFilePathPage
Resource5=IDD_ABOUTBOX (English (U.S.))
Class8=CAudioCompressionPage
Resource6=IDD_OPTIONS (English (U.S.))
Class9=CVideoCompressionPage
Resource7=IDD_CONFIGURE_VIDEO (English (U.S.))

[CLS:CGMFConvertApp]
Type=0
HeaderFile=GMFConvert.h
ImplementationFile=GMFConvert.cpp
Filter=N
LastObject=CGMFConvertApp
BaseClass=CWinApp
VirtualFilter=AC

[CLS:CGMFConvertDoc]
Type=0
HeaderFile=GMFConvertDoc.h
ImplementationFile=GMFConvertDoc.cpp
Filter=N
LastObject=CGMFConvertDoc
BaseClass=CDocument
VirtualFilter=DC

[CLS:CGMFConvertView]
Type=0
HeaderFile=GMFConvertView.h
ImplementationFile=GMFConvertView.cpp
Filter=C
LastObject=CGMFConvertView
BaseClass=CListView
VirtualFilter=VWC


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=ID_CLEAR_LIST
BaseClass=CFrameWnd
VirtualFilter=fWC




[CLS:CAboutDlg]
Type=0
HeaderFile=GMFConvert.cpp
ImplementationFile=GMFConvert.cpp
Filter=D
LastObject=CAboutDlg

[DLG:IDD_ABOUTBOX]
Type=1
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Class=CAboutDlg

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command3=ID_FILE_NEW
Command4=ID_FILE_OPEN
Command5=ID_FILE_SAVE
Command6=ID_FILE_SAVE_AS
Command10=ID_FILE_MRU_FILE1
Command11=ID_APP_EXIT
Command12=ID_EDIT_UNDO
Command13=ID_EDIT_CUT
Command14=ID_EDIT_COPY
Command15=ID_EDIT_PASTE
Command29=ID_VIEW_TOOLBAR
Command30=ID_VIEW_STATUS_BAR
Command32=ID_APP_ABOUT
CommandCount=32

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command17=ID_NEXT_PANE
Command18=ID_PREV_PANE
CommandCount=21


[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
Class=CAboutDlg
ControlCount=5
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Control5=IDC_STATIC,static,1342181376

[TB:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_CLEAR_LIST
Command2=ID_ADD_FILE
Command3=ID_ADD_DIR
Command4=ID_EDIT_CUT
Command5=ID_SELECT_ALL
Command6=ID_INVERT_SEL
Command7=ID_UNSELECT_ALL
Command8=ID_CONFIG_CONVERT
Command9=ID_RUN_CONVERT
Command10=ID_PAUSE_CONVERT
Command11=ID_STOP_CONVERT
Command12=ID_OPTIONS
Command13=ID_APP_ABOUT
CommandCount=13

[MNU:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_CLEAR_LIST
Command2=ID_ADD_FILE
Command3=ID_ADD_DIR
Command4=ID_OPTIONS
Command5=ID_APP_EXIT
Command6=ID_EDIT_CUT
Command7=ID_SELECT_ALL
Command8=ID_INVERT_SEL
Command9=ID_UNSELECT_ALL
Command10=ID_CONFIG_CONVERT
Command11=ID_RUN_CONVERT
Command12=ID_PAUSE_CONVERT
Command13=ID_STOP_CONVERT
Command14=ID_VIEW_TOOLBAR
Command15=ID_VIEW_STATUS_BAR
Command16=ID_APP_ABOUT
CommandCount=16

[ACL:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_CLEAR_LIST
Command2=ID_ADD_DIR
Command3=ID_ADD_FILE
Command4=ID_INVERT_SEL
Command5=ID_OPTIONS
Command6=ID_SELECT_ALL
Command7=ID_UNSELECT_ALL
Command8=ID_STOP_CONVERT
Command9=ID_EDIT_CUT
Command10=ID_CONFIG_CONVERT
Command11=ID_RUN_CONVERT
Command12=ID_PAUSE_CONVERT
CommandCount=12

[DLG:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[DLG:IDD_OPTIONS (English (U.S.))]
Type=1
Class=COptionsDlg
ControlCount=6
Control1=IDC_STATIC,button,1342177287
Control2=IDC_DURATION_ADD,button,1342242819
Control3=IDC_RECURSIVE_ADDDIR,button,1342242819
Control4=IDOK,button,1342242817
Control5=IDCANCEL,button,1342242816
Control6=IDC_DEFAULTS,button,1342242816

[CLS:COptionsDlg]
Type=0
HeaderFile=OptionsDlg.h
ImplementationFile=OptionsDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_DEFAULTS
VirtualFilter=dWC

[CLS:COutputFilePathPage]
Type=0
HeaderFile=OutputFilePathPage.h
ImplementationFile=OutputFilePathPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=COutputFilePathPage
VirtualFilter=idWC

[DLG:IDD_CONFIGURE_AUDIO (English (U.S.))]
Type=1
Class=CAudioCompressionPage
ControlCount=13
Control1=IDC_STATIC,static,1342308352
Control2=IDC_AUDIO_FILTERS,listbox,1353777537
Control3=IDC_STATIC,static,1342308352
Control4=IDC_AUDIO_FORMATS,listbox,1353777537
Control5=IDC_STATIC,button,1342177287
Control6=IDC_STATIC,static,1342308352
Control7=IDC_AUDIO_FORMAT_TAG,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_AUDIO_FORMAT_DESC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_BLOCK_ALIGN,static,1342308352
Control12=IDC_STATIC,static,1342308352
Control13=IDC_DATA_RATE,static,1342308352

[CLS:CAudioCompressionPage]
Type=0
HeaderFile=AudioCompressionPage.h
ImplementationFile=AudioCompressionPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CAudioCompressionPage
VirtualFilter=idWC

[DLG:IDD_CONFIGURE_VIDEO (English (U.S.))]
Type=1
Class=CVideoCompressionPage
ControlCount=18
Control1=IDC_STATIC_VIDEO_FILTERS,static,1342308352
Control2=IDC_VIDEO_FILTERS,listbox,1353777537
Control3=IDC_STATIC_VIDEO_SETTINGS,button,1342177287
Control4=IDC_VIDEO_QUALITY_CHECK,button,1342242819
Control5=IDC_VIDEO_QUALITY_SLIDER,msctls_trackbar32,1342242840
Control6=IDC_VIDEO_QUALITY_EDIT,edit,1350639746
Control7=IDC_VIDEO_QUALITY_UNITS,static,1342308352
Control8=IDC_VIDEO_BITRATE_CHECK,button,1342242819
Control9=IDC_VIDEO_BITRATE_EDIT,edit,1350639746
Control10=IDC_VIDEO_BITRATE_UNITS,static,1342308352
Control11=IDC_VIDEO_KEYFRAME_CHECK,button,1342242819
Control12=IDC_VIDEO_KEYFRAME_EDIT,edit,1350639744
Control13=IDC_VIDEO_PFRAME_CHECK,button,1342242819
Control14=IDC_VIDEO_PFRAME_EDIT,edit,1350639744
Control15=IDC_VIDEO_WINDOWSIZE_CHECK,button,1342242819
Control16=IDC_VIDEO_WINDOWSIZE_EDIT,edit,1350639744
Control17=IDC_VCM_CONFIGURE,button,1342242816
Control18=IDC_VCM_ABOUT,button,1342242816

[CLS:CVideoCompressionPage]
Type=0
HeaderFile=VideoCompressionPage.h
ImplementationFile=VideoCompressionPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CVideoCompressionPage
VirtualFilter=idWC

[DLG:IDD_CONFIGURE_FILE (English (U.S.))]
Type=1
Class=COutputFilePathPage
ControlCount=29
Control1=IDC_STATIC,button,1342308359
Control2=IDC_OUTPUT_FOLDER_SAME,button,1342308361
Control3=IDC_OUTPUT_FOLDER_USE,button,1342177289
Control4=IDC_OUTPUT_FOLDER_EDIT,edit,1350631552
Control5=IDC_OUTPUT_FOLDER_BROWSE,button,1342242944
Control6=IDC_STATIC,button,1342308359
Control7=IDC_OUTPUT_FILENAME_SAME,button,1342308361
Control8=IDC_OUTPUT_FILENAME_USE,button,1342177289
Control9=IDC_OUTPUT_FILENAME_EDIT,edit,1350631552
Control10=IDC_OUTPUT_FILENAME_BROWSE,button,1342242944
Control11=IDC_STATIC,button,1342308359
Control12=IDC_OUTPUT_FILETYPE_AVI,button,1342308361
Control13=IDC_OUTPUT_FILETYPE_WAV,button,1342177289
Control14=IDC_STATIC,static,1342308352
Control15=IDC_OUTPUT_FILETYPE_EXT_EDIT,edit,1350631552
Control16=IDC_STATIC_AVI_MUX_SETTINGS,button,1342177287
Control17=IDC_STATIC_INTERLEAVING_MODE,button,1342308359
Control18=IDC_INTERLEAVING_MODE_NONE,button,1342308361
Control19=IDC_INTERLEAVING_MODE_CAPTURE,button,1342177289
Control20=IDC_INTERLEAVING_MODE_FULL,button,1342177289
Control21=IDC_STATIC_INTERLEAVING_PARAMS,button,1342177287
Control22=IDC_STATIC_INTERLEAVING_FREQUENCY,static,1342308354
Control23=IDC_INTERLEAVING_FREQUENCY,edit,1350639744
Control24=IDC_STATIC_INTERLEAVING_PREROLL,static,1342308354
Control25=IDC_INTERLEAVING_PREROLL,edit,1350639744
Control26=IDC_STATIC_AVI_SPECIFIC_SETTINGS,button,1342177287
Control27=IDC_AVI_MUX_MASTER_CHECK,button,1342242819
Control28=IDC_AVI_MUX_MASTER_EDIT,edit,1350639744
Control29=IDC_AVI_MUX_INDEX,button,1342242819

