/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2011 Aleksey Kochetov

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
#include "PropScriptPage.h"
#include <COMMON/DlgDataExt.h>


CPropScriptPage::CPropScriptPage(SQLToolsSettings& settings)
: CPropertyPage(CPropScriptPage::IDD),
m_settings(settings)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
}

void CPropScriptPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropServerPage)
    //}}AFX_DATA_MAP
    DDX_Radio(pDX, IDC_SCRIPT_EXECUTION_STYLE1,     m_settings.m_ExecutionStyle);
    DDX_Radio(pDX, IDC_SCRIPT_EXECUTION_STYLE_ALT1, m_settings.m_ExecutionStyleAlt);

    DDX_Check(pDX, IDC_SCRIPT_HALT_ON_ERROR,       m_settings.m_ExecutionHaltOnError);
    DDX_Check(pDX, IDC_SCRIPT_HIGHLIGHT_EXECUTED,  m_settings.m_HighlightExecutedStatement);
    DDX_Check(pDX, IDC_SCRIPT_AUTOSCROLL_EXECUTED, m_settings.m_AutoscrollExecutedStatement);
    DDX_Check(pDX, IDC_SCRIPT_AUTOSCROLL_TO_EXECUTED, m_settings.m_AutoscrollToExecutedStatement);
    DDX_Check(pDX, IDC_SCRIPT_BUFFERED_OUTPUT,     m_settings.m_BufferedExecutionOutput);
    DDX_Check(pDX, IDC_IGNORE_COMMMENTS_AFTER_SEMICOLON, m_settings.m_IgnoreCommmentsAfterSemicolon);
}
