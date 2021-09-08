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
    2018-03-30 bug fix, compatibility with WinXp
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    LPCWSTR CMDIMainFrame::m_cszClassName = L"Kochware.SQLTools.MainFrame";
    static LPCWSTR lpszQuickViewer    = L"Object Viewer";
    static LPCWSTR lpszObjectsList    = L"Objects List";
    static LPCWSTR lpszFindInFiles    = L"Find in Files Results";
    static LPCWSTR lpszFilePanel      = L"File Panel";
    static LPCWSTR lpszFileHistory    = L"History";
    static LPCWSTR lpszOpenFiles      = L"Open Files";
    static LPCWSTR lpszPropTreePanel  = L"Properties";

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
    ON_COMMAND(ID_VIEW_OPEN_FILES, OnViewOpenFiles)
    ON_UPDATE_COMMAND_UI(ID_VIEW_OPEN_FILES, OnUpdate_ViewOpenFiles)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_VIEW_FILE_TOOLBAR, OnViewFileToolbar)
    ON_COMMAND(ID_VIEW_SQL_TOOLBAR, OnViewSqlToolbar)
    ON_COMMAND(ID_VIEW_CONNECT_TOOLBAR, OnViewConnectToolbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FILE_TOOLBAR, OnUpdate_FileToolbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SQL_TOOLBAR, OnUpdate_SqlToolbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_CONNECT_TOOLBAR, OnUpdate_ConnectToolbar)
    // Global help commands
    ON_COMMAND(ID_HELP, CWorkbookMDIFrame::OnHelp)
    ON_COMMAND(ID_HELP_FINDER, CWorkbookMDIFrame::OnHelpFinder)
    ON_COMMAND(ID_CONTEXT_HELP, CWorkbookMDIFrame::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CWorkbookMDIFrame::OnHelpFinder)
    ON_WM_COPYDATA()
    ON_WM_ACTIVATEAPP()   // 22.03.2003 bug fix, checking for changes works even the program is inactive - checking only on activation now
    ON_WM_CLOSE()
    ON_MESSAGE(WM_SETMESSAGESTRING, &CMDIMainFrame::OnSetMessageString)
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
    ID_INDICATOR_ENCODING,
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
    : m_wndOpenFilesFrame(m_wndOpenFiles)
{
    m_bCanConvertControlBarToMDIChild = TRUE;
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
const int IDC_MF_OPENFILES_BAR = (AFX_IDW_CONTROLBAR_LAST - 15);

int CMDIMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWorkbookMDIFrame::OnCreate(lpCreateStruct) == -1)
        return -1;

    // FRM
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
    CDockingManager::SetDockingMode(DT_SMART);
    EnableAutoHidePanes(CBRS_ALIGN_ANY);

    SetupMDITabbedGroups();

    if (!m_wndToolBar.CreateEx(
           this, TBSTYLE_FLAT,
            WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, 
            CRect(4, 2, 4, 2), AFX_IDW_CONTROLBAR_FIRST - 1)
    || !m_wndToolBar.LoadToolBar(IDT_FILE))
    {
        TRACE0("Failed to create tool bar\n");
        return -1;
    }

    // FRM
    m_wndToolBar.SetWindowText(L"File");
    {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING|MF_DISABLED,  ID_FILE_MRU_FIRST, _T("Recent File"));
        m_wndToolBar.ReplaceButton(ID_FILE_OPEN, 
            CMFCToolBarMenuButton(ID_FILE_OPEN, menu.GetSafeHmenu(), 1, nullptr, FALSE));
    }
    {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING|MF_DISABLED,  ID_WORKSPACE_MRU_FIRST, _T("Recent Workspace"));
        m_wndToolBar.ReplaceButton(ID_WORKSPACE_OPEN, 
            CMFCToolBarMenuButton(ID_WORKSPACE_OPEN, menu.GetSafeHmenu(), 4, nullptr, FALSE));
    }
    {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING|MF_DISABLED,  ID_WORKSPACE_QUICK_MRU_FIRST, _T("Recent Quicksaved Snapshot"));
        m_wndToolBar.ReplaceButton(ID_WORKSPACE_OPEN_AUTOSAVED, 
            CMFCToolBarMenuButton(ID_WORKSPACE_OPEN_AUTOSAVED, menu.GetSafeHmenu(), 6, nullptr, FALSE));
    }

    if (!m_wndSqlToolBar.CreateEx(
           this, TBSTYLE_FLAT,
            WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, 
            CRect(4, 2, 4, 2), AFX_IDW_CONTROLBAR_FIRST - 2)
    || !m_wndSqlToolBar.LoadToolBar(IDT_SQL))
    {
        TRACE0("Failed to create tool bar\n");
        return -1;
    }

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
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    m_wndSqlToolBar.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndToolBar);
    //DockPane(&m_wndConnectionBar);
    DockPane(&m_wndSqlToolBar);
    DockPaneLeftOf(&m_wndConnectionBar, &m_wndSqlToolBar);


    if (!m_wndObjectViewerFrame.Create(lpszQuickViewer, this, CSize(200, 300), TRUE, IDC_MF_DBTREE_BAR)
    || !m_wndObjectViewer.Create(&m_wndObjectViewerFrame))
    {
        TRACE("Failed to create Object Viewer\n");
        return -1;
    }
    HICON hObjectViewerIcon = (HICON)::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_OBJECT_VIEWER), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndObjectViewerFrame.SetIcon(hObjectViewerIcon, FALSE);
    m_wndObjectViewerFrame.SetClient(&m_wndObjectViewer);
    m_wndObjectViewerFrame.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndObjectViewerFrame, AFX_IDW_DOCKBAR_RIGHT);

    if (!m_wndPropTreeFrame.Create(lpszQuickViewer, this, CSize(200, 300), TRUE, IDC_MF_PROPTREE_BAR)
    || !m_wndPropTree.CreateEx(0, NULL, L"PropertyGrid", WS_CHILD | WS_VISIBLE, CRect(0,0,0,0), &m_wndPropTreeFrame, 1)
    )
    {
        TRACE("Failed to create Properties Viewer\n");
        return -1;
    }
    HICON hPropIcon = (HICON)::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROPERTIES), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndPropTreeFrame.SetIcon(hPropIcon/*AfxGetApp()->LoadIcon(IDI_PROPERTIES)*/, FALSE);
    m_wndPropTreeFrame.SetClient(&m_wndPropTree);
    m_wndPropTreeFrame.EnableDocking(CBRS_ALIGN_ANY);
    //DockPane(&m_wndPropTreeFrame);
    m_wndPropTreeFrame.AttachToTabWnd(&m_wndObjectViewerFrame, DM_SHOW, FALSE);


    if (!m_wndDbSourceFrame.Create(lpszObjectsList, this, CSize(680, 400), TRUE, IDC_MF_DBSOURCE_BAR)
    || !m_wndDbSourceWnd.Create(&m_wndDbSourceFrame))
    {
        TRACE("Failed to create DB Objects Viewer\n");
        return -1;
    }
    HICON hObjectListIcon = (HICON)::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_OBJECT_LIST), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndDbSourceFrame.SetIcon(hObjectListIcon, FALSE);
    m_wndDbSourceFrame.SetClient(&m_wndDbSourceWnd);
    m_wndDbSourceFrame.EnableDocking(CBRS_ALIGN_ANY);
    AddPane(&m_wndDbSourceFrame); // TEST if that is enough 
    m_wndDbSourceFrame.FloatPane(CRect(CPoint(100, 100), CSize(680, 400)));
    m_wndDbSourceFrame.ShowPane(FALSE, FALSE, TRUE); // hidden by default
    m_wndDbSourceFrame.SetCanBeTabbedDocument(TRUE);
    if (AfxGetApp()->GetProfileInt(m_cszProfileName, L"IsObjectListTabbed", FALSE))
        m_wndDbSourceFrame.ConvertToTabbedDocument();
    m_wndDbSourceFrame.ShowPane(FALSE, FALSE, TRUE); // hidden by default


    if (!m_wndGrepFrame.Create(lpszFindInFiles, this, CSize(600, 200), TRUE, IDC_MF_GREP_BAR)
    || !m_wndGrepViewer.Create(&m_wndGrepFrame)) {
        TRACE("Failed to create Grep Viewer\n");
        return -1;
    }
    HICON hFindInFilesIcon = (HICON)::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_FIND_IN_FILES), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndGrepFrame.SetIcon(hFindInFilesIcon, FALSE);
    m_wndGrepFrame.SetClient(&m_wndGrepViewer);
    m_wndGrepFrame.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndGrepFrame, AFX_IDW_DOCKBAR_BOTTOM);
    m_wndGrepFrame.ShowPane(FALSE, FALSE, TRUE); // hidden by default



    if (!m_wndOpenFilesFrame.Create(lpszOpenFiles, this, CSize(200, 300), TRUE, IDC_MF_OPENFILES_BAR)
    || !m_wndOpenFiles.CreateEx(0, WS_CHILD | WS_VISIBLE |LVS_REPORT|LVS_SORTASCENDING|LVS_NOCOLUMNHEADER |LVS_SHOWSELALWAYS|LVS_SINGLESEL, CRect(0,0,0,0), &m_wndOpenFilesFrame, 1)
    )
    {
        TRACE("Failed to create Open Files controlr\n");
        return -1;
    }
    m_wndOpenFiles.InsertColumn(0, (LPCTSTR)NULL);
    m_wndOpenFiles.SetExtendedStyle(m_wndOpenFiles.GetExtendedStyle()|LVS_EX_FULLROWSELECT);
    m_wndOpenFiles.ModifyStyleEx(0, WS_EX_STATICEDGE/*WS_EX_CLIENTEDGE*/, 0);
    m_wndOpenFiles.SetImageList(&m_wndFilePanel.GetSysImageList(), LVSIL_SMALL);

    HICON hOpenFilesIcon = (HICON)::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_OPEN_FILES), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndOpenFilesFrame.SetIcon(hOpenFilesIcon, FALSE);
    m_wndOpenFilesFrame.SetClient(&m_wndOpenFiles);
    m_wndOpenFilesFrame.EnableDocking(CBRS_ALIGN_ANY);
    //DockPane(&m_wndOpenFilesFrame);

    if (CWorkbookMDIFrame::DoCreate() == -1)
    {
        TRACE0("CMDIMainFrame::OnCreate: Failed to create CWorkbookMDIFrame\n");
        return -1;
    }

    m_wndOpenFilesFrame.AttachToTabWnd(&m_wndFilePanel, DM_SHOW, FALSE);

    Global::SetStatusHwnd(m_wndStatusBar.m_hWnd);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame message handlers

