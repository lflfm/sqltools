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
#include "SQLTools.h"
#include "ConnectionBar.h"


    static UINT BASED_CODE styles[] =
    {
	    ID_SQL_CONNECT,
	    ID_SQL_DISCONNECT,
        ID_SQL_RECONNECT,
	    ID_SEPARATOR,
	    ID_SEPARATOR, // for indicator
    };

    static const int cnIndicatorInx = 4;
    static const int cnIndicatorId  = 111;
    static const CSize g_buttonSize = CSize(19, 19);
    static const CRect g_toolbarBorders = CRect(1, 1, 1, 1);

// ConnectionBar
/*
IMPLEMENT_DYNAMIC(ConnectionBar, CToolBar)
*/
ConnectionBar::ConnectionBar()
{
	m_btnDelta.cx = m_sizeButton.cx - m_sizeImage.cx;
	m_btnDelta.cy = m_sizeButton.cy - m_sizeImage.cy;
}

ConnectionBar::~ConnectionBar()
{
}

BOOL ConnectionBar::Create (CWnd* pParent, UINT id)
{
	if (!CreateEx(pParent, 0, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, g_toolbarBorders, id)
    || !LoadBitmap(IDB_CONNECT)
    || !SetButtons(styles, sizeof(styles)/sizeof(UINT)))
	{
        TRACE0("ConnectionBar::Create: CreateEx failed\n");
		return FALSE;
	}
    ModifyStyle(0, TBSTYLE_FLAT);
    EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);

    SetSizes(CSize(g_buttonSize.cx + m_btnDelta.cx, g_buttonSize.cy + m_btnDelta.cy), g_buttonSize);
	SetButtonInfo(cnIndicatorInx, cnIndicatorId, TBBS_SEPARATOR, 240);

    CRect rect;
	GetItemRect(cnIndicatorInx, &rect);
    rect.InflateRect(CSize(-2, -2));

    ModifyStyleEx(0, WS_EX_COMPOSITED); // to impove animation smoothness
	if (!m_wndLabel.CreateEx(WS_EX_STATICEDGE, "Static", NULL,
            WS_CHILDWINDOW|WS_VISIBLE|SS_OWNERDRAW, rect, this, cnIndicatorId))
	{
        TRACE0("ConnectionBar::Create: m_wndLabel.CreateEx failed\n");
		return FALSE;
	}

	HFONT hfont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

    if (!hfont)
        hfont = (HFONT)::GetStockObject(SYSTEM_FONT);

    m_wndLabel.SendMessage(WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);

    return TRUE;
}

/*
BEGIN_MESSAGE_MAP(ConnectionBar, CToolBar)
END_MESSAGE_MAP()
*/

// ConnectionBar message handlers

void CConnectionLabel::SetConnectionDescription (LPCSTR pText, COLORREF color)
{
    m_text = pText;
    m_textColor = color;
    if (m_hWnd) Invalidate(TRUE);
}

void CConnectionLabel::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if (lpDrawItemStruct)
    {
        HBRUSH hbrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
        ::FillRect(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem, hbrush);
        ::DeleteObject(hbrush);

        ::SetTextColor(lpDrawItemStruct->hDC, m_textColor);

        RECT rc = lpDrawItemStruct->rcItem;
        ::DrawText(lpDrawItemStruct->hDC, m_text, -1, &rc, DT_CALCRECT|DT_VCENTER|DT_SINGLELINE);

        BOOL fit = ((rc.right - rc.left) < (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left)) ? TRUE : FALSE;

        ::DrawText(lpDrawItemStruct->hDC, m_text, -1, &lpDrawItemStruct->rcItem, fit ? DT_CENTER|DT_VCENTER|DT_SINGLELINE : DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    }
}