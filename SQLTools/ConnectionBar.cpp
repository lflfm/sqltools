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
#include "SQLTools.h"
#include "ConnectionBar.h"
#include "ConnectionButton.h"

// ConnectionBar
/*
IMPLEMENT_DYNAMIC(ConnectionBar, CMFCToolBar)
*/
ConnectionBar::ConnectionBar()
{
}

ConnectionBar::~ConnectionBar()
{
}

BOOL ConnectionBar::Create (CWnd* pParent, UINT id)
{
	if (!CreateEx(pParent, TBSTYLE_FLAT, 
        WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, 
        CRect(1,1,1,1), id)
    || !LoadToolBar(IDT_CONNECT)
    )
	{
        TRACE0("ConnectionBar::Create: CreateEx failed\n");
		return FALSE;
	}
    EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);

    ModifyStyleEx(0, WS_EX_COMPOSITED); // to impove animation smoothness

	ConnectionButton comboButtonConfig(ID_SQL_CONN_DESCRIPTION, GetCmdMgr()->GetCmdImage(ID_SQL_CONN_DESCRIPTION, FALSE), CBS_DROPDOWNLIST);
	ReplaceButton(ID_SQL_CONN_DESCRIPTION, comboButtonConfig);

    return TRUE;
}

/*
BEGIN_MESSAGE_MAP(ConnectionBar, CMFCToolBar)
END_MESSAGE_MAP()
*/

LabelWithWaitBar* ConnectionBar::GetLabel () const
{
	ConnectionButton* pConnectionBar = NULL;

	CObList listButtons;
	if (CMFCToolBar::GetCommandButtons(ID_SQL_CONN_DESCRIPTION, listButtons) > 0)
	{
		for (POSITION pos = listButtons.GetHeadPosition(); pConnectionBar == NULL && pos != NULL; )
			pConnectionBar = DYNAMIC_DOWNCAST(ConnectionButton, listButtons.GetNext(pos));
	}

    return pConnectionBar ? pConnectionBar->GetLabel() : NULL;
}

const CString ConnectionBar::GetText() const 
{ 
    if (LabelWithWaitBar* label = GetLabel())
        label->GetText();

    return L"";
}

void ConnectionBar::SetConnectionDescription (LPCTSTR pText, COLORREF color)
{ 
    if (LabelWithWaitBar* label = GetLabel())
        label->SetText(pText, color);
}

void ConnectionBar::SetColor (COLORREF color)
{ 
    if (LabelWithWaitBar* label = GetLabel())
        label->SetText(label->GetText(), color); 
}

void ConnectionBar::StartAnimation (bool animation)
{ 
    if (LabelWithWaitBar* label = GetLabel())
        label->StartAnimation(animation); 
}