void CMDIMainFrame::OnClose()
{
    AfxGetApp()->WriteProfileInt(m_cszProfileName, L"IsObjectListTabbed", m_wndDbSourceFrame.IsMDITabbed());

    // let's try to close connection first 
    // because an user can cancel if there is an open transaction

    if (theApp.GetConnectOpen())
    {
        if (theApp.GetConnectOpen() && theApp.GetActivePrimeExecution())
        {
            if (AfxMessageBox(L"There is an active query/statement. \n\nDo you really want to ignore that and close application?", 
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


void CMDIMainFrame::TogglePane (CDockablePane& pane)
{
    bool focus = has_focus(pane);

    if (!pane.IsWindowVisible()) // in case if it is hidden behind another docking control
        pane.ShowPane(TRUE, FALSE, TRUE);
    else 
        pane.ShowPane(!pane.IsVisible(), FALSE, TRUE);

    if (focus && !pane.IsVisible())
        SetFocus();
}


void CMDIMainFrame::OnSqlObjViewer()
{
    TogglePane(m_wndObjectViewerFrame);
}

void CMDIMainFrame::OnUpdate_SqlObjViewer(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndObjectViewerFrame.IsWindowVisible());
}

void CMDIMainFrame::OnViewProperties()
{
    TogglePane(m_wndPropTreeFrame);
}

void CMDIMainFrame::OnUpdate_ViewProperties(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndOpenFilesFrame.IsWindowVisible());
}

void CMDIMainFrame::OnViewOpenFiles()
{
    TogglePane(m_wndOpenFilesFrame);
}

void CMDIMainFrame::OnUpdate_ViewOpenFiles(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndPropTreeFrame.IsWindowVisible());
}

void CMDIMainFrame::OnSqlDbSource()
{
    TogglePane(m_wndDbSourceFrame);
}

void CMDIMainFrame::OnUpdate_SqlDbSource(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndDbSourceFrame.IsWindowVisible());
}

void CMDIMainFrame::OnFileGrep ()
{
    if (CGrepDlg(this, &m_wndGrepViewer, nullptr).DoModal() == IDOK)
       m_wndGrepFrame.ShowPane(TRUE, FALSE, TRUE);
}

void CMDIMainFrame::OnUpdate_FileGrep (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndGrepViewer.QuickIsRunning());
}

