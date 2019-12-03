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
#include "ExtractDDLSettings.h"
#include "ExtractSchemaOptionPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CExtractSchemaOptionPage::CExtractSchemaOptionPage (ExtractDDLSettings& DDLSettings) 
: CPropertyPage(CExtractSchemaOptionPage::IDD),
  m_DDLSettings(DDLSettings)
{
}


void CExtractSchemaOptionPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_ESP_OPTIMAL_VIEW_ORDER,    m_DDLSettings.m_OptimalViewOrder);
    DDX_Check(pDX, IDC_ESP_GROUP_BY_DDL,          m_DDLSettings.m_GroupByDDL);

	DDX_Check(pDX, IDC_ESP_LOWER_NAMES,           m_DDLSettings.m_LowerNames);
	DDX_Check(pDX, IDC_ESP_SCHEMA_NAME,           m_DDLSettings.m_ShemaName);
	DDX_Check(pDX, IDC_ESP_SQLPLUS_COMPATIBILITY, m_DDLSettings.m_SQLPlusCompatibility);
    DDX_Check(pDX, IDC_ESP_NO_STORAGE_FOR_PKUK,   m_DDLSettings.m_NoStorageForConstraint);
    DDX_Radio(pDX, IDC_ESP_STORAGE_DISABLE,       m_DDLSettings.m_StorageClause);
    DDX_Check(pDX, IDC_ESP_ALWAYS_PUT_TABLESPACE, m_DDLSettings.m_AlwaysPutTablespace);
    DDX_Check(pDX, IDC_ESP_COMMENTS_AFTER_COLUMN, m_DDLSettings.m_CommentsAfterColumn);
    DDX_Text (pDX, IDC_ESP_COMMENTS_POS,          m_DDLSettings.m_CommentsPos);
    DDX_Check(pDX, IDC_ESP_VIEW_WITH_FORCE,       m_DDLSettings.m_ViewWithForce);
    DDX_Check(pDX, IDC_ESP_SEQ_WITH_START,        m_DDLSettings.m_SequnceWithStart);

    OnCommentsAfterColumn();
}


BEGIN_MESSAGE_MAP(CExtractSchemaOptionPage, CDialog)
	ON_BN_CLICKED(IDC_ESP_COMMENTS_AFTER_COLUMN, OnCommentsAfterColumn)
END_MESSAGE_MAP()


void CExtractSchemaOptionPage::OnCommentsAfterColumn() 
{
    if (m_hWnd) 
    {
        HWND hCheck = ::GetDlgItem(m_hWnd, IDC_ESP_COMMENTS_AFTER_COLUMN);
        HWND hText = ::GetDlgItem(m_hWnd, IDC_ESP_COMMENTS_POS);
        ::EnableWindow(hText, ::SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
}

LRESULT CExtractSchemaOptionPage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}
