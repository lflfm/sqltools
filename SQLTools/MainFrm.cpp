/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/

#include "stdafx.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/ExceptionHelper.h"
#include "COMMON/GUICommandDictionary.h"
#include "SQLTools.h"
#include "MainFrm.h"
#include "Tools/Grep/GrepDlg.h"
#include "ConnectionTasks.h"
#include "OpenEditor/RecentFileList.h"
#include "OpenEditor/FavoritesList.h"
#include "OpenEditor/OEDocument.h" // for GetGlobalSettings

/*
    22.03.2003 bug fix, checking for changes works even the program is inactive - checking only on activation now
    23.03.2003 bug fix, "Find in Files" does not show the output automatically
    30.03.2003 bug fix, updated a control bars initial state
    30.03.2003 improvement, command line support, try sqltools /h to get help
    16.11.2003 bug fix, shortcut F1 has been disabled until sqltools help can be worked out 
    20.04.2005 bug fix, (ak) changing style after toolbar creation is a workaround for toolbar background artifacts
    2016.07.06 improvement, new implementation of recent file list
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    LPCSTR CMDIMainFrame::m_cszClassName = "Kochware.SQLTools.MainFrame";
    static const char lpszProfileName[] = "Workspace_1.6.0\\";
    static const char lpszQuickViewer[] = "Object Viewer";
    static const char lpszObjectsList[] = "Objects List";
    static const char lpszFindInFiles[] = "Find in Files";
    static const char lpszFilePanel[]   = "File Panel";
    static const char lpszPropTreePanel[]  = "Properties";

/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame

IMPLEMENT_DYNAMIC(CMDIMainFrame, CWorkbookMDIFrame)

BEGIN_MESSAGE_MAP(CMDIMainFrame, CWorkbookMDIFrame)
	//{{AFX_MSG_MAP(CMDIMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_SQL_OBJ_VIEWER, OnSqlObjViewer)
	ON_UPDATE_COMMAND_UI(ID_SQL_OBJ_VIEWER, OnUpdate_SqlObjViewer)
	ON_COMMAND(ID_SQL_DB_SOURCE, OnSqlDbSource)
	ON_UPDATE_COMMAND_UI(ID_SQL_DB_SOURCE, OnUpdate_SqlDbSource)
	ON_COMMAND(ID_FILE_FIND_IN_FILE, OnFileGrep)
	ON_COMMAND(ID_FILE_SHOW_GREP_OUTPUT, OnFileShowGrepOutput)
	ON_UPDATE_COMMAND_UI(ID_FILE_SHOW_GREP_OUTPUT, OnUpdate_FileShowGrepOutput)
	ON_UPDATE_COMMAND_UI(ID_FILE_FIND_IN_FILE, OnUpdate_FileGrep)
	ON_COMMAND(ID_VIEW_PROPERTIES, OnViewProperties)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROPERTIES, OnUpdate_ViewProperties)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_VIEW_FILE_TOOLBAR, OnViewFileToolbar)
	ON_COMMAND(ID_VIEW_SQL_TOOLBAR, OnViewSqlToolbar)
	ON_COMMAND(ID_VIEW_CONNECT_TOOLBAR, OnViewConnectToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FILE_TOOLBAR, OnUpdate_FileToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SQL_TOOLBAR, OnUpdate_SqlToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CONNECT_TOOLBAR, OnUpdate_ConnectToolbar)
	ON_NOTIFY(TCN_SELCHANGE, IDC_WORKBOOK_TAB, OnChangeWorkbookTab)
	// Global help commands
    ON_COMMAND(ID_HELP, CWorkbookMDIFrame::OnHelp)
	ON_COMMAND(ID_HELP_FINDER, CWorkbookMDIFrame::OnHelpFinder)
	ON_COMMAND(ID_CONTEXT_HELP, CWorkbookMDIFrame::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CWorkbookMDIFrame::OnHelpFinder)
    ON_WM_COPYDATA()
    ON_WM_ACTIVATEAPP()   // 22.03.2003 bug fix, checking for changes works even the program is inactive - checking only on activation now
//    ON_WM_SIZE()
    ON_WM_CLOSE()
    ON_MESSAGE(WM_SETMESSAGESTRING, &CMDIMainFrame::OnSetMessageString)
    ON_NOTIFY(TBN_DROPDOWN, (AFX_IDW_CONTROLBAR_FIRST - 1), OnPlanDropDown)
    ON_MESSAGE(CSQLToolsApp::UM_REQUEST_QUEUE_NOT_EMPTY, RequestQueueNotEmpty)
    ON_MESSAGE(CSQLToolsApp::UM_REQUEST_QUEUE_EMPTY,     RequestQueueEmpty)

    ON_WM_ENDSESSION()
    ON_WM_QUERYENDSESSION()
END_MESSAGE_MAP()

static UINT g_indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_OCIGRID,
    ID_INDICATOR_WORKSPACE_NAME,
    ID_INDICATOR_FILE_TYPE,
    ID_INDICATOR_BLOCK_TYPE,
//    ID_INDICATOR_FILE_LINES,
    ID_INDICATOR_POS,
//    ID_INDICATOR_SCROLL_POS,
    ID_INDICATOR_OVR,
};

/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame construction/destruction

CMDIMainFrame::CMDIMainFrame()
{
    m_cszProfileName = lpszProfileName;
}

CMDIMainFrame::~CMDIMainFrame()
{
}

void CMDIMainFrame::EvOnConnect ()
{
    m_wndDbSourceWnd.EvOnConnect();
    m_wndObjectViewer.EvOnConnect();
}

void CMDIMainFrame::EvOnDisconnect ()
{
    m_wndDbSourceWnd.EvOnDisconnect();
    m_wndObjectViewer.EvOnDisconnect();
}

BOOL CMDIMainFrame::PreCreateWindow (CREATESTRUCT& cs)
{
    WNDCLASS wndClass;
    BOOL bRes = CWorkbookMDIFrame::PreCreateWindow(cs);
    HINSTANCE hInstance = AfxGetInstanceHandle();

    // see if the class already exists
    if (!::GetClassInfo(hInstance, m_cszClassName, &wndClass))
    {
        // get default stuff
        ::GetClassInfo(hInstance, cs.lpszClass, &wndClass);
        wndClass.lpszClassName = m_cszClassName;
        wndClass.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
        // register a new class
        if (!AfxRegisterClass(&wndClass))
            AfxThrowResourceException();
    }
    cs.lpszClass = m_cszClassName;

    return bRes;
}

const int IDC_MF_DBSOURCE_BAR  = (AFX_IDW_CONTROLBAR_LAST - 10);
const int IDC_MF_DBTREE_BAR    = (AFX_IDW_CONTROLBAR_LAST - 11);
const int IDC_MF_DBPLAN_BAR    = (AFX_IDW_CONTROLBAR_LAST - 12);
const int IDC_MF_GREP_BAR      = (AFX_IDW_CONTROLBAR_LAST - 13);
const int IDC_MF_PROPTREE_BAR  = (AFX_IDW_CONTROLBAR_LAST - 14);

int CMDIMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWorkbookMDIFrame::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndFileToolBar.CreateEx(this, 0,
                                  //WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY ,
                                  WS_CHILD | WS_VISIBLE | CBRS_GRIPPER | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
                                  CRect(1, 1, 1, 1),
                                  AFX_IDW_CONTROLBAR_FIRST - 1)
    || !m_wndFileToolBar.LoadToolBar(IDT_FILE))
    {
		TRACE0("Failed to create tool bar\n");
        return -1;
    }
    // 20.4.2005 bug fix, (ak) changing style after toolbar creation is a workaround for toolbar background artifacts
    m_wndFileToolBar.ModifyStyle(0, TBSTYLE_FLAT);
    m_wndFileToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
    m_wndFileToolBar.SetButtonInfo(1, ID_FILE_OPEN, BTNS_DROPDOWN, 1);
    m_wndFileToolBar.SetButtonInfo(4+1/*1 for separator*/, ID_WORKSPACE_OPEN, BTNS_DROPDOWN, 4);
    m_wndFileToolBar.SetButtonInfo(6+2/*2 for separators*/, ID_WORKSPACE_OPEN_AUTOSAVED, BTNS_DROPDOWN, 6);

    m_wndFileToolBar.EnableDocking(CBRS_ALIGN_ANY);

    if (!m_wndSqlToolBar.CreateEx(this, 0,
                                  //WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY ,
                                  WS_CHILD | WS_VISIBLE | CBRS_GRIPPER | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
                                  CRect(1, 1, 1, 1),
                                  AFX_IDW_CONTROLBAR_FIRST - 2)
    || !m_wndSqlToolBar.LoadToolBar(IDT_SQL))
    {
		TRACE0("Failed to create tool bar\n");
        return -1;
    }
    m_wndSqlToolBar.ModifyStyle(0, TBSTYLE_FLAT);
    m_wndSqlToolBar.EnableDocking(CBRS_ALIGN_ANY);

    if (!m_wndConnectionBar.Create(this, AFX_IDW_CONTROLBAR_FIRST - 3))
    {
		TRACE0("Failed to create tool bar\n");
        return -1;
    }

    if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;
	}
    SetIndicators();

	EnableDocking(CBRS_ALIGN_ANY);
