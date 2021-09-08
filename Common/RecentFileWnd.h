
#pragma once

#include "Common/RecentFilesListCtrl.h"
#include "CppTooltip/PPTooltip.h"

class FileExplorerWnd;

class RecentFileWnd : public CDockablePane
{
// Construction
public:
    RecentFileWnd();
    virtual ~RecentFileWnd();

// Attributes
protected:
    RecentFilesListCtrl m_recentFilesListCtrl;
    CPPToolTip m_tooltip;
    BOOL m_Tooltips;
    BOOL m_PreviewInTooltips;
    UINT m_PreviewLines;
    FileExplorerWnd* m_pFileExplorerWnd;

// Implementation
public:
    RecentFilesListCtrl&  GetRecentFilesListCtrl () { return m_recentFilesListCtrl; }

    void DoOpenFile (LPCWSTR path, unsigned int item);
    void SetTooltips (BOOL tooltips);
    void NotifyDisplayTooltip (NMHDR * pNMHDR);
    void SetSysImageList (CImageList* pImageList);
    void SetFileExplorerWnd (FileExplorerWnd* pFileExplorerWnd) { m_pFileExplorerWnd = pFileExplorerWnd; }

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnNotifyDisplayTooltip(NMHDR * pNMHDR, LRESULT * result);
    afx_msg void OnRecentFilesListCtrl_DblClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRecentFilesListCtrl_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRecentFilesListCtrl_OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    afx_msg void OnOpenDocument();
};

