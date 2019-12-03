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
#ifndef __FilePanelWnd_h__
#define __FilePanelWnd_h__


#include "Common/FakeRClick.h"
#include "DirTreeCtrl/DirTreeCtrl.h"
#include "CppTooltip/PPTooltip.h"
#include "Common/RecentFilesListCtrl.h"


class CFilePanelWnd : public CTabCtrl
{
	DECLARE_DYNAMIC(CFilePanelWnd)

public:
	CFilePanelWnd ();
	virtual ~CFilePanelWnd ();

    void OnShowControl (BOOL);

    CListCtrl&  GetOpenFilesListCtrl () { return m_openFilesList; }
    CImageList& GetSysImageList() { return m_explorerTree.GetSysImageList(); }
    RecentFilesListCtrl&  GetRecentFilesListCtrl () { return m_recentFilesListCtrl; }

    void DoOpenFile (const string&, unsigned int);

    void OpenFiles_Append (LVITEM&);
    void OpenFiles_UpdateByParam (LPARAM param, LVITEM&);
    void OpenFiles_RemoveByParam (LPARAM param);
    void OpenFiles_ActivateByParam (LPARAM param);
    LPARAM OpenFiles_GetCurSelParam ();

    const CString& GetCurDrivePath () const;
    void  SetCurDrivePath (const CString&);
    BOOL  SetCurPath (const CString&);

    // format: "C++ files (*.cpp;*.h;*.rc)", "*.cpp;*.h;*.rc"
    void SetFileCategoryFilter (const CStringArray& names, const CStringArray& filters, int select);
    int  GetSelectedFilter () const { return m_selectedFilter; }

    void SetTooltips              (BOOL    tooltips             );
    void SetPreviewInTooltips     (BOOL    previewInTooltips    ) { m_PreviewInTooltips     = previewInTooltips;     }
    void SetPreviewLines          (UINT    previewLines         ) { m_PreviewLines          = previewLines;          }
    void SetShellContexMenu       (BOOL    shellContexMenu      ) { m_ShellContexMenu       = shellContexMenu;       }
    void SetShellContexMenuFilter (CString shellContexMenuFilter) { m_ShellContexMenuFilter = shellContexMenuFilter; }

protected:
    int OpenFiles_FindByParam (LPARAM param);

protected:
    BOOL         m_isExplorerInitialized;
    BOOL         m_isDrivesInitialized;
    BOOL         m_isLabelEditing;
    CString      m_curDrivePath;

    BOOL         m_Tooltips;
    BOOL         m_PreviewInTooltips;
    UINT         m_PreviewLines;
    BOOL         m_ShellContexMenu;
    CString      m_ShellContexMenuFilter;

    CImageList   m_explorerStateImageList;
    CListCtrl    m_openFilesList;
    CDirTreeCtrl m_explorerTree;
    CComboBoxEx  m_drivesCBox, m_filterCBox;
    CFakeRClick  m_drivesRClick, m_filterRClick;
    CStringArray m_driverPaths;
	CPPToolTip   m_tooltip;
    CStringArray m_filterNames, m_filterData;
    int          m_selectedFilter;
    CString      m_filterCache;
    RecentFilesListCtrl m_recentFilesListCtrl;

    void SelectDrive (const CString&, BOOL force = FALSE);
    void DisplayDrivers (BOOL force = FALSE, BOOL curOnly = FALSE);
    void ChangeTab (int, BOOL force = FALSE);
    void ActivateOpenFile ();

    BOOL GetActiveDocumentPath (NMITEMACTIVATE*, CString&);

    void OnExplorerTree_NotifyDisplayTooltip (NMHDR * pNMHDR);
    void OnRecentFilesListCtrl_NotifyDisplayTooltip (NMHDR * pNMHDR);
    void DisplayFileInfoTooltip (NMHDR * pNMHDR, LPCSTR path);

protected:
	DECLARE_MESSAGE_MAP()

public:
    afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy ();
    afx_msg void OnSize (UINT nType, int cx, int cy);
    afx_msg void OnTab_SelChange (NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDrive_SetFocus ();
    afx_msg void OnDrive_SelChange ();
    afx_msg void OnOpenFiles_Click (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnOpenFiles_KeyDown (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnOpenFiles_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnOpenFiles_OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_DblClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_BeginLabelEdit (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_EndLabelEdit (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDrivers_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTimer(UINT nIDEvent);
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

    afx_msg void OnRecentFilesListCtrl_DblClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRecentFilesListCtrl_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRecentFilesListCtrl_OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult);
};

inline
const CString& CFilePanelWnd::GetCurDrivePath () const
    { return m_curDrivePath; }

inline
void CFilePanelWnd::SetCurDrivePath (const CString& curDrivePath)
    { SelectDrive(curDrivePath); }

#endif//__FilePanelWnd_h__