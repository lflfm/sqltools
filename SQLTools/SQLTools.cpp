/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2017 Aleksey Kochetov

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

#include <ShObjIdl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")
using namespace Gdiplus;

#include "SQLTools.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/ExceptionHelper.h"
#include "FileWatch/FileWatch.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "SQLWorksheet/SQLWorksheetDoc.h"
#include "Dlg/AboutDlg.h"
#include "OpenEditor/OEGeneralPage.h"
#include "CommandLineparser.h"
#include "VersionInfo.rh"
#include <COMMON/OsInfo.h>
#include <COMMON/ExceptionHelper.h>
#include "COMMON/NVector.h"
#include <COMMON/ConfigFilesLocator.h>
#include "DbBrowser\ObjectTree_Builder.h"
#include "ServerBackgroundThread/Thread.h"
#include "ServerBackgroundThread/TaskQueue.h"
#include "ThreadCommunication/MessageOnlyWindow.h"
#include "OpenEditor/OEWorkspaceManager.h"
#include "OpenEditor/RecentFileList.h"
#include "OpenEditor/FavoritesList.h"
#include "COMMON\StrHelpers.h" // for print_time

/*
    30.03.2003 improvement, command line support, try sqltools /h to get help
    02.06.2003 bug fix, unknown exception if Oracle client is not installed or included in PATH
    24.09.2007 some improvements taken from sqltools++ by Randolf Geist
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Common;

/////////////////////////////////////////////////////////////////////////////
// CSQLToolsApp
static ITaskbarList3* g_pTaskbarList = 0;
GdiplusStartupInput g_gdiplusStartupInput;
ULONG_PTR           g_gdiplusToken;

const UINT CSQLToolsApp::m_msgCommandLineProcess = WM_USER + 1;

SQLToolsSettings* CSQLToolsApp::m_pSettings = NULL;

IMPLEMENT_DYNCREATE(CSQLToolsApp, CWinAppEx)

BEGIN_MESSAGE_MAP(CSQLToolsApp, CWinAppEx)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_EDIT_PERMANENT_SETTINGS, OnEditPermanetSettings)
    ON_COMMAND(ID_APP_SETTINGS, OnAppSettings)
    ON_COMMAND(ID_SQL_CONNECT, OnSqlConnect)
    ON_COMMAND(ID_SQL_RECONNECT, OnSqlReconnect)
    ON_COMMAND(ID_SQL_COMMIT, OnSqlCommit)
    ON_COMMAND(ID_SQL_ROLLBACK, OnSqlRollback)
    ON_COMMAND(ID_SQL_DISCONNECT, OnSqlDisconnect)
    ON_COMMAND(ID_SQL_DBMS_OUTPUT, OnSqlDbmsOutput)
    ON_COMMAND(ID_SQL_SUBSTITUTION_VARIABLES, OnSqlSubstitutionVariables)
    ON_UPDATE_COMMAND_UI(ID_SQL_SUBSTITUTION_VARIABLES, OnUpdate_SqlSubstitutionVariables)

    ON_COMMAND(ID_SQL_HELP, OnSqlHelp)
    ON_COMMAND(ID_SQL_SESSION_STATISTICS, OnSqlSessionStatistics)
    ON_COMMAND(ID_SQL_REFRESH_OBJECT_CACHE, OnSqlRefreshObjectCache)
    ON_COMMAND(ID_SQL_READ_ONLY, OnSqlToggleReadOnly)
    ON_COMMAND(ID_SQL_EXTRACT_SCHEMA, OnSqlExtractSchema)
    ON_COMMAND(ID_SQL_TABLE_TRANSFORMER, OnSqlTableTransformer)
    ON_COMMAND(ID_SQLTOOLS_ON_THE_WEB, OnSQLToolsOnTheWeb)
    ON_COMMAND(ID_APP_SETTINGS_FOLDER, OnAppSettingsFoder)
    ON_UPDATE_COMMAND_UI_RANGE(ID_SQL_CONNECT, ID_SQL_TABLE_TRANSFORMER, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCIGRID, OnUpdate_EditIndicators)
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinAppEx::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinAppEx::OnFileOpen)
    // File based document commands
    ON_COMMAND(ID_FILE_CLOSE_ALL, OnFileCloseAll)
    ON_COMMAND(ID_FILE_SAVE_ALL, OnFileSaveAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_ALL, OnUpdate_FileSaveAll)
    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinAppEx::OnFilePrintSetup)
    ON_COMMAND_RANGE(ID_APP_DBLKEYACCEL_FIRST, ID_APP_DBLKEYACCEL_LAST, OnDblKeyAccel)

    ON_UPDATE_COMMAND_UI(ID_INDICATOR_POS,        OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_SCROLL_POS, OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_TYPE,  OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR,        OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_TYPE,  OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_BLOCK_TYPE, OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_LINES, OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_WORKSPACE_NAME, OnUpdate_IndicatorWorkspaceName)

    ON_THREAD_MESSAGE(CFileWatch::m_msgFileWatchNotify, OnFileWatchNotify)
    ON_THREAD_MESSAGE(m_msgCommandLineProcess, OnCommandLineProcess)

    ON_COMMAND(ID_FILE_NEW_EXT, OnFileNewExt)

    ON_COMMAND(ID_SQL_HALT_ON_ERROR, OnSqlHaltOnError)
    ON_UPDATE_COMMAND_UI(ID_SQL_HALT_ON_ERROR, OnUpdate_SqlHaltOnError)
    ON_COMMAND(ID_SQL_CANCEL, &CSQLToolsApp::OnSqlCancel)
    ON_UPDATE_COMMAND_UI(ID_SQL_CANCEL, &CSQLToolsApp::OnUpdateSqlCancel)

    ON_COMMAND(ID_WORKSPACE_COPY          , OnWorkspaceCopy         )
    ON_COMMAND(ID_WORKSPACE_PASTE         , OnWorkspacePaste        )
    ON_COMMAND(ID_WORKSPACE_SAVE          , OnWorkspaceSave         )
    ON_COMMAND(ID_WORKSPACE_SAVE_AS       , OnWorkspaceSaveAs       )
    ON_COMMAND(ID_WORKSPACE_SAVE_QUICK    , OnWorkspaceSaveQuick    )
    ON_COMMAND(ID_WORKSPACE_OPEN          , OnWorkspaceOpen         )
    ON_COMMAND(ID_WORKSPACE_OPEN_QUICK    , OnWorkspaceOpenQuick    )
    ON_COMMAND(ID_WORKSPACE_OPEN_AUTOSAVED, OnWorkspaceOpenAutosaved)
    ON_COMMAND(ID_WORKSPACE_CLOSE         , OnWorkspaceClose        )

    ON_UPDATE_COMMAND_UI(ID_WORKSPACE_PASTE, OnUpdate_WorkspacePaste)
    ON_UPDATE_COMMAND_UI(ID_WORKSPACE_CLOSE, OnUpdate_WorkspaceClose)

    ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
    ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)

    ON_UPDATE_COMMAND_UI(ID_WORKSPACE_MRU_FILE1, OnUpdateRecentWorkspaceMenu)
    ON_COMMAND_EX_RANGE(ID_WORKSPACE_MRU_FILE1, ID_WORKSPACE_MRU_FILE16, OnOpenRecentWorkspace)

    ON_UPDATE_COMMAND_UI(ID_WORKSPACE_QUICK_MRU_FIRST, OnUpdateRecentQuickWorkspace)
    ON_COMMAND_EX_RANGE(ID_WORKSPACE_QUICK_MRU_FIRST, ID_WORKSPACE_QUICK_MRU_LAST, OnOpenQuickWorkspace)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSQLToolsApp construction

    CWinThread* CSQLToolsApp::m_pServerBackgroundThread = NULL;
    CString CSQLToolsApp::m_displayNotConnected = "Not connected";

SQLToolsSettings& CSQLToolsApp::getSettings ()
{
    if (!m_pSettings) 
        m_pSettings = new SQLToolsSettings;
    return *m_pSettings;
}

CSQLToolsApp::CSQLToolsApp()
{
    m_hMutex = NULL;
    m_dblKeyAccelInx = -1;
    m_accelTable = NULL;
    m_pDocManager = new COEDocManager;
    m_pPLSDocTemplate = 0;

    SetConnectOpen(false);
    SetConnectSlow(false);
    SetDatabaseOpen(false);
    SetCanReconnect(false);
    SetConnectReadOnly(false);
    SetCurrentSchema("");
    SetServerVersion(OCI8::esvServerUnknown);

    SetConnectUID     ("");
    SetConnectPassword("");
    SetConnectAlias   ("");
    SetConnectMode    (OCI8::ecmDefault);

    SetActivePrimeExecution(false);

    EnableHtmlHelp();

    COEGeneralPage::m_keymapLayoutList = "Custom;Default;Default(MS);";

    m_displayConnectionString = m_displayNotConnected;

    // Initialize GDI+.
    GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);
}

#include "SQLWorksheet/HistoryFileManager.h"

CSQLToolsApp::~CSQLToolsApp()
{
    try { EXCEPTION_FRAME;

        HistoryFileManager::GetInstance().Flush();

        COEDocument::DestroySettingsManager();

        if (m_pSettings) 
        {
            delete m_pSettings;
            m_pSettings = NULL;
        }

        GdiplusShutdown(g_gdiplusToken);

    }
    _DESTRUCTOR_HANDLER_;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSQLToolsApp object

CSQLToolsApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSQLToolsApp initialization

    static void app_info (std::string& name, std::string& info, std::string& filePrefix)
    {
        name = SQLTOOLS_VERSION;
        info = string("OCI.DLL: ") + getDllVersionProperty("OCI.DLL", "FileVersion");

        if (const char* oracle_home = getenv("ORACLE_HOME"))
        {
            info += "\r\nORACLE_HOME = ";
            info += oracle_home;
        }
        if (const char* tns_admin = getenv("TNS_ADMIN"))
        {
            info += "\r\nTNS_ADMIN = ";
            info += tns_admin;
        }
        if (!theApp.GetServerVersionStr().empty())
        {
            info += "\nConnected to: ";
            info += theApp.GetServerVersionStr().c_str();
        }

        filePrefix = "SQLTools_BugReport";
    }

    static void update_application_title ()
    {
        theApp.UpdateApplicationTitle();
    }

BOOL CSQLToolsApp::InitInstance()
{
#if 0
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
                  |_CRTDBG_DELAY_FREE_MEM_DF
                  |_CRTDBG_CHECK_ALWAYS_DF
                  |_CRTDBG_CHECK_CRT_DF
                  |_CRTDBG_LEAK_CHECK_DF);
#endif

    try { EXCEPTION_FRAME;

        SetRegistryBase(L"Workspace_2.0.1");

        ThreadCommunication::MessageOnlyWindow::GetWindow(); // let's make sure this window is created in the main thread
        m_pRecentFileList = counted_ptr<RecentFileList>(new RecentFileList);
        m_pFavoritesList = counted_ptr<FavoritesList>(new FavoritesList);

        InitCommonControls();
        Common::SetAppInfoFn(app_info);
        SEException::InstallSETranslator();
        set_terminate(Common::terminate);

        if (!AfxOleInit()) { AfxMessageBox(L"OLE Initialization failed!"); return FALSE; }

        AfxOleGetMessageFilter()->EnableNotRespondingDialog(FALSE);

        if (!g_pTaskbarList)
            CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList3, (void**)&g_pTaskbarList);

        getSettings().AddSubscriber(this);
        
        if (Common::ConfigFilesLocator::Initialize(L"GNU\\SQLToolsU", L"SharedDataU", L"settings.xml"))
        {
            Common::ConfigFilesLocator::ImportFromOldConfiguration (L"GNU\\SQLTools", L"SharedData", L"settings.xml", 
                L"connections.xml,custom.keymap,favorites.xml,history.xml,languages.xml,schemaddl.xml,settings.xml,sqltools.xml,SQLToolsApp.ini,templates.xml");
        }
        LoadSettings();
        COEDocument::LoadSettingsManager(); // Load editor settings

        setlocale(LC_ALL, COEDocument::GetSettingsManager().GetGlobalSettings()->GetLocale().c_str());

        if (!m_commandLineParser.Parse())
            return FALSE;

        if (!AllowThisInstance())
            return FALSE;

        SetRegistryKey(L"KochWare");
        
        m_pRecentFileList->Open(Common::ConfigFilesLocator::GetBaseFolder() + L"\\history.xml");
        m_pFavoritesList->Open(Common::ConfigFilesLocator::GetBaseFolder() + L"\\favorites.xml");

        InitContextMenuManager(); // FRM
        m_commandLineParser.SetStartingDefaults();

        m_pPLSDocTemplate = new COEMultiDocTemplate(
            IDR_SQLTYPE,
            RUNTIME_CLASS(CPLSWorksheetDoc),
            RUNTIME_CLASS(CMDIChildFrame),
            RUNTIME_CLASS(COEditorView),
            m_pRecentFileList);

        AddDocTemplate(m_pPLSDocTemplate);

        OEWorkspaceManager::Get().Init(
            (COEDocManager*)m_pDocManager, m_pPLSDocTemplate, 
            m_pRecentFileList, m_pFavoritesList);
        OEWorkspaceManager::Get().SetWorkspaceExtetion(L".sqlt-workspace");
        OEWorkspaceManager::Get().SetSnapshotExtetion(L".sqlt-snapshot");
        OEWorkspaceManager::Get().SetUpdateApplicationTitle(update_application_title);

        m_orgMainWndTitle = SQLTOOLS_RELEASE_STR;
        m_orgMainWndTitle += 'b';
        m_orgMainWndTitle += SQLTOOLS_BUILD_S;

        InitGUICommand();
        // create main MDI Frame window
        CMDIMainFrame* pMainFrame = new CMDIMainFrame;
        SetupFileManager(pMainFrame);

        // FRM
        EnableLoadWindowPlacement(COEDocument::GetSettingsManager().GetGlobalSettings()->GetSaveMainWinPosition() ? TRUE : FALSE);

        if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
            return FALSE;
        
        pMainFrame->SetTitle(m_orgMainWndTitle);
        pMainFrame->GetConnectionBar().SetConnectionDescription(m_displayConnectionString, RGB(0,0,0));

        m_pMainWnd = pMainFrame;
        m_pMainWnd->DragAcceptFiles();     // Enable drag/drop open
        EnableShellOpen();                 // Enable DDE Execute open
        UpdateAccelAndMenu();

        // it has to be called after creating of main window but befor command line processing 
        SetupRecentFileList(pMainFrame);

        PostThreadMessage(m_msgCommandLineProcess, 0, 0);

        m_pMainWnd->ShowWindow(m_nCmdShow); // The main window has been
        m_pMainWnd->UpdateWindow();         // initialized, so show and update it.

        m_pServerBackgroundThread =
            AfxBeginThread(ServerBackgroundThread::ThreadProc, 0, THREAD_PRIORITY_ABOVE_NORMAL);

        if (!m_pServerBackgroundThread)
            AfxMessageBox(L"Background Thread #1 initialization failed!");

        // FRM enable File panel here
        pMainFrame->GetFilePanelWnd().CompleteStartup();
    }
    catch (CException* x)           { DEFAULT_HANDLER(x);   return FALSE; }
    catch (const std::exception& x) { DEFAULT_HANDLER(x);   return FALSE; }
    catch (...)                     { DEFAULT_HANDLER_ALL;  return FALSE; }

    if (m_hMutex)
        ReleaseMutex(m_hMutex);

    return TRUE;
}

int CSQLToolsApp::ExitInstance()
{
    if (m_pMainWnd)
        DestroyAcceleratorTable(((CMDIMainFrame*)m_pMainWnd)->m_hAccelTable); // there is a better way

   if (m_hMutex)
       CloseHandle(m_hMutex);

    if (m_pServerBackgroundThread)
        ServerBackgroundThread::BkgdRequestQueue::Get().Shutdown();

    CWinThread* pWorkspaceManagerThread = OEWorkspaceManager::Get().Shutdown();

    m_pRecentFileList->Save();

    if (m_pServerBackgroundThread)
        WaitForSingleObject(*m_pServerBackgroundThread, 1500);

    if (pWorkspaceManagerThread)
    {
        DWORD retVal = WaitForSingleObject(*pWorkspaceManagerThread, 1500);
        
        if (retVal == WAIT_OBJECT_0 || retVal == WAIT_FAILED)
        OEWorkspaceManager::Destroy();
    }
    else
        OEWorkspaceManager::Destroy();

    m_pFavoritesList.reset(0);

    getSettings().RemoveSubscriber(this);

    int retVal = CWinAppEx::ExitInstance();

    SEException::UninstallSETranslator();

    return retVal;
}


void CSQLToolsApp::UpdateApplicationTitle ()
{
    CString mainWndTitle;

    if (GetConnectOpen())
    {
        mainWndTitle = m_displayConnectionString;
        mainWndTitle.MakeUpper();
        mainWndTitle += " - ";
    }
   
    mainWndTitle += m_orgMainWndTitle;

    if (OEWorkspaceManager::Get().HasActiveWorkspace() 
    && COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceShowNameInTitleBar())
    {
        CString buffer = ::PathFindFileName(OEWorkspaceManager::Get().GetWorkspacePath().c_str());
        LPWSTR ext = ::PathFindExtension(buffer.GetBuffer());
        if (ext && *ext == '.') *ext = 0;
        m_workspaceName = buffer;
        mainWndTitle += " - ";
        mainWndTitle += '{';
        mainWndTitle += buffer;
        mainWndTitle += '}';
    }
    else
        m_workspaceName.Empty();

    if (m_pMainWnd && *m_pMainWnd)
    {
        m_pMainWnd->SetWindowText(mainWndTitle);
        if (m_pMainWnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)))
            ((CFrameWnd*)m_pMainWnd)->SetTitle(mainWndTitle);
    }

    free((void*)m_pszAppName);
    m_pszAppName = wcsdup(mainWndTitle);
}

/////////////////////////////////////////////////////////////////////////////
// CSQLToolsApp commands
// App command to run the dialog

void CSQLToolsApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

void CSQLToolsApp::OnEditPermanetSettings()
{
    try { EXCEPTION_FRAME;

        COEDocument::ShowSettingsDialog();
        setlocale(LC_ALL, COEDocument::GetSettingsManager().GetGlobalSettings()->GetLocale().c_str());
        UpdateAccelAndMenu();
//        ((CMDIMainFrame*)m_pMainWnd)->SetCloseFileOnTabDblClick(
//            COEDocument::GetSettingsManager().GetGlobalSettings()->GetDoubleClickCloseTab() ? TRUE : FALSE);
        SetupFileManager((CMDIMainFrame*)m_pMainWnd);
    }
    _DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnSqlHelp()
{
    CString path;
    AppGetPath(path);
    ::HtmlHelp(*AfxGetMainWnd(), path + L"\\sqlqkref.chm", HH_DISPLAY_TOPIC, 0);
}

void CSQLToolsApp::OnSQLToolsOnTheWeb()
{
    HINSTANCE result = ShellExecute( NULL, L"open", L"http://www.sqltools.net", NULL, NULL, SW_SHOW);

    if((UINT)result <= HINSTANCE_ERROR)
    {
        MessageBeep((UINT)-1);
        AfxMessageBox(L"Cannot open a default browser.", MB_OK|MB_ICONSTOP);
    }
}

void CSQLToolsApp::OnAppSettingsFoder()
{
    HINSTANCE result = ShellExecute( NULL, L"open", Common::ConfigFilesLocator::GetBaseFolder().c_str(), NULL, NULL, SW_SHOW);

    if((UINT)result <= HINSTANCE_ERROR)
    {
        MessageBeep((UINT)-1);
        AfxMessageBox(L"Cannot open a folder.", MB_OK|MB_ICONSTOP);
    }
}

void CSQLToolsApp::OnUpdate_EditIndicators (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(FALSE);
}

void CSQLToolsApp::OnUpdate_IndicatorWorkspaceName  (CCmdUI* pCmdUI)
{
    pCmdUI->SetText(m_workspaceName);
}

void CSQLToolsApp::OnFileWatchNotify (WPARAM, LPARAM)
{
    try { EXCEPTION_FRAME;

        TRACE("CSQLToolsApp::OnFileWatchNotify\n");
        CFileWatch::NotifyClients();
    }
    _DEFAULT_HANDLER_
}

//#include <COMMON/ErrorDlg.h>
void CSQLToolsApp::OnServerBackground_ResultQueueNotEmpty ()
{
    try { EXCEPTION_FRAME;

        TRACE("CSQLToolsApp::OnServerBackground_ResultQueueNotEmpty\n");

        ServerBackgroundThread::TaskQueue& resultQueue = ServerBackgroundThread::BkgdResultQueue::Get();

        while (true)
        {
            ServerBackgroundThread::TaskPtr task = resultQueue.Pop();

            if (!task.get())
                break;

            if (task->IsOk())
            {
                task->ReturnInForeground();
                if (!task->IsSilent())
                {
                    string message = "BkgrThread: Task \"" + task->GetName() + "\" completed";

                    double executionTime = task->GetExecutionTime();
                    
                    if (executionTime)
                        message += " in " + Common::print_time(executionTime);

                    message += ".";
                    Global::SetStatusText(message, true);
                }
            }
            else
            {
                task->ReturnErrorInForeground();

                Global::SetStatusText("BkgrThread: Task \"" + task->GetName() + "\" failed.", true);

                //TODO#3: create an aternative template for CErrorDlg and use it here
                //CErrorDlg(task->GetLastError().c_str(), "").DoModal();

                istringstream error(task->GetLastError());
                string message, line;

                for (int i = 0; i < 25 && getline(error, line); ++i)
                    message += line + "\n";
                
                if (error)
                    message += "...";

                AfxMessageBox(Common::wstr("The background task \"" + task->GetName() + "\" failed with the error:\n\n" + message).c_str(), MB_ICONERROR | MB_OK); 
            }
        }
    }
    _DEFAULT_HANDLER_
}

void CSQLToolsApp::OnActivateApp (BOOL bActive)
{
    if (!bActive && COEDocument::GetSettingsManager().GetGlobalSettings()->GetFileSaveOnSwith())
        SaveAllModified(true/*skipNew*/, true/*saveSilently*/, false/*canUseWorksapce*/, false/*onExit*/);

    if (bActive)
        CFileWatch::ResumeThread();
    else
        CFileWatch::SuspendThread();
}