#ifdef _SCB_REPLACE_MINIFRAME
    // EnableDocking overrides m_pFloatingFrameClass so change it to RUNTIME_CLASS(CSCBMiniDockFrameWnd)
    m_pFloatingFrameClass = RUNTIME_CLASS(CSCBMiniDockFrameWnd);
#endif //_SCB_REPLACE_MINIFRAME

    DockControlBar(&m_wndFileToolBar);
    DockControlBar(&m_wndConnectionBar);
    DockControlBarLeftOf(&m_wndSqlToolBar, &m_wndConnectionBar);
    //DockControlBar(&m_wndSqlToolBar);

    if (!m_wndObjectViewerFrame.Create(lpszQuickViewer, this, CSize(200, 300), TRUE, IDC_MF_DBTREE_BAR)
    || !m_wndObjectViewer.Create(&m_wndObjectViewerFrame))
    {
		TRACE("Failed to create Object Viewer\n");
		return -1;
    }
    m_wndObjectViewer.LoadAndSetImageList(IDB_SQL_GENERAL_LIST);
    //m_wndObjectViewer.ModifyStyleEx(0, WS_EX_CLIENTEDGE);

    ShowControlBar(&m_wndObjectViewerFrame, FALSE, FALSE);
    m_wndObjectViewerFrame.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndObjectViewerFrame.SetSCBStyle(m_wndObjectViewerFrame.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndObjectViewerFrame.SetBarStyle(m_wndObjectViewerFrame.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndObjectViewerFrame.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	DockControlBar(&m_wndObjectViewerFrame, AFX_IDW_DOCKBAR_RIGHT);

    if (!m_wndPropTreeFrame.Create(lpszQuickViewer, this, CSize(200, 300), TRUE, IDC_MF_PROPTREE_BAR)
    //|| !m_wndPropTree.Create(WS_CHILD | WS_VISIBLE, CRect(0,0,0,0), &m_wndPropTreeFrame, 1))
    || !m_wndPropTree.CreateEx(0, NULL, "PropertyGrid", WS_CHILD | WS_VISIBLE, CRect(0,0,0,0), &m_wndPropTreeFrame, 1)
    )
    {
		TRACE("Failed to create Properties Viewer\n");
		return -1;
    }
    //m_wndPropTree.ModifyStyleEx(0, WS_EX_CLIENTEDGE);

    ShowControlBar(&m_wndPropTreeFrame, FALSE, FALSE);
    m_wndPropTreeFrame.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndPropTreeFrame.SetSCBStyle(m_wndPropTreeFrame.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndPropTreeFrame.SetBarStyle(m_wndPropTreeFrame.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndPropTreeFrame.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	DockControlBar(&m_wndPropTreeFrame, AFX_IDW_DOCKBAR_RIGHT);

    if (!m_wndDbSourceFrame.Create(lpszObjectsList, this, CSize(680, 400), TRUE, IDC_MF_DBSOURCE_BAR)
    || !m_wndDbSourceWnd.Create(&m_wndDbSourceFrame))
    {
		TRACE("Failed to create DB Objects Viewer\n");
		return -1;
    }
    ShowControlBar(&m_wndDbSourceFrame, FALSE, FALSE);
    m_wndDbSourceFrame.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndDbSourceFrame.SetSCBStyle(m_wndDbSourceFrame.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndDbSourceFrame.SetBarStyle(m_wndDbSourceFrame.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndDbSourceFrame.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	DockControlBar(&m_wndDbSourceFrame, AFX_IDW_DOCKBAR_LEFT);
	FloatControlBar(&m_wndDbSourceFrame, CPoint(200,200));

    if (!m_wndGrepFrame.Create(lpszFindInFiles, this, CSize(600, 200), TRUE, IDC_MF_GREP_BAR)
    || !m_wndGrepViewer.Create(&m_wndGrepFrame)) {
		TRACE("Failed to create Grep Viewer\n");
		return -1;
    }
    m_wndGrepViewer.ModifyStyleEx(0, WS_EX_CLIENTEDGE);

    ShowControlBar(&m_wndGrepFrame, FALSE, FALSE);
    m_wndGrepFrame.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndGrepFrame.SetSCBStyle(m_wndGrepFrame.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndGrepFrame.SetBarStyle(m_wndGrepFrame.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndGrepFrame.EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);
	DockControlBar(&m_wndGrepFrame, AFX_IDW_DOCKBAR_BOTTOM);

    if (CWorkbookMDIFrame::DoCreate(FALSE) == -1)
    {
        TRACE0("CMDIMainFrame::OnCreate: Failed to create CWorkbookMDIFrame\n");
        return -1;
    }

    //DockControlBarLeftOf(&m_wndFilePanelBar, &m_wndObjectViewerFrame);
    ShowControlBar(&m_wndFilePanelBar, FALSE, FALSE);

    CWorkbookMDIFrame::LoadBarState();

    // 30.03.2003 bug fix, updated a control bars initial state
    ShowControlBar(&m_wndFilePanelBar, !m_wndFilePanelBar.IsFloating() && m_wndFilePanelBar.IsVisible(), FALSE);
    ShowControlBar(&m_wndDbSourceFrame, !m_wndDbSourceFrame.IsFloating() && m_wndDbSourceFrame.IsVisible(), FALSE);
    ShowControlBar(&m_wndObjectViewerFrame, !m_wndObjectViewerFrame.IsFloating() && m_wndObjectViewerFrame.IsVisible(), FALSE);
    ShowControlBar(&m_wndPropTreeFrame, !m_wndPropTreeFrame.IsFloating() && m_wndPropTreeFrame.IsVisible(), FALSE);
    ShowControlBar(&m_wndGrepFrame, !m_wndGrepFrame.IsFloating() && m_wndGrepFrame.IsVisible(), FALSE);

    Global::SetStatusHwnd(m_wndStatusBar.m_hWnd);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame message handlers

void CMDIMainFrame::OnClose()
{
    // let's try to close connection first 
    // because an user can cancel if there is an open transaction

    if (theApp.GetConnectOpen())
    {
        if (theApp.GetConnectOpen() && theApp.GetActivePrimeExecution())
        {
            if (AfxMessageBox("There is an active query/statement. \n\nDo you really want to ignore that and close application?", 
                MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2) == IDYES
            )
            {
                try { EXCEPTION_FRAME;

                    theApp.DoCancel(true/*silent*/);

                    ConnectionTasks::SubmitDisconnectTask(true);
                }
                _DEFAULT_HANDLER_

                CWorkbookMDIFrame::OnClose();
            }
        }
        else
        {
            try { EXCEPTION_FRAME;

                ConnectionTasks::SubmitDisconnectTask(true);
            }
            _DEFAULT_HANDLER_
        }
    }
    else
        CWorkbookMDIFrame::OnClose();
}

void CMDIMainFrame::OnSqlObjViewer()
{
    bool focus = has_focus(m_wndObjectViewerFrame);

    ShowControlBar(&m_wndObjectViewerFrame, !m_wndObjectViewerFrame.IsVisible(), FALSE);
    
    if (focus && !m_wndObjectViewerFrame.IsVisible())
        SetFocus();
}

void CMDIMainFrame::OnUpdate_SqlObjViewer(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndObjectViewerFrame.IsWindowVisible());
}

void CMDIMainFrame::OnViewProperties()
{
    bool focus = has_focus(m_wndPropTreeFrame);

    ShowControlBar(&m_wndPropTreeFrame, !m_wndPropTreeFrame.IsVisible(), FALSE);
    
    if (focus && !m_wndPropTreeFrame.IsVisible())
        SetFocus();
}

void CMDIMainFrame::OnUpdate_ViewProperties(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndPropTreeFrame.IsWindowVisible());
}

void CMDIMainFrame::OnSqlDbSource()
{
    bool focus = has_focus(m_wndDbSourceFrame);

    ShowControlBar(&m_wndDbSourceFrame, !m_wndDbSourceFrame.IsVisible(), FALSE);
    
    if (focus && !m_wndDbSourceFrame.IsVisible())
        SetFocus();
}

void CMDIMainFrame::OnUpdate_SqlDbSource(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndDbSourceFrame.IsWindowVisible());
}

void CMDIMainFrame::OnFileGrep ()
{
    if (m_wndGrepViewer.IsRunning()
    && AfxMessageBox("Abort already running grep process?", MB_ICONQUESTION|MB_YESNO) == IDYES)
        m_wndGrepViewer.AbortProcess();

    if (!m_wndGrepViewer.IsRunning()
    && CGrepDlg(this, &m_wndGrepViewer).DoModal() == IDOK)
       ShowControlBar(&m_wndGrepFrame, TRUE, FALSE); // 23.03.2003 bug fix, "Find in Files" does not show the output automatically
}

void CMDIMainFrame::OnUpdate_FileGrep (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndGrepViewer.QuickIsRunning());
}

void CMDIMainFrame::OnFileShowGrepOutput()
{
    bool focus = has_focus(m_wndGrepFrame);

    ShowControlBar(&m_wndGrepFrame, !m_wndGrepFrame.IsVisible(), FALSE);
    
    if (focus && !m_wndGrepFrame.IsVisible())
        SetFocus();
}

void CMDIMainFrame::OnUpdate_FileShowGrepOutput(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndGrepFrame.IsWindowVisible());
}

void CMDIMainFrame::OnViewFileToolbar ()
{
    ShowControlBar(&m_wndFileToolBar, !m_wndFileToolBar.IsVisible(), FALSE);
}

void CMDIMainFrame::OnViewSqlToolbar ()
{
    ShowControlBar(&m_wndSqlToolBar, !m_wndSqlToolBar.IsVisible(), FALSE);
}

void CMDIMainFrame::OnViewConnectToolbar ()
{
    ShowControlBar(&m_wndConnectionBar, !m_wndConnectionBar.IsVisible(), FALSE);
}

void CMDIMainFrame::OnUpdate_FileToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndFileToolBar.IsVisible());
}

