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

/***************************************************************************/
/*      Purpose: Grid manager implementation for grid component            */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~            */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

// 2017-11-28 implemented range selection

#include "stdafx.h"
#include <sstream>
#include "GridManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OG2
{
#pragma message ("\tImprovement: probably dcc pool should be created")

    using namespace std;

    DrawCellContexts PaintGridManager::m_Dcc;


PaintGridManager::PaintGridManager (CWnd* client, AGridSource* source, bool shouldDeleteSource)
: NavGridManager(client, source, shouldDeleteSource)
{
    //m_Dcc.m_pSubscriber = this;
    InitColors();
}

void PaintGridManager::InitColors ()
{
    m_Dcc.m_TextColor           = GetSysColor(COLOR_WINDOWTEXT);
    m_Dcc.m_BkColor[0]          = GetSysColor(COLOR_WINDOW);
    m_Dcc.m_BkColor[1]          = RGB(255-8, 255-8, 255-8);
    m_Dcc.m_FixTextColor        = GetSysColor(COLOR_BTNTEXT);
    m_Dcc.m_FixBkColor          = GetSysColor(COLOR_BTNFACE);
    m_Dcc.m_FixShdColor         = GetSysColor(COLOR_BTNSHADOW);
    m_Dcc.m_LightColor          = GetSysColor(COLOR_BTNHILIGHT);
    m_Dcc.m_UnusedColor         = GetSysColor(COLOR_WINDOW/*COLOR_APPWORKSPACE*/);
    m_Dcc.m_HightLightColor     = GetSysColor(COLOR_HIGHLIGHT);
    m_Dcc.m_HightLightTextColor = GetSysColor(COLOR_HIGHLIGHTTEXT);

    m_Dcc.m_BkBrush[0].DeleteObject();
    m_Dcc.m_BkBrush[0].CreateSolidBrush(m_Dcc.m_BkColor[0]);
    m_Dcc.m_BkBrush[1].DeleteObject();
    m_Dcc.m_BkBrush[1].CreateSolidBrush(m_Dcc.m_BkColor[1]);
    m_Dcc.m_FixBrush.DeleteObject();
    m_Dcc.m_FixBrush.CreateSolidBrush(m_Dcc.m_FixBkColor);
    m_Dcc.m_HightLightBrush.DeleteObject();
    m_Dcc.m_HightLightBrush.CreateSolidBrush(m_Dcc.m_HightLightColor);
    m_Dcc.m_HightLightPen.DeleteObject();
    m_Dcc.m_HightLightPen.CreatePen(PS_SOLID, 1, m_Dcc.m_HightLightColor);
}

void PaintGridManager::EvSetFocus (bool /*focus*/)
{
    if (IsCurrentVisible())
        RepaintCurent();
}

void PaintGridManager::RepaintCurent () const
{
    if (IsCurrentVisible())
    {
        CRect rc;
        GetCurRect(rc);
        m_pClientWnd->InvalidateRect(rc, FALSE);
        if (m_Options.m_ColSelect)
            RepaintCurCol();
        if (m_Options.m_RowSelect)
            RepaintCurRow();
    }
}

void PaintGridManager::erase (CDC& dc, const CRect& rect) const
{
    dc.FillSolidRect(rect, m_Dcc.m_BkColor[0]);
}

void PaintGridManager::drawLines (eDirection dir, CDC& dc, int lim[4], COLORREF color) const
{
    int shift = m_Rulers[dir].GetLineSize()/2;
    shift += m_Rulers[dir].GetLineSize() - 2*shift;

    if (m_Rulers[dir].GetLineSize()
    && lim[0] != lim[1] && lim[2] != lim[3])
    {
        CPen pen(PS_SOLID, m_Rulers[dir].GetLineSize(), color);
        CPen* oldPen = dc.SelectObject(&pen);

        for (int i = 1; i < m_Rulers[dir].GetCount()+1; i++)  // new
        {
            int x = m_Rulers[dir].GetWidth(i) - shift;
            if (dir == edHorz)
            {
                if (x >= lim[0] && x <= lim[1] + m_Rulers[edHorz].GetLineSize())
                {
                    dc.MoveTo(x, lim[2]);
                    dc.LineTo(x, lim[3]-shift);
                }
            }
            else
            {
                if (x >= lim[2] && x <= lim[3] + m_Rulers[edVert].GetLineSize())
                {
                    dc.MoveTo(lim[0]            , x);
                    dc.LineTo(lim[1]-shift, x);
                }
            }
        }
        dc.SelectObject(*oldPen);
    }
}

void PaintGridManager::drawUnused (CDC& dc, const CRect& rect) const
{
    CRect rc(rect);
    rc.right += m_Rulers[edHorz].GetLineSize();
    rc.bottom += m_Rulers[edVert].GetLineSize();
    dc.FillSolidRect(rc, m_Dcc.m_UnusedColor);
}

void PaintGridManager::DrawFocus (CDC& dc) const
{
    if (GetFocus() == *m_pClientWnd
    && m_Rulers[edHorz].GetDataItemCount()
    && m_Rulers[edVert].GetDataItemCount())
    {
        CRect rc;
        bool visible = false;

        if (m_Options.m_ColSelect && !m_Options.m_CellFocusRect
        && m_Rulers[edHorz].IsCurrentVisible())
        {
            int loc[2];
            m_Rulers[edHorz].GetCurrentInnerLoc(loc);
            rc.left   = loc[0];
            rc.right  = loc[1];
            m_Rulers[edVert].GetDataInnerLoc(loc);
            rc.top    = loc[0];
            rc.bottom = loc[1];
            visible = true;
        }
        else if (m_Options.m_RowSelect && !m_Options.m_CellFocusRect
        && m_Rulers[edVert].IsCurrentVisible())
        {
            int loc[2];
            m_Rulers[edHorz].GetDataInnerLoc(loc);
            rc.left   = loc[0];
            rc.right  = loc[1];
            m_Rulers[edVert].GetCurrentInnerLoc(loc);
            rc.top    = loc[0];
            rc.bottom = loc[1];
            visible = true;
        }
        else if (IsCurrentVisible())
        {
            GetCurRect(rc);
            visible = true;
        }

        if (visible)
            dc.DrawFocusRect(rc);
    }
}

void PaintGridManager::Paint (CDC& dc, bool erase_flag, const CRect& rect)
{
    try {
    if (erase_flag)
        erase(dc, rect);

    int cellRanges[4];
    cellRanges[0] = m_Rulers[edHorz].PointToItem(rect.left);
    cellRanges[1] = m_Rulers[edVert].PointToItem(rect.top);
    cellRanges[2] = m_Rulers[edHorz].PointToItem(rect.right);
    cellRanges[3] = m_Rulers[edVert].PointToItem(rect.bottom);

    m_Dcc.m_Dc = &dc;
    m_Dcc.m_CellMargin.cx = -m_Rulers[edHorz].GetCellMargin();
    m_Dcc.m_CellMargin.cy = -m_Rulers[edVert].GetCellMargin();
    m_Dcc.m_PaintRect = rect;
    m_Dcc.m_CharSize[edHorz] = m_Rulers[edHorz].GetCharSize();
    m_Dcc.m_CharSize[edVert] = m_Rulers[edVert].GetCharSize();


    CDC memDc;
    memDc.CreateCompatibleDC(&dc);
    CFont* font = m_Dcc.m_Dc->GetCurrentFont();
    CFont* pOrgFont = memDc.SelectObject(font);
    m_Dcc.m_Dc = &memDc;

    for (int j = cellRanges[1]; j <= cellRanges[3]; j++)
    {
        int jj = m_Rulers[edVert].ToAbsolute(j);
        m_Rulers[edVert].GetInnerLoc(j, m_Dcc.m_Rect.top, m_Dcc.m_Rect.bottom, m_Dcc.m_Type[1]);

        CBitmap bitmap;
        CRect rowRc(rect.left, m_Dcc.m_Rect.top, rect.right, m_Dcc.m_Rect.bottom + m_Rulers[edVert].GetLineSize());
        bitmap.CreateCompatibleBitmap(&dc, rowRc.Width(), rowRc.Height());
        CBitmap *pOrgBitmap = memDc.SelectObject(&bitmap);
        memDc.SetViewportOrg(-rowRc.left, -rowRc.top);
        //memDc.SetROP2(R2_NOTXORPEN);
        memDc.SetBkMode(TRANSPARENT);
        memDc.FillSolidRect(rowRc, m_Dcc.m_BkColor[0]);

        int firstSelCol, lastSelCol, firstSelRow, lastSelRow;
        bool colSelection = m_Rulers[edHorz].GetNormalizedSelection(firstSelCol, lastSelCol);
        bool rowSelection = m_Rulers[edVert].GetNormalizedSelection(firstSelRow, lastSelRow);

        for (int i = cellRanges[0]; i <= cellRanges[2]; i++)
        {
            int ii = m_Rulers[edHorz].ToAbsolute(i);
            m_Rulers[edHorz].GetInnerLoc(i, m_Dcc.m_Rect.left, m_Dcc.m_Rect.right, m_Dcc.m_Type[0]);
            
            if (m_Dcc.IsDataCell()) // Data cells
            {
                if (i <= m_Rulers[edHorz].GetLastDataItem()
                && j <= m_Rulers[edVert].GetLastDataItem())
                {
                    bool cur_col = (ii == m_Rulers[edHorz].GetCurrent()) ? true : false;
                    bool cur_row = (jj == m_Rulers[edVert].GetCurrent()) ? true : false;

                    if (cur_col && cur_row) m_Dcc.m_Select = IsEditMode() ? esBorder : esRect;
                    else if (colSelection || rowSelection)
                    {
                        int sel = 0;

                        if (!colSelection || (firstSelCol <= ii && ii <= lastSelCol))
                            sel |= 0x01;
                        if (!rowSelection || (firstSelRow <= jj && jj <= lastSelRow))
                            sel |= 0x02;

                        m_Dcc.m_Select = (sel == 0x03) ? esRect : esNone;
                    }
                    else if (cur_col && m_Options.m_ColSelect) m_Dcc.m_Select = esRect;
                    else if (cur_row && m_Options.m_RowSelect) m_Dcc.m_Select = esRect;
                    else m_Dcc.m_Select = esNone;

                    m_Dcc.m_Col = ii;
                    m_Dcc.m_Row = jj;

                    m_pSource->PaintCell(m_Dcc);
                }
                else
                    drawUnused(*m_Dcc.m_Dc, m_Dcc.m_Rect);
            } 
            else // Fixed cells
            { 
                m_Dcc.m_Select = esNone;

                if (m_Dcc.m_Type[0] == efNone && (firstSelCol <= ii && ii <= lastSelCol))
                    m_Dcc.m_Select = esRect;
                if (m_Dcc.m_Type[1] == efNone && (firstSelRow <= jj && jj <= lastSelRow))
                    m_Dcc.m_Select = esRect;

                if (!m_Dcc.IsCornerCell())
                {
                    if (m_Rulers[edHorz].IsDataItem(i)
                    || m_Rulers[edVert].IsDataItem(j))
                    {
                        m_Dcc.m_Col = m_Dcc.m_Type[0] == efNone ? ii : i;
                        m_Dcc.m_Row = m_Dcc.m_Type[1] == efNone ? jj : j;
                        m_pSource->PaintCell(m_Dcc);
                    }
                    else
                        drawUnused(*m_Dcc.m_Dc, m_Dcc.m_Rect);
                } 
                else 
                {
                    m_Dcc.m_Col =
                    m_Dcc.m_Row = -1;
                    m_pSource->PaintCell(m_Dcc);
                }
            }
        }

        if (!m_Options.m_PaleColors)
        {
            dc.BitBlt(rowRc.left, rowRc.top, rowRc.Width(), rowRc.Height(),
                &memDc, rowRc.left, rowRc.top, SRCCOPY);
        }
        else
        {
            /* Alpha blending -  for read only painting */
            static BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255-32, 0/*AC_SRC_ALPHA*/};

            dc.FillSolidRect(rowRc.left, rowRc.top, rowRc.Width(), rowRc.Height(), RGB(95,95,95));

            dc.AlphaBlend(rowRc.left, rowRc.top, rowRc.Width(), rowRc.Height(),
                &memDc, rowRc.left, rowRc.top, rowRc.Width(), rowRc.Height(), blend);
        }

        memDc.SelectObject(pOrgBitmap);
    }
    memDc.SelectObject(pOrgFont);
    m_Dcc.m_Dc = &dc;

    int lim[4];
    m_Rulers[edHorz].GetDataLoc(lim);
    m_Rulers[edVert].GetDataLoc(lim+2);
    drawLines(edHorz, dc, lim, m_Dcc.m_FixBkColor);
    drawLines(edVert, dc, lim, m_Dcc.m_FixBkColor);

    if (m_Rulers[edVert].GetFirstFixCount())
    {
        m_Rulers[edHorz].GetLeftFixDataLoc(lim);
        m_Rulers[edVert].GetLeftFixLoc(lim+2);
        drawLines(edHorz, dc, lim, m_Dcc.m_FixTextColor);
        drawLines(edVert, dc, lim, m_Dcc.m_FixTextColor);
    }
    if (m_Rulers[edHorz].GetFirstFixCount())
    {
        m_Rulers[edHorz].GetLeftFixLoc(lim);
        m_Rulers[edVert].GetLeftFixDataLoc(lim+2);
        drawLines(edHorz, dc, lim, m_Dcc.m_FixTextColor);
        drawLines(edVert, dc, lim, m_Dcc.m_FixTextColor);
    }
    if (m_Rulers[edVert].GetLastFixCount())
    {
        m_Rulers[edHorz].GetLeftFixDataLoc(lim);
        m_Rulers[edVert].GetRightFixLoc(lim+2);
        drawLines(edHorz, dc, lim, m_Dcc.m_FixTextColor);
        drawLines(edVert, dc, lim, m_Dcc.m_FixTextColor);
        lim[3] = lim[2];
        lim[2] -= m_Rulers[edVert].GetLineSize();
        drawLines(edVert, dc, lim, m_Dcc.m_FixTextColor);
    }
    if (m_Rulers[edHorz].GetLastFixCount())
    {
        m_Rulers[edHorz].GetRightFixLoc(lim);
        m_Rulers[edVert].GetLeftFixDataLoc(lim+2);
        drawLines(edHorz, dc, lim, m_Dcc.m_FixTextColor);
        drawLines(edVert, dc, lim, m_Dcc.m_FixTextColor);
        lim[1] = lim[0];
        lim[0] -= m_Rulers[edHorz].GetLineSize();
        drawLines(edHorz, dc, lim, m_Dcc.m_FixTextColor);
    }

    DrawFocus(dc);

    } catch (const out_of_range& x) {
        CRect rc;
        m_pClientWnd->GetClientRect(rc);
        dc.FillSolidRect(rect, RGB(255,255,255));
        ostringstream o;
        o << "Unexpected exception \"" << x.what() << "\" is being caught in PaintGridManager::Paint.\n\n"
             "Due to this problem, I recommend save the file and open it in a new window.\n"
             "Please make a post about this problem in <http://www.sqltools.net/cgi-bin/yabb2/YaBB.pl>."
             "Thank you\n";
        string text = o.str();
        dc.SetBkColor(RGB(255,255,255));
        dc.SetTextColor(RGB(255,0,0));
        ::DrawTextA(dc.m_hDC, text.data(), text.length(), rc, DT_LEFT|DT_TOP);
    }
}

