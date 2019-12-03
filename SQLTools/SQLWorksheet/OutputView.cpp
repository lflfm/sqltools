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

/*
    22.03.2004 bug fix, Output: only the first line of multiline error/message is visible
*/

#include "stdafx.h"
#include "SQLTools.h"
#include "COMMON\AppGlobal.h"
#include "resource.h"
#include "OutputView.h"
#include "SQLWorksheetDoc.h"
#include <OpenGrid/GridSourceBase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace OG2;
    using namespace Common;

    CImageList COutputView::m_imageList;

COutputView::COutputView()
{
    m_uPopupMenuId = IDR_OUTPUT_POPUP;

    SetVisualAttributeSetName("Output Window");

    _ASSERTE(m_pStrSource);

    m_NoMoreError = false;
    m_pStrSource->SetCols(3);
    m_pStrSource->SetColCharWidth(0, 5);
    m_pStrSource->SetColCharWidth(1, 5);
    m_pStrSource->SetColCharWidth(2, 20);
    m_pStrSource->SetColHeader(0, "Line");
    m_pStrSource->SetColHeader(1, "Pos");
    m_pStrSource->SetColHeader(2, "Text");
    m_pStrSource->SetShowRowEnumeration(false);
    m_pStrSource->SetShowTransparentFixCol(true);

    m_pStrSource->SetFixSize(eoVert, edHorz, 3);

    if (!m_imageList.m_hImageList)
        m_imageList.Create(IDB_PANE_ICONS, 16, 64, RGB(0,255,0));

    m_pStrSource->SetImageList(&m_imageList, FALSE);

    m_pStrSource->SetMaxShowLines(6);
    m_pManager->m_Options.m_HorzLine = false;
    m_pManager->m_Options.m_VertLine = false;
    m_pManager->m_Options.m_RowSelect = true;
    m_pManager->m_Options.m_ExpandLastCol = true;
    m_pManager->m_Options.m_ClearRecords = true;

    m_enableBuffer = true;
}

COutputView::~COutputView()
{
}

BEGIN_MESSAGE_MAP(COutputView, StrGridView)
    ON_WM_CREATE()
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditGroup)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_EDIT_CLEAR_ALL, OnEditClearAll)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR_ALL, OnUpdateEditGroup)
    ON_WM_LBUTTONDBLCLK()
    ON_WM_TIMER()
    ON_COMMAND(ID_GRID_OUTPUT_OPEN, OnGridOutputOpen)
    ON_COMMAND(ID_OUTPUT_TEXT_OPEN, OnOutputTextOpen)
    ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COutputView message handlers

int COutputView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (StrGridView::OnCreate(lpCreateStruct) == -1)
		return -1;

    m_pManager->Refresh();

	return 0;
}

int COutputView::putLine (int type, const char* str, int line, int pos, OpenEditor::LineId lineId, bool skip)
{
    m_NoMoreError = false;
    char szLine[18], szPos[18];
    if (line != -1) itoa(line+1, szLine, 10);
    else szLine[0] = 0;
    if (pos != -1) itoa(pos+1, szPos, 10);
    else szPos[0] = 0;

    int row = m_pStrSource->Append();
    m_pStrSource->SetString(row, 0, szLine);
    m_pStrSource->SetString(row, 1, szPos);
    m_pStrSource->SetString(row, 2, str);
    m_pStrSource->SetImageIndex(row, type);
    
    m_lineIDs.append(lineId);
    if (skip) m_skippedErrors.insert(row);

    _ASSERTE(m_pStrSource->GetCount(edVert) == static_cast<int>(m_lineIDs.size()));

    return row;
}

void COutputView::PutLine (int type, const char* str, int line, int pos, OpenEditor::LineId lineId, bool skip)
{
    if (!m_enableBuffer)
    {
        int row = putLine(type, str, line, pos, lineId, skip);
        // 22.03.2004 bug fix, Output: only the first line of multiline error/message is visible
        m_pManager->OnChangedRows(row, row);
    }
    else
    {
        if (m_buffer.empty()) 
            SetTimer(FLUSH_BUFFER_TIMER, 500, NULL);

        m_buffer.resize(m_buffer.size() + 1);

        Line& last = *m_buffer.rbegin();
        last.type   = type  ;
        last.str    = str   ;
        last.line   = line  ;
        last.pos    = pos   ;
        last.lineId = lineId;
        last.skip   = skip  ;
    }
}

void COutputView::Refresh ()
{
    m_pManager->MoveToHome(edVert);
    m_pManager->Refresh();
}

void COutputView::FirstError (bool onlyError)
{
    flushBuffer();
    m_NoMoreError = true;
    NextError(onlyError);
}