void CMDIMainFrame::OnFileShowGrepOutput()
{
    TogglePane(m_wndGrepFrame);
}

void CMDIMainFrame::OnUpdate_FileShowGrepOutput(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndGrepFrame.IsWindowVisible());
}

void CMDIMainFrame::OnViewFileToolbar ()
{

    ShowPane(&m_wndToolBar, !m_wndToolBar.IsVisible(), FALSE, FALSE);
}

void CMDIMainFrame::OnViewSqlToolbar ()
{
    ShowPane(&m_wndSqlToolBar, !m_wndSqlToolBar.IsVisible(), FALSE, FALSE);
}

void CMDIMainFrame::OnViewConnectToolbar ()
{
    ShowPane(&m_wndConnectionBar, !m_wndConnectionBar.IsVisible(), FALSE, FALSE);
}

void CMDIMainFrame::OnUpdate_FileToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndToolBar.IsVisible());
}

void CMDIMainFrame::OnUpdate_SqlToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndSqlToolBar.IsVisible());
}

void CMDIMainFrame::OnUpdate_ConnectToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndConnectionBar.IsVisible());
}

CObjectViewerWnd& CMDIMainFrame::ShowTreeViewer ()
{
    ShowPane(&m_wndObjectViewerFrame, TRUE, FALSE, TRUE);
    return m_wndObjectViewer;
}

