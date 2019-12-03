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

#include <string>
#include <OpenGrid/GridView.h>
#include <arg_shared.h>

    using namespace OG2;
    class HistorySource;
    using arg::counted_ptr;

// CHistoryView view
class CHistoryView : public OG2::GridView
{
	DECLARE_DYNCREATE(CHistoryView)
    counted_ptr<HistorySource> m_pHistorySource;

    virtual void OnSettingsChanged ();
public:
	CHistoryView();
	virtual ~CHistoryView();

    void Load  (const char* fileNmame);
    void Save  (const char* fileNmame);
    void AddStatement (time_t startTime, const std::string& duration, const std::string& connectDesc, const std::string& sqlSttm);

    virtual bool IsEmpty () const;

    void StepForward ();
    void StepBackward ();
    bool GetHistoryEntry (std::string&);

public:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnLButtonDblClk (UINT nFlags, CPoint point);
};

inline
void CHistoryView::StepForward () 
{
    OnMoveDown();
}

inline
void CHistoryView::StepBackward ()
{
    OnMoveUp();
}

