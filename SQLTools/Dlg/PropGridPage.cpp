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

// 11.09.2002 bug fix, spin control behavior
// 06.10.2002 changes, some simplifying

#include "stdafx.h"
#include "SQLTools.h"
#include "PropGridPage.h"
#include <OpenGrid/OCISource.h>
#include <COMMON/StrHelpers.h>
#include <COMMON/DlgDataExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropGridPage1 property page

CPropGridPage1::CPropGridPage1(SQLToolsSettings& settings) 
    : CPropertyPage(IDD_PROP_GRID1),
    m_settings(settings)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CPropGridPage1)
	//}}AFX_DATA_INIT
}

void CPropGridPage1::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropGridPage1)
    //}}AFX_DATA_MAP
    DDX_Text(pDX, IDC_GRID_PROP_ROWPREFETCH,         m_settings.m_GridPrefetchLimit);
    DDX_Text(pDX, IDC_GRID_PROP_MAX_LOB_LENGTH,      m_settings.m_GridMaxLobLength);
    DDX_Text(pDX, IDC_GRID_PROP_MAX_BLOB_HEX_LENGTH, m_settings.m_GridBlobHexRowLength);
    DDX_Text(pDX, IDC_GRID_PROP_MAX_COLUMN_LENGTH,   m_settings.m_GridMaxColLength);
    DDX_Text(pDX, IDC_GRID_PROP_MULTILINE_COUNT,     m_settings.m_GridMultilineCount);


    DDV_MinMaxUInt(pDX, m_settings.m_GridPrefetchLimit,     0, INT_MAX);
    DDV_MinMaxUInt(pDX, m_settings.m_GridMaxLobLength,      255, 1024*1024);
    DDV_MinMaxUInt(pDX, m_settings.m_GridBlobHexRowLength,  1, 512);
    DDV_MinMaxUInt(pDX, m_settings.m_GridMaxColLength,      1, 255);
    DDV_MinMaxUInt(pDX, m_settings.m_GridMultilineCount,    1, 16);

    SendDlgItemMessage(IDC_GRID_PROP_ROWPREFETCH_SPIN,          UDM_SETRANGE32, 0, INT_MAX);
    SendDlgItemMessage(IDC_GRID_PROP_MAX_LOB_LENGTH_SPIN,       UDM_SETRANGE32, 255, 1024*1024);
    SendDlgItemMessage(IDC_GRID_PROP_MAX_BLOB_HEX_LENGTH_SPIN,  UDM_SETRANGE32, 1, 512);
    SendDlgItemMessage(IDC_GRID_PROP_DEF_DATE_LENGTH_SPIN,      UDM_SETRANGE32, 1, 255);
    SendDlgItemMessage(IDC_GRID_PROP_MAX_COLUMN_LENGTH_SPIN,    UDM_SETRANGE32, 1, 255);
    SendDlgItemMessage(IDC_GRID_PROP_MULTILINE_COUNT_SPIN,      UDM_SETRANGE32, 1, 16);

    static UDACCEL accels[3] = { { 0, 64 }, { 2, 1024 }, { 5, 4048 } };
    SendDlgItemMessage(IDC_GRID_PROP_ROWPREFETCH_SPIN,    UDM_SETACCEL, (WPARAM)(sizeof(accels)/sizeof(accels)[0]), (LPARAM)&accels);
    SendDlgItemMessage(IDC_GRID_PROP_MAX_LOB_LENGTH_SPIN, UDM_SETACCEL, (WPARAM)(sizeof(accels)/sizeof(accels)[0]), (LPARAM)&accels);

    DDX_Check(pDX, IDC_GRID_PROP_SKIP_LOBS,              m_settings.m_GridSkipLobs);
    DDX_Check(pDX, IDC_GRID_PROP_ALLOW_LESS_THAN_HEADER, m_settings.m_GridAllowLessThanHeader);
    DDX_Check(pDX, IDC_GRID_PROP_ALLOW_REMEM_COL_WIDTH,  m_settings.m_GridAllowRememColWidth);
    DDX_Check(pDX, IDC_GRID_PROP_HEADERS_IN_LOWERCASE,   m_settings.m_GridHeadersInLowerCase);
    DDX_Check(pDX, IDC_GRID_PROP_WRAPAROUND,             m_settings.m_GridWraparound);
}

//BEGIN_MESSAGE_MAP(CPropGridPage1, CPropertyPage)
//END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropGridPage2 property page

CPropGridPage2::CPropGridPage2(SQLToolsSettings& settings) 
    : CPropertyPage(IDD_PROP_GRID2),
    m_settings(settings)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CPropGridPage2)
	//}}AFX_DATA_INIT
}

void CPropGridPage2::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropGridPage2)
    //}}AFX_DATA_MAP
    DDX_Text(pDX, IDC_GRID_PROP_NUMBER_FORMAT,       m_settings.m_GridNlsNumberFormat);
    DDX_Text(pDX, IDC_GRID_PROP_DATE_FORMAT,         m_settings.m_GridNlsDateFormat);
    DDX_Text(pDX, IDC_GRID_PROP_TIME_FORMAT,         m_settings.m_GridNlsTimeFormat);
    DDX_Text(pDX, IDC_GRID_PROP_TIMESTAMP_FORMAT,    m_settings.m_GridNlsTimestampFormat);
    DDX_Text(pDX, IDC_GRID_PROP_TIMESTAMP_TZ_FORMAT, m_settings.m_GridNlsTimestampTzFormat);
    DDX_Text(pDX, IDC_GRID_PROP_NULL_REPRESENTATION, m_settings.m_GridNullRepresentation);

    DDV_MaxChars(pDX, m_settings.m_GridNullRepresentation, 255);
}

//BEGIN_MESSAGE_MAP(CPropGridPage2, CPropertyPage)
//END_MESSAGE_MAP()
