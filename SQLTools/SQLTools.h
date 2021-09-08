/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2016 Aleksey Kochetov

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

#pragma once
#ifndef __SQLTOOLS_H__
#define __SQLTOOLS_H__

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <memory>
#include "resource.h"               // main symbols
#include "OpenEditor/OEDocManager.h"
#include <OCI8/Connect.h>
#include "SQLToolsSettings.h"
#include "CommandLineparser.h"
#include <arg_shared.h>

#define APP_DECLARE_PROPERTY(T,N) \
    protected: \
        T m_##N; \
    public: \
        const T& Get##N () const { return m_##N; }; \
        void  Set##N (const T& val) { m_##N = val; }

    class CMDIMainFrame;
    class RecentFileList;
    class FavoritesList;

    using arg::counted_ptr;

class CSQLToolsApp : public CWinAppEx, SettingsSubscriber
{
	DECLARE_DYNCREATE(CSQLToolsApp)

    static const int UM_REQUEST_QUEUE_NOT_EMPTY = WM_USER + 1;
    static const int UM_REQUEST_QUEUE_EMPTY     = WM_USER + 2;

    CString m_displayConnectionString;
    CString m_orgMainWndTitle, m_workspaceName;
    static CString m_displayNotConnected;

    COEMultiDocTemplate* m_pPLSDocTemplate;
    counted_ptr<RecentFileList> m_pRecentFileList;
    counted_ptr<FavoritesList>  m_pFavoritesList;
    CCommandLineParser m_commandLineParser;

    HANDLE m_hMutex;
    HACCEL m_accelTable;
    int m_dblKeyAccelInx;

    void InitGUICommand ();
    void UpdateAccelAndMenu ();
    BOOL AllowThisInstance ();
    BOOL HandleAnotherInstanceRequest (COPYDATASTRUCT* pCopyDataStruct);

    friend CWinThread* GetServerBackgroundThread ();
    static CWinThread* m_pServerBackgroundThread;

    void AfterOpeningConnect ();
    void AfterClosingConnect ();
    void AfterToggleReadOnly ();

	virtual BOOL SaveAllModified(); // save before exit
    BOOL SaveAllModified (bool skipNew, bool saveSilently, bool canUseWorksapce, bool onExit);

    static SQLToolsSettings* m_pSettings;
    static SQLToolsSettings& getSettings ();

    friend const SQLToolsSettings& GetSQLToolsSettings ();
    friend SQLToolsSettings& GetSQLToolsSettingsForUpdate ();
    friend bool ShowDDLPreferences (SQLToolsSettings&, bool bLocal = false);

    void LoadSettings ();
    void SaveSettings ();

    // overrided SettingsSubscriber method
    virtual void OnSettingsChanged ();

    void SetupFileManager (CMDIMainFrame* pMainWnd);
    void SetupRecentFileList (CMDIMainFrame* pMainWnd);

    APP_DECLARE_PROPERTY(bool, ConnectOpen);
    APP_DECLARE_PROPERTY(bool, ConnectSlow);
    APP_DECLARE_PROPERTY(bool, DatabaseOpen);
    APP_DECLARE_PROPERTY(bool, CanReconnect);
    APP_DECLARE_PROPERTY(bool, ConnectReadOnly);

    APP_DECLARE_PROPERTY(string, ConnectUID);
    APP_DECLARE_PROPERTY(string, ConnectPassword);
    APP_DECLARE_PROPERTY(string, ConnectAlias);
    APP_DECLARE_PROPERTY(OCI8::EConnectionMode, ConnectMode);

    APP_DECLARE_PROPERTY(string, ConnectDisplayString);
    APP_DECLARE_PROPERTY(string, User);
    APP_DECLARE_PROPERTY(string, CurrentSchema);
    APP_DECLARE_PROPERTY(string, ConnectGlobalName);
    APP_DECLARE_PROPERTY(string, ServerVersionStr);
    APP_DECLARE_PROPERTY(OCI8::EServerVersion, ServerVersion);

    APP_DECLARE_PROPERTY(arg::counted_ptr<OCI8::BreakHandler>, BreakHandler);
    APP_DECLARE_PROPERTY(bool, ActivePrimeExecution);

public:
    static const UINT m_msgCommandLineProcess;

    CSQLToolsApp ();
    ~CSQLToolsApp ();

    CMultiDocTemplate* GetPLSDocTemplate()      { return m_pPLSDocTemplate; }
    LPCWSTR GetDisplayConnectionString ()        { return m_displayConnectionString; }
    counted_ptr<RecentFileList> GetRecentFileList () { return m_pRecentFileList; }
    counted_ptr<FavoritesList>  GetFavoritesList ()  { return m_pFavoritesList;  }