bool COutputView::NextError (bool onlyError)
{
    flushBuffer();

    if (m_NoMoreError)
    {
        m_NoMoreError = false;
        m_pManager->MoveToHome(edVert);
    }
    else
    {
        m_NoMoreError = !m_pManager->MoveToRight(edVert);
    }

    while (onlyError && !m_NoMoreError
    && (
        m_pStrSource->GetImageIndex(m_pManager->GetCurrentPos(edVert)) != 4
        || m_skippedErrors.find(m_pManager->GetCurrentPos(edVert)) != m_skippedErrors.end()
        )
    )
        m_NoMoreError = !m_pManager->MoveToRight(edVert);

    if (m_NoMoreError)
    {
        MessageBeep(MB_ICONHAND);
        Global::SetStatusText("No more errors!", FALSE);
    }
    else
    {
        if (m_pStrSource->GetCount(edVert))
            Global::SetStatusText(getString(2).c_str(), FALSE);
    }

    return !m_NoMoreError;
}


bool COutputView::PrevError (bool onlyError)
{
    flushBuffer();

    if (m_NoMoreError)
    {
        m_NoMoreError = false;
        m_pManager->MoveToEnd(edVert);
    }
    else
    {
        m_NoMoreError = !m_pManager->MoveToLeft(edVert);
    }

    while (onlyError && !m_NoMoreError
    && m_pStrSource->GetImageIndex(m_pManager->GetCurrentPos(edVert)) != 4)
        m_NoMoreError = !m_pManager->MoveToLeft(edVert);

    if (m_NoMoreError)
    {
        MessageBeep(MB_ICONHAND);
        Global::SetStatusText("No more errors!", FALSE);
    }
    else
    {
        if (m_pStrSource->GetCount(edVert))
            Global::SetStatusText(getString(2).c_str(), FALSE);
    }

    return !m_NoMoreError;
}

bool COutputView::GetCurrError (int& line, int& pos, OpenEditor::LineId& lineId)
{
    flushBuffer();

    bool retVal = false;

    if (m_pStrSource->GetCount(edVert))
    {
        const string& strLine = getString(0);
        if (strLine.size())
        {
            line = atoi(strLine.c_str()) - 1;
            pos  = atoi(getString(1).c_str()) - 1;
            lineId = m_lineIDs.at(m_pManager->GetCurrentPos(edVert));
            retVal = true;
        }
    }
    return retVal;
}

bool COutputView::IsEmpty () const
{
    return m_pStrSource->GetCount(edVert) ? false : true;
}

void COutputView::OnEditCopy()
{
    if (!IsEmpty() && OpenClipboard())
    {
        if (EmptyClipboard())
        {
            const string& text = getString(2);
            HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, text.length()+1);
            memcpy((char*)GlobalLock(hData), text.c_str(), text.length()+1);
            SetClipboardData(CF_TEXT, hData);
        }
        CloseClipboard();
    }
}

void COutputView::OnUpdateEditGroup(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!IsEmpty());
}

void COutputView::OnEditClearAll ()
{
    Clear();
}

void COutputView::OnGridOutputOpen()
{
    CPLSWorksheetDoc* pDoc = DoOpenInEditor(GetSQLToolsSettings(), etfPlainText);
    if (pDoc)
        pDoc->SetClassSetting("Text");
}

void COutputView::OnOutputTextOpen()
{
    m_pManager->m_Rulers[0].SetFirstSelected(2);
    m_pManager->m_Rulers[0].SetLastSelected(2);

    CPLSWorksheetDoc* pDoc = DoOpenInEditor(GetSQLToolsSettings(), etfPlainText);
    if (pDoc)
        pDoc->SetClassSetting("Text");

    m_pManager->m_Rulers[0].ResetSelection();
}

void COutputView::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    GridView::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if (!IsEmpty() && m_pStrSource->GetCount(edVert) > 0)
    {
        pPopupMenu->EnableMenuItem(ID_OUTPUT_TEXT_OPEN, MF_BYCOMMAND|MF_ENABLED);
    }
}

void COutputView::Clear ()
{   
    m_buffer.clear();
    StrGridView::Clear();
    m_lineIDs.clear();
    m_skippedErrors.clear();
}

inline
const string& COutputView::getString (int col)
{
	return m_pStrSource->GetString(m_pManager->GetCurrentPos(edVert), col);
}


void COutputView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CDocument* doc = GetDocument();
    if (doc)
        doc->OnCmdMsg(ID_SQL_CURR_ERROR, 0, 0, 0);
    else 
        StrGridView::OnLButtonDblClk(nFlags, point);
}

void COutputView::flushBuffer()
{
    if (!m_buffer.empty())
    {
        int first = m_pStrSource->GetCount(edVert);
        int last = first;

        std::vector<Line>::const_iterator it = m_buffer.begin();
        
        for (; it != m_buffer.end(); ++it)
            last = putLine(it->type, it->str.c_str(), it->line, it->pos, it->lineId, it->skip);

        m_buffer.clear();

        m_pManager->OnChangedRows(first, last);
    }
}

void COutputView::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == FLUSH_BUFFER_TIMER)
    {
        flushBuffer();
        KillTimer(FLUSH_BUFFER_TIMER);
    }
    StrGridView::OnTimer(nIDEvent);
}

void COutputView::OnSettingsChanged ()
{
    m_enableBuffer = GetSQLToolsSettings().GetBufferedExecutionOutput();

    if (!m_enableBuffer)
        flushBuffer();

    __super::OnSettingsChanged();
}
