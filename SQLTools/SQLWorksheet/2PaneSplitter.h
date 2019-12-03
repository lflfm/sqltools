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

#pragma once
#ifndef __2PaneSplitter_H__
#define __2PaneSplitter_H__

#include "Booklet.h"


class C2PaneSplitter : public CSplitterWnd
{
	DECLARE_DYNCREATE(C2PaneSplitter)
public:
	C2PaneSplitter();

// Attributes
protected:
    CView*    m_pwndSQLEdit;
    CBooklet* m_pwndBooklet;
    int LowPaneHight[2];
    bool m_ititialized;
    int m_defaultHight;
    int m_defaultHightRow;

// Operations
public:
    BOOL Create (CWnd* pParent, CCreateContext* pContext);
    void MoveDown (); 
    void MoveUp ();
    void MoveToDefault ();
	void NextPane();
	void PrevPane();
	void NextTab();
	void PrevTab();
    void SetDefaultHight (int row, int h) { m_defaultHight = h; m_defaultHightRow = row; }

	//{{AFX_VIRTUAL(C2PaneSplitter)
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~C2PaneSplitter();
    virtual void StopTracking (BOOL bAccept);
    void SetActiveView (CView* pView);
    void SetEditorView (CView* pEditor)             { m_pwndSQLEdit = pEditor; }
    CView* GetEditorView ()                         { return m_pwndSQLEdit; }
    const CView* GetEditorView () const             { return m_pwndSQLEdit; }

	//{{AFX_MSG(C2PaneSplitter)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

#endif//__2PaneSplitter_H__
