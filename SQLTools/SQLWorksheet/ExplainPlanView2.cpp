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

#include "stdafx.h"
#include "ExplainPlanView2.h"
#include "ExplainPlanSource.h"
#include "MainFrm.h"
#include "DbBrowser\ObjectViewerWnd.h"
#include "SQLWorksheet/SQLWorksheetDoc.h"

    using namespace OG2;

ExplainPlanView2::ExplainPlanView2 ()
{
    m_source.reset(new ExplainPlanSource);

    m_pManager = new GridManager(this, m_source.get(), false);
    m_pManager->m_Options.m_RowSelect     = true;
    m_pManager->m_Options.m_ExpandLastCol = true;
    m_pManager->m_Options.m_HorzScroller  = true;
    m_pManager->m_Options.m_VertScroller  = true;
    m_pManager->m_Options.m_HorzLine      = false;
    m_pManager->m_Options.m_VertLine      = false;
    m_pManager->m_Options.m_RowSizing     = false;
    m_pManager->m_Options.m_Wraparound    = false;
    m_pManager->m_Options.m_CellFocusRect = true;

    SetVisualAttributeSetName("History Window");
}


ExplainPlanView2::~ExplainPlanView2 ()
{
    try { EXCEPTION_FRAME;

        delete m_pManager;
        m_pManager = 0;

    }
    _DESTRUCTOR_HANDLER_
}

void ExplainPlanView2::SetSource (counted_ptr<ExplainPlanSource> source)
{
    m_pManager->SetSource(source.get(), false);
    source.swap(m_source);
    ApplyColumnFit();
}

void ExplainPlanView2::ApplyColumnFit (int nItem /*= -1*/)
{
    if (!IsEmpty()
    && m_source->GetCount(edVert) > 0)
    {
		{
			CClientDC dc(this);
			OnPrepareDC(&dc, 0);
			PaintGridManager::m_Dcc.m_Dc = &dc;

			bool byData     = true/*GetSQLToolsSettings().GetGridColumnFitType() == 0*/;
			bool byHeader   = true/*!byData || !GetSQLToolsSettings().GetGridAllowLessThanHeader()*/;
			int maxColLen   = 100/*GetSQLToolsSettings().GetGridMaxColLength()*/;
			int columnCount = m_source->GetCount(edHorz);
			int rowCount    = m_source->GetCount(edVert);
            int colStart    = 0;

            if (nItem >= 0 && nItem < columnCount)
            {
                colStart = nItem;
                columnCount = nItem + 1;
            }

			PaintGridManager::m_Dcc.m_Type[edHorz] = efNone;

			for (int col = colStart; col < columnCount; col++)
			{
				int width = 0, h_width = 0;

				PaintGridManager::m_Dcc.m_Col = col;

				if (byHeader)
				{
					PaintGridManager::m_Dcc.m_Row = 0;
					PaintGridManager::m_Dcc.m_Type[edVert] = efFirst;
					m_source->CalculateCell(PaintGridManager::m_Dcc, maxColLen);
					h_width = PaintGridManager::m_Dcc.m_Rect.Width();
				}
				if (byData)
				{
					PaintGridManager::m_Dcc.m_Type[edVert] = efNone;
					for (int row = 0; row < rowCount; row++)
					{
                        string text;
                        m_source->GetCellStr(text, row, col);
                        if (!text.empty())
                        {
						    PaintGridManager::m_Dcc.m_Row = row;
						    m_source->CalculateCell(PaintGridManager::m_Dcc, maxColLen);
						    width = max(width, PaintGridManager::m_Dcc.m_Rect.Width());
                        }
					}
				}
                
                if (width == 0) // make unused column almost invisible
                    width = 6;
                else
                    width = max(width, h_width);

				m_pManager->m_Rulers[edHorz].SetPixelSize(col, width);
			}
		}
        m_pManager->LightRefresh(edHorz);
	}
}

string ExplainPlanView2::GetObjectName () const
{
    string buffer;

    if (m_source.get() && m_source->GetCount(edVert) > 0)
    {
        int row = m_pManager->GetCurrentPos(edVert);

        const plan_table_record& record = m_source->GetRow(row);

        if (record.object_type == "TABLE"
        && !record.object_owner.empty()
        && !record.object_name.empty()
        )
        {
            buffer += '"' + record.object_owner + '"';
            buffer += '.';
            buffer += '"' + record.object_name + '"';
            return buffer;
        }
    }

    return buffer;
}

