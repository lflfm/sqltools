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

#pragma once
#ifndef __WorkbookMDIFrame_H__
#define __WorkbookMDIFrame_H__

#include "FileExplorerWnd.h"
#include "RecentFileWnd.h"


class CWorkbookMDIFrame : public CMDIFrameWndEx
{
    DECLARE_DYNCREATE(CWorkbookMDIFrame)
protected:
    CWorkbookMDIFrame();
    virtual ~CWorkbookMDIFrame();

protected:
    int IDC_MF_FILEPANEL;
    int IDC_MF_HISTORY;
    LPCWSTR m_cszProfileName;
    LPCWSTR m_cszMainFrame;
    LPCWSTR m_cszWP;
    BOOL m_MDITabsCtrlTabSwitchesToPrevActive;

    HWND m_hActiveChild, m_hLastChild, m_hSkipChild;
    BOOL m_bMDINextSeq;

    CString             m_strProfileName;
    FileExplorerWnd     m_wndFilePanel;
    RecentFileWnd       m_wndRecentFile;

    CFont               m_font; // default gui font for bar controls
    CString             m_defaultFileExtension;
    bool m_ctxmenuFileProperties, m_ctxmenuTortoiseGit, m_ctxmenuTortoiseSvn;

    int DoCreate ();

    static bool has_focus (HWND hWndControl);

    friend class CMDIChildFrame;
    void OnActivateChild (CMDIChildWnd*);
    int  GetImageByDocument (const CDocument*);

    friend class OpenFilesWnd;
    void ActivateChild (CMDIChildWnd*);

    afx_msg void OnSysCommand (UINT nID, LONG lParam);

public:

    FileExplorerWnd& GetFilePanelWnd () { return m_wndFilePanel; }
    const FileExplorerWnd& GetFilePanelWnd () const { return m_wndFilePanel; }

    RecentFileWnd& GetRecentFileWnd () { return m_wndRecentFile; }
    RecentFilesListCtrl&  GetRecentFilesListCtrl () { return m_wndRecentFile.GetRecentFilesListCtrl(); }

    void SetDefaultFileExtension (const char* ext)  { m_defaultFileExtension = ext; }
    void GetOrderedChildren (std::vector<CMDIChildWnd*>&);

    void SetShellContexMenuFileProperties (bool flag) { m_ctxmenuFileProperties = flag; }
    void SetShellContexMenuTortoiseGit    (bool flag) { m_ctxmenuTortoiseGit = flag; }
    void SetShellContexMenuTortoiseSvn    (bool flag) { m_ctxmenuTortoiseSvn = flag; }

    void EnsureActiveTabVisible ();

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg void OnViewFilePanel();
    afx_msg void OnViewHistory();
    afx_msg void OnUpdateViewFilePanel(CCmdUI *pCmdUI);
    afx_msg void OnUpdateViewHistory(CCmdUI *pCmdUI);
    afx_msg void OnViewFilePanelSync();
    afx_msg LRESULT OnGetTabToolTip (WPARAM wp, LPARAM lp);
    afx_msg BOOL OnToolTipText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    afx_msg void OnLastWindow ();
    afx_msg void OnPrevWindow ();
    afx_msg void OnNextWindow ();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bTabDrop);
};


#endif//__WorkbookMDIFrame_H__
