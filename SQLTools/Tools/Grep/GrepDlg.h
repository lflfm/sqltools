/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2020 Aleksey Kochetov

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

#ifndef __GREPDLG_H__
#define __GREPDLG_H__
#pragma once

#include <COMMON/EditWithSelPos.h>

    class CGrepView;
    class COEditorView;

class CGrepDlg : public CDialog
{
    HICON m_hIcon;
    CGrepView* m_pResultView;
    COEditorView* m_pEditorView;

public:
    CGrepDlg (CWnd* pParent, CGrepView* pResultView, COEditorView* pEditorView);

    enum { IDD = IDD_FILE_FIND };
    BOOL    m_ShareSearchContext;
    BOOL	m_MatchWholeWord;
    BOOL	m_MatchCase;
    BOOL	m_RegExp;
    BOOL	m_BackslashExpressions;
    BOOL	m_LookInSubfolders;
    BOOL	m_SearchInMemory;
    BOOL	m_CollapsedList;
    CString	m_MaskList;
    CString	m_FolderList;
    CString	m_SearchText;

    std::wstring m_currentFolder;
    std::wstring m_workspaceFolder;
    std::wstring m_backupFolder;
    std::wstring m_finalFolderList;

    CEditWithSelPos m_edtSearchText;

    CButton	  m_wndInsertExp;
    CComboBox m_wndFolderList;
    CComboBox m_wndMaskList;
    CComboBox m_wndSearchText;

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnOK ();
    virtual void OnCancel ();

protected:
    virtual BOOL OnInitDialog();
    void SavePlacement ();
    afx_msg void OnSelectFolder();
    void ShowPopupMenu (UINT btnId, CMenu* pPopup);

    DECLARE_MESSAGE_MAP()
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    afx_msg void OnClose();
    afx_msg void OnBnClicked_RegExp ();
    afx_msg void OnBnClicked_InsertExpr();
    afx_msg void OnInsertRegexpFind (UINT nID);
    afx_msg void OnChangeFolder (UINT nID);
    afx_msg void OnHelp ();
};

#endif//__GREPDLG_H__
