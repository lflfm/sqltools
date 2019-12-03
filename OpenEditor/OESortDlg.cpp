/* 
    Copyright (C) 2004 Aleksey Kochetov

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
#include "OESortDlg.h"
#include "Common/DlgDataExt.h"


// COESortDlg dialog
IMPLEMENT_DYNAMIC(COESortDlg, CDialog)
COESortDlg::COESortDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COESortDlg::IDD, pParent)
{
    mKeyOrder           = AfxGetApp()->GetProfileInt("Editor\\Sort", "KeyOrder1", 1);
    mKeyStartColumn     = AfxGetApp()->GetProfileInt("Editor\\Sort", "KeyStartColumn1", 1);
    mKeyLength          = AfxGetApp()->GetProfileInt("Editor\\Sort", "KeyLength1", 0);
    mRemoveDuplicates   = AfxGetApp()->GetProfileInt("Editor\\Sort", "RemoveDuplicates", 0) ? true : false;
    mIgnoreCase         = AfxGetApp()->GetProfileInt("Editor\\Sort", "IgnoreCase", 0) ? true : false;
}

COESortDlg::~COESortDlg()
{
}

void COESortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_OES_ASC, mKeyOrder);
	
    DDX_Text(pDX, IDC_OES_START_COL, mKeyStartColumn);
    DDV_MinMaxInt(pDX, mKeyStartColumn,  1, INT_MAX);
    SendDlgItemMessage(IDC_OES_START_COL_SPIN, UDM_SETRANGE32, 1, INT_MAX);

	DDX_Text(pDX, IDC_OES_KEY_LENGTH, mKeyLength);
    DDV_MinMaxInt(pDX, mKeyLength, 0, INT_MAX);
    SendDlgItemMessage(IDC_OES_KEY_LENGTH_SPIN, UDM_SETRANGE32, 0, INT_MAX);

	DDX_Check(pDX, IDC_OES_REMOVE_DUPLICATES, mRemoveDuplicates);
	DDX_Check(pDX, IDC_OES_IGNORE_CASE, mIgnoreCase);
}

BEGIN_MESSAGE_MAP(COESortDlg, CDialog)
END_MESSAGE_MAP()

// COESortDlg message handlers

void COESortDlg::OnOK()
{
    CDialog::OnOK();

    AfxGetApp()->WriteProfileInt("Editor\\Sort", "KeyOrder1",         mKeyOrder        );
    AfxGetApp()->WriteProfileInt("Editor\\Sort", "KeyStartColumn1",   mKeyStartColumn  );
    AfxGetApp()->WriteProfileInt("Editor\\Sort", "KeyLength1",        mKeyLength       );
    AfxGetApp()->WriteProfileInt("Editor\\Sort", "RemoveDuplicates",  mRemoveDuplicates);
    AfxGetApp()->WriteProfileInt("Editor\\Sort", "IgnoreCase",        mIgnoreCase      );
}
