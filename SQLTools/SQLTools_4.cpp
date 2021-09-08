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
#include "SQLTools.h"
#include "MainFrm.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/AppUtilities.h"
#include "COMMON/ExceptionHelper.h"
#include "COMMON/VisualAttributesPage.h"
#include "COMMON/PropertySheetMem.h"
#include "Dlg/PropServerPage.h"
#include "Dlg/PropScriptPage.h"
#include "Dlg/PropGridPage.h"
#include "Dlg/PropGridOutputPage.h"
#include "Dlg/DBSCommonPage.h"
#include "Dlg/DBSTablePage.h"
#include "Dlg/ObjectViewerPage.h"
#include "Dlg/PropHistoryPage.h"
#include "OpenEditor/OEDocument.h"
#include "SQLToolsSettingsXmlStreamer.h"
#include "OpenEditor/OEDocument.h"
#include "SessionCache.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "ServerBackgroundThread\Thread.h"
#include <ActivePrimeExecutionNote.h>

/*
    31.03.2003 bug fix, redundant sqltools settings saving
*/
    
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    static bool g_isLoading = false;

    struct OnSettingsChangedTask : ServerBackgroundThread::Task
    {
        int  m_keepAlivePeriod;
        bool m_outputEnabled;
        int  m_outputSize;
        string m_dateFormat;

        OnSettingsChangedTask () 
            : ServerBackgroundThread::Task("OnSettingsChanged", 0)
        {
            m_keepAlivePeriod = GetSQLToolsSettings().GetKeepAlivePeriod();
            m_outputEnabled   = GetSQLToolsSettings().GetOutputEnable();
            m_outputSize      = GetSQLToolsSettings().GetOutputSize();
            m_dateFormat      = GetSQLToolsSettings().GetDateFormat();
        }

        virtual void DoInBackground (OciConnect& connect)
        {
            ActivePrimeExecutionOnOff onOff;
            ServerBackgroundThread::g_keepAlivePeriod = m_keepAlivePeriod;
            connect.EnableOutput(m_outputEnabled, m_outputSize);
            connect.SetNlsDateFormat(m_dateFormat);
            connect.AlterSessionNlsParams();
        }
    };

                        
void CSQLToolsApp::OnSettingsChanged ()
{
    try { EXCEPTION_FRAME;
        ServerBackgroundThread::FrgdRequestQueue::Get()
            .Push(ServerBackgroundThread::TaskPtr(new OnSettingsChangedTask));

        if (GetConnectOpen()
        && GetSQLToolsSettings().GetObjectCache())
        {
            SessionCache::InitIfNecessary();

            if (!GetConnectSlow())
                SessionCache::Reload(false);
        }

        if (m_pMainWnd)
        {
            UpdateApplicationTitle();
            ((CMDIMainFrame*)m_pMainWnd)->SetIndicators();
            ((CMDIMainFrame*)m_pMainWnd)->SetupMDITabbedGroups(); //FRM2
        }
    }
    _DEFAULT_HANDLER_

    if (!g_isLoading)
        SaveSettings();
}

void CSQLToolsApp::LoadSettings ()
{
    SQLToolsSettingsXmlStreamer(L"sqltools.xml") >> getSettings();

    // 31.03.2003 bug fix, redundant sqltools settings saving
    g_isLoading = true;
    try 
    { 
        OnSettingsChanged(); 
    }
    catch (...) 
    { 
        g_isLoading = false; throw; 
    }
    g_isLoading = false;
}

void CSQLToolsApp::SaveSettings ()
{
    static bool s_isBackupDone = false;
    SQLToolsSettingsXmlStreamer(L"sqltools.xml", !s_isBackupDone) << getSettings();
    s_isBackupDone = true;
}


