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

class CMDIMainFrame : public CWorkbookMDIFrame
{
    DECLARE_DYNAMIC(CMDIMainFrame)
public:
    static LPCSTR m_cszClassName;

    using CWorkbookMDIFrame::m_bSaveMainWinPosition;

	CMDIMainFrame();
	~CMDIMainFrame();

// Operations
public:
    CObjectViewerWnd& ShowTreeViewer ();
    ConnectionBar& GetConnectionBar ()     { return m_wndConnectionBar; }

    void ShowProperties (std::vector<std::pair<string, string> >& properties, bool readOnly = true);
    
    void SetIndicators ();

// Events
    void EvOnConnect ();
    void EvOnDisconnect ();

	//{{AFX_VIRTUAL(CMDIMainFrame)
	//}}AFX_VIRTUAL

protected:  // control bar embedded members
    CStatusBar   m_wndStatusBar;
    CToolBar     m_wndFileToolBar;
    CToolBar     m_wndSqlToolBar;
    CWorkbookBar m_wndWorkbookBar;
    
    ConnectionBar m_wndConnectionBar;

    CWorkbookControlBar m_wndDbSourceFrame;
    CDbSourceWnd        m_wndDbSourceWnd;

    CWorkbookControlBar m_wndObjectViewerFrame;
    CObjectViewerWnd    m_wndObjectViewer;

    CWorkbookControlBar m_wndGrepFrame;
    CGrepView           m_wndGrepViewer;

    CWorkbookControlBar m_wndPropTreeFrame;
    PropTreeCtrl2       m_wndPropTree;

    CString m_orgTitle;

// Generated message map functions
protected:
	//{{AFX_MSG(CMDIMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSqlObjViewer();
	afx_msg void OnUpdate_SqlObjViewer(CCmdUI* pCmdUI);
	afx_msg void OnSqlPlanViewer();
	afx_msg void OnUpdate_SqlPlanViewer(CCmdUI* pCmdUI);
	afx_msg void OnSqlDbSource();
	afx_msg void OnUpdate_SqlDbSource(CCmdUI* pCmdUI);
	afx_msg void OnFileGrep();
	afx_msg void OnFileShowGrepOutput();
	afx_msg void OnUpdate_FileShowGrepOutput(CCmdUI* pCmdUI);
	afx_msg void OnUpdate_FileGrep(CCmdUI* pCmdUI);
	afx_msg void OnViewProperties();
	afx_msg void OnUpdate_ViewProperties(CCmdUI* pCmdUI);
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
//    afx_msg void OnSize(UINT nType, int cx, int cy);
    void UpdateViewerAccelerationKeyLabels ();
    afx_msg LRESULT OnSetMessageString (WPARAM, LPARAM);
    afx_msg void OnPlanDropDown (NMHDR*, LRESULT*);

    afx_msg LRESULT RequestQueueNotEmpty (WPARAM, LPARAM);
    afx_msg LRESULT RequestQueueEmpty (WPARAM, LPARAM);
    afx_msg void OnEndSession(BOOL bEnding);
    afx_msg BOOL OnQueryEndSession();
};

//{{AFX_INSERT_LOCATION}}

#endif//__MAINFRM_H__
