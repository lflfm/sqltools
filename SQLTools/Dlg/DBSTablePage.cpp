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
#include "DBSTablePage.h"
#include <COMMON/DlgDataExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CDBSTablePage::CDBSTablePage (SQLToolsSettings& property)
	: CPropertyPage(CDBSTablePage::IDD, NULL),
    m_DDLSettings(property)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CDBSTablePage)
	//}}AFX_DATA_INIT
}


void CDBSTablePage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDBSTablePage)
    //}}AFX_DATA_MAP
    DDX_Check(pDX, IDC_DBSP_GROUP_BY_DDL,          m_DDLSettings.m_GroupByDDL);
    DDX_Check(pDX, IDC_DBSP_CONSTRAINTS,           m_DDLSettings.m_Constraints);
    DDX_Check(pDX, IDC_DBSP_INDEXES,               m_DDLSettings.m_Indexes);
    DDX_Check(pDX, IDC_DBSP_NO_STORAGE_FOR_PKUK,   m_DDLSettings.m_NoStorageForConstraint);
    DDX_Radio(pDX, IDC_DBSP_STORAGE_DISABLE,       m_DDLSettings.m_StorageClause);
    DDX_Check(pDX, IDC_DBSP_ALWAYS_PUT_TABLESPACE, m_DDLSettings.m_AlwaysPutTablespace);
    DDX_Check(pDX, IDC_DBSP_TRIGGERS,              m_DDLSettings.m_Triggers);
    DDX_Check(pDX, IDC_DBSP_TABLE,                 m_DDLSettings.m_TableDefinition);
    DDX_Check(pDX, IDC_DBSP_COMMENTS_AFTER_COLUMN, m_DDLSettings.m_CommentsAfterColumn);
    DDX_Text (pDX, IDC_DBSP_COMMENTS_POS,          m_DDLSettings.m_CommentsPos);
    DDX_Check(pDX, IDC_DBSP_ALWAYS_WRITE_COLUMN_LENGTH_SEMATICS, m_DDLSettings.m_AlwaysWriteColumnLengthSematics);

    OnDbspCommentsAfterColumn();
}


BEGIN_MESSAGE_MAP(CDBSTablePage, CPropertyPage)
	//{{AFX_MSG_MAP(CDBSTablePage)
	ON_BN_CLICKED(IDC_DBSP_COMMENTS_AFTER_COLUMN, OnDbspCommentsAfterColumn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBSTablePage message handlers

void CDBSTablePage::OnDbspCommentsAfterColumn() 
{
    if (m_hWnd) {
        HWND hCheck = ::GetDlgItem(m_hWnd, IDC_DBSP_COMMENTS_AFTER_COLUMN);
        HWND hText = ::GetDlgItem(m_hWnd, IDC_DBSP_COMMENTS_POS);
        ::EnableWindow(hText, ::SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
}