void CSQLToolsApp::OnFileCloseAll ()
{
    if (OEWorkspaceManager::Get().HasActiveWorkspace())
        OEWorkspaceManager::Get().WorkspaceCloseActive();
    else
        if (m_pDocManager != NULL && m_pDocManager->IsKindOf(RUNTIME_CLASS(COEDocManager)))
            ((COEDocManager*)m_pDocManager)->SaveAndCloseAllDocuments();
}

BOOL CSQLToolsApp::SaveAllModified () // save before exit
{
    if (OEWorkspaceManager::Get().HasActiveWorkspace())
        OEWorkspaceManager::Get().WorkspaceCloseActive();

    return SaveAllModified(false/*skipNew*/, false/*saveSilently*/, true/*canUseWorksapce*/, true/*onExit*/);
}

BOOL CSQLToolsApp::SaveAllModified (bool skipNew, bool saveSilently, bool canUseWorksapce, bool onExit)
{
    if (m_pDocManager != NULL)
    {
        if (m_pDocManager->IsKindOf(RUNTIME_CLASS(COEDocManager)))
            return ((COEDocManager*)m_pDocManager)->SaveAllModified(skipNew, saveSilently, canUseWorksapce, onExit);
        else 
            return m_pDocManager->SaveAllModified();
    }
    return TRUE;
}

