/* 
    Copyright (C) 2002 Aleksey Kochetov

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

// 17.01.2005 (ak) simplified settings class hierarchy
// 2016.06.28 improvement, added new setiing for Workspace saving and Backup space management

#include "stdafx.h"
#include <COMMON/ExceptionHelper.h>
#include <Common/DirSelectDlg.h>
#include <Common/DlgDataExt.h>
#include "OpenEditor/OEBackupPage.h"

static LPCSTR cszDefaultFolder = "<Default location>";


COEBackupPage::COEBackupPage (SettingsManager& manager)
	: CPropertyPage(COEBackupPage::IDD)
    , m_manager(manager)
    , m_WorkspaceAutosaveInterval(0)
    , m_WorkspaceAutosaveFileLimit(0)
    , m_WorkspaceAutosaveDeleteOnExit(false)
    , m_FileBackupKeepDays(0)
    , m_FileBackupFolderMaxSize(0)
    , m_FileBackupSpaceManagment(false)
{
    const OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();

    m_BackupName         = settings->GetFileBackupName().c_str();
    m_BackupDirectory    = settings->GetFileBackupDirectoryV2().c_str();

    if (m_BackupDirectory.IsEmpty())
        m_BackupDirectory = cszDefaultFolder;

    switch (settings->GetFileBackupV2())
    {
    case OpenEditor::ebmNone:
        m_BackupMethod = 0;
        break;
    case OpenEditor::ebmCurrentDirectory: 
        m_BackupMethod = 1;
        break;
    case OpenEditor::ebmBackupDirectory:
        m_BackupMethod = 2;
        break;
    }

    m_WorkspaceAutosaveInterval     = settings->GetWorkspaceAutosaveInterval ();
    m_WorkspaceAutosaveFileLimit    = settings->GetWorkspaceAutosaveFileLimit();
    m_WorkspaceAutosaveDeleteOnExit = settings->GetWorkspaceAutosaveDeleteOnExit();

    m_FileBackupKeepDays         = settings->GetFileBackupKeepDays        ();
    m_FileBackupFolderMaxSize    = settings->GetFileBackupFolderMaxSize   ();
    m_FileBackupSpaceManagment   = settings->GetFileBackupSpaceManagment   ();
}

COEBackupPage::~COEBackupPage()
{
}

void COEBackupPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_OE_BACKUP_TO_NOTHING, m_BackupMethod);
    DDX_Text(pDX, IDC_OE_BACKUP_NAME, m_BackupName);
    DDX_Text(pDX, IDC_OE_BACKUP_DIR, m_BackupDirectory);
    GetDlgItem(IDC_OE_BACKUP_NAME)->EnableWindow(m_BackupMethod == 1);

    DDX_Text(pDX, IDC_OE_AUTOSAVE_INTERVAL, m_WorkspaceAutosaveInterval);
    SendDlgItemMessage(IDC_OE_AUTOSAVE_INTERVAL_SPIN, UDM_SETRANGE32, 0, 60*60);
    DDV_MinMaxUInt(pDX, m_WorkspaceAutosaveInterval, 0, 60*60);

    DDX_Text(pDX, IDC_OE_AUTOSAVE_FILE_LIMIT, m_WorkspaceAutosaveFileLimit);
    SendDlgItemMessage(IDC_OE_AUTOSAVE_FILE_LIMIT_SPIN, UDM_SETRANGE32, 1, 1024 * 1024);
    DDV_MinMaxUInt(pDX, m_WorkspaceAutosaveFileLimit, 1, 1024 * 1024);

    DDX_Check(pDX, IDC_OE_AUTOSAVE_DELETE_SMAPSHOT_ON_EXIT, m_WorkspaceAutosaveDeleteOnExit);

    DDX_Check(pDX, IDC_OE_AUTOSAVE_FOLDER_MANAGMENT, m_FileBackupSpaceManagment);
    GetDlgItem(IDC_OE_AUTOSAVE_KEEP_DAYS)->EnableWindow(m_FileBackupSpaceManagment);
    GetDlgItem(IDC_OE_AUTOSAVE_FOLDER_MAX_SIZE)->EnableWindow(m_FileBackupSpaceManagment);

    DDX_Text(pDX, IDC_OE_AUTOSAVE_KEEP_DAYS, m_FileBackupKeepDays);
    SendDlgItemMessage(IDC_OE_AUTOSAVE_KEEP_DAYS_SPIN, UDM_SETRANGE32, 0, 365 * 10);
    DDV_MinMaxUInt(pDX, m_FileBackupKeepDays, 0, 365 * 10);

    DDX_Text(pDX, IDC_OE_AUTOSAVE_FOLDER_MAX_SIZE, m_FileBackupFolderMaxSize);
    SendDlgItemMessage(IDC_OE_AUTOSAVE_FOLDER_MAX_SIZE_SPIN, UDM_SETRANGE32, 0, 1024 * 1024);
    DDV_MinMaxUInt(pDX, m_FileBackupFolderMaxSize, 0, 1024 * 1024);
}


BEGIN_MESSAGE_MAP(COEBackupPage, CPropertyPage)
    ON_BN_CLICKED(IDC_OE_BACKUP_TO_NOTHING, OnUpdateData)
    ON_BN_CLICKED(IDC_OE_BACKUP_TO_FILE, OnUpdateData)
    ON_BN_CLICKED(IDC_OE_BACKUP_TO_DIR,  OnUpdateData)
    ON_BN_CLICKED(IDC_OE_AUTOSAVE_FOLDER_MANAGMENT, OnUpdateData)
    ON_BN_CLICKED(IDC_OE_BACKUP_DIR_CHANGE, OnBnClicked_FileBackupDirChange)
    ON_BN_CLICKED(IDC_OE_BACKUP_DIR_CLEAR, &COEBackupPage::OnBnClicked_FileBackupDirClear)
END_MESSAGE_MAP()


// COEBackupPage message handlers

BOOL COEBackupPage::OnApply()
{
    try { EXCEPTION_FRAME;

        OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();
        
        settings->SetFileBackupName((LPCSTR)m_BackupName, false /*notify*/);

        CString backupFolder = m_BackupDirectory == cszDefaultFolder ? "" : m_BackupDirectory;

        if (!backupFolder.IsEmpty() && !PathFileExists(backupFolder))
        {
            MessageBeep((UINT)-1);
            if (AfxMessageBox("The backup folder \"" + backupFolder + "\" does not exist!\n\nWould you like to reset it to defult?", MB_ICONEXCLAMATION|MB_YESNO) == IDYES)
            {
                m_BackupDirectory = cszDefaultFolder;
                backupFolder.Empty();
            }
            else
                return FALSE;
        }

        settings->SetFileBackupDirectoryV2((LPCSTR)backupFolder, false /*notify*/);
        
        switch (m_BackupMethod)
        {
        case 0: settings->SetFileBackupV2(OpenEditor::ebmNone); break;
        case 1: settings->SetFileBackupV2(OpenEditor::ebmCurrentDirectory); break;
        case 2: settings->SetFileBackupV2(OpenEditor::ebmBackupDirectory); break;
        }

        settings->SetWorkspaceAutosaveInterval    (m_WorkspaceAutosaveInterval );
        settings->SetWorkspaceAutosaveFileLimit   (m_WorkspaceAutosaveFileLimit);
        settings->SetWorkspaceAutosaveDeleteOnExit(m_WorkspaceAutosaveDeleteOnExit);

        settings->SetFileBackupKeepDays        (m_FileBackupKeepDays        );
        settings->SetFileBackupFolderMaxSize   (m_FileBackupFolderMaxSize   );
        settings->SetFileBackupSpaceManagment  (m_FileBackupSpaceManagment  );
    }
    _OE_DEFAULT_HANDLER_;

	return TRUE;
}

void COEBackupPage::OnUpdateData()
{
    UpdateData();
}

void COEBackupPage::OnBnClicked_FileBackupDirChange()
{
    Common::CDirSelectDlg dirDlg("Choose the folder for saving backups:", this, m_BackupDirectory != cszDefaultFolder ? m_BackupDirectory : "");

    if (dirDlg.DoModal() == IDOK) 
	{
        dirDlg.GetPath(m_BackupDirectory);
        UpdateData(FALSE);
    }

}


void COEBackupPage::OnBnClicked_FileBackupDirClear()
{
    m_BackupDirectory = cszDefaultFolder;
    UpdateData(FALSE);
}