void CMDIMainFrame::OnUpdate_SqlToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndSqlToolBar.IsVisible());
}

void CMDIMainFrame::OnUpdate_ConnectToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndConnectionBar.IsVisible());
}

#define MAX_TIT_LEN 48
    static void append_striped_string (CString& outStr, const char* inStr)
    {
        for (int i(0); i < MAX_TIT_LEN && inStr && *inStr && *inStr != '\n'; i++, inStr++) {
            if (*inStr == '\t') outStr += ' ';
            else                outStr += *inStr;
        }
        if (i == MAX_TIT_LEN || (inStr && inStr[0] == '\n' && inStr[1] != 0))
            outStr += "...";
    }

CObjectViewerWnd& CMDIMainFrame::ShowTreeViewer ()
{
    ShowControlBar(&m_wndObjectViewerFrame, TRUE, FALSE);
    return m_wndObjectViewer;
}

void CMDIMainFrame::SetIndicators ()
{
    if (m_wndStatusBar)
    {
        CArray<UINT> indicators;
    
        for (int i = 0; i < sizeof(g_indicators)/sizeof(g_indicators[0]); ++i)
        {
            if (g_indicators[i] != ID_INDICATOR_WORKSPACE_NAME
            || COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceShowNameInStatusBar())
                indicators.Add(g_indicators[i]);
        }

        m_wndStatusBar.SetIndicators(indicators.GetData(), indicators.GetCount());
    }
}

