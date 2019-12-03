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

// SQLWorksheet\ExplainPlanTextView.cpp : implementation file
//

#include "stdafx.h"
#include "SQLTools.h"
#include "SQLWorksheet\ExplainPlanTextView.h"


// CExplainPlanTextView
//IMPLEMENT_DYNCREATE(CExplainPlanTextView, CEditView)

CExplainPlanTextView::CExplainPlanTextView()
{
}

CExplainPlanTextView::~CExplainPlanTextView()
{
}

BEGIN_MESSAGE_MAP(CExplainPlanTextView, CEditView)
    ON_CONTROL_REFLECT(EN_CHANGE, OnEditChange)
END_MESSAGE_MAP()

//
// CExplainPlanTextView message handlers
//
void CExplainPlanTextView::OnEditChange()
{
    // do nothing
}

void CExplainPlanTextView::OnInitialUpdate()
{
    CEditView::OnInitialUpdate();

    VisualAttribute attr;

	m_Font.CreateFont(
          -attr.PointToPixel(9), 0, 0, 0,
          FW_NORMAL,
          0,
          0,
          0, ANSI_CHARSET,//DEFAULT_CHARSET,
          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
          "Courier New"
        );

	SetFont(&m_Font);

    GetEditCtrl().SetReadOnly();
}
