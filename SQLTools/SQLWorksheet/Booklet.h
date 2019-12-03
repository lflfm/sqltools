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
// 07.02.2005 (Ken Clubok) R1105003: Bind variables

#pragma once
#ifndef __BOOKLET_H__
#define __BOOKLET_H__

#include <vector>

    class CPLSWorksheetDoc;

class CBooklet : public CView
{
	DECLARE_DYNCREATE(CBooklet)

public:
	CBooklet ();
	virtual ~CBooklet ();

// Operations
public:
    void  ActivateTab (int);
    void  ActivateTab (CView*);
    void  ActivatePlanTab ();

    BOOL  GetActiveTab (int&) const;
    BOOL  GetActiveTab (CView*&) const;
    int   GetTabCount () const                      { return m_Tabs.size(); }
    bool  IsTabPinned () const                      { return m_tabPinned; }

    void  OnIdleUpdateCmdUI ();

 	//{{AFX_VIRTUAL(CBooklet)
    void CView::OnDraw(CDC*) {}
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CBooklet)
	afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize (UINT nType, int cx, int cy);
	afx_msg void OnQuery      ();
    afx_msg void OnStatistics ();
    afx_msg void OnPlan       ();
    afx_msg void OnOutput     ();
    afx_msg void OnHistory    ();
	afx_msg void OnBinds      ();
    afx_msg void OnPin        ();
    afx_msg void OnGridCommand (UINT nID);
    afx_msg void OnPlanDropDown (NMHDR*, LRESULT*);
    afx_msg void OnPlanText ();
    afx_msg void OnPlanTree ();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    // Attributes
private:
    CView* getActiveView () const                   { return m_Tabs.at(m_nActiveItem); }

    bool m_tabPinned;
    int m_nActiveItem;
    std::vector<CView*> m_Tabs;
    CToolBarCtrl m_toolbar;
    CPLSWorksheetDoc* m_pDocument;
public:
    virtual void OnInitialUpdate();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}

#endif//__BOOKLET_H__