CGrepView& CMDIMainFrame::ShowGrepViewer ()
{
    ShowPane(&m_wndGrepFrame, TRUE, FALSE, TRUE);
    return m_wndGrepViewer;
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
    ShowPane(&m_wndPropTreeFrame, TRUE, FALSE, TRUE);
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

    static void updateLabel (CDockablePane& ctrlBar, UINT command, CString orgTitle)
    {
        int inx = orgTitle.Find(L" (");
        if (inx != -1)
            orgTitle = orgTitle.Left(inx);

        std::string label;
        if (Common::GUICommandDictionary::GetCommandAccelLabel(static_cast<Common::Command>(command), label))
            ctrlBar.SetWindowText(orgTitle + L" (" + Common::wstr(label).c_str() + L")");
    }

void CMDIMainFrame::UpdateViewerAccelerationKeyLabels ()
{
    updateLabel(m_wndObjectViewerFrame, ID_SQL_OBJ_VIEWER, lpszQuickViewer);
    updateLabel(m_wndDbSourceFrame,     ID_SQL_DB_SOURCE, lpszObjectsList);
    updateLabel(m_wndGrepFrame,         ID_FILE_SHOW_GREP_OUTPUT, lpszFindInFiles);
    updateLabel(m_wndFilePanel,         ID_VIEW_FILE_PANEL, lpszFilePanel);
    updateLabel(m_wndRecentFile,        ID_VIEW_HISTORY,    lpszFileHistory);
    updateLabel(m_wndPropTreeFrame,     ID_VIEW_PROPERTIES, lpszPropTreePanel);
    updateLabel(m_wndOpenFilesFrame,    ID_VIEW_OPEN_FILES, lpszOpenFiles);
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

// TODO implement the same in OE
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

        // for WinXP compatibility
        HINSTANCE user32_dll = NULL;

        if (!user32_dll) 
            user32_dll = LoadLibrary(L"USER32.DLL");

        typedef BOOL (WINAPI *ShutdownBlockReasonCreate_t)(HWND, LPCWSTR);
        FARPROC p_ShutdownBlockReasonCreate = NULL;

        if (!p_ShutdownBlockReasonCreate && user32_dll) 
            p_ShutdownBlockReasonCreate = GetProcAddress(user32_dll, "ShutdownBlockReasonCreate");

        //ShutdownBlockReasonCreate(m_hWnd, L"Auto-saving the data!");
        if (p_ShutdownBlockReasonCreate)
            ((ShutdownBlockReasonCreate_t)p_ShutdownBlockReasonCreate)(m_hWnd, L"Auto-saving the data!");

        OEWorkspaceManager::Get().InstantAutosave();

        typedef BOOL (WINAPI *ShutdownBlockReasonDestroy_t)(HWND);
        FARPROC p_ShutdownBlockReasonDestroy = NULL;

        if (!p_ShutdownBlockReasonDestroy && user32_dll) 
            p_ShutdownBlockReasonDestroy = GetProcAddress(user32_dll, "ShutdownBlockReasonDestroy");

        //ShutdownBlockReasonDestroy(m_hWnd);
        if (p_ShutdownBlockReasonDestroy)
            ((ShutdownBlockReasonDestroy_t)p_ShutdownBlockReasonDestroy)(m_hWnd);
    }
    _DEFAULT_HANDLER_

    return CWorkbookMDIFrame::OnQueryEndSession();
}

