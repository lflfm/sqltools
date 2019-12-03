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

#include "stdafx.h"
#include <iomanip>
#include "SQLTools.h"
#include "HistoryView.h"
#include "HistorySource.h"
#include "HistoryFileManager.h"
#include "OpenGrid/GridSourceBase.h"
#include "COMMON/StrHelpers.h"

    using namespace std;
    using namespace Common;

// CHistoryView

IMPLEMENT_DYNCREATE(CHistoryView, GridView)

CHistoryView::CHistoryView()
: m_pHistorySource(HistoryFileManager::GetInstance().CreateSource())
{
    m_pManager = new GridManager(this, m_pHistorySource.get(), false);
    m_pManager->m_Options.m_RowSelect     = true;
    m_pManager->m_Options.m_ExpandLastCol = true;
    m_pManager->m_Options.m_HorzScroller  = true;
    m_pManager->m_Options.m_VertScroller  = true;

    SetVisualAttributeSetName("History Window");
}

CHistoryView::~CHistoryView()
{
    try { EXCEPTION_FRAME;

        delete m_pManager;
    }
    _DESTRUCTOR_HANDLER_;
}

void CHistoryView::Load (const char* fileNmame)
{
    HistoryFileManager::GetInstance().Load(*m_pHistorySource, fileNmame);
}

void CHistoryView::Save (const char* fileNmame)
{
    HistoryFileManager::GetInstance().Save(*m_pHistorySource, fileNmame);
}

void CHistoryView::AddStatement (time_t startTime, const std::string& duration, const std::string& connectDesc, const std::string& sqlSttm)
{
    m_pHistorySource->AddStatement(startTime, duration, connectDesc, sqlSttm);
    m_pManager->MoveToHome(edVert);
}


// SettingsSubscriber
void CHistoryView::OnSettingsChanged ()
{
    GridView::OnSettingsChanged();
    m_pHistorySource->OnSettingsChanged();

    if (!GetSQLToolsSettings().GetHistoryEnabled())
        m_pManager->Clear();
}

// CHistoryView message handlers

BEGIN_MESSAGE_MAP(CHistoryView, GridView)
    ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

void CHistoryView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CDocument* doc = GetDocument();
    if (doc)
        doc->OnCmdMsg(ID_SQL_HISTORY_GET, 0, 0, 0);
    else 
        GridView::OnLButtonDblClk(nFlags, point);
}

bool CHistoryView::GetHistoryEntry (std::string& text)
{
    bool retVal = false;

    if (m_pHistorySource->GetRowCount())
    {
        text = m_pHistorySource->GetSqlStatement(m_pManager->GetCurrentPos(edVert));
        if (!text.empty() && *text.rbegin() != '\n')
            text += '\n';
        retVal = true;
    }
    return retVal;
}

bool CHistoryView::IsEmpty () const
{
    return m_pHistorySource->GetRowCount() ? false : true;
}