struct SettingsDialogCallback : COEDocument::SettingsDialogCallback
{
    SQLToolsSettings    m_settings;
    CPropServerPage     m_serverPage;
    CPropScriptPage     m_scriptPage;
    CDBSCommonPage      m_commonPage;
    CDBSTablePage       m_tablePage;
    CObjectViewerPage   m_objViewerPage;
    CPropGridPage1      m_gridPage1;
    CPropGridPage2      m_gridPage2;
    CPropGridOutputPage1 m_gridOutputPage1;
    CPropGridOutputPage2 m_gridOutputPage2;
    CPropHistoryPage    m_histPage;
    CVisualAttributesPage m_vaPage;
    bool                m_ok;

    SettingsDialogCallback (const SQLToolsSettings& settings)
        : m_settings(settings),
        m_serverPage(m_settings),
        m_scriptPage(m_settings),
        m_commonPage(m_settings),
        m_tablePage(m_settings),
        m_objViewerPage(m_settings),
        m_gridPage1(m_settings),
        m_gridPage2(m_settings),
        m_gridOutputPage1(m_settings),
        m_gridOutputPage2(m_settings),
        m_histPage(m_settings),
        m_ok(false)
    {
        m_vaPage.Init(m_settings.GetVASets());
        m_vaPage.m_psp.pszTitle = L"SQLTools::Font and Color";
        m_vaPage.m_psp.dwFlags |= PSP_USETITLE;
    }

    virtual void BeforePageAdding (Common::CPropertySheetMem& sheet)
    {
        sheet.SetTreeWidth(240);
        sheet.AddPage(&m_serverPage);
        sheet.AddPage(&m_scriptPage);
        sheet.AddPage(&m_commonPage);
        sheet.AddPage(&m_tablePage);
        sheet.AddPage(&m_objViewerPage);
        sheet.AddPage(&m_gridPage1);
        sheet.AddPage(&m_gridPage2);
        sheet.AddPage(&m_gridOutputPage1);
        sheet.AddPage(&m_gridOutputPage2);
        sheet.AddPage(&m_histPage);
        sheet.AddPage(&m_vaPage);
    }

    virtual void AfterPageAdding  (Common::CPropertySheetMem& /*sheet*/)
    {
    }

    virtual void OnOk ()
    {
        m_ok = true;
    }

    virtual void OnCancel () 
    {
    }
};

void CSQLToolsApp::OnAppSettings ()
{
    SettingsDialogCallback callback(GetSQLToolsSettings());
    COEDocument::ShowSettingsDialog(&callback);
    
    if (callback.m_ok)
    {
        GetSQLToolsSettingsForUpdate() = callback.m_settings;
        GetSQLToolsSettingsForUpdate().NotifySettingsChanged();

        setlocale(LC_ALL, COEDocument::GetSettingsManager().GetGlobalSettings()->GetLocale().c_str());
        UpdateAccelAndMenu();

        SetupFileManager((CMDIMainFrame*)m_pMainWnd);
    }
}


bool ShowDDLPreferences (SQLToolsSettings& settings, bool bLocal)
{
    BOOL showOnShiftOnly = AfxGetApp()->GetProfileInt(L"showOnShiftOnly", L"DDLPreferences",  FALSE);

    static UINT gStartPage = 0;
    Common::CPropertySheetMem sheet(bLocal ? L"Local DDL Preferences" : L"Global DDL Preferences", gStartPage);
    sheet.SetTreeViewMode(/*bTreeViewMode =*/TRUE, /*bPageCaption =*/FALSE, /*bTreeImages =*/FALSE);
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;
	sheet.SetTreeWidth(180);

    if (!bLocal || !showOnShiftOnly || HIBYTE(GetKeyState(VK_SHIFT))) 
    {
        CDBSCommonPage  commonPage(settings);
        //commonPage.m_psp.pszTitle = "Common DDL";
        //commonPage.m_psp.dwFlags |= PSP_USETITLE;

        CDBSTablePage   tablePage(settings);
        //tablePage.m_psp.pszTitle = "Table Specific DDL";
        //tablePage.m_psp.dwFlags |= PSP_USETITLE;

        commonPage.m_bShowShowOnShiftOnly = bLocal ? TRUE : FALSE;
        commonPage.m_bShowOnShiftOnly = showOnShiftOnly;

        sheet.AddPage(&commonPage);
        sheet.AddPage(&tablePage);
        
        if (sheet.DoModal() == IDOK) 
        {
            if (!bLocal)
            {
                GetSQLToolsSettingsForUpdate() = settings;
                GetSQLToolsSettingsForUpdate().NotifySettingsChanged();
            }
            else
                AfxGetApp()->WriteProfileInt(L"showOnShiftOnly", L"DDLPreferences",  commonPage.m_bShowOnShiftOnly);

            return true;
        } 
        else
            return false;
    } 
    else
        return true;
}

    using namespace OpenEditor;

