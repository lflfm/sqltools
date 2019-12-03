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

#include "FilePanelWnd.h"
#include "WorkbookBar.h"


    class CWorkbookControlBar : public baseCMyBar
    {
        afx_msg void OnContextMenu (CWnd*, CPoint);
        afx_msg void OnCbar_Docking();
        afx_msg void OnCbar_Hide();
	    DECLARE_MESSAGE_MAP()
    };


class CWorkbookMDIFrame : public CMDIFrameWnd
{
	DECLARE_DYNCREATE(CWorkbookMDIFrame)
protected:
	CWorkbookMDIFrame();
	virtual ~CWorkbookMDIFrame();

protected:
    int IDC_MF_WORKBOOK_BAR;
    int IDC_MF_FILEPANEL_BAR;
    int IDC_MF_FILEPANEL;
    const char* m_cszProfileName;
    const char* m_cszMainFrame;
    const char* m_cszWP;

    HWND m_hActiveChild, m_hLastChild, m_hSkipChild;
    BOOL m_bMDINextSeq;

    BOOL                m_bCloseFileOnTabDblClick;
    BOOL                m_bSaveMainWinPosition;
    BOOL                m_bShowed;
    CString             m_strProfileName;
    CWorkbookBar        m_wndWorkbookBar;
    CWorkbookControlBar	m_wndFilePanelBar;
	CFilePanelWnd	    m_wndFilePanel;
	CFont	            m_font; // default gui font for bar controls
    CString             m_defaultFileExtension;

    int DoCreate (BOOL loadBarState = TRUE);

    BOOL VerifyBarState ();
    BOOL LoadBarState   ();
    void DockControlBarLeftOf (CControlBar* Bar, CControlBar* LeftOf);
    static bool has_focus (HWND hWndControl);

    friend class CMDIChildFrame;
    void OnCreateChild   (CMDIChildWnd*);
    void OnDestroyChild  (CMDIChildWnd*);
    void OnActivateChild (CMDIChildWnd*);
    void OnRenameChild   (CMDIChildWnd*, LPCTSTR);
    int  GetImageByDocument (const CDocument*);

    friend class CFilePanelWnd;
    void ActivateChild (CMDIChildWnd*);

    afx_msg void OnChangeWorkbookTab (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblClickOnWorkbookTab (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSysCommand (UINT nID, LONG lParam);

protected:
	DECLARE_MESSAGE_MAP()
public:

    CFilePanelWnd& GetFilePanelWnd () { return m_wndFilePanel; }
    const CFilePanelWnd& GetFilePanelWnd () const { return m_wndFilePanel; }

    void SetCloseFileOnTabDblClick (BOOL closeFileOnTabDblClick) { m_bCloseFileOnTabDblClick = closeFileOnTabDblClick; }
    void SetDefaultFileExtension (const char* ext)  { m_defaultFileExtension = ext; }
    void GetOrderedChildren (std::vector<CMDIChildWnd*>&) const;

    afx_msg void OnClose();
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnViewWorkbook ();
	afx_msg void OnUpdateViewWorkbook (CCmdUI* pCmdUI);
    afx_msg void OnViewFilePanel();
    afx_msg void OnUpdateViewFilePanel(CCmdUI *pCmdUI);
    afx_msg void OnViewFilePanelSync();
    afx_msg void OnUpdateViewFilePanelSync(CCmdUI *pCmdUI);
	afx_msg BOOL OnToolTipText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnLastWindow ();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};


#endif//__WorkbookMDIFrame_H__
