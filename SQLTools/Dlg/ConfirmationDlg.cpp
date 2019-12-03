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
#include "ConfirmationDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CConfirmationDlg::CConfirmationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfirmationDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfirmationDlg)
//	m_strText = _T("");
	//}}AFX_DATA_INIT
}


void CConfirmationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfirmationDlg)
	DDX_Text(pDX, ID_CNF_TEXT, m_strText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfirmationDlg, CDialog)
	//{{AFX_MSG_MAP(CConfirmationDlg)
	ON_BN_CLICKED(IDYES, OnYes)
	ON_BN_CLICKED(ID_CNF_ALL, OnAll)
	ON_BN_CLICKED(IDNO, OnNo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfirmationDlg message handlers

void CConfirmationDlg::OnYes() 
{
    EndDialog(IDYES);
}

void CConfirmationDlg::OnAll() 
{
    EndDialog(ID_CNF_ALL);
}

void CConfirmationDlg::OnNo() 
{
    EndDialog(IDNO);
}