void CSQLToolsApp::OnFileSaveAll ()
{
    SaveAllModified(false/*skipNew*/, true/*saveSilently*/, false/*canUseWorksapce*/, false/*onExit*/);
}

void CSQLToolsApp::OnUpdate_FileSaveAll (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_pDocManager != NULL && m_pDocManager->GetOpenDocumentCount());
}

BOOL CSQLToolsApp::OnIdle (LONG lCount)
{
    static int counter = 0;
    static clock64_t lastClock = 0;
    
    BOOL more = FALSE;

    try { EXCEPTION_FRAME;

        if (!lCount)
        {
            clock64_t currClock = clock64();
            double interval = double(currClock - lastClock) / CLOCKS_PER_SEC;
            
            if (interval < .125)
                return FALSE;

            lastClock = currClock;

            //more = CWinAppEx::OnIdle(lCount);
            //more |= m_pPLSDocTemplate->OnIdle(lCount, ((CMDIMainFrame*)m_pMainWnd)->MDIGetActive());
            more = CWinThread::OnIdle(lCount);
            if (COEDocument::GetSettingsManager().GetGlobalSettings()->GetSyntaxGutter())
                more |= m_pPLSDocTemplate->OnIdle(lCount, ((CMDIMainFrame*)m_pMainWnd)->MDIGetActive());
        }
        else if (lCount == 1 && !(++counter % 10)) // call it only on 10th try
        {
            //more = CWinAppEx::OnIdle(lCount);
            //more |= m_pPLSDocTemplate->OnIdle(lCount, ((CMDIMainFrame*)m_pMainWnd)->MDIGetActive());
            more = CWinThread::OnIdle(lCount);
        }
    }
    catch (...) { 
        /*it's silent*/ 
    }

    return more;
}

