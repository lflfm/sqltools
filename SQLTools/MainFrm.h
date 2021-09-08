/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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

#ifndef __MAINFRM_H__
#define __MAINFRM_H__
#pragma once

#include "COMMON/WorkbookMDIFrame.h"
#include "DbBrowser/TreeViewer.h"
#include "DbBrowser/DbSourceWnd.h"
#include "DbBrowser/ObjectViewerWnd.h"
#include "Tools/Grep/GrepView.h"
#include "ConnectionBar.h"
#include "PropTree\PropTreeCtrl2.h"
#include "OpenFilesWnd.h"

class CMDIMainFrame : public CWorkbookMDIFrame
{
    DECLARE_DYNAMIC(CMDIMainFrame)
public:
    static LPCWSTR m_cszClassName;

    CMDIMainFrame();
    ~CMDIMainFrame();

// Operations
public:
    CObjectViewerWnd& ShowTreeViewer ();
    ConnectionBar& GetConnectionBar ()     { return m_wndConnectionBar; }

    CGrepView& ShowGrepViewer ();
    CGrepView& GetGrepView ()              { return m_wndGrepViewer; }

    void ShowProperties (std::vector<std::pair<string, string> >& properties, bool readOnly = true);
    
    void SetIndicators ();

    OpenFilesWnd& GetOpenFilesWnd ()             { return m_wndOpenFiles; }
    const OpenFilesWnd& GetOpenFilesWnd () const { return m_wndOpenFiles; }

// Events
    void EvOnConnect ();
    void EvOnDisconnect ();

    //{{AFX_VIRTUAL(CMDIMainFrame)
    //}}AFX_VIRTUAL

protected:  // control bar embedded members
    CMFCStatusBar m_wndStatusBar;
    CMFCToolBar   m_wndToolBar;
    CMFCToolBar   m_wndSqlToolBar;
    
    ConnectionBar m_wndConnectionBar;

    DockableContainer m_wndDbSourceFrame;
    CDbSourceWnd      m_wndDbSourceWnd;

    DockableContainer m_wndObjectViewerFrame;
    CObjectViewerWnd  m_wndObjectViewer;

    DockableContainer m_wndGrepFrame;
    CGrepView         m_wndGrepViewer;

    DockableContainer m_wndPropTreeFrame;
    PropTreeCtrl2     m_wndPropTree;

    OpenFilesDockableContainer m_wndOpenFilesFrame;
    OpenFilesWnd m_wndOpenFiles;

    CString m_orgTitle;

    void TogglePane (CDockablePane&);

// Generated message map functions
protected:
    //{{AFX_MSG(CMDIMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnClose();
    afx_msg void OnSqlObjViewer();
    afx_msg void OnUpdate_SqlObjViewer(CCmdUI* pCmdUI);
    //afx_msg void OnSqlPlanViewer();
    //afx_msg void OnUpdate_SqlPlanViewer(CCmdUI* pCmdUI);
    afx_msg void OnSqlDbSource();
    afx_msg void OnUpdate_SqlDbSource(CCmdUI* pCmdUI);
    afx_msg void OnFileGrep();
    afx_msg void OnFileShowGrepOutput();
    afx_msg void OnUpdate_FileShowGrepOutput(CCmdUI* pCmdUI);
    afx_msg void OnUpdate_FileGrep(CCmdUI* pCmdUI);
    afx_msg void OnViewProperties();
    afx_msg void OnUpdate_ViewProperties(CCmdUI* pCmdUI);
    afx_msg void OnViewOpenFiles();
    afx_msg void OnUpdate_ViewOpenFiles(CCmdUI* pCmdUI);
    //}}AFX_MSG
    afx_msg void OnViewFileToolbar();
    afx_msg void OnViewSqlToolbar();
    afx_msg void OnViewConnectToolbar();
    afx_msg void OnUpdate_FileToolbar (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_SqlToolbar (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_ConnectToolbar (CCmdUI* pCmdUI);
    DECLARE_MESSAGE_MAP()
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
    afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
    afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    void UpdateViewerAccelerationKeyLabels ();
    afx_msg LRESULT OnSetMessageString (WPARAM, LPARAM);
    afx_msg LRESULT RequestQueueNotEmpty (WPARAM, LPARAM);
    afx_msg LRESULT RequestQueueEmpty (WPARAM, LPARAM);
    afx_msg void OnEndSession(BOOL bEnding);
    afx_msg BOOL OnQueryEndSession();
    void SetupMDITabbedGroups ();

    void OnCreateChild   (CMDIChildWnd*);
    void OnDestroyChild  (CMDIChildWnd*);
    void OnActivateChild (CMDIChildWnd*);
    void OnRenameChild   (CMDIChildWnd*, LPCTSTR);
};

//{{AFX_INSERT_LOCATION}}

#endif//__MAINFRM_H__