    int  GetDblKeyAccelInx ()   const           { return m_dblKeyAccelInx; }
    bool IsWaitingForSecondAccelKey ()   const  { return m_dblKeyAccelInx != -1; }
    void CancelWaitingForSecondAccelKey ()      { m_dblKeyAccelInx = -1; }

    void OnActivateApp (BOOL bActive);

    enum TaskbarProgressState { IDLE_STATE, WORKING_STATE, WAITING_STATE, ERROR_STATE };
    void SetTaskbarProgressState (TaskbarProgressState);

	virtual BOOL InitInstance();
	virtual int ExitInstance();

    void UpdateApplicationTitle ();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnAppAbout();
	afx_msg void OnEditPermanetSettings();
	afx_msg void OnAppSettings();
	afx_msg void OnSqlConnect();
	afx_msg void OnSqlReconnect();
	afx_msg void OnSqlCommit();
	afx_msg void OnSqlRollback();
	afx_msg void OnSqlDisconnect();
	afx_msg void OnSqlDbmsOutput();
    afx_msg void OnSqlSubstitutionVariables();
	afx_msg void OnSqlHelp();
	afx_msg void OnSqlSessionStatistics();
    afx_msg void OnSqlRefreshObjectCache();
    afx_msg void OnSqlToggleReadOnly();
	afx_msg void OnSqlExtractSchema();
	afx_msg void OnSqlTableTransformer();
	afx_msg void OnSQLToolsOnTheWeb();
    afx_msg void OnAppSettingsFoder();
	afx_msg void OnUpdate_SqlGroup(CCmdUI* pCmdUI);
    afx_msg void OnUpdate_SqlSubstitutionVariables(CCmdUI* pCmdUI);
	afx_msg void OnUpdate_EditIndicators (CCmdUI* pCmdUI);
	afx_msg void OnUpdate_IndicatorWorkspaceName (CCmdUI* pCmdUI);
    afx_msg void OnDblKeyAccel (UINT nID);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnFileWatchNotify (WPARAM, LPARAM);
    afx_msg void OnFileCloseAll();
    afx_msg void OnFileSaveAll();
    afx_msg void OnUpdate_FileSaveAll(CCmdUI *pCmdUI);
    virtual BOOL OnIdle(LONG lCount);
    afx_msg void OnCommandLineProcess (WPARAM, LPARAM);
    afx_msg void OnFileNewExt ();
    afx_msg void OnSqlHaltOnError();
    afx_msg void OnUpdate_SqlHaltOnError(CCmdUI* pCmdUI);

    void OnServerBackground_ResultQueueNotEmpty ();
    afx_msg void OnSqlCancel();
    afx_msg void OnUpdateSqlCancel(CCmdUI *pCmdUI);
    void DoCancel (bool silent);

    afx_msg void OnWorkspaceCopy       ();
    afx_msg void OnWorkspacePaste      ();
    afx_msg void OnWorkspaceSave       (); 
    afx_msg void OnWorkspaceSaveAs     (); 
    afx_msg void OnWorkspaceSaveQuick  (); 
    afx_msg void OnWorkspaceOpen       ();
    afx_msg void OnWorkspaceOpenQuick  ();
    afx_msg void OnWorkspaceOpenAutosaved ();
    afx_msg void OnWorkspaceClose      ();

    afx_msg void OnUpdate_WorkspacePaste (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_WorkspaceClose (CCmdUI* pCmdUI);

	afx_msg void OnUpdateRecentFileMenu (CCmdUI* pCmdUI);
	afx_msg BOOL OnOpenRecentFile (UINT nID);

	afx_msg void OnUpdateRecentWorkspaceMenu (CCmdUI* pCmdUI);
	afx_msg BOOL OnOpenRecentWorkspace (UINT nID);

	afx_msg void OnUpdateRecentQuickWorkspace (CCmdUI* pCmdUI);
	afx_msg BOOL OnOpenQuickWorkspace (UINT nID);
};

inline
const SQLToolsSettings& GetSQLToolsSettings () { return CSQLToolsApp::getSettings(); }

inline
SQLToolsSettings& GetSQLToolsSettingsForUpdate () { return CSQLToolsApp::getSettings(); }

inline
CWinThread* GetServerBackgroundThread () { return CSQLToolsApp::m_pServerBackgroundThread; }

extern CSQLToolsApp theApp;

struct ActivePrimeExecutionFrame
{
    ActivePrimeExecutionFrame ()
    {
        theApp.SetActivePrimeExecution(true);
    }

    ~ActivePrimeExecutionFrame ()
    {
        theApp.SetActivePrimeExecution(false);
    }
};

#endif//__SQLTOOLS_H__