void PaintGridManager::RepaintAll () const
{
    m_pClientWnd->Invalidate(FALSE);
}

void PaintGridManager::RepaintCurCol () const
{
    int loc[4];
    m_Rulers[edHorz].GetCurrentInnerLoc(loc);
    m_Rulers[edVert].GetClientArea(loc+2);
    m_pClientWnd->InvalidateRect(CRect(loc[0],loc[2],loc[1],loc[3]), FALSE);
}

void PaintGridManager::RepaintCurRow () const
{
    int loc[4];
    m_Rulers[edHorz].GetClientArea(loc);
    m_Rulers[edVert].GetCurrentInnerLoc(loc+2);
    m_pClientWnd->InvalidateRect(CRect(loc[0],loc[2],loc[1],loc[3]), FALSE);
}

void PaintGridManager::InvalidateRows (int startRow, int endRow)
{
    if (startRow > endRow)
        swap(startRow, endRow);

    int top = m_Rulers[edVert].GetTopmost();
    int bottom  = top + m_Rulers[edVert].GetDataItemCount() - 1;

    if (startRow > bottom || endRow < top) return;

    bottom = min(endRow, bottom) - top;
    top = max(startRow, top) - top;

    int clientX[2];
    m_Rulers[edHorz].GetClientArea(clientX);

    int topY[2];
    m_Rulers[edVert].GetInnerLoc(top, topY);

    int bottomY[2];
    m_Rulers[edVert].GetInnerLoc(bottom, bottomY);

    m_pClientWnd->InvalidateRect(CRect(clientX[0], topY[0], clientX[1], bottomY[1]), FALSE);
}

