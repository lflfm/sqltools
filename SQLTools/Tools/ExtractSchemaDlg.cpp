/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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
#include <set>
#include <string>
#include "SQLTools.h"
#include <COMMON\AppGlobal.h>
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\Loader.h"
#include "ExtractSchemaDlg.h"
#include "ExtractSchemaHelpers.h"
#include "ExtractDDLSettingsXmlStreamer.h"
#include <COMMON\ErrorDlg.h>
#include "ServerBackgroundThread\TaskQueue.h"

using namespace OraMetaDict;
using namespace ServerBackgroundThread;
using namespace ExtractSchemaHelpers;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CExtractSchemaDlg

CExtractSchemaDlg::CExtractSchemaDlg(CWnd* pParentWnd)
: CPropertySheet("Extract Schema DDL", pParentWnd),
  m_MainPage  (m_DDLSettings),
  m_FilterPage(m_DDLSettings),
  m_OptionPage(m_DDLSettings)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    m_psh.dwFlags &= ~PSH_HASHELP;

    ExtractDDLSettingsXmlStreamer("schemaddl.xml") >> m_DDLSettings;

    m_DDLSettings.m_strDbAlias = theApp.GetConnectGlobalName();

    m_MainPage.m_psp.dwFlags &= ~PSP_HASHELP;
    m_FilterPage.m_psp.dwFlags &= ~PSP_HASHELP;
    m_OptionPage.m_psp.dwFlags &= ~PSP_HASHELP;

    AddPage(&m_MainPage);
    AddPage(&m_FilterPage);
    AddPage(&m_OptionPage);
}


BEGIN_MESSAGE_MAP(CExtractSchemaDlg, CPropertySheet)
    ON_COMMAND(IDOK, OnOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExtractSchemaDlg message handlers

BOOL CExtractSchemaDlg::OnInitDialog() 
{
    BOOL bResult = CPropertySheet::OnInitDialog();
        
    HWND hItem = ::GetDlgItem(m_hWnd, IDOK); 
    _ASSERTE(hItem);
    ::SetWindowText(hItem, "Load");

    hItem = ::GetDlgItem(m_hWnd, IDCANCEL);
    _ASSERTE(hItem);
    ::SetWindowText(hItem, "Close");
        
    return bResult;
}

	struct BackgroundTask_ExtractSchema : Task
    {
        ExtractDDLSettings m_settings;
        HWND m_hDialogWnd, m_hStatusWnd;
        string m_error_log;

        BackgroundTask_ExtractSchema (const ExtractDDLSettings& settings, HWND hDialogWnd, HWND hStatusWnd)
            : Task("Extract Schema", 0),
            m_settings(settings),
            m_hDialogWnd(hDialogWnd),
            m_hStatusWnd(hStatusWnd)
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                string path;
                MakeFolders(path, m_settings);

                Dictionary dict;
                LoadSchema(connect, dict, m_hStatusWnd, m_settings);
                OverrideTablespace(dict, m_settings);

                std::vector<const OraMetaDict::DbObject*> views;
                OptimizeViewCreationOrder(connect, dict, m_hStatusWnd, views, m_error_log, m_settings);
                WriteSchema(dict, views, m_hStatusWnd, path, m_error_log, m_settings);

                NextAction(NULL, m_hStatusWnd, m_error_log.empty() ? "Process completed." : "Process completed with errors.");
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            // otherwise the app loses focus
            //AfxGetMainWnd()->SetForegroundWindow();
            //SetFocus();
            
            //TODO#2: check if the dialog does not exist then notify in different way

            ::EnableWindow(::GetDlgItem(m_hDialogWnd, IDOK), TRUE);

            if (!m_error_log.empty())
                CErrorDlg(m_error_log.c_str(), "ExtractShemaDDL_ErrorLog").DoModal();

        }

        void ReturnErrorInForeground ()
        {
            ::EnableWindow(::GetDlgItem(m_hDialogWnd, IDOK), TRUE);

            if (!m_error_log.empty())
                CErrorDlg(m_error_log.c_str(), "ExtractShemaDDL_ErrorLog").DoModal();

        }
    };


void CExtractSchemaDlg::OnOK()
{
    try { EXCEPTION_FRAME;

        CWaitCursor wait;

        if (m_MainPage.m_hWnd)   m_MainPage.UpdateDataAndSaveHistory();
        if (m_FilterPage.m_hWnd) m_FilterPage.UpdateData();
        if (m_OptionPage.m_hWnd) m_OptionPage.UpdateData();

        ExtractDDLSettingsXmlStreamer("schemaddl.xml") << m_DDLSettings;

        if (GetActiveIndex())
            SetActivePage(0);

        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_ExtractSchema(m_DDLSettings, m_hWnd, ::GetDlgItem(m_MainPage, IDC_ES_STATUS))));

        ::EnableWindow(::GetDlgItem(m_hWnd, IDOK), FALSE);
    } 
    _DEFAULT_HANDLER_
}
