/* 
    Copyright (C) 2016 Aleksey Kochetov

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
#include "OpenEditor/OEWorkspacePage.h"


OEWorkspacePage::OEWorkspacePage (SettingsManager& manager)
	: CPropertyPage(OEWorkspacePage::IDD)
    , m_manager(manager)
{
    const OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();
    m_WorkspaceDetectChangesOnOpen    = settings->GetWorkspaceDetectChangesOnOpen   ();
    m_WorkspaceFileSaveInActiveOnExit = settings->GetWorkspaceFileSaveInActiveOnExit();
    m_WorkspaceUseRelativePath        = settings->GetWorkspaceUseRelativePath();
    m_WorkspaceShowNameInTitleBar     = settings->GetWorkspaceShowNameInTitleBar();
    m_WorkspaceShowNameInStatusBar    = settings->GetWorkspaceShowNameInStatusBar();
}

OEWorkspacePage::~OEWorkspacePage()
{
}

void OEWorkspacePage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_OE_WORKSPACE_DETECT_CHANGES_ON_OPEN, m_WorkspaceDetectChangesOnOpen);
    DDX_Check(pDX, IDC_OE_WORKSPACE_SAVE_IN_ACTIVE_ON_EXIT, m_WorkspaceFileSaveInActiveOnExit);
    DDX_Check(pDX, IDC_OE_WORKSPACE_USE_RELATIVE_PATH,      m_WorkspaceUseRelativePath);
    DDX_Check(pDX, IDC_OE_WORKSPACE_SHOW_NAME_IN_APP_TITLE, m_WorkspaceShowNameInTitleBar );
    DDX_Check(pDX, IDC_OE_WORKSPACE_SHOW_NAME_IN_STATUSBAR, m_WorkspaceShowNameInStatusBar);
}


BEGIN_MESSAGE_MAP(OEWorkspacePage, CPropertyPage)
END_MESSAGE_MAP()


// OEWorkspacePage message handlers

BOOL OEWorkspacePage::OnApply()
{
    try { EXCEPTION_FRAME;

        OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();
        settings->SetWorkspaceDetectChangesOnOpen   (m_WorkspaceDetectChangesOnOpen   , false /*notify*/);
        settings->SetWorkspaceFileSaveInActiveOnExit(m_WorkspaceFileSaveInActiveOnExit, false /*notify*/);
        settings->SetWorkspaceUseRelativePath       (m_WorkspaceUseRelativePath,        false /*notify*/);
        settings->SetWorkspaceShowNameInTitleBar    (m_WorkspaceShowNameInTitleBar,     false /*notify*/);
        settings->SetWorkspaceShowNameInStatusBar   (m_WorkspaceShowNameInStatusBar,    false /*notify*/);
    }
    _OE_DEFAULT_HANDLER_;

	return TRUE;
}
