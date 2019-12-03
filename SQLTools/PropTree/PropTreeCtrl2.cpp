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
#include "PropTree\PropTreeCtrl2.h"
#include "OpenGrid\GridSourceBase.h"
#include "COMMON/GUICommandDictionary.h"


PropTreeCtrl2::PropTreeCtrl2()
//: m_accelTable(NULL)
{
    OG2::GridStringSource* source = new OG2::GridStringSource;

    source->SetShowFixCol(true);
    source->SetFixSize(OG2::eoVert, OG2::edHorz, 1);
    source->SetCols(2);
    source->SetColCharWidth(0, 15);
    source->SetColHeader(0, "Name");
    source->SetColCharWidth(1, 15);
    source->SetColHeader(1, "Value");
    source->SetMaxShowLines(1);
    source->SetShowRowEnumeration(false);
    source->SetShowTransparentFixCol(true);

    m_ShouldDeleteManager = true;
    m_pManager = new OG2::GridManager(this, source, true);
    m_pManager->m_Options.m_RowSelect     = true;
    m_pManager->m_Options.m_HorzLine      = false;
    m_pManager->m_Options.m_VertLine      = false;
    m_pManager->m_Options.m_ExpandLastCol = true;
    m_pManager->m_Options.m_HorzScroller  = true;
    m_pManager->m_Options.m_VertScroller  = true;
    m_pManager->m_Options.m_CellFocusRect = true;

    SetVisualAttributeSetName("History Window");
}

PropTreeCtrl2::~PropTreeCtrl2()
{
    //try { EXCEPTION_FRAME;

	   // if (m_accelTable)
		  //  DestroyAcceleratorTable(m_accelTable);
    //}
    //_DESTRUCTOR_HANDLER_
}

void PropTreeCtrl2::ShowProperties (std::vector<std::pair<string, string> >& properties, bool /*readOnly = true*/)
{
	m_pManager->m_pSource->DeleteAll();

    std::vector<std::pair<string, string> >::const_iterator it = properties.begin();

    //OG2::AGridSource::NotificationDisabler disabler(m_pManager->m_pSource);

    for (; it != properties.end(); ++it)
    {
        int row = m_pManager->m_pSource->Append();
        ((OG2::GridStringSource*)m_pManager->m_pSource)->SetString(row, 0, it->first.c_str());
	    ((OG2::GridStringSource*)m_pManager->m_pSource)->SetString(row, 1, it->second.c_str());
    }

    //disabler.Enable();
    //m_pManager->m_pSource->Notify_ChangedRowsQuantity(row, 1);
    //ApplyColumnFit();
    
    //make sure that scroller is shown!
}

//BOOL PropTreeCtrl2::PreTranslateMessage(MSG* pMsg)
//{
////	if (m_accelTable
////    && TranslateAccelerator(AfxGetMainWnd()->m_hWnd, m_accelTable, pMsg))
////        return TRUE;
//
//    return GridWnd::PreTranslateMessage(pMsg);
//}

BEGIN_MESSAGE_MAP(PropTreeCtrl2, OG2::GridWnd)
//    ON_WM_CREATE()
END_MESSAGE_MAP()

// PropTreeCtrl2 message handlers
//int PropTreeCtrl2::OnCreate(LPCREATESTRUCT lpCreateStruct)
//{
//    if (GridWnd::OnCreate(lpCreateStruct) == -1)
//        return -1;
//
//	if (!m_accelTable)
//	{
//		CMenu menu;
//        menu.CreatePopupMenu();
//
//        menu.AppendMenu(MF_STRING, ID_SQL_OBJ_VIEWER, "dummy");
//        menu.AppendMenu(MF_STRING, ID_VIEW_FILE_PANEL, "dummy");
//        menu.AppendMenu(MF_STRING, ID_SQL_DB_SOURCE, "dummy");
//        menu.AppendMenu(MF_STRING, ID_FILE_SHOW_GREP_OUTPUT, "dummy");
//        menu.AppendMenu(MF_STRING, ID_VIEW_PROPERTIES, "dummy");
//
//        //menu.AppendMenu(MF_STRING, ID_EDIT_COPY, "dummy");
//		m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(menu.m_hMenu);
//	}
//
//    return 0;
//}