BEGIN_MESSAGE_MAP(ExplainPlanView2, OG2::GridView)
    ON_WM_LBUTTONDBLCLK()
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_SQL_DESCRIBE, &ExplainPlanView2::OnSqlDescribe)
    ON_UPDATE_COMMAND_UI(ID_SQL_DESCRIBE, &ExplainPlanView2::OnUpdate_SqlDescribe)
    ON_COMMAND(ID_EXPLAIN_PLAN_ITEM_PROPERTIES, &ExplainPlanView2::OnExplainPlanIemProperties)
    ON_WM_INITMENUPOPUP()
    ON_UPDATE_COMMAND_UI(ID_GRID_OPEN_WITH_IE, OnUpdate_Unsupported)
    ON_UPDATE_COMMAND_UI(ID_GRID_OPEN_WITH_EXCEL, OnUpdate_Unsupported)
	ON_WM_KEYDOWN()
    ON_COMMAND(ID_GRID_OUTPUT_OPEN, OnGridOutputOpen)
	ON_COMMAND(ID_GRID_OUTPUT_EXPORT, OnFileExport)
END_MESSAGE_MAP()

void ExplainPlanView2::OnLButtonDblClk (UINT /*nFlags*/, CPoint point)
{
    //GridView::OnLButtonDblClk(nFlags, point);

    int row = m_pManager->m_Rulers[edVert].GetCurrent();

    if (m_pManager->IsDataCell(point))
        OnExplainPlanIemProperties();
    else if (m_pManager->IsFixedCol(point.x))
        m_source->Toggle(row);
}

void ExplainPlanView2::OnContextMenu (CWnd* /*pWnd*/, CPoint pos)
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_PLAN_POPUP));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);

    CPoint pt = pos;
    ScreenToClient(&pt);
    if (!m_pManager->IsDataCell(pt))
        for (int i = 0; i < 4; ++i)
            pPopup->RemoveMenu(0, MF_BYPOSITION);

    if (pos.x == -1 && pos.y == -1) // invoked by keyboard
    {
        if (m_pManager->IsCurrentVisible())
        {
            CRect rc;
            m_pManager->GetCurRect(rc);
            ClientToScreen(rc);
            pos.x = rc.left + 3;
            pos.y = rc.bottom;
        }
        else
        {
            int loc[2];
            m_pManager->m_Rulers[edHorz].GetDataArea(loc);
            pos.x = loc[0];
            m_pManager->m_Rulers[edVert].GetDataArea(loc);
            pos.x = loc[0];
            ClientToScreen(&pos);
        }
    }

    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, this);
}

void ExplainPlanView2::OnSqlDescribe ()
{
    string name = GetObjectName();
    if (!name.empty())
    {
        CObjectViewerWnd& viewer = ((CMDIMainFrame*)AfxGetApp()->m_pMainWnd)->ShowTreeViewer();
        viewer.ShowObject(name);
        viewer.SetFocus();
    }
}

void ExplainPlanView2::OnUpdate_SqlDescribe (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetObjectName().empty());
}

void ExplainPlanView2::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    GridView::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    pPopupMenu->EnableMenuItem(ID_SQL_DESCRIBE, !GetObjectName().empty() ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
    pPopupMenu->EnableMenuItem(ID_EXPLAIN_PLAN_ITEM_PROPERTIES, m_pManager->GetSource()->GetCount(edVert) > 0 ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
}

void ExplainPlanView2::OnExplainPlanIemProperties ()
{
    if (m_source.get())
    {
        int row = m_pManager->GetCurrentPos(edVert);
        if (row < m_pManager->GetSource()->GetCount(edVert))
        {
            std::vector<std::pair<string, string> > properties;
            m_source->GetProperties(row, properties);
            ((CMDIMainFrame*)AfxGetApp()->m_pMainWnd)->ShowProperties(properties, true);
        }
    }
}

void ExplainPlanView2::OnUpdate_Unsupported (CCmdUI *pCmdUI)
{
    if (pCmdUI)
        pCmdUI->Enable(FALSE);
}

void ExplainPlanView2::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_RETURN)
        OnExplainPlanIemProperties();
    else
        GridView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void ExplainPlanView2::OnGridOutputOpen()
{
    CPLSWorksheetDoc* pDoc = DoOpenInEditor(GetSQLToolsSettings(), etfPlainText);
    if (pDoc)
        pDoc->SetClassSetting("Text");
}

void ExplainPlanView2::OnFileExport ()
{
    DoFileSave(GetSQLToolsSettings(), etfPlainText, false);
}