void CMDIMainFrame::ShowProperties (std::vector<std::pair<string, string> >& properties, bool readOnly) 
{
    ShowControlBar(&m_wndPropTreeFrame, TRUE, FALSE);
    m_wndPropTree.ShowProperties(properties, readOnly); 
}

BOOL CMDIMainFrame::OnCopyData (CWnd*, COPYDATASTRUCT* pCopyDataStruct)
{
    // 30.03.2003 improvement, command line support, try sqltools /h to get help
	if (theApp.HandleAnotherInstanceRequest(pCopyDataStruct))
    {
        if (IsIconic()) OpenIcon();
        return TRUE;
    }
    return FALSE;
}

void CMDIMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
    CWorkbookMDIFrame::OnActivateApp(bActive, dwThreadID);

    theApp.OnActivateApp(bActive);
}

//void CMDIMainFrame::OnSize (UINT nType, int cx, int cy)
//{
//    CWorkbookMDIFrame::OnSize(nType, cx, cy);
//
//    if (nType == SIZE_MINIMIZED)
//    {
//        m_orgTitle = GetTitle();
//        CString title(m_orgTitle);
//        title += '\n';
//        title += m_wndConnectionBar.GetText();
//        title += '\n';
//        SetTitle(title);
//        OnUpdateFrameTitle(FALSE);
//    }
//    else if (!m_orgTitle.IsEmpty())
//    {
//        SetTitle(m_orgTitle);
//        OnUpdateFrameTitle(FALSE);
//        m_orgTitle.Empty();
//    }
//}

    static void update_label (CWorkbookControlBar& ctrlBar, UINT command, LPCSTR orgTitle)
    {
        std::string label;
        if (Common::GUICommandDictionary::GetCommandAccelLabel(static_cast<Common::Command>(command), label))
            label = " (" + label + ")";
        ctrlBar.SetWindowText((orgTitle + label).c_str());
    }

