/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2014 Aleksey Kochetov

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

#include <OpenGrid\GridView.h>
#include <arg_shared.h>

    class ExplainPlanSource;
    using arg::counted_ptr;

class ExplainPlanView2 : public OG2::GridView
{
    counted_ptr<ExplainPlanSource> m_source;

public:
    ExplainPlanView2  ();
    virtual ~ExplainPlanView2 ();

    void SetSource (counted_ptr<ExplainPlanSource> source);
    void ApplyColumnFit (int nItem = -1);
    string GetObjectName () const;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void OnSqlDescribe();
    afx_msg void OnUpdate_SqlDescribe(CCmdUI *pCmdUI);
    afx_msg void OnExplainPlanIemProperties();
    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnUpdate_Unsupported (CCmdUI *pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnGridOutputOpen();
    afx_msg void OnFileExport();
};

