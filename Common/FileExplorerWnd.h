/*
    Copyright (C) 2002, 2018 Aleksey Kochetov

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
#ifndef __FilePanelWnd_h__
#define __FilePanelWnd_h__


#include "Common/FakeRClick.h"
#include "DirTreeCtrl/DirTreeCtrl.h"
#include "CppTooltip/PPTooltip.h"
#include "Common/RecentFilesListCtrl.h"


class FileExplorerWnd : public CDockablePane
{
    DECLARE_DYNAMIC(FileExplorerWnd)

public:
    FileExplorerWnd ();
    virtual ~FileExplorerWnd ();

    CImageList& GetSysImageList() { return m_explorerTree.GetSysImageList(); }

    const CString& GetCurDrivePath () const;
    void  SetCurDrivePath (const CString&);
    BOOL  SetCurPath (const CString&);

    // format: "C++ files (*.cpp;*.h;*.rc)", "*.cpp;*.h;*.rc"
    void SetFileCategoryFilter (const CStringArray& names, const CStringArray& filters, int select);
    int  GetSelectedFilter () const { return m_selectedFilter; }

    void SetTooltips              (BOOL    tooltips             );
    void SetPreviewInTooltips     (BOOL    previewInTooltips    ) { m_PreviewInTooltips     = previewInTooltips;     }
    void SetPreviewLines          (UINT    previewLines         ) { m_PreviewLines          = previewLines;          }

    void SetShellContexMenuFileProperties (bool flag) { m_ctxmenuFileProperties = flag; }
    void SetShellContexMenuTortoiseGit    (bool flag) { m_ctxmenuTortoiseGit = flag; }
    void SetShellContexMenuTortoiseSvn    (bool flag) { m_ctxmenuTortoiseSvn = flag; }

    void CompleteStartup          ();

protected:
    BOOL         m_startup;
    BOOL         m_isExplorerInitialized;
    BOOL         m_isDrivesInitialized;
    BOOL         m_isLabelEditing;
    CString      m_curDrivePath;

    BOOL         m_Tooltips;
    BOOL         m_PreviewInTooltips;
    UINT         m_PreviewLines;
    bool m_ctxmenuFileProperties, m_ctxmenuTortoiseGit, m_ctxmenuTortoiseSvn;

    CImageList   m_explorerStateImageList;
    CDirTreeCtrl m_explorerTree;
    CComboBoxEx  m_drivesCBox, m_filterCBox;
    CFakeRClick  m_drivesRClick, m_filterRClick;
    CStringArray m_driverPaths;
    CPPToolTip   m_tooltip;
    CStringArray m_filterNames, m_filterData;
    int          m_selectedFilter;
    CString      m_filterCache;

    void SelectDrive (const CString&, BOOL force = FALSE);
    void DisplayDrivers (BOOL force = FALSE, BOOL curOnly = FALSE);

    void NotifyDisplayTooltip (NMHDR * pNMHDR);

protected:
    DECLARE_MESSAGE_MAP()

public:
    afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy ();
    afx_msg void OnPaint();
    afx_msg void OnSize (UINT nType, int cx, int cy);
    afx_msg void OnDrive_SetFocus ();
    afx_msg void OnDrive_SelChange ();
    afx_msg void OnExplorerTree_DblClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_BeginLabelEdit (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_EndLabelEdit (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDrivers_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFpwOpen();
    afx_msg void OnFpwShellOpen();
    afx_msg void OnFpwRename();
    afx_msg void OnFpwDelete();
    afx_msg void OnFpwRefresh();
    afx_msg void OnFpwRefreshDrivers();
    afx_msg void OnNotifyDisplayTooltip(NMHDR * pNMHDR, LRESULT * result);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnFilter_SelChange ();
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};

inline
const CString& FileExplorerWnd::GetCurDrivePath () const
    { return m_curDrivePath; }

inline
void FileExplorerWnd::SetCurDrivePath (const CString& curDrivePath)
    { SelectDrive(curDrivePath); }

#endif//__FilePanelWnd_h__