void CMDIMainFrame::UpdateViewerAccelerationKeyLabels ()
{
    update_label(m_wndObjectViewerFrame, ID_SQL_OBJ_VIEWER, lpszQuickViewer);
    update_label(m_wndDbSourceFrame, ID_SQL_DB_SOURCE, lpszObjectsList);
    update_label(m_wndGrepFrame, ID_FILE_SHOW_GREP_OUTPUT, lpszFindInFiles);
    update_label(m_wndFilePanelBar, ID_VIEW_FILE_PANEL, lpszFilePanel);
    update_label(m_wndPropTreeFrame, ID_VIEW_PROPERTIES, lpszPropTreePanel);
}

LRESULT CMDIMainFrame::OnSetMessageString (WPARAM wParam, LPARAM lParam)
{
    bool mru_file = wParam >= ID_FILE_MRU_FIRST && wParam <= ID_FILE_MRU_LAST;
    bool mru_wks = !mru_file && wParam >= ID_WORKSPACE_MRU_FIRST && wParam <= ID_WORKSPACE_MRU_LAST;
    
    if (mru_file || mru_wks)
    {
        // imitate CFrameWnd::OnSetMessageString
        UINT nIDLast = m_nIDLastMessage;
        m_nFlags &= ~WF_NOPOPMSG;

        if (CWnd* pMessageBar = GetMessageBar())
        {
            int inx = wParam - (mru_file ? ID_FILE_MRU_FIRST : ID_WORKSPACE_MRU_FIRST);
            
            counted_ptr<RecentFileList> list = theApp.GetRecentFileList();
            
            CString fname;
            if (list.get())
            {   
                if (mru_file)
                    list->GetFileName(inx, fname);
                else
                    list->GetWorkspaceName(inx, fname);
            }
            pMessageBar->SetWindowText(fname);
        }

        // imitate CFrameWnd::OnSetMessageString
        m_nIDLastMessage = (UINT)nIDLast;   // new ID (or 0)
        m_nIDTracking    = (UINT)nIDLast;   // so F1 on toolbar buttons work
        return nIDLast;
    }

    return CWorkbookMDIFrame::OnSetMessageString(wParam, lParam);
}

