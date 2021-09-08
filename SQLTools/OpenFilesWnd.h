#pragma once
#include <afxcmn.h>
#include "Common\DockableContainer.h"


class OpenFilesWnd : public CListCtrl
{
    DECLARE_DYNCREATE(OpenFilesWnd)

    bool m_ctxmenuFileProperties, m_ctxmenuTortoiseGit, m_ctxmenuTortoiseSvn;

public:
    OpenFilesWnd ();

    void SetShellContexMenuFileProperties (bool flag) { m_ctxmenuFileProperties = flag; }
    void SetShellContexMenuTortoiseGit    (bool flag) { m_ctxmenuTortoiseGit = flag; }
    void SetShellContexMenuTortoiseSvn    (bool flag) { m_ctxmenuTortoiseSvn = flag; }

    void OpenFiles_Append (LVITEM&);
    void OpenFiles_UpdateByParam (LPARAM param, LVITEM&);
    void OpenFiles_RemoveByParam (LPARAM param);
    void OpenFiles_ActivateByParam (LPARAM param);
    LPARAM OpenFiles_GetCurSelParam ();
    int OpenFiles_FindByParam (LPARAM param);
    
    void ActivateOpenFile ();
    BOOL GetActiveDocumentPath (NMITEMACTIVATE* pItem, CString& path);

    DECLARE_MESSAGE_MAP()

    afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    LRESULT WindowProc (UINT message, WPARAM wParam, LPARAM lParam);
};

class OpenFilesDockableContainer : public DockableContainer
{
    friend OpenFilesWnd;
    OpenFilesWnd& m_openFilesWnd;
public:
    OpenFilesDockableContainer (OpenFilesWnd& openFilesWnd) 
        : m_openFilesWnd(openFilesWnd) {}

    virtual afx_msg void OnSize (UINT nType, int cx, int cy)
    {
        __super::OnSize(nType, cx, cy);
        if (cx > 0 && m_openFilesWnd.m_hWnd)
        {
            int width = cx - 2 * ::GetSystemMetrics(SM_CXEDGE);
            if (::GetWindowLong(m_openFilesWnd.m_hWnd, GWL_STYLE) & WS_VSCROLL)
                width -= ::GetSystemMetrics(SM_CXVSCROLL);
            m_openFilesWnd.SetColumnWidth(0,  width);
        }
    }
};