void CSQLToolsApp::OnFileNewExt ()
{
    COEMultiDocTemplate::EnablePickDocType enabler(*m_pPLSDocTemplate);
    OnFileNew();
}


void CSQLToolsApp::OnSqlCancel()
{
    DoCancel(false);
}


void CSQLToolsApp::OnUpdateSqlCancel(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(GetConnectOpen() && GetActivePrimeExecution());
}

void CSQLToolsApp::DoCancel (bool silent)
{
    try { EXCEPTION_FRAME;

        if (GetConnectOpen() && GetActivePrimeExecution() && m_BreakHandler.get()) 
        {
            CSingleLock lk(&m_BreakHandler->GetCS(), TRUE);

            if (silent 
            || !GetSQLToolsSettings().GetCancelConfirmation()
            || AfxMessageBox(L"Do you really want to cancel the server process?",  
                  MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2) == IDYES
            )
                m_BreakHandler->Break();
        }
    }
    _DEFAULT_HANDLER_
}

void CSQLToolsApp::SetTaskbarProgressState (TaskbarProgressState state)
{
    if (g_pTaskbarList)
    {
        switch (state)
        {
        case IDLE_STATE:
            g_pTaskbarList->SetProgressState(*AfxGetMainWnd(), TBPF_NOPROGRESS);
            break;
        case WORKING_STATE: 
            g_pTaskbarList->SetProgressState(*AfxGetMainWnd(), TBPF_INDETERMINATE);
            break;
        case WAITING_STATE: 
            g_pTaskbarList->SetProgressState(*AfxGetMainWnd(), TBPF_PAUSED);
            g_pTaskbarList->SetProgressValue(*AfxGetMainWnd(), 100, 100);
            //FlashWindow(*AfxGetMainWnd(), TRUE);
            break;
        case ERROR_STATE:
            g_pTaskbarList->SetProgressState(*AfxGetMainWnd(), TBPF_ERROR);
            g_pTaskbarList->SetProgressValue(*AfxGetMainWnd(), 100, 100);
            //FlashWindow(*AfxGetMainWnd(), TRUE);
            break;
        }
    }
}

