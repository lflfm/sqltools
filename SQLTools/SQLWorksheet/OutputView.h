/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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

#ifndef __OUTPUTVIEW_H__
#define __OUTPUTVIEW_H__
#pragma once

#include <OpenGrid/GridView.h>
#include <COMMON/QuickArray.h>

    class CSQLToolsApp;

class COutputView : public OG2::StrGridView
{
public:
	COutputView();
	virtual ~COutputView();

    void PutLine (int type, const char* str, int line, int pos, OpenEditor::LineId lineId, bool skip);
    void PutError (const char* str, int line =-1, int pos =-1, OpenEditor::LineId lineId = OpenEditor::LineId(), bool skip = false);
    void PutMessage (const char* str, int line =-1, int pos =-1, OpenEditor::LineId lineId = OpenEditor::LineId());
    void PutDbmsMessage (const char* str, int line =-1, int pos =-1, OpenEditor::LineId lineId = OpenEditor::LineId());
    void Refresh ();
    void Clear ();
    void FlushBuffer ();

    bool IsEmpty () const;
    void FirstError (bool onlyError = false);
    bool NextError (bool onlyError = false);
    bool PrevError (bool onlyError = false);
    bool GetCurrError (int& line, int& pos, OpenEditor::LineId& lineId);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditGroup(CCmdUI* pCmdUI);
	afx_msg void OnEditClearAll();
    afx_msg void OnGridOutputOpen();
    afx_msg void OnOutputTextOpen();
    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	DECLARE_MESSAGE_MAP()

private:
    const string& COutputView::getString (int col);

    Common::QuickArray<OpenEditor::LineId> m_lineIDs;
    std::set<int> m_skippedErrors;
    bool m_NoMoreError;
    static CImageList m_imageList;

    static const int FLUSH_BUFFER_TIMER = 1;
    struct Line { int type; string str; int line; int pos; OpenEditor::LineId lineId; bool skip; };
    std::vector<Line> m_buffer;
    bool m_enableBuffer;

    void flushBuffer ();
    int  putLine (int type, const char* str, int line, int pos, OpenEditor::LineId lineId, bool skip);
    virtual void OnSettingsChanged ();

public:
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};

    inline
    void COutputView::PutError (const char* str, int line, int pos, OpenEditor::LineId lineId, bool skip)
    {
        PutLine(4, str, line, pos, lineId, skip);
    }
    inline
    void COutputView::PutMessage (const char* str, int line, int pos, OpenEditor::LineId lineId)
    {
        PutLine(2, str, line, pos, lineId, false);
    }
    inline
    void COutputView::PutDbmsMessage (const char* str, int line, int pos, OpenEditor::LineId lineId)
    {
        PutLine(5, str, line, pos, lineId, false);
    }
    inline
    void COutputView::FlushBuffer ()
    {
        flushBuffer();
    }

//{{AFX_INSERT_LOCATION}}

#endif//__OUTPUTVIEW_H__
