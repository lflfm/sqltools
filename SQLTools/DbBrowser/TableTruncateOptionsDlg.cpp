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

// TableTruncateOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SQLTools.h"
#include "TableTruncateOptionsDlg.h"


// CTableTruncateOptionsDlg dialog

IMPLEMENT_DYNAMIC(CTableTruncateOptionsDlg, CDialog)
CTableTruncateOptionsDlg::CTableTruncateOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTableTruncateOptionsDlg::IDD, pParent)
    /*
    , m_checkDependencies(FALSE)
    , m_disableFKs(FALSE)
    , m_scriptOnly(FALSE)
    , m_keepAllocated(FALSE)
    */
{
    m_checkDependencies = AfxGetApp()->GetProfileInt("TableTruncateOptions", "CheckDependencies",  TRUE);
    m_disableFKs        = AfxGetApp()->GetProfileInt("TableTruncateOptions", "DisableFKs", TRUE);
    m_scriptOnly        = AfxGetApp()->GetProfileInt("TableTruncateOptions", "ScriptOnly", FALSE);
    m_keepAllocated     = AfxGetApp()->GetProfileInt("TableTruncateOptions", "KeepAllocated", FALSE);
}

CTableTruncateOptionsDlg::~CTableTruncateOptionsDlg()
{
}

void CTableTruncateOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_DBT_CHECK_DEPENDENCIES, m_checkDependencies);
    DDX_Check(pDX, IDC_DBT_DISABLE_FKS, m_disableFKs);
    DDX_Check(pDX, IDC_DBT_SCRIPT_ONLY, m_scriptOnly);
    DDX_Check(pDX, IDC_DBT_KEEP_ALLOCATED, m_keepAllocated);
}


//BEGIN_MESSAGE_MAP(CTableTruncateOptionsDlg, CDialog)
//END_MESSAGE_MAP()


// CTableTruncateOptionsDlg message handlers

void CTableTruncateOptionsDlg::OnOK()
{
    CDialog::OnOK();
    AfxGetApp()->WriteProfileInt("TableTruncateOptions", "CheckDependencies",  m_checkDependencies);
    AfxGetApp()->WriteProfileInt("TableTruncateOptions", "DisableFKs",         m_disableFKs       );
    AfxGetApp()->WriteProfileInt("TableTruncateOptions", "ScriptOnly",         m_scriptOnly       );
    AfxGetApp()->WriteProfileInt("TableTruncateOptions", "KeepAllocated",      m_keepAllocated    );
}

LRESULT CTableTruncateOptionsDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}
