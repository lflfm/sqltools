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

#ifndef __GREPVIEW_H__
#define __GREPVIEW_H__
#pragma once

    class CGrepThread;

class CGrepView : public CTreeCtrl
{
    int          m_nCounter;
    CString      m_strLastFileName;
    HTREEITEM    m_hLastItem;
    CGrepThread* m_pThread;
    CEvent       m_evRun, m_evAbort;

public:
	CGrepView();
	virtual ~CGrepView();

protected:
	DECLARE_DYNCREATE(CGrepView)

// Operations
public:
    BOOL Create (CWnd* pFrameWnd);

    BOOL IsRunning () const            { return WaitForSingleObject(m_evRun,   0) == WAIT_OBJECT_0; }
    BOOL QuickIsRunning () const       { return m_pThread ? TRUE : FALSE; }
    BOOL IsAbort () const              { return WaitForSingleObject(m_evAbort, 0) == WAIT_OBJECT_0; }

    void StartProcess (CGrepThread*);
    afx_msg void AbortProcess ();
    void EndProcess ();

    void AddEntry (const char* szEntry, BOOL bExpanded, BOOL bInfoOnly = FALSE);
    void ClearContent ();
   
    void OnExpandAll (UINT);

	//{{AFX_VIRTUAL(CGrepView)
	protected:
    virtual void OnDraw(CDC*) {}     // overridden to draw this view
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CGrepView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
    afx_msg void OnEndProcess ();
	afx_msg void OnOpenFile ();
	afx_msg void OnCollapseAll ();
	afx_msg void OnExpandAll ();
	afx_msg void OnClear ();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

#endif//__GREPVIEW_H__