void CMDIMainFrame::OnPlanDropDown (NMHDR* pNMHDR, LRESULT* lResult)
{
    LPNMTOOLBAR pNMTB = (LPNMTOOLBAR)pNMHDR;

    if (pNMTB)
    {
        CMenu menu;
        menu.CreatePopupMenu();

        switch (pNMTB->iItem)
        {
        case ID_FILE_OPEN:
            menu.AppendMenu(MF_STRING|MF_DISABLED,  ID_FILE_MRU_FIRST, "Recent File");
            break;
        case ID_WORKSPACE_OPEN:
            menu.AppendMenu(MF_STRING|MF_DISABLED,  ID_WORKSPACE_MRU_FIRST, "Recent Workspace");
            break;
        case ID_WORKSPACE_OPEN_AUTOSAVED:
            menu.AppendMenu(MF_STRING|MF_DISABLED,  ID_WORKSPACE_QUICK_MRU_FIRST, "Recent Quicksaved Snapshot");
            break;
        }

        CRect rc;
        m_wndFileToolBar.SendMessage(TB_GETRECT, pNMTB->iItem, (LPARAM)&rc);
        m_wndFileToolBar.ClientToScreen(&rc);

        menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left, rc.bottom, this, &rc);
    }

    *lResult = TBDDRET_DEFAULT;
}

