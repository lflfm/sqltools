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

#include "stdafx.h"
#include "GridManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2
{

    static HCURSOR g_hSizeCurs[2]  = { NULL, NULL };
    static LPCWSTR g_pszCursName[2] = { L"IDC_COL_SIZE", L"IDC_ROW_SIZE" };

ResizeGridManager::ResizeGridManager (CWnd* client , AGridSource* source, bool shouldDeleteSource)
: EditGridManager(client, source, shouldDeleteSource),
  m_Capture(false), m_oldPos(-1,-1),
  m_ResizingItem(-1)
{
    for (int i(0); i < 2; i++)
    {
        if (!g_hSizeCurs[i])
            g_hSizeCurs[i] = AfxGetApp()->LoadCursor(g_pszCursName[i]);
    }
}

void ResizeGridManager::EvLButtonDown (unsigned modKeys, const CPoint& point)
{
    if (checkMousePos(edVert, point, m_ResizingItem))
    {
        m_Capture     = true;
        m_CaptureType = edVert;
    }
    else if (checkMousePos(edHorz, point, m_ResizingItem))
    {
        m_Capture     = true;
        m_CaptureType = edHorz;
    }

    if (!m_Capture)
        EditGridManager::EvLButtonDown(modKeys, point);
    else
    {
        int lim[6];
        if (m_CaptureType == edHorz)
        {
            m_Rulers[edHorz].GetInnerLoc(m_ResizingItem,lim);
            m_Rulers[edVert].GetClientArea(lim+2);
            m_Rulers[edHorz].GetClientArea(lim+4);
            lim[1]=lim[5];
        }
        else
        {
            m_Rulers[edHorz].GetClientArea(lim);
            m_Rulers[edVert].GetInnerLoc(m_ResizingItem,lim+2);
            m_Rulers[edVert].GetClientArea(lim+4);
            lim[3]=lim[5];
        }
        CRect clntRc;
        m_pClientWnd->GetClientRect(clntRc);
        CPoint ltp(lim[0],lim[2]), rbp(lim[1],lim[3]);
        m_pClientWnd->ClientToScreen(&ltp);
        m_pClientWnd->ClientToScreen(&rbp);
        ClipCursor(CRect(ltp,rbp));
        m_pClientWnd->SetCapture();
    }
}

void ResizeGridManager::LightRefresh (eDirection dir)
{
    if (m_Rulers[dir].GetClientSize() > 0)
        RecalcRuller(dir, /*adjust*/false, false/*invalidate*/);

    if (!m_Rulers[dir].IsCurrentFullVisible())
        setScrollMode(true);

    m_pClientWnd->Invalidate(FALSE);
    InvalidateEdit();
}

void ResizeGridManager::EvLButtonUp (unsigned /*modKeys*/, const CPoint& point)
{
    if (m_Capture)
    {
        drawResizeLine(point);
        m_oldPos.x = m_oldPos.y = -1;
        ReleaseCapture();
        ClipCursor(NULL);
        m_Capture = false;

        int lim[2];
        m_Rulers[m_CaptureType].GetLoc(m_ResizingItem, lim);
        int size = ((m_CaptureType == edHorz) ? point.x : point.y) - lim[0];

        if ((m_CaptureType == edVert && m_Options.m_RowSizingAsOne)
        || (m_CaptureType == edHorz && m_Options.m_ColSizingAsOne))
        {
            m_Rulers[m_CaptureType].SetSizeAsOne(size);
        }
        else
        {
            int item = m_Rulers[m_CaptureType].ToAbsolute(m_ResizingItem);
            ASSERT(item >= 0); // redundent check, it never gonna happen
            m_Rulers[m_CaptureType].SetPixelSize(item >= 0 ? item : 0, size, true);
        }
        LightRefresh(m_CaptureType);
    }
}

void ResizeGridManager::EvMouseMove (unsigned /*modKeys*/, const CPoint& point)
{
    if (!m_Capture)
    {
        int dummy;
        checkMousePos(edVert, point, dummy);
        checkMousePos(edHorz, point, dummy);
    } else
        drawResizeLine(point);
}

bool ResizeGridManager::checkMousePos (eDirection dir, const CPoint& point, int& item) const
{
    int lim[4];
    if (dir == edHorz)
    {
        m_Rulers[edHorz].GetLeftFixDataLoc(lim);
        m_Rulers[edVert].GetLeftFixLoc(lim+2);
        if (!m_Options.m_ColSizing)
            return false;
    }
    else
    {
        m_Rulers[edHorz].GetLeftFixLoc(lim);
        m_Rulers[edVert].GetLeftFixDataLoc(lim+2);
        if (!m_Options.m_RowSizing)
            return false;
    }
    if (point.x >= lim[0] && point.x < lim[1]
        && point.y >= lim[2] && point.y < lim[3])
    {
        int pt = (dir == edHorz) ? point.x : point.y;
        item = m_Rulers[dir].PointToItem(pt);
        m_Rulers[dir].GetLoc(item, lim);
        if (((item > m_Rulers[dir].GetFirstFixCount()) && (pt - lim[0] <= 4))
        ||  ((item >= m_Rulers[dir].GetFirstFixCount()) && (lim[1] - pt <= 4)))
        {
            if ((item > m_Rulers[dir].GetFirstFixCount())
            && (pt - lim[0] <= 4)) item--;
            SetCursor(g_hSizeCurs[dir]);
            return true;
        }
    }
    return false;
}

void ResizeGridManager::drawResizeLine (const CPoint& point)
{
    CRect clRc;
    m_pClientWnd->GetClientRect(clRc);
    SetCursor(g_hSizeCurs[m_CaptureType]);
    CClientDC dc(m_pClientWnd);
    dc.SetROP2(R2_NOTXORPEN);
    if (m_CaptureType == edHorz)
    {
        dc.MoveTo(m_oldPos.x, 0);
        dc.LineTo(m_oldPos.x, clRc.bottom);
        m_oldPos.x = point.x;
        dc.MoveTo(m_oldPos.x, 0);
        dc.LineTo(m_oldPos.x, clRc.bottom);
    }
    else
    {
        dc.MoveTo(0,          m_oldPos.y);
        dc.LineTo(clRc.right, m_oldPos.y);
        m_oldPos.y = point.y;
        dc.MoveTo(0,          m_oldPos.y);
        dc.LineTo(clRc.right, m_oldPos.y);
    }
}

void ResizeGridManager::EnableRowSizingAsOne (bool bEnable)
{
    if (m_Options.m_RowSizingAsOne != bEnable)
    {
        m_Options.m_RowSizingAsOne = bEnable;
        LightRefresh(edVert);
    }
}

void ResizeGridManager::EnableColSizingAsOne (bool bEnable)
{
    if (m_Options.m_ColSizingAsOne != bEnable)
    {
        m_Options.m_ColSizingAsOne = bEnable;
        LightRefresh(edHorz);
    }
}

void ResizeGridManager::SetRowSizeForAsOne (int nHeight)
{
    if (m_Rulers[edVert].GetSizeAsOne() != nHeight)
    {
        m_Rulers[edVert].SetSizeAsOne(nHeight);

        if (m_Options.m_ColSizingAsOne)
            LightRefresh(edVert);
    }
}

void ResizeGridManager::SetColSizeForAsOne (int nWidth)
{
    if (m_Rulers[edVert].GetSizeAsOne() != nWidth)
    {
        m_Rulers[edVert].SetSizeAsOne(nWidth);

        if (m_Options.m_ColSizingAsOne)
            LightRefresh(edHorz);
    }
}

}//namespace OG2