// FRM2
void CMDIMainFrame::SetupMDITabbedGroups ()
{
    auto settings = COEDocument::GetSettingsManager().GetGlobalSettings();

    CWorkbookMDIFrame::m_MDITabsCtrlTabSwitchesToPrevActive = settings->GetMDITabsCtrlTabSwitchesToPrevActive() ? TRUE : FALSE;

    CMDITabInfo mdiTabParams;
    mdiTabParams.m_tabLocation = settings->GetMDITabsOnTop() ? CMFCTabCtrl::LOCATION_TOP : CMFCTabCtrl::LOCATION_BOTTOM;
    mdiTabParams.m_style = settings->GetMDITabsRectangularLook() ? CMFCTabCtrl::STYLE_3D : CMFCTabCtrl::STYLE_3D_ONENOTE;
    mdiTabParams.m_bActiveTabCloseButton = settings->GetMDITabsActiveTabCloseButton() ? TRUE : FALSE;
    mdiTabParams.m_bAutoColor = settings->GetMDITabsAutoColor() ? TRUE : FALSE;
    mdiTabParams.m_bDocumentMenu = settings->GetMDITabsDocumentMenuButton() ? TRUE : FALSE;

    mdiTabParams.m_bTabIcons = FALSE;    
    mdiTabParams.m_bTabCustomTooltips = TRUE;
    mdiTabParams.m_bFlatFrame = TRUE;
    EnableMDITabbedGroups(TRUE, mdiTabParams);

    RecalcLayout();
}

void CMDIMainFrame::OnCreateChild (CMDIChildWnd* pWnd)
{
    static CString strTitle;
    pWnd->GetWindowText(strTitle);

    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask    = LVIF_TEXT|LVIF_PARAM;
        item.pszText = (LPWSTR)(LPCWSTR)strTitle;
        item.lParam  = (LPARAM)pWnd;
        m_wndOpenFiles.OpenFiles_Append(item);
    }
}

void CMDIMainFrame::OnDestroyChild (CMDIChildWnd* pWnd)
{
    m_wndOpenFiles.OpenFiles_RemoveByParam((LPARAM)pWnd);
}

void CMDIMainFrame::OnActivateChild (CMDIChildWnd* pWnd)
{
    __super::OnActivateChild(pWnd);
    m_wndOpenFiles.OpenFiles_ActivateByParam((LPARAM)pWnd);
}

void CMDIMainFrame::OnRenameChild (CMDIChildWnd* pWnd, LPCTSTR szTitle)
{
    LPARAM selected = m_wndOpenFiles.OpenFiles_GetCurSelParam();
    int iImage = GetImageByDocument(pWnd->GetActiveDocument());

    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask    = LVIF_TEXT|LVIF_PARAM;
        item.pszText = (LPTSTR)szTitle;
        item.lParam  = (LPARAM)pWnd;
        //if (iImage != -1) 
        //{
            item.iImage = iImage;
            item.mask |= LVIF_IMAGE;
        //}
        m_wndOpenFiles.OpenFiles_UpdateByParam((LPARAM)pWnd, item);
    }

    // 2017-12-01 bug fix, restoring selection after item being renamed 
    if (selected)
        m_wndOpenFiles.OpenFiles_ActivateByParam(selected);
}