void CSQLToolsApp::SetupFileManager (CMDIMainFrame* pMainWnd)
{
    CStringArray names, filters;
    names.Add("All files (*.*)");
    filters.Add("*.*");

    const SettingsManager& settingMgrl = COEDocument::GetSettingsManager();

    for (int i(0), count(settingMgrl.GetClassCount()); i < count; i++)
    {
        const ClassSettingsPtr classSettings = settingMgrl.GetClassByPos(i);
        CString extensions = classSettings->GetExtensions().c_str();
        CString name = classSettings->GetName().c_str();
        CString filter;

        int curPos = 0;
        CString token = extensions.Tokenize(L" ",curPos);
        while (!token.IsEmpty())
        {
            if (!filter.IsEmpty()) filter += ';';
            filter += "*.";
            filter += token;
            token = extensions.Tokenize(L" ",curPos);
        }
        name += " ("; name += filter; name += ")";
        names.Add(name);
        filters.Add(filter);
    }

    int currentFilter = pMainWnd->GetFilePanelWnd().GetSelectedFilter();
    FileExplorerWnd& fileManager = pMainWnd->GetFilePanelWnd();
    fileManager.SetFileCategoryFilter(names, filters, currentFilter);

    GlobalSettingsPtr globalSerringsPtr = COEDocument::GetSettingsManager().GetGlobalSettings();
    fileManager.SetTooltips              (globalSerringsPtr->GetFileManagerTooltips             ());
    fileManager.SetPreviewInTooltips     (globalSerringsPtr->GetFileManagerPreviewInTooltips    ());
    fileManager.SetPreviewLines          (globalSerringsPtr->GetFileManagerPreviewLines         ());

    fileManager.SetShellContexMenuFileProperties (globalSerringsPtr->GetFileManagerShellContexMenuProperties());
    fileManager.SetShellContexMenuTortoiseGit    (globalSerringsPtr->GetFileManagerShellContexMenuTortoiseGIT());
    fileManager.SetShellContexMenuTortoiseSvn    (globalSerringsPtr->GetFileManagerShellContexMenuTortoiseSVN());

    pMainWnd->SetShellContexMenuFileProperties (globalSerringsPtr->GetFileManagerShellContexMenuProperties());
    pMainWnd->SetShellContexMenuTortoiseGit    (globalSerringsPtr->GetFileManagerShellContexMenuTortoiseGIT());
    pMainWnd->SetShellContexMenuTortoiseSvn    (globalSerringsPtr->GetFileManagerShellContexMenuTortoiseSVN());

    pMainWnd->GetOpenFilesWnd().SetShellContexMenuFileProperties (globalSerringsPtr->GetFileManagerShellContexMenuProperties());
    pMainWnd->GetOpenFilesWnd().SetShellContexMenuTortoiseGit    (globalSerringsPtr->GetFileManagerShellContexMenuTortoiseGIT());
    pMainWnd->GetOpenFilesWnd().SetShellContexMenuTortoiseSvn    (globalSerringsPtr->GetFileManagerShellContexMenuTortoiseSVN());
}

void CSQLToolsApp::SetupRecentFileList (CMDIMainFrame* pMainWnd)
{
    if (m_pRecentFileList.get())
        pMainWnd->GetRecentFilesListCtrl().Link(m_pRecentFileList); // FRM
}

