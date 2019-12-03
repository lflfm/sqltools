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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OEFileManagerPage_h__
#define __OEFileManagerPage_h__

#include "resource.h"
#include "OpenEditor/OESettings.h"

    using OpenEditor::SettingsManager;

class COEFileManagerPage : public CPropertyPage
{
public:
	COEFileManagerPage (SettingsManager& manager);
	virtual ~COEFileManagerPage();

// Dialog Data
	enum { IDD = IDD_OE_FILE_MANAGER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnApply();

    SettingsManager& m_manager;

    BOOL m_FileManagerTooltips;
    BOOL m_FileManagerPreviewInTooltips;
    UINT m_FileManagerPreviewLines;
    BOOL m_FileManagerShellContexMenu;
    std::string m_FileManagerShellContexMenuFilter;

    bool m_HistoryRestoreEditorState;
    int  m_HistoryFiles;
    int  m_HistoryWorkspaces;
    int  m_HistoryFilesInRecentMenu;
    int  m_HistoryWorkspacesInRecentMenu;
    int  m_HistoryQSWorkspacesInRecentMenu;
    bool m_HistoryOnlyFileNameInRecentMenu;
    bool m_HistoryOnlyWorkspaceNameInRecentMenu;

    afx_msg void OnUpdateData();
};

#endif//__OEFileManagerPage_h__
