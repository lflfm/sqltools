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

// DeleteTableDataDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SQLTools.h"
#include "DeleteTableDataDlg.h"


// CDeleteTableDataDlg dialog

IMPLEMENT_DYNAMIC(CDeleteTableDataDlg, CDialog)
CDeleteTableDataDlg::CDeleteTableDataDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDeleteTableDataDlg::IDD, pParent)
    /*
    , m_commit(0)
    , m_commitAfter(0)
    */
{
    m_commit      = AfxGetApp()->GetProfileInt(L"DeleteTableDataOptions", L"Commit",  0);
    m_commitAfter = AfxGetApp()->GetProfileInt(L"DeleteTableDataOptions", L"CommitAfter", 0);
}

CDeleteTableDataDlg::~CDeleteTableDataDlg()
{
}

void CDeleteTableDataDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Radio(pDX, IDC_DBD_DONT_COMMIT, m_commit);
    DDX_Text(pDX, IDC_DBD_COMMIT_AFTER_ROW, m_commitAfter);
}


//BEGIN_MESSAGE_MAP(CDeleteTableDataDlg, CDialog)
//END_MESSAGE_MAP()

// CDeleteTableDataDlg message handlers

void CDeleteTableDataDlg::OnOK()
{
    CDialog::OnOK();
    AfxGetApp()->WriteProfileInt(L"DeleteTableDataOptions", L"Commit",  m_commit);
    AfxGetApp()->WriteProfileInt(L"DeleteTableDataOptions", L"CommitAfter", m_commitAfter);
}

LRESULT CDeleteTableDataDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}