void CSQLToolsApp::OnWorkspaceCopy ()
{
    try
    {
        CWaitCursor wait;
        OEWorkspaceManager::Get().WorkspaceCopy();
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnWorkspacePaste ()
{
    try
    {
        CWaitCursor wait;
        OEWorkspaceManager::Get().WorkspacePaste();
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnWorkspaceSave ()
{
    try
    {
        CWaitCursor wait;
        OEWorkspaceManager::Get().WorkspaceSave(false);
        Global::SetStatusText("Workspace saved.");
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnWorkspaceSaveAs ()
{
    try
    {
        CWaitCursor wait;
        OEWorkspaceManager::Get().WorkspaceSave(true);
        Global::SetStatusText("Workspace saved.");
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnWorkspaceSaveQuick ()
{
    try
    {
        CWaitCursor wait;
        OEWorkspaceManager::Get().WorkspaceSaveQuick();
        Global::SetStatusText("Quick Snapshot created.");
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnWorkspaceOpen ()
{
    try
    {
        CWaitCursor wait;
        OEWorkspaceManager::Get().WorkspaceOpen();
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnWorkspaceOpenQuick ()
{
    try
    {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING|MF_DISABLED,  ID_WORKSPACE_QUICK_MRU_FIRST, L"Recent Quicksaved Snapshot");

        CRect rc;
        m_pMainWnd->GetWindowRect(&rc);
        menu.TrackPopupMenu(TPM_CENTERALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, 
            (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2, m_pMainWnd, &rc);
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnWorkspaceOpenAutosaved ()
{
    try
    {
        CWaitCursor wait;
        OEWorkspaceManager::Get().WorkspaceOpenAutosaved();
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnWorkspaceClose ()
{
    try
    {
        CWaitCursor wait;
        OEWorkspaceManager::Get().WorkspaceCloseActive();
    }
    _OE_DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnUpdate_WorkspacePaste (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsClipboardFormatAvailable(OEWorkspaceManager::Get().GetWorkspaceClipboardFormat()));
}

void CSQLToolsApp::OnUpdate_WorkspaceClose (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(OEWorkspaceManager::Get().HasActiveWorkspace());
}

void CSQLToolsApp::OnUpdateRecentFileMenu (CCmdUI* pCmdUI)
{
    if (pCmdUI->m_pSubMenu != NULL)
        return;

    if (m_pRecentFileList.get())
        m_pRecentFileList->UpdateFileMenu(pCmdUI);
    else
        pCmdUI->Enable(FALSE);
}

BOOL CSQLToolsApp::OnOpenRecentFile (UINT nID)
{
    ASSERT_VALID(this);
    ENSURE_ARG(nID >= ID_FILE_MRU_FILE1);
    ENSURE_ARG(nID < ID_FILE_MRU_LAST);

    int nIndex = nID - ID_FILE_MRU_FILE1;

    if (m_pRecentFileList.get())
    {
        CString fname;
        if (m_pRecentFileList->GetFileName(nIndex, fname))
        {
            // DoOpenFile offers to remove a file from history if it does not exist 
            ((CMDIMainFrame*)m_pMainWnd)->GetRecentFileWnd().DoOpenFile((LPCTSTR)fname, (unsigned int)-1);  // FRM
        }
    }

    return TRUE;
}

void CSQLToolsApp::OnUpdateRecentWorkspaceMenu (CCmdUI* pCmdUI)
{
    if (pCmdUI->m_pSubMenu != NULL)
        return;

    if (m_pRecentFileList.get())
        m_pRecentFileList->UpdateWorkspaceMenu(pCmdUI);
    else
        pCmdUI->Enable(FALSE);
}

BOOL CSQLToolsApp::OnOpenRecentWorkspace (UINT nID)
{
    ASSERT_VALID(this);
    ENSURE_ARG(nID >= ID_WORKSPACE_MRU_FILE1);
    ENSURE_ARG(nID < ID_WORKSPACE_MRU_LAST);

    int nIndex = nID - ID_WORKSPACE_MRU_FILE1;

    if (m_pRecentFileList.get())
    {
        CString fname;
        if (m_pRecentFileList->GetWorkspaceName(nIndex, fname))
        {
            if (::PathFileExists(fname))
                OEWorkspaceManager::Get().WorkspaceOpen(fname, false);
            else if (AfxMessageBox(("The file \"" + fname + "\" does not exist.\n\nWould you like to remove it from the history?")
                , MB_ICONEXCLAMATION|MB_YESNO) == IDYES)
            {
                m_pRecentFileList->RemoveWorkspace((LPCWSTR)fname);
            }
        }
    }

    return TRUE;
}

void CSQLToolsApp::OnUpdateRecentQuickWorkspace (CCmdUI* pCmdUI)
{
    if (pCmdUI->m_pSubMenu != NULL)
        return;

    int size = COEDocument::GetSettingsManager().GetGlobalSettings()->GetHistoryQSWorkspacesInRecentMenu();

    if (m_pFavoritesList.get())
        m_pFavoritesList->UpdateWorkspaceQuickSaveMenu(pCmdUI, size);
    else
        pCmdUI->Enable(FALSE);
}

BOOL CSQLToolsApp::OnOpenQuickWorkspace (UINT nID)
{
    ASSERT_VALID(this);
    ENSURE_ARG(nID >= ID_WORKSPACE_QUICK_MRU_FIRST);
    ENSURE_ARG(nID < ID_WORKSPACE_QUICK_MRU_LAST);

    int nIndex = nID - ID_WORKSPACE_QUICK_MRU_FIRST;

    if (m_pFavoritesList.get())
    {
        std::vector<std::string> list;
        m_pFavoritesList->GetWorkspaceQuickSaves(list);
        string path = list.at(nIndex);

        OEWorkspaceManager::Get().WorkspaceOpen(wstr(path).c_str(), true);

        m_pFavoritesList->RemoveWorkspaceQuickSave(path);
    }

    return TRUE;
}
