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
#include "resource.h"
#include "OESplitLinesDlg.h"
#include "Common/DlgDataExt.h"
#include "COMMON\StrHelpers.h"
#include ".\oesplitlinesdlg.h"

OESplitLinesDlg::OESplitLinesDlg(CWnd* pParent /*=NULL*/)
: CDialog(OESplitLinesDlg::IDD, pParent)
{
   m_nLeftMargin             = AfxGetApp()->GetProfileInt   ("Editor", "SplitLinesDlg.m_nLeftMargin"           , 1);
   m_nRightMargin            = AfxGetApp()->GetProfileInt   ("Editor", "SplitLinesDlg.m_nRightMargin"          , 80);
   m_bAdvancedOptions        = AfxGetApp()->GetProfileInt   ("Editor", "SplitLinesDlg.m_bAdvancedOptions"      , false) ? true : false;
   m_sInsertNewLineAfter     = AfxGetApp()->GetProfileString("Editor", "SplitLinesDlg.m_sInsertNewLineAfter"   , ",");
   m_sIgnorePreviousBetween  = AfxGetApp()->GetProfileString("Editor", "SplitLinesDlg.m_sIgnorePreviousBetween", "()");
   m_sDontChangeBetween      = AfxGetApp()->GetProfileString("Editor", "SplitLinesDlg.m_sDontChangeBetween"    , "''");
}

BOOL OESplitLinesDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_OESL_INSERT_AFTER), m_bAdvancedOptions);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_OESL_IGNORE_PREVIOUS), m_bAdvancedOptions);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_OESL_DONT_CHANGE_BETWEEN), m_bAdvancedOptions);

    return TRUE;  // return TRUE unless you set the focus to a control
}

void OESplitLinesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OESL_RIGHT_MARGIN, m_nRightMargin);
    DDV_MinMaxInt(pDX, m_nRightMargin, 1, INT_MAX);

    DDX_Text(pDX, IDC_OESL_LEFT_MARGIN, m_nLeftMargin); 
    DDV_MinMaxInt(pDX, m_nLeftMargin, 1, m_nRightMargin);

    DDX_Check(pDX, IDC_OESL_ADVANCED_OPTIONS, m_bAdvancedOptions); 

    DDX_Text(pDX, IDC_OESL_INSERT_AFTER, m_sInsertNewLineAfter);

    DDX_Text(pDX, IDC_OESL_IGNORE_PREVIOUS, m_sIgnorePreviousBetween);
    if (pDX->m_bSaveAndValidate)
    {
        Common::trim_symmetric(m_sIgnorePreviousBetween);
        int size = m_sIgnorePreviousBetween.size();
        if (size > 0 && (size/2)*2 != size)
        {
            AfxMessageBox("\"Ignore previous between\" must contain one or multiple pairs of characters.", MB_OK|MB_ICONSTOP);
            pDX->Fail();
        }
    }

    DDX_Text(pDX, IDC_OESL_DONT_CHANGE_BETWEEN, m_sDontChangeBetween);
    if (pDX->m_bSaveAndValidate)
    {
        Common::trim_symmetric(m_sDontChangeBetween);
        int size = m_sDontChangeBetween.size();
        if (size > 0 && (size/2)*2 != size)
        {
            AfxMessageBox("\"Don't change between\" must contain one or multiple pairs of characters.", MB_OK|MB_ICONSTOP);
            pDX->Fail();
        }
    }
}

// OESplitLinesDlg message handlers

void OESplitLinesDlg::OnOK()
{
    CDialog::OnOK();

    AfxGetApp()->WriteProfileInt   ("Editor", "SplitLinesDlg.m_nLeftMargin"           , m_nLeftMargin                   );
    AfxGetApp()->WriteProfileInt   ("Editor", "SplitLinesDlg.m_nRightMargin"          , m_nRightMargin                  );
    AfxGetApp()->WriteProfileInt   ("Editor", "SplitLinesDlg.m_bAdvancedOptions"      , m_bAdvancedOptions              );
    AfxGetApp()->WriteProfileString("Editor", "SplitLinesDlg.m_sInsertNewLineAfter"   , m_sInsertNewLineAfter.c_str()   );
    AfxGetApp()->WriteProfileString("Editor", "SplitLinesDlg.m_sIgnorePreviousBetween", m_sIgnorePreviousBetween.c_str());
    AfxGetApp()->WriteProfileString("Editor", "SplitLinesDlg.m_sDontChangeBetween"    , m_sDontChangeBetween.c_str()    );
}

void OESplitLinesDlg::GetIgnoreForceNewLine (std::vector<std::pair<char,char> >& data) const
{
    int size = m_sIgnorePreviousBetween.size();
    size = (size/2)*2;
    for (int i = 0; i < size; i += 2)
        data.push_back(std::pair<char,char>(m_sIgnorePreviousBetween.at(i), m_sIgnorePreviousBetween.at(i+1)));
}

void OESplitLinesDlg::GetDontChaneBeetwen (std::vector<std::pair<char,char> >& data) const
{
    int size = m_sDontChangeBetween.size();
    size = (size/2)*2;
    for (int i = 0; i < size; i += 2)
        data.push_back(std::pair<char,char>(m_sDontChangeBetween.at(i), m_sDontChangeBetween.at(i+1)));
}

BEGIN_MESSAGE_MAP(OESplitLinesDlg, CDialog)
    ON_BN_CLICKED(IDC_OESL_ADVANCED_OPTIONS, OnBnClicked_AdvancedOptions)
END_MESSAGE_MAP()

void OESplitLinesDlg::OnBnClicked_AdvancedOptions ()
{
    UpdateData();
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_OESL_INSERT_AFTER), m_bAdvancedOptions);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_OESL_IGNORE_PREVIOUS), m_bAdvancedOptions);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_OESL_DONT_CHANGE_BETWEEN), m_bAdvancedOptions);
}

