/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2019 Aleksey Kochetov

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
#include "DockableContainer.h"


// DockableContainer

IMPLEMENT_DYNAMIC(DockableContainer, CDockablePane)

DockableContainer::DockableContainer()
    : m_pClient(NULL), m_bCanBeTabbedDocument(FALSE)
{

}

DockableContainer::~DockableContainer()
{
}


BEGIN_MESSAGE_MAP(DockableContainer, CDockablePane)
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
END_MESSAGE_MAP()

// DockableContainer message handlers

void DockableContainer::OnSize (UINT nType, int cx, int cy)
{
    CDockablePane::OnSize(nType, cx, cy);

    if (m_pClient != NULL
    && cx > 0 && cy > 0
    && nType != SIZE_MAXHIDE && nType != SIZE_MINIMIZED) 
    {
        CRect rect;
        GetClientRect(&rect);
        m_pClient->SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOZORDER);
    }
}


void DockableContainer::OnSetFocus(CWnd* pOldWnd)
{
    CDockablePane::OnSetFocus(pOldWnd);

    if (m_pClient != NULL)
        m_pClient->SetFocus();
}
