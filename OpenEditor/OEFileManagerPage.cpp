/* 
    Copyright (C) 2011 Aleksey Kochetov

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
#include <COMMON/ExceptionHelper.h>
#include <COMMON/DlgDataExt.h>
#include "OpenEditor/OEFileManagerPage.h"


COEFileManagerPage::COEFileManagerPage (SettingsManager& manager)
	: CPropertyPage(COEFileManagerPage::IDD)
    , m_manager(manager)
{
    const OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();

    m_FileManagerTooltips              = settings->GetFileManagerTooltips              () ? TRUE : FALSE;
    m_FileManagerPreviewInTooltips     = settings->GetFileManagerPreviewInTooltips     () ? TRUE : FALSE;
    m_FileManagerPreviewLines          = settings->GetFileManagerPreviewLines          ();
    m_FileManagerShellContexMenu       = settings->GetFileManagerShellContexMenu       () ? TRUE : FALSE;
    m_FileManagerShellContexMenuFilter = settings->GetFileManagerShellContexMenuFilter ();

    m_HistoryRestoreEditorState        = settings->GetHistoryRestoreEditorState      ();
    m_HistoryFiles                     = settings->GetHistoryFiles                   ();
    m_HistoryWorkspaces                = settings->GetHistoryWorkspaces              ();
    m_HistoryFilesInRecentMenu         = settings->GetHistoryFilesInRecentMenu       ();
    m_HistoryWorkspacesInRecentMenu    = settings->GetHistoryWorkspacesInRecentMenu  ();
    m_HistoryQSWorkspacesInRecentMenu  = settings->GetHistoryQSWorkspacesInRecentMenu();

    m_HistoryOnlyFileNameInRecentMenu      = settings->GetHistoryOnlyFileNameInRecentMenu     ();
    m_HistoryOnlyWorkspaceNameInRecentMenu = settings->GetHistoryOnlyWorkspaceNameInRecentMenu();
}

COEFileManagerPage::~COEFileManagerPage()
{
}

void COEFileManagerPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_OE_FLMGR_TOOLTIPS,             m_FileManagerTooltips);
    DDX_Check(pDX, IDC_OE_FLMGR_PREVIEWINTOOLTIPS,    m_FileManagerPreviewInTooltips);
    DDX_Check(pDX, IDC_OE_FLMGR_SHELLCONTEXMENU,      m_FileManagerShellContexMenu);

    DDX_Text(pDX, IDC_OE_FLMGR_PREVIEWLINES, m_FileManagerPreviewLines);
    DDV_MinMaxUInt(pDX, m_FileManagerPreviewLines, 1, 20);
    SendDlgItemMessage(IDC_OE_FLMGR_SPIN1, UDM_SETRANGE32, 1, 20);

    DDX_Text(pDX, IDC_OE_FLMGR_SHELLCONTEXMENUFILTER, m_FileManagerShellContexMenuFilter);

    DDX_Check(pDX, IDC_OE_FLMGR_RESTORE_EDITOR_STATE           , m_HistoryRestoreEditorState     ); 

    DDX_Text(pDX,  IDC_OE_FLMGR_FILES_IN_HISTORY , m_HistoryFiles);
    DDV_MinMaxUInt(pDX, m_HistoryFiles, 100, 5000);
    SendDlgItemMessage(IDC_OE_FLMGR_FILES_IN_HISTORY_SPIN, UDM_SETRANGE32, 100, 5000);

    DDX_Text(pDX,  IDC_OE_FLMGR_WORKSPACES_IN_HISTORY, m_HistoryWorkspaces);
    DDV_MinMaxUInt(pDX, m_HistoryWorkspaces, 20, 1000);
    SendDlgItemMessage(IDC_OE_FLMGR_WORKSPACES_IN_HISTORY_SPIN, UDM_SETRANGE32, 20, 1000);

    DDX_Text(pDX,  IDC_OE_FLMGR_RECENT_FILES_IN_MENU, m_HistoryFilesInRecentMenu);
    DDV_MinMaxUInt(pDX, m_HistoryFilesInRecentMenu, 0, 16);
    SendDlgItemMessage(IDC_OE_FLMGR_RECENT_FILES_IN_MENU_SPIN, UDM_SETRANGE32, 0, 16);

    DDX_Text(pDX,  IDC_OE_FLMGR_RECENT_WORKSPACES_IN_MENU, m_HistoryWorkspacesInRecentMenu);
    DDV_MinMaxUInt(pDX, m_HistoryWorkspacesInRecentMenu, 0, 16);
    SendDlgItemMessage(IDC_OE_FLMGR_RECENT_WORKSPACES_IN_MENU_SPIN, UDM_SETRANGE32, 0, 16);

    DDX_Text(pDX,  IDC_OE_FLMGR_RECENT_QUICK_WORKSPACES_IN_MENU, m_HistoryQSWorkspacesInRecentMenu);
    DDV_MinMaxUInt(pDX, m_HistoryQSWorkspacesInRecentMenu, 0, 16);
    SendDlgItemMessage(IDC_OE_FLMGR_RECENT_QUICK_WORKSPACES_IN_MENU_SPIN, UDM_SETRANGE32, 0, 16);

    GetDlgItem(IDC_OE_FLMGR_PREVIEWINTOOLTIPS)->    EnableWindow(m_FileManagerTooltips);
    GetDlgItem(IDC_OE_FLMGR_PREVIEWLINES)->         EnableWindow(m_FileManagerTooltips && m_FileManagerPreviewInTooltips);
    GetDlgItem(IDC_OE_FLMGR_SHELLCONTEXMENUFILTER)->EnableWindow(m_FileManagerShellContexMenu);

    DDX_Check(pDX, IDC_OE_FLMGR_RECENT_FILE_ONLY_NAME_IN_MENU, m_HistoryOnlyFileNameInRecentMenu);
    DDX_Check(pDX, IDC_OE_FLMGR_RECENT_WORKSPACE_ONLY_NAME_IN_MENU, m_HistoryOnlyWorkspaceNameInRecentMenu);
}


BEGIN_MESSAGE_MAP(COEFileManagerPage, CPropertyPage)
    ON_BN_CLICKED(IDC_OE_FLMGR_TOOLTIPS             , OnUpdateData)
    ON_BN_CLICKED(IDC_OE_FLMGR_PREVIEWINTOOLTIPS    , OnUpdateData)
    ON_BN_CLICKED(IDC_OE_FLMGR_PREVIEWLINES         , OnUpdateData)
    ON_BN_CLICKED(IDC_OE_FLMGR_SHELLCONTEXMENU      , OnUpdateData)
    ON_BN_CLICKED(IDC_OE_FLMGR_SHELLCONTEXMENUFILTER, OnUpdateData)
END_MESSAGE_MAP()


// COEFileManagerPage message handlers

BOOL COEFileManagerPage::OnApply()
{
    try { EXCEPTION_FRAME;

        OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();
        
        settings->SetFileManagerTooltips              (m_FileManagerTooltips             ? true : false, false);
        settings->SetFileManagerPreviewInTooltips     (m_FileManagerPreviewInTooltips    ? true : false, false);
        settings->SetFileManagerPreviewLines          (m_FileManagerPreviewLines                       , false);
        settings->SetFileManagerShellContexMenu       (m_FileManagerShellContexMenu      ? true : false, false);
        settings->SetFileManagerShellContexMenuFilter (m_FileManagerShellContexMenuFilter              , false);

        settings->SetHistoryRestoreEditorState       (m_HistoryRestoreEditorState      , false);
        settings->SetHistoryFiles                    (m_HistoryFiles                   , false);
        settings->SetHistoryWorkspaces               (m_HistoryWorkspaces              , false);
        settings->SetHistoryFilesInRecentMenu        (m_HistoryFilesInRecentMenu       , false);
        settings->SetHistoryWorkspacesInRecentMenu   (m_HistoryWorkspacesInRecentMenu  , false);
        settings->SetHistoryQSWorkspacesInRecentMenu (m_HistoryQSWorkspacesInRecentMenu, false);

        settings->SetHistoryOnlyFileNameInRecentMenu     (m_HistoryOnlyFileNameInRecentMenu     , false);
        settings->SetHistoryOnlyWorkspaceNameInRecentMenu(m_HistoryOnlyWorkspaceNameInRecentMenu, false);
    }
    _OE_DEFAULT_HANDLER_;

	return TRUE;
}

void COEFileManagerPage::OnUpdateData()
{
    UpdateData();
}