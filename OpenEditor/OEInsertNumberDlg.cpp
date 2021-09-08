/* 
    Copyright (C) 2011 Aleksey Kochetov

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
#include "OpenEditor\OEInsertNumberDlg.h"
#include "Common/DlgDataExt.h"


// OEInsertNumberDlg dialog

OEInsertNumberDlg::OEInsertNumberDlg(CWnd* pParent /*=NULL*/)
	: CDialog(OEInsertNumberDlg::IDD, pParent)
{
    m_nFirstNumber     = AfxGetApp()->GetProfileInt(L"Editor", L"InsertNumberDlg.FirstNumber",    1);
    m_nIncrement       = AfxGetApp()->GetProfileInt(L"Editor", L"InsertNumberDlg.Increment",      1);
    m_bLeadingZeros    = AfxGetApp()->GetProfileInt(L"Editor", L"InsertNumberDlg.LeadingZeros",   0) ? true : false;
    m_bSkipEmptyLines  = AfxGetApp()->GetProfileInt(L"Editor", L"InsertNumberDlg.SkipEmptyLines", 0) ? true : false;
    m_nNumberFormat    = AfxGetApp()->GetProfileInt(L"Editor", L"InsertNumberDlg.NumberFormat",   0);
}

/*
BEGIN_MESSAGE_MAP(OEInsertNumberDlg, CDialog)
END_MESSAGE_MAP()
*/

void OEInsertNumberDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

    DDX_Text  (pDX, IDC_OECN_FIRST_NUMBER    , m_nFirstNumber   ); 
    DDX_Text  (pDX, IDC_OECN_INCREMENT       , m_nIncrement     );
    DDX_Check (pDX, IDC_OECN_LEADING_ZEROS   , m_bLeadingZeros  );
    DDX_Check (pDX, IDC_OECN_SKIP_EMPTY_LINES, m_bSkipEmptyLines);
    DDX_Radio (pDX, IDC_OECN_DECIMAL         , m_nNumberFormat  );
}


void OEInsertNumberDlg::OnOK()
{
    CDialog::OnOK();

    AfxGetApp()->WriteProfileInt(L"Editor", L"InsertNumberDlg.FirstNumber",    m_nFirstNumber   );
    AfxGetApp()->WriteProfileInt(L"Editor", L"InsertNumberDlg.Increment",      m_nIncrement     );
    AfxGetApp()->WriteProfileInt(L"Editor", L"InsertNumberDlg.LeadingZeros",   m_bLeadingZeros   ? 1 : 0);
    AfxGetApp()->WriteProfileInt(L"Editor", L"InsertNumberDlg.SkipEmptyLines", m_bSkipEmptyLines ? 1 : 0);
    AfxGetApp()->WriteProfileInt(L"Editor", L"InsertNumberDlg.NumberFormat",   m_nNumberFormat  );
}