LRESULT CMDIMainFrame::RequestQueueNotEmpty (WPARAM, LPARAM)
{
    try { EXCEPTION_FRAME;
        GetConnectionBar().StartAnimation(true);

        theApp.SetTaskbarProgressState(CSQLToolsApp::WORKING_STATE);
    }
    _DEFAULT_HANDLER_

    return 0L;
}

LRESULT CMDIMainFrame::RequestQueueEmpty (WPARAM, LPARAM)
{
    try { EXCEPTION_FRAME;
        GetConnectionBar().StartAnimation(false);

        theApp.SetTaskbarProgressState(CSQLToolsApp::IDLE_STATE);
        // cleanup if some task failed reset on exit
        theApp.SetActivePrimeExecution(false);
    }
    _DEFAULT_HANDLER_
    return 0L;
}


#include "OpenEditor/OEWorkspaceManager.h"

void CMDIMainFrame::OnEndSession(BOOL bEnding)
{
    try { EXCEPTION_FRAME;

        CWinThread* pWorkspaceManagerThread = OEWorkspaceManager::Get().ImmediateShutdown();

        if (pWorkspaceManagerThread)
	        WaitForSingleObject(*pWorkspaceManagerThread, 5000);
    }
    _DEFAULT_HANDLER_

    CWorkbookMDIFrame::OnEndSession(bEnding);
}

BOOL CMDIMainFrame::OnQueryEndSession()
{
    try { EXCEPTION_FRAME;

        ShutdownBlockReasonCreate(m_hWnd, L"Auto-saving the data!");

        OEWorkspaceManager::Get().InstantAutosave();
        
        ShutdownBlockReasonCreate(m_hWnd, L"");
    }
    _DEFAULT_HANDLER_

    return CWorkbookMDIFrame::OnQueryEndSession();
}
