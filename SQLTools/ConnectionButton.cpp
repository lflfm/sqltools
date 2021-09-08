/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2020 Aleksey Kochetov

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
#include "ConnectionButton.h"


IMPLEMENT_SERIAL(ConnectionButton, CMFCToolBarButton, 1)

ConnectionButton::ConnectionButton ()
    : m_iWidth(280), m_pLabel(0)
{
}

ConnectionButton::ConnectionButton (UINT uiId, int iImage, DWORD dwStyle, int iWidth) 
    : CMFCToolBarButton(uiId, iImage), m_pLabel(0)
{
	m_iWidth = (iWidth == 0) ? 280 : iWidth;
}


ConnectionButton::~ConnectionButton ()
{
    if (m_pLabel != NULL)
    {
		m_pLabel->DestroyWindow();
        delete m_pLabel;
    }
}

void ConnectionButton::CopyFrom (const CMFCToolBarButton& s)
{
	CMFCToolBarButton::CopyFrom(s);
}

void ConnectionButton::OnChangeParentWnd (CWnd* pWndParent)
{
	CMFCToolBarButton::OnChangeParentWnd(pWndParent);

	if (m_pLabel->GetSafeHwnd() != NULL)
	{
		CWnd* pWndParentCurr = m_pLabel->GetParent();
		ENSURE(pWndParentCurr != NULL);

		if (pWndParent != NULL && pWndParentCurr->GetSafeHwnd() == pWndParent->GetSafeHwnd())
		{
			return;
		}

		m_pLabel->DestroyWindow();
		delete m_pLabel;
		m_pLabel = NULL;
	}

	if (pWndParent == NULL || pWndParent->GetSafeHwnd() == NULL)
	{
		return;
	}

    // create label here
	CRect rect = m_rect;
	rect.InflateRect(-2, 0);

    m_pLabel = new LabelWithWaitBar();
	if (!m_pLabel->CreateEx(WS_EX_STATICEDGE, L"Static", NULL, WS_CHILDWINDOW|WS_VISIBLE|SS_OWNERDRAW, rect, pWndParent, m_nID))
	{
        TRACE0("ConnectionBar::Create: m_wndLabel.CreateEx failed\n");
		return;
	}

	HFONT hfont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

    if (!hfont)
        hfont = (HFONT)::GetStockObject(SYSTEM_FONT);

    m_pLabel->SendMessage(WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);

}

SIZE ConnectionButton::OnCalculateSize (CDC* pDC, const CSize& sizeDefault, BOOL bHorz)
{
	m_bHorz = bHorz;
	m_sizeText = CSize(0, 0);

	if (bHorz)
		return CSize(m_iWidth, sizeDefault.cy);

    return CMFCToolBarButton::OnCalculateSize(pDC, sizeDefault, bHorz);
}

void ConnectionButton::OnMove ()
{
	if (m_pLabel->GetSafeHwnd() != NULL)
		AdjustRect();
}

void ConnectionButton::OnSize (int iSize)
{
	m_iWidth = iSize;
	m_rect.right = m_rect.left + m_iWidth;

	if (m_pLabel->GetSafeHwnd() != NULL)
		AdjustRect();
}

void ConnectionButton::AdjustRect ()
{
    if (m_pLabel)
	    m_pLabel->SetWindowPos(NULL, m_rect.left + 1, m_rect.top + 1, m_rect.Width() - 2, m_rect.Height() - 2, SWP_NOZORDER | SWP_NOACTIVATE);
}
