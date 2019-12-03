/* 
    This code contributed by Ken Clubok

	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2005 Aleksey Kochetov

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

// 13.03.2005 (ak) R1105003: Bind variables

#include "stdafx.h"
#include "resource.h"

#include "SQLTools.h"

/*
#include <COMMON\ExceptionHelper.h>
#include <OCI8\ACursor.h>
*/
#include "OpenGrid\GridManager.h"
#include "BindSource.h"
#include "BindGridView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace OG2;

/////////////////////////////////////////////////////////////////////////////
// GridView

IMPLEMENT_DYNCREATE(BindGridView, GridView)

/** @brief Default constructor for BindGridView.
 *  Sets up visual attributes for grid.
 */
BindGridView::BindGridView()
{
    SetVisualAttributeSetName("Bind Window");

	m_pBindSource = new BindGridSource;
	m_pManager = new GridManager(this, m_pBindSource, FALSE);

    m_pBindSource->SetCols(3);
    m_pBindSource->SetColCharWidth(0, 15);
    m_pBindSource->SetColCharWidth(1, 15);
    m_pBindSource->SetColCharWidth(2, 20);
    m_pBindSource->SetColHeader(0, "Name");
    m_pBindSource->SetColHeader(1, "Type");
    m_pBindSource->SetColHeader(2, "Value");
    m_pBindSource->SetShowRowEnumeration(true);
    //m_pBindSource->SetShowTransparentFixCol(true);

    m_pBindSource->SetFixSize(eoVert, edHorz, 3);

    m_pBindSource->SetMaxShowLines(6);

	m_pManager->m_Options.m_Editing = true;
    m_pManager->m_Options.m_HorzLine = true;
    m_pManager->m_Options.m_VertLine = true;
    m_pManager->m_Options.m_RowSelect = true;
    m_pManager->m_Options.m_ExpandLastCol = true;
}

/** @brief Default destructor for BindGridSource.
 */
BindGridView::~BindGridView()
{
    try { EXCEPTION_FRAME;

        delete m_pManager;
        delete m_pBindSource;
    }
    _DESTRUCTOR_HANDLER_;
}

/** @brief See BindGridSource::Refresh().
 */
void BindGridView::Refresh(const vector<string>& data)
{
	m_pBindSource->Refresh(data);
    m_pManager->Refresh();
}
