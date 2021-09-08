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

#pragma once

#include "stdafx.h"
#include "SQLTools.h"
#include "PropTree\PropTreeCtrl.h"


PropTreeCtrl::PropTreeCtrl()
{
    //SetColumn(130);
}

PropTreeCtrl::~PropTreeCtrl()
{
}

void PropTreeCtrl::ShowProperties (std::vector<std::pair<string, string> >& properties, bool readOnly /*= true*/)
{
	DeleteAllItems();

	CPropTreeItem* pRoot = 0;

	pRoot = InsertItem(new CPropTreeItem());
	pRoot->SetLabelText(_T("Properties"));
	pRoot->SetInfoText(_T("This is a root level item"));
	pRoot->Expand();

    std::vector<std::pair<string, string> >::const_iterator it = properties.begin();

    for (; it != properties.end(); ++it)
    {
	    CPropTreeItemEdit* pEdit;
	    pEdit = (CPropTreeItemEdit*)InsertItem(new CPropTreeItemEdit(), pRoot);
        pEdit->SetLabelText(Common::wstr(it->first).c_str());
	    //pEdit->SetInfoText(_T("Edit text attribute"));
	    pEdit->SetItemValue(Common::wstr(it->second).c_str());
        if (readOnly) pEdit->ReadOnly();
    }
}

BEGIN_MESSAGE_MAP(PropTreeCtrl, CPropTree)
END_MESSAGE_MAP()

// PropTreeCtrl message handlers