void PaintGridManager::InvalidateCols (int startCol, int endCol)
{
    if (startCol > endCol)
        swap(startCol, endCol);

    int left = m_Rulers[edVert].GetTopmost();
    int right  = left + m_Rulers[edVert].GetDataItemCount() - 1;

    if (startCol > right || endCol < left) return;

    left = max(startCol, left);
    right = min(endCol, right);

    int clientY[2];
    m_Rulers[edVert].GetClientArea(clientY);

    int leftX[2];
    m_Rulers[edHorz].GetInnerLoc(left, leftX);

    int rightX[2];
    m_Rulers[edHorz].GetInnerLoc(right, rightX);

    m_pClientWnd->InvalidateRect(CRect(leftX[0], clientY[0], rightX[1], clientY[1]), FALSE);
}

void PaintGridManager::OnChangedRows (int startRow, int endRow)
{
    _ASSERTE(startRow <= endRow);

    if ( ((startRow <= m_Rulers[edVert].GetTopmost()) && (m_Rulers[edVert].GetTopmost() <= endRow))
       || (m_Rulers[edVert].GetTopmost() <= startRow) && (startRow <= (m_Rulers[edVert].GetTopmost() + m_Rulers[edVert].GetCount()))
       )
    {
        // changed to RecalcRuller because after Append row data can be assigned a multiline text
        RecalcRuller(edVert, false/*adjust*/, true/*invalidate*/);
        //InvalidateRows(startRow, endRow);
    }
}

void PaintGridManager::OnChangedCols (int startCol, int endCol)
{
    _ASSERTE(startCol <= endCol);

    if ( ((startCol <= m_Rulers[edHorz].GetTopmost()) && (m_Rulers[edHorz].GetTopmost() <= endCol))
       || (m_Rulers[edHorz].GetTopmost() <= startCol) && (startCol <= (m_Rulers[edHorz].GetTopmost() + m_Rulers[edHorz].GetCount()))
       )
    {
        RecalcRuller(edHorz, false/*adjust*/, true/*invalidate*/);
        //InvalidateRows(startCol, endCol);
    }
}

void PaintGridManager::OnChangedRowsQuantity (int row, int quantity)
{
    m_Rulers[edVert].OnChangedQuantity(row, quantity);
}

void PaintGridManager::OnChangedColsQuantity (int col, int quantity)
{
    m_Rulers[edHorz].OnChangedQuantity(col, quantity);
}

}//namespace OG2
