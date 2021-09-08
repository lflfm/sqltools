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
#include <COMMON/DlgDataExt.h>
#include "PropHistoryPage.h"


// CPropHistoryPage dialog
CPropHistoryPage::CPropHistoryPage (SQLToolsSettings& settings)
	: CPropertyPage(CPropHistoryPage::IDD),
    m_settings(settings)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
}

CPropHistoryPage::~CPropHistoryPage ()
{
}

void CPropHistoryPage::DoDataExchange (CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_PROP_HIST_ENABLE, m_settings.m_HistoryEnabled);
    DDX_Radio(pDX, IDC_PROP_HIST_ACTION_COPY, m_settings.m_HistoryAction);
    DDX_Check(pDX, IDC_PROP_HIST_KEEP_SELECTION, m_settings.m_HistoryKeepSelection);
    _ASSERTE(m_settings.m_HistoryAction == SQLToolsSettings::Copy
        || m_settings.m_HistoryAction == SQLToolsSettings::Paste
        || m_settings.m_HistoryAction == SQLToolsSettings::ReplaceAll);

    DDX_Text(pDX, IDC_PROP_HIST_COUNT, m_settings.m_HistoryMaxCout);
    DDV_MinMaxUInt(pDX, m_settings.m_HistoryMaxCout, 0, INT_MAX);
    DDX_Text(pDX, IDC_PROP_HIST_SIZE, m_settings.m_HistoryMaxSize);
    DDV_MinMaxUInt(pDX, m_settings.m_HistoryMaxSize, 0, INT_MAX);

    DDX_Check(pDX, IDC_PROP_HIST_VALID_ONLY, m_settings.m_HistoryValidOnly);
    DDX_Check(pDX, IDC_PROP_HIST_DISTINCT,   m_settings.m_HistoryDistinct );
    DDX_Check(pDX, IDC_PROP_HIST_GLOBAL,     m_settings.m_HistoryGlobal  );
    DDX_Check(pDX, IDC_PROP_HIST_PERSISTENT, m_settings.m_HistoryPersitent);

    SendDlgItemMessage(IDC_PROP_HIST_COUNT_SPIN, UDM_SETRANGE32, 1, INT_MAX);
    SendDlgItemMessage(IDC_PROP_HIST_SIZE_SPIN,  UDM_SETRANGE32, 1, INT_MAX);
}


BOOL CPropHistoryPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    OnBnClicked_Enable();

    return TRUE;
}

BEGIN_MESSAGE_MAP(CPropHistoryPage, CPropertyPage)
    ON_BN_CLICKED(IDC_PROP_HIST_ENABLE, OnBnClicked_Enable)
    ON_BN_CLICKED(IDC_PROP_HIST_GLOBAL, OnBnClicked_Global)
END_MESSAGE_MAP()

// CPropHistoryPage message handlers
void CPropHistoryPage::OnBnClicked_Enable()
{
    BOOL enable = ::SendDlgItemMessage(m_hWnd, IDC_PROP_HIST_ENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED;
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROP_HIST_COUNT), enable);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROP_HIST_COUNT_SPIN), enable);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROP_HIST_SIZE), enable);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROP_HIST_SIZE_SPIN), enable);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROP_HIST_DISTINCT), enable);
//    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROP_HIST_INHERIT), enable);
//    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROP_HIST_PERSISTENT), enable);
}

void CPropHistoryPage::OnBnClicked_Global()
{
    AfxMessageBox(L"Changing GLOBAL option will not affect already open windows!", MB_OK|MB_ICONEXCLAMATION);
}